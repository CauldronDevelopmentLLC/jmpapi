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

#pragma once

#include "Headers.h"

#include <cbang/event/Request.h>
#include <cbang/event/OAuth2Login.h>
#include <cbang/db/maria/EventDB.h>
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
    std::string nextField;
    bool closeField;

    unsigned result = 0;

    cb::SmartPointer<cb::MariaDB::EventDB> db;
    cb::SmartPointer<cb::JSON::Writer> writer;

  public:
    Transaction(App &app, cb::Event::RequestMethod method, const cb::URI &uri,
                const cb::Version &version);

    App &getApp() {return app;}

    void setSessionCookie();
    void setFields(const cb::JSON::ValuePtr &fields);

    typedef typename cb::MariaDB::EventDB::Callback<Transaction>::member_t
    event_db_member_functor_t;
    void query(event_db_member_functor_t member, const std::string &s);

    // From cb::Event::Request
    cb::SmartPointer<cb::JSON::Writer> getJSONWriter();
    void sendError(cb::Event::HTTPStatus code, const std::string &msg);
    void sendJSONError(cb::Event::HTTPStatus code = HTTP_INTERNAL_SERVER_ERROR,
                       const std::string &msg = "");
    void write();

    // From cb::Event::OAuth2Login
    void processProfile(cb::Event::Request &req,
                        const cb::JSON::ValuePtr &profile);

    // Event::WebServer request callbacks
    bool apiLogin (const cb::JSON::ValuePtr &config);
    bool apiLogout(const cb::JSON::ValuePtr &config);

    // MariaDB::EventDB callbacks
    typedef cb::MariaDB::EventDB::state_t state_t;
    void session(state_t state);
    void login  (state_t state = cb::MariaDB::EventDB::EVENTDB_DONE);
    void logout (state_t state = cb::MariaDB::EventDB::EVENTDB_DONE);

    void returnHList (state_t state);
    void returnList  (state_t state);
    void returnBool  (state_t state);
    void returnU64   (state_t state);
    void returnS64   (state_t state);
    void returnFields(state_t state);
    void returnDict  (state_t state);
    void returnOne   (state_t state);
    void returnOk    (state_t state);
    void returnPass  (state_t state);
  };
}
