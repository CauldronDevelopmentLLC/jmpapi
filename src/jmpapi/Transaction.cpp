/******************************************************************************\

                          This file is part of JmpAPI.

               Copyright (c) 2014-2018, Cauldron Development LLC
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

       In addition, BSD licensing may be granted on a case by case basis
       by written permission from at least one of the copyright holders.
          You may request written permission by emailing the authors.

                 For information regarding this software email:
                                Joseph Coffland
                         joseph@cauldrondevelopment.com

\******************************************************************************/

#include "Transaction.h"
#include "App.h"

#include <cbang/event/Buffer.h>
#include <cbang/event/Event.h>
#include <cbang/event/HTTP.h>

#include <cbang/json/JSON.h>
#include <cbang/log/Logger.h>
#include <cbang/util/DefaultCatch.h>
#include <cbang/db/maria/EventDB.h>
#include <cbang/net/URI.h>

#include <mysql/mysqld_error.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


Transaction::Transaction(App &app, evhttp_request *req) :
  Request(req), Event::OAuth2Login(app.getEventClient()), app(app) {}


void Transaction::setSessionCookie() {
  setCookie(app.getSessionCookieName(), getSession()->getID(), "", "/");
}


bool Transaction::lookupSession() {
  // Check if Session is already loaded
  if (!getSession().isNull()) return false;

  // Check if we have a session ID
  string sid = getSessionID(app.getSessionManager().getSessionCookie());
  if (sid.empty()) return false;

  // Lookup Session in SessionManager
  try {
    setSession(app.getSessionManager().lookupSession(sid));
    LOG_DEBUG(3, "User: " << getUser());

    return false;
  } catch (const Exception &) {}

  // Lookup Session in DB
  string sql = app.getOptions()["session-sql"].toString("");
  if (sql.empty()) return false;

  setSession(new Session(sid, getClientIP()));
  query(&Transaction::session, sql, getSession());

  return true;
}


void Transaction::query(event_db_member_functor_t member, const string &sql,
                        const SmartPointer<const JSON::Value> &dict) {
  if (db.isNull()) db = app.getDBConnection();
  db->query(this, member, sql, dict);
}


SmartPointer<JSON::Writer> Transaction::getJSONWriter() {
  if (writer.isNull()) {
    if (app.getOptions()["jsonp"].hasValue()) {
      string callback = app.getOptions()["jsonp"];
      if (getArgs().hasString(callback))
        return writer = Request::getJSONPWriter(getArg(callback));
    }

    writer = Request::getJSONWriter();
  }

  return writer;
}


void Transaction::sendError(int code, const string &msg) {
  sendJSONError(code, msg);
}


void Transaction::sendJSONError(int code, const std::string &msg) {
  // Release JSON writer
  writer.release();

  // Drop DB connection
  if (!db.isNull()) db->close();

  if (isFinalized()) {
    LOG_ERROR(msg);
    return;
  }

  resetOutput();

  code = code ? code : HTTP_INTERNAL_SERVER_ERROR;

  getJSONWriter()->beginDict();
  writer->insert("error", msg.empty() ?
                 Event::HTTPStatus((HTTPStatus::enum_t)code).toString() : msg);
  writer->insert("status", code);
  writer->endDict();

  reply(code);
}


void Transaction::finalize() {
  if (!writer.isNull()) {
    writer->close();
    writer.release();
  }

  Request::finalize();
}


void Transaction::processProfile(const JSON::ValuePtr &profile) {
  if (!profile.isNull())
    try {
      string provider = profile->getString("provider");
      string provider_id = profile->getString("id");

      // Fix up Facebook avatar
      if (provider == "facebook")
        profile->insert("avatar", "http://graph.facebook.com/" +
                        provider_id + "/picture?type=small");

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
      session.insert("provider_id", provider_id);
      session.insert("name",        profile->getString("name"));
      session.insert("avatar",      profile->getString("avatar"));

      // DB login
      if (!config->hasString("sql")) login();
      else query(&Transaction::login, config->getString("sql"), getSession());

      return;
    } CATCH_ERROR;

  LOG_ERROR("Invalid login profile: " << *profile);

  sendJSONError(HTTP_UNAUTHORIZED);
}


bool Transaction::apiLogin(const JSON::ValuePtr &config) {
  JSON::Dict &args = parseArgs();
  this->config = config;

  string provider = args.getString("provider", "");
  if (provider.empty()) {
    if (getSession().isNull() || !getSession()->hasGroup("authenticated")) {
      sendJSONError(HTTP_UNAUTHORIZED, "Not logged in");
      return true;
    }

    getSession()->write(*getJSONWriter()); // Respond with Session JSON

  } else if (provider == "list") {
    getJSONWriter()->beginList();
    if (app.getGoogleAuth().isConfigured()) writer->append("google");
    if (app.getGitHubAuth().isConfigured()) writer->append("github");
    if (app.getFacebookAuth().isConfigured()) writer->append("facebook");
    writer->endList();

  } else {
    // Open new Session, if necessary
    if (getSession().isNull())
      setSession(app.getSessionManager().openSession(getClientIP()));

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

    const URI &uri = getURI();
    string sid = getSession()->getID();
    if (uri.has("state")) return OAuth2Login::requestToken(*this, *auth, sid);

    setSessionCookie(); // Needed to pass anti-forgery

    URI redirectURL = auth->getRedirectURL(uri.getPath(), sid);
    if (args.hasString("redirect_uri"))
      redirectURL.set("redirect_uri", args.getString("redirect_uri"));

    getJSONWriter()->beginDict();
    writer->insert("id", sid);
    writer->insert("redirect", redirectURL);
    writer->endDict();
  }

  reply();
  return true;
}


