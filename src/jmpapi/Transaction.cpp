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
  Request(req), Event::OAuth2Login(app.getEventClient()), app(app),
  jsonFields(0) {LOG_DEBUG(5, "Transaction()");}


Transaction::~Transaction() {LOG_DEBUG(5, "~Transaction()");}


void Transaction::setSessionCookie() {
  string sid = getSession()->getID();
  setCookie(app.getSessionCookieName(), sid, "", "/");
}


string Transaction::getUserID() const {
  return getSession()->getString("provider") + ":" +
    getSession()->getString("provider_id");
}


void Transaction::query(event_db_member_functor_t member, const string &sql,
                        const SmartPointer<const JSON::Value> &dict) {
  if (db.isNull()) db = app.getDBConnection();
  db->query(this, member, sql, dict);
}


SmartPointer<JSON::Writer> Transaction::getJSONWriter() {
  if (writer.isNull()) writer = Request::getJSONWriter();
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

  resetOutput();

  SmartPointer<JSON::Writer> writer = getJSONWriter();

  code = code ? code : HTTP_INTERNAL_SERVER_ERROR;

  writer->beginDict();
  writer->insert("error", msg.empty() ?
                 Event::HTTPStatus((HTTPStatus::enum_t)code).toString() : msg);
  writer->insert("status", code);
  writer->endDict();
  writer->close();

  reply(code);
}


void Transaction::processProfile(const SmartPointer<JSON::Value> &profile) {
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

  sendJSONError(HTTP_UNAUTHORIZED);
}


bool Transaction::apiLogin(const JSON::ValuePtr &config) {
  this->config = config;
  const URI &uri = getURI();

  // Check Session
  if (getSession().isNull())
    setSession(app.getSessionManager().openSession(getClientIP()));

  else if (getSession()->hasGroup("authenticated")) {
    if (!config->hasString("sql")) login();
    else query(&Transaction::login, config->getString("sql"), getSession());
    return true;
  }

  OAuth2 *auth;
  JSON::Dict &args = parseArgs();
  string provider = args.getString("provider", "google");

  if (provider == "google") auth = &app.getGoogleAuth();
  else if (provider == "github") auth = &app.getGitHubAuth();
  else if (provider == "facebook") auth = &app.getFacebookAuth();
  else THROWCS("Unsupported login provider: " << provider,
               HTTP_INTERNAL_SERVER_ERROR);

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
  writer.release();
  reply();

  return true;
}


bool Transaction::apiLogout(const JSON::ValuePtr &config) {
  // DB logout
  if (!config->hasString("sql")) logout();
  else query(&Transaction::logout, config->getString("sql"), getSession());

  return true;
}


string Transaction::nextJSONField() {
  if (!jsonFields) return "";

  const char *start = jsonFields;
  const char *end = jsonFields;
  while (*end && *end != ' ') end++;

  jsonFields = *end ? end + 1 : 0;

  return string(start, end - start);
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
    getSession()->write(*getJSONWriter());
    writer.release();
    reply();
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


void Transaction::returnList(MariaDB::EventDBCallback::state_t state) {
  if (state != MariaDB::EventDBCallback::EVENTDB_ROW) returnJSON(state);

  switch (state) {
  case MariaDB::EventDBCallback::EVENTDB_ROW:
    writer->beginAppend();

    if (db->getFieldCount() == 1) db->writeField(*writer, 0);
    else db->writeRowDict(*writer);
    break;

  case MariaDB::EventDBCallback::EVENTDB_BEGIN_RESULT:
    getJSONWriter()->beginList();
    break;

  case MariaDB::EventDBCallback::EVENTDB_END_RESULT:
    writer->endList();
    break;

  default: break;
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


void Transaction::returnJSONFields(MariaDB::EventDBCallback::state_t state) {
  switch (state) {
  case MariaDB::EventDBCallback::EVENTDB_ROW:
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

  case MariaDB::EventDBCallback::EVENTDB_BEGIN_RESULT: {
    if (writer.isNull()) getJSONWriter()->beginDict();

    string field = nextJSONField();
    if (field.empty()) THROW("Unexpected result set");
    if (field[0] == '*') writer->insertDict(field.substr(1));
    else writer->insertList(field);
    break;
  }

  case MariaDB::EventDBCallback::EVENTDB_END_RESULT:
    if (writer->inList()) writer->endList();
    else writer->endDict();
    break;

  case MariaDB::EventDBCallback::EVENTDB_DONE:
    if (!writer.isNull()) writer->endDict();
    // Fall through

  default: return returnOk(state);
  }
}


void Transaction::returnOk(MariaDB::EventDBCallback::state_t state) {
  switch (state) {
  case MariaDB::EventDBCallback::EVENTDB_DONE:
    writer.release();
    reply();
    break;

  case MariaDB::EventDBCallback::EVENTDB_RETRY:
    writer.release();
    resetOutput();
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
    THROWXS(db->getError(), error);

    break;
  }

  default:
    sendJSONError();
    THROWX("Unexpected DB response", HTTP_INTERNAL_SERVER_ERROR);
    return;
  }
}
