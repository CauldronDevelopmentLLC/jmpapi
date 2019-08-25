/******************************************************************************\

                          This file is part of JmpAPI.

               Copyright (c) 2014-2019, Cauldron Development LLC
                              All rights reserved.

          The JmpAPI Webserver is free software: you can redistribute
         it and/or modify it under the terms of the GNU General Public
          License as published by the Free Software Foundation, either
        version 2 of the License, or (at your option) any later version.

          The JmpAPI Webserver is distributed in the hope that it will
         be useful, but WITHOUT ANY WARRANTY; without even the implied
            warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
         PURPOSE.  See the GNU General Public License for more details.

       You should have received a copy of the GNU General Public License
                     along with this software.  If not, see
                        <http://www.gnu.org/licenses/>.

                 For information regarding this software email:
                                Joseph Coffland
                         joseph@cauldrondevelopment.com

\******************************************************************************/

#include "Transaction.h"
#include "App.h"
#include "Resolver.h"

#include <cbang/event/Buffer.h>
#include <cbang/event/Event.h>
#include <cbang/event/HTTP.h>

#include <cbang/json/JSON.h>
#include <cbang/log/Logger.h>
#include <cbang/Catch.h>
#include <cbang/db/maria/EventDB.h>
#include <cbang/net/URI.h>

#include <mysql/mysqld_error.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


Transaction::Transaction(App &app, RequestMethod method, const URI &uri,
                         const Version &version) :
  Request(method, uri, version), Event::OAuth2Login(app.getEventClient()),
  app(app), currentField(0) {}


void Transaction::setSessionCookie() {
  setCookie(app.getSessionCookieName(), getSession()->getID(), "", "/");
}


bool Transaction::lookupSession(const string &sql) {
  // Check if Session is already loaded
  if (!getSession().isNull()) return false;

  // Check if we have a session ID
  string sid = getSessionID(app.getSessionCookieName());
  if (sid.empty()) return false;

  // Lookup Session in SessionManager
  try {
    setSession(app.getSessionManager().lookupSession(sid));
    LOG_DEBUG(3, "User: " << getUser());

    return false;
  } catch (const Exception &) {}

  // Lookup Session in DB
  if (sql.empty()) return false;
  setSession(new Session(sid, getClientIP()));
  query(&Transaction::session, sql);

  return true;
}


void Transaction::setFields(const JSON::ValuePtr &fields) {
  this->fields = fields;
  currentField = 0;
  nextField.clear();
}


void Transaction::query(event_db_member_functor_t member, const string &sql) {
  if (isReplying()) THROW("DB query initiated but Transaction was finalized!");

  if (db.isNull()) db = app.getDBConnection();

  result = 0; // Reset result count
  db->query(this, member, Resolver(*this).format(sql, "NULL"));
}


SmartPointer<JSON::Writer> Transaction::getJSONWriter() {
  if (writer.isNull()) {
    if (app.getOptions()["jsonp"].hasValue()) {
      string callback = app.getOptions()["jsonp"];
      if (getArgs()->hasString(callback))
        return writer = Request::getJSONPWriter(getArg(callback));
    }

    writer = Request::getJSONWriter();
  }

  return writer;
}


void Transaction::sendError(Event::HTTPStatus  code, const string &msg) {
  sendJSONError(code, msg);
}


void Transaction::sendJSONError(Event::HTTPStatus code, const string &msg) {
  // Release JSON writer
  writer.release();

  // Drop DB connection
  if (!db.isNull()) db->close();

  if (isReplying()) {
    LOG_ERROR(msg);
    return;
  }

  resetOutput();

  if (!code) code = HTTP_INTERNAL_SERVER_ERROR;

  getJSONWriter()->beginDict();
  writer->insert("error", msg.empty() ? code.toString() : msg);
  writer->insert("status", code);
  writer->endDict();

  reply(code);
}


void Transaction::write() {
  if (!writer.isNull()) {
    writer->close();
    writer.release();
  }

  Request::write();
}


void Transaction::processProfile(Event::Request &req,
                                 const JSON::ValuePtr &profile) {
  if (!profile.isNull()) {
    try {
      string provider = profile->getString("provider");

      // Fix up Facebook avatar
      if (provider == "facebook")
        profile->insert("avatar", "http://graph.facebook.com/" +
                        profile->getString("id") + "/picture?type=small");

      // Fix up for GitHub name
      if ((!profile->hasString("name") ||
           String::trim(profile->getString("name")).empty()) &&
          profile->hasString("login"))
        profile->insert("name", profile->getString("login"));

      LOG_DEBUG(3, "Profile: " << *profile);

      // Fill session
      Session &session = *getSession();
      session.setUser(profile->getString("email"));
      session.insert("provider",    provider);
      session.insert("provider_id", profile->getString("id"));
      session.insert("name",        profile->getString("name"));
      session.insert("avatar",      profile->getString("avatar"));

      // DB login
      if (!config->hasString("sql")) login();
      else query(&Transaction::login, config->getString("sql"));

      return;
    } CATCH_ERROR;

    LOG_ERROR("Invalid login profile: " << *profile);
  }

  sendJSONError(HTTP_UNAUTHORIZED);
}


