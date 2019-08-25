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
#include "Resolver.h"

#include <jmpapi/tmpl/Template.h>

#include <cbang/json/Value.h>
#include <cbang/event/Enum.h>

#include <functional>


namespace JmpAPI {
  class App;

  class ProxyRequest : public cb::Event::Enum {
    std::string url;
    cb::Event::RequestMethod method;
    Headers headers;
    cb::SmartPointer<Template> data;
    cb::SmartPointer<Template> tmpl;

  public:
    ProxyRequest(const cb::JSON::Value &config);

    typedef std::function<void (cb::Event::HTTPStatus status,
                                const cb::JSON::ValuePtr &data)> done_cb_t;

    void request(const ResolverPtr &resolver, done_cb_t done);
  };
}
