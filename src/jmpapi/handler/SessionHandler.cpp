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

#include "SessionHandler.h"

#include <jmpapi/App.h>
#include <jmpapi/Transaction.h>

#include <cbang/log/Logger.h>

using namespace JmpAPI;
using namespace cb;
using namespace std;


SessionHandler::SessionHandler(const JSON::Value &config) :
  sql(config.getString("sql")) {}


bool SessionHandler::operator()(Event::Request &req) {
  // Check if Session is already loaded
  if (req.getSession().isSet()) return false;

  // Check if we have a session ID
  auto &tran = req.cast<Transaction>();
  auto &app  = tran.getApp();
  string sid = req.getSessionID(app.getSessionCookieName());
  if (sid.empty()) return false;

  // Lookup Session in SessionManager
  try {
    req.setSession(app.getSessionManager().lookupSession(sid));
    LOG_DEBUG(3, "User: " << req.getUser());

    return false;
  } catch (const Exception &) {}

  // Lookup Session in DB
  if (sql.empty()) return false;
  req.setSession(new Session(sid, req.getClientIP()));
  tran.query(&Transaction::session, sql);

  return true;
}