bool Transaction::apiLogin(const JSON::ValuePtr &config) {
  auto &args = *parseArgs();
  this->config = config;

  string provider = args.getString("provider", "");
  if (provider.empty()) {
    if (getSession().isNull() || !getSession()->hasGroup("authenticated")) {
      sendJSONError(HTTP_UNAUTHORIZED, "Not logged in");
      return true;
    }

    getSession()->write(*getJSONWriter()); // Respond with Session JSON

  } else if (provider == "providers") {
    getJSONWriter()->beginList();
    if (app.getGoogleAuth().isConfigured()) writer->append("google");
    if (app.getGitHubAuth().isConfigured()) writer->append("github");
    if (app.getFacebookAuth().isConfigured()) writer->append("facebook");
    writer->endList();

  } else {
    // Get OAuth2 login provider
    OAuth2 *auth = 0;
    if (provider == "google") auth = &app.getGoogleAuth();
    else if (provider == "github") auth = &app.getGitHubAuth();
    else if (provider == "facebook") auth = &app.getFacebookAuth();

    if (!auth || !auth->isConfigured()) {
      sendJSONError(HTTP_BAD_REQUEST,
                    SSTR("Unsupported login provider: " << provider));
      return true;
    }

    OAuth2Login::setOAuth2(SmartPointer<OAuth2>::Phony(auth));

    const URI &uri = getURI();
    if (uri.has("state") && !getSession().isNull())
      return OAuth2Login::requestToken
        (*this, getSession()->getID(),
         getSession()->getString("redirect_uri", ""));

    // Open new Session
    setSession(app.getSessionManager().openSession(getClientIP()));
    setSessionCookie(); // Needed to pass anti-forgery

    string sid = getSession()->getID();
    URI redirectURL = auth->getRedirectURL(uri.getPath(), sid);
    if (args.hasString("redirect_uri")) {
      string uri = args.getString("redirect_uri");
      redirectURL.set("redirect_uri", uri);
      getSession()->insert("redirect_uri", uri);
    }

    getJSONWriter()->beginDict();
    writer->insert("id", sid);
    writer->insert("redirect", redirectURL);
    writer->endDict();
  }

  reply();
  return true;
}


bool Transaction::apiLogout(const JSON::ValuePtr &config) {
  if (!config->hasString("sql") || getSession().isNull()) logout();
  else query(&Transaction::logout, config->getString("sql"));

  return true;
}


void Transaction::session(MariaDB::EventDB::state_t state) {
  Session &session = *getSession(); // Must have Session

  switch (state) {
  case MariaDB::EventDB::EVENTDB_BEGIN_RESULT:
  case MariaDB::EventDB::EVENTDB_END_RESULT:
    break;

  case MariaDB::EventDB::EVENTDB_ROW: {
    if (!session.hasString("user")) session.read(*db->getRowDict());
    else session.addGroup(db->getString(0));
    break;
  }

  case MariaDB::EventDB::EVENTDB_DONE:
    if (session.hasString("user")) {
      // Session was found in DB
      session.addGroup("authenticated");
      app.getSessionManager().addSession(getSession());
    }

    // Restart request processing
    Event::HTTP::dispatch(app.getServer(), *this);
    break;

  default: returnOk(state); return; // For error handling
  }
}


void Transaction::login(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_BEGIN_RESULT: break;
  case MariaDB::EventDB::EVENTDB_END_RESULT: result++; break;

  case MariaDB::EventDB::EVENTDB_ROW: {
    if (result) getSession()->addGroup(db->getString(0)); // Groups
    else if (db->getField(1).isNumber()) // Session vars
      getSession()->insert(db->getString(0), db->getDouble(1));
    else getSession()->insert(db->getString(0), db->getString(1));
    break;
  }

  case MariaDB::EventDB::EVENTDB_DONE:
    // Authenticate Session
    getSession()->addGroup("authenticated");

    // Set Cookie
    setSessionCookie();

    // Reply
    if (config->hasString("redirect")) redirect(config->getString("redirect"));
    else reply();
    break;

  default: returnOk(state); return; // For error handling
  }
}


void Transaction::logout(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_DONE:
    // Session
    if (!getSession().isNull())
      app.getSessionManager().closeSession(getSession()->getID());

    // Clear session cookie
    setCookie(app.getSessionCookieName(), "", "", "/", 1);

    // Reply
    reply();
    break;

  default: returnOk(state); return; // For error handling
  }
}


void Transaction::returnHeadList(MariaDB::EventDB::state_t state) {
  if (state == MariaDB::EventDB::EVENTDB_ROW) {
    if (writer.isNull()) {
      getJSONWriter()->beginList();

      // Write list header
      writer->appendList();
      for (unsigned i = 0; i < db->getFieldCount(); i++)
        writer->append(db->getField(i).getName());
      writer->endList();
    }

    writer->beginAppend();
    db->writeRowList(*writer);

  } else returnList(state);
}



