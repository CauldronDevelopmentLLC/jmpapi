/******************************************************************************\

                           This file is part of JmpAPI.

                Copyright (c) 2014-2023, Cauldron Development LLC
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

#include "ArgFilterHandler.h"
#include "ArgFilterProcess.h"

#include <jmpapi/App.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


ArgFilterHandler::ArgFilterHandler(
  App &app, const JSON::Value &config,
  SmartPointer<HTTP::RequestHandler> &child) : app(app), child(child) {

  if (config.isString()) Subprocess::parse(config.toString(), cmd);
  else {
    if (!config.isList()) THROW("Invalid arg-filter config");

    for (unsigned i = 0; i < config.size(); i++)
      cmd.push_back(config.getAsString(i));
  }
}


bool ArgFilterHandler::operator()(HTTP::Request &req) {
  app.getProcPool().enqueue(
    new ArgFilterProcess(app.getEventBase(), child, cmd, req));

  return true;
}
