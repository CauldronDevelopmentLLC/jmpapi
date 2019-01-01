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

#pragma once

#include <cbang/event/Request.h>
#include <cbang/event/OAuth2Login.h>
#include <cbang/db/maria/EventDBCallback.h>
#include <cbang/json/Value.h>


namespace cb {
  class OAuth2Login;
  namespace MariaDB {class EventDB;}
  namespace JSON {class Writer;}
}


namespace JmpAPI {
  class App;

  class Transaction : public cb::Event::Request, public cb::Event::OAuth2Login {
    App &app;
    cb::JSON::ValuePtr config;
    cb::JSON::ValuePtr fields;
    unsigned currentField;
    cb::SmartPointer<cb::MariaDB::EventDB> db;
    cb::SmartPointer<cb::JSON::Writer> writer;

  public:
    Transaction(App &app, evhttp_request *req);

    void setSessionCookie();
    bool lookupSession();

    void setFields(const cb::JSON::ValuePtr &fields);

    typedef typename cb::MariaDB::EventDBMemberFunctor<Transaction>::member_t
    event_db_member_functor_t;
    void query(event_db_member_functor_t member, const std::string &s,
               const cb::JSON::Value &dict);

    // From cb::Event::Request
    cb::SmartPointer<cb::JSON::Writer> getJSONWriter();
    void sendError(int code, const std::string &msg);
    void sendJSONError(int code = HTTP_INTERNAL_SERVER_ERROR,
                       const std::string &msg = "");
    void finalize();

    // From cb::Event::OAuth2Login
    void processProfile(const cb::JSON::ValuePtr &profile);

    // Event::WebServer request callbacks
    bool apiLogin(const cb::JSON::ValuePtr &config);
    bool apiLogout(const cb::JSON::ValuePtr &config);

    // MariaDB::EventDB callbacks
    void session(cb::MariaDB::EventDBCallback::state_t state);
    void login(cb::MariaDB::EventDBCallback::state_t state =
               cb::MariaDB::EventDBCallback::EVENTDB_DONE);
    void logout(cb::MariaDB::EventDBCallback::state_t state =
                cb::MariaDB::EventDBCallback::EVENTDB_DONE);

    void returnHeadList(cb::MariaDB::EventDBCallback::state_t state);
    void returnList(cb::MariaDB::EventDBCallback::state_t state);
    void returnBool(cb::MariaDB::EventDBCallback::state_t state);
    void returnU64(cb::MariaDB::EventDBCallback::state_t state);
    void returnS64(cb::MariaDB::EventDBCallback::state_t state);
    void returnFields(cb::MariaDB::EventDBCallback::state_t state);
    void returnJSON(cb::MariaDB::EventDBCallback::state_t state);
    void returnOk(cb::MariaDB::EventDBCallback::state_t state);
  };
}
