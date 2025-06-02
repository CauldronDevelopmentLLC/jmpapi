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

#pragma once

#include <cbang/api/Resolver.h>

#include <cbang/json/Value.h>
#include <cbang/http/Enum.h>

#include <functional>


namespace JmpAPI {
  class API;

  class Template : public cb::HTTP::Enum {
    API &api;

  public:
    Template(API &api) : api(api) {}
    virtual ~Template() {}

    API &getAPI() const {return api;}

    typedef std::function<void (
    cb::HTTP::Status status, const cb::JSON::ValuePtr &data)> cb_t;

    virtual void apply(const cb::API::ResolverPtr &resolver, cb_t done) = 0;

    cb::SmartPointer<Template> parse(const cb::JSON::ValuePtr &tmpl);
  };
}
