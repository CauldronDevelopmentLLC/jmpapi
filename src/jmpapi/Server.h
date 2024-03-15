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

#include <cbang/http/WebServer.h>
#include <cbang/json/Value.h>


namespace JmpAPI {
  class App;
  class User;

  class Server : public cb::HTTP::WebServer,
                 public cb::HTTP::HandlerFactory {
    App &app;

  public:
    Server(App &app);

    typedef cb::SmartPointer<cb::HTTP::RequestHandler> RequestHandlerPtr;

    RequestHandlerPtr createAccessHandler(const cb::JSON::Value &config);
    RequestHandlerPtr createEndpoint(const cb::JSON::ValuePtr &config);
    RequestHandlerPtr createValidationHandler(const cb::JSON::Value &config);
    RequestHandlerPtr createAPIHandler(
      const std::string &pattern, const cb::JSON::Value &config,
      const RequestHandlerPtr &parentValidation = 0);
    void loadCategory(const std::string &name, const cb::JSON::Value &cat);
    void loadCategories(const cb::JSON::Value &cats);

    void init();

    // From cb::Event::HTTPHandler
    cb::SmartPointer<cb::HTTP::Request> createRequest(
      const cb::SmartPointer<cb::HTTP::Conn> &connection,
      cb::HTTP::Method method, const cb::URI &uri,
      const cb::Version &version);
  };
}
