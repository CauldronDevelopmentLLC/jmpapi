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

#include <cbang/String.h>
#include <cbang/json/Dict.h>
#include <cbang/event/Request.h>

#include <functional>


namespace JmpAPI {
  class Resolver;
  typedef cb::SmartPointer<Resolver> ResolverPtr;


  class Resolver : public cb::JSON::Dict {
    ResolverPtr parent;
    cb::Event::Request *req = 0;
    cb::JSON::ValuePtr ctx;

  public:
    Resolver() {}
    Resolver(const std::string &key, const cb::JSON::ValuePtr &value);
    Resolver(cb::Event::Request &req);
    Resolver(const ResolverPtr &parent, const cb::JSON::ValuePtr &ctx);

    cb::Event::Request *getRequest() {return req;}
    const cb::JSON::ValuePtr &getContext() {return ctx;}

    ResolverPtr makeChild(const cb::JSON::ValuePtr &ctx);

    cb::JSON::ValuePtr select(const std::string &name);
    std::string format(const std::string &s, cb::String::format_cb_t cb = 0);
    std::string format(const std::string &s, const std::string &defaultValue);
    void resolve(cb::JSON::Value &value);
  };
}
