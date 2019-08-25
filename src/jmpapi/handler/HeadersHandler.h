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

#include <jmpapi/Headers.h>

#include <cbang/event/HTTPRequestHandler.h>


namespace JmpAPI {
  class HeadersHandler : public cb::Event::HTTPRequestHandler, public Headers {
  public:
    HeadersHandler() {}
    HeadersHandler(const cb::JSON::ValuePtr &hdrs) : Headers(hdrs) {}

    // From cb::Event::HTTPRequestHandler
    bool operator()(cb::Event::Request &req) {
      set(req);
      return false;
    }
  };
}