bool Transaction::apiLogout(const JSON::ValuePtr &config) {
  // DB logout
  if (!config->hasString("sql") || getSession().isNull()) logout();
  else query(&Transaction::logout, config->getString("sql"), getSession());

  return true;
}


void Transaction::session(MariaDB::EventDBCallback::state_t state) {
  switch (state) {
  case MariaDB::EventDBCallback::EVENTDB_BEGIN_RESULT:
  case MariaDB::EventDBCallback::EVENTDB_END_RESULT:
    break;

  case MariaDB::EventDBCallback::EVENTDB_ROW: {
    SmartPointer<Session> session = getSession();
    if (!session->hasString("provider")) session->read(*db->getRowDict());
    else session->addGroup(db->getString(0));
    break;
  }

  case MariaDB::EventDBCallback::EVENTDB_DONE:
    if (getSession()->hasString("provider")) {
      // Session was found in DB
      getSession()->addGroup("authenticated");
      app.getSessionManager().addSession(getSession());

    } else // Session not found in DB, open a new one
      setSession(app.getSessionManager().openSession(getClientIP()));

    // Restart request processing
    Event::HTTP::dispatch(app.getServer(), *this);
    break;

  default: returnOk(state); return; // For error handling
  }
}


void Transaction::login(MariaDB::EventDBCallback::state_t state) {
  switch (state) {
  case MariaDB::EventDBCallback::EVENTDB_BEGIN_RESULT:
  case MariaDB::EventDBCallback::EVENTDB_END_RESULT:
    break;

  case MariaDB::EventDBCallback::EVENTDB_ROW: {
    getSession()->addGroup(db->getString(0));
    break;
  }

  case MariaDB::EventDBCallback::EVENTDB_DONE:
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


void Transaction::logout(MariaDB::EventDBCallback::state_t state) {
  switch (state) {
  case MariaDB::EventDBCallback::EVENTDB_DONE:
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


void Transaction::returnHeadList(MariaDB::EventDBCallback::state_t state) {
  if (state == MariaDB::EventDBCallback::EVENTDB_ROW) {
    writer->beginAppend();
    db->writeRowList(*writer);

  } else returnList(state);

  // Write list header
  if (state == MariaDB::EventDBCallback::EVENTDB_BEGIN_RESULT) {
    writer->appendList();
    for (unsigned i = 0; i < db->getFieldCount(); i++)
      writer->append(db->getField(i).getName());
    writer->endList();
  }
}



void Transaction::returnList(MariaDB::EventDBCallback::state_t state) {
  switch (state) {
  case MariaDB::EventDBCallback::EVENTDB_ROW:
    if (writer.isNull()) getJSONWriter()->beginList();

    writer->beginAppend();

    if (db->getFieldCount() == 1) db->writeField(*writer, 0);
    else db->writeRowDict(*writer);
    break;

  case MariaDB::EventDBCallback::EVENTDB_BEGIN_RESULT: break;
  case MariaDB::EventDBCallback::EVENTDB_END_RESULT: break;

  case MariaDB::EventDBCallback::EVENTDB_DONE:
    if (writer.isNull()) getJSONWriter()->beginList(); // Empty list
    writer->endList();
    returnOk(state);
    break;

  default: returnOk(state); break;
  }
}


void Transaction::returnBool(MariaDB::EventDBCallback::state_t state) {
  switch (state) {
  case MariaDB::EventDBCallback::EVENTDB_ROW:
    getJSONWriter()->writeBoolean(db->getBoolean(0));
    break;

  default: return returnJSON(state);
  }
}



void Transaction::returnU64(MariaDB::EventDBCallback::state_t state) {
  switch (state) {
  case MariaDB::EventDBCallback::EVENTDB_ROW:
    getJSONWriter()->write(db->getU64(0));
    break;

  default: return returnJSON(state);
  }
}


void Transaction::returnS64(MariaDB::EventDBCallback::state_t state) {
  switch (state) {
  case MariaDB::EventDBCallback::EVENTDB_ROW:
    getJSONWriter()->write(db->getS64(0));
    break;

  default: return returnJSON(state);
  }
}


void Transaction::returnJSON(MariaDB::EventDBCallback::state_t state) {
  switch (state) {
  case MariaDB::EventDBCallback::EVENTDB_ROW:
    if (writer.isNull()) {
      getJSONWriter();
      if (db->getFieldCount() == 1) db->writeField(*writer, 0);
      else db->writeRowDict(*writer);

    } else return returnOk(state); // Error
    break;

  case MariaDB::EventDBCallback::EVENTDB_BEGIN_RESULT:
  case MariaDB::EventDBCallback::EVENTDB_END_RESULT:
    break;

  case MariaDB::EventDBCallback::EVENTDB_DONE:
    if (writer.isNull()) sendJSONError(HTTP_NOT_FOUND);
    else return returnOk(state); // Success
    break;

  default: return returnOk(state);
  }
}


void Transaction::returnOk(MariaDB::EventDBCallback::state_t state) {
  switch (state) {
  case MariaDB::EventDBCallback::EVENTDB_DONE:
    reply();
    break;

  case MariaDB::EventDBCallback::EVENTDB_RETRY:
    if (!isFinalized()) resetOutput();
    break;

  case MariaDB::EventDBCallback::EVENTDB_ERROR: {
    int error = HTTP_INTERNAL_SERVER_ERROR;

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

  case MariaDB::EventDBCallback::EVENTDB_ROW:
    LOG_ERROR("Unexpected DB row");
    sendJSONError(HTTP_INTERNAL_SERVER_ERROR, "Unexpected DB row");
    break;

  default:
    sendJSONError();
    THROWXS("Unexpected DB response: " << state, HTTP_INTERNAL_SERVER_ERROR);
    return;
  }
}