void Transaction::returnList(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_ROW:
    if (writer.isNull()) getJSONWriter()->beginList();

    writer->beginAppend();

    if (db->getFieldCount() == 1) db->writeField(*writer, 0);
    else db->writeRowDict(*writer);
    break;

  case MariaDB::EventDB::EVENTDB_BEGIN_RESULT: break;
  case MariaDB::EventDB::EVENTDB_END_RESULT: break;

  case MariaDB::EventDB::EVENTDB_DONE:
    if (writer.isNull()) getJSONWriter()->beginList(); // Empty list
    writer->endList();
    returnOk(state);
    break;

  default: returnOk(state); break;
  }
}


void Transaction::returnBool(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_ROW:
    getJSONWriter()->writeBoolean(db->getBoolean(0));
    break;

  default: return returnOne(state);
  }
}



void Transaction::returnU64(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_ROW:
    getJSONWriter()->write(db->getU64(0));
    break;

  default: return returnOne(state);
  }
}


void Transaction::returnS64(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_ROW:
    getJSONWriter()->write(db->getS64(0));
    break;

  default: return returnOne(state);
  }
}


void Transaction::returnFields(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_ROW:
    if (writer.isNull()) getJSONWriter()->beginDict();

    if (!nextField.empty()) {
      closeField = true;
      if (nextField[0] == '*') {
        if (nextField.length() != 1) writer->insertDict(nextField.substr(1));
        else closeField = false;
      } else writer->insertList(nextField);
      nextField.clear();
    }

    if (writer->inDict()) db->insertRow(*writer, 0, -1, false);

    else if (db->getFieldCount() == 1) {
      writer->beginAppend();
      db->writeField(*writer, 0);

    } else {
      writer->appendDict();
      db->insertRow(*writer, 0, -1, false);
      writer->endDict();
    }
    break;

  case MariaDB::EventDB::EVENTDB_BEGIN_RESULT: {
    closeField = false;
    if (fields.isNull()) THROW("Fields cannot be null");
    if (currentField == fields->size()) THROW("Unexpected DB result");
    nextField = fields->getString(currentField++);
    if (nextField.empty()) THROW("Empty field name");
    break;
  }

  case MariaDB::EventDB::EVENTDB_END_RESULT:
    if (closeField) {
      if (writer->inList()) writer->endList();
      else writer->endDict();
    }
    break;

  case MariaDB::EventDB::EVENTDB_DONE:
    if (writer.isNull()) sendJSONError(HTTP_NOT_FOUND);
    else {
      writer->endDict();
      return returnOk(state); // Success
    }
    break;

  default: return returnOk(state);
  }
}


void Transaction::returnDict(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_ROW:
    if (writer.isNull()) db->writeRowDict(*getJSONWriter());
    else return returnOk(state); // Error
    break;

  default: return returnOne(state);
  }
}


void Transaction::returnOne(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_ROW:
    if (writer.isNull() && db->getFieldCount() == 1)
      db->writeField(*getJSONWriter(), 0);
    else return returnOk(state); // Error
    break;

  case MariaDB::EventDB::EVENTDB_BEGIN_RESULT:
  case MariaDB::EventDB::EVENTDB_END_RESULT:
    break;

  case MariaDB::EventDB::EVENTDB_DONE:
    if (writer.isNull()) sendJSONError(HTTP_NOT_FOUND);
    else return returnOk(state); // Success
    break;

  default: return returnOk(state);
  }
}


void Transaction::returnOk(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_DONE:
    reply();
    break;

  case MariaDB::EventDB::EVENTDB_RETRY:
    if (!isReplying()) resetOutput();
    break;

  case MariaDB::EventDB::EVENTDB_ERROR: {
    Event::HTTPStatus error = HTTP_INTERNAL_SERVER_ERROR;

    switch (db->getErrorNumber()) {
    case ER_SIGNAL_NOT_FOUND: error = HTTP_NOT_FOUND;   break;
    case ER_DUP_ENTRY:        error = HTTP_CONFLICT;    break;
    case ER_SIGNAL_EXCEPTION: error = HTTP_BAD_REQUEST; break;
    default: break;
    }

    LOG_ERROR("DB:" << db->getErrorNumber() << ": " << db->getError());
    sendJSONError(error, db->getError());
    if (error == HTTP_INTERNAL_SERVER_ERROR) THROW(db->getError());

    break;
  }

  case MariaDB::EventDB::EVENTDB_ROW:
    LOG_ERROR("Unexpected DB row");
    sendJSONError(HTTP_INTERNAL_SERVER_ERROR, "Unexpected DB row");
    break;

  default:
    sendJSONError();
    THROWX("Unexpected DB response: " << state, HTTP_INTERNAL_SERVER_ERROR);
    return;
  }
}
