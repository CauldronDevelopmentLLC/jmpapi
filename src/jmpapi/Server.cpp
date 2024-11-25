/******************************************************************************\

                           This file is part of JmpAPI.

                Copyright (c) 2014-2024, Cauldron Development LLC
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

#include "Server.h"
#include "App.h"

#include <cbang/api/handler/RedirectHandler.h>
#include <cbang/http/FileHandler.h>
#include <cbang/http/IndexHandler.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/json/Value.h>
#include <cbang/log/Logger.h>

using namespace cb;
using namespace std;
using namespace JmpAPI;



Server::Server(App &app) :
  HTTP::Server(app.getEventBase(), PhonyPtr(&app.getSSLContext())), app(app) {
  addOptions(app.getOptions());
  setAutoIndex(false);
}


void Server::init() {
  HTTP::Server::init(app.getOptions());
  addHandler(PhonyPtr(&app.getAPI()));
  addHandler(new cb::API::StatusHandler(HTTP_NOT_FOUND));
}
