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
#include <cbang/http/Request.h>

#include <functional>


namespace JmpAPI {
  class Resolver;
  typedef cb::SmartPointer<Resolver> ResolverPtr;
  typedef cb::SmartPointer<cb::HTTP::Request> RequestPtr;


  class Resolver : virtual public cb::RefCounted {
    RequestPtr req;
    cb::JSON::ValuePtr ctx;
    ResolverPtr parent;

    Resolver(const cb::JSON::ValuePtr &ctx, const ResolverPtr &parent);

  public:
    Resolver(const RequestPtr &req);
    virtual ~Resolver() {}

    Resolver &getRoot();
    RequestPtr getRequest() const {return req;}
    const cb::JSON::ValuePtr &getContext() const {return ctx;}
    const cb::JSON::ValuePtr &getArgs() const;

    ResolverPtr makeChild(const cb::JSON::ValuePtr &ctx);

    virtual cb::JSON::ValuePtr select(const std::string &name) const;
    std::string format(const std::string &s,
                       cb::String::format_cb_t cb = 0) const;
    std::string format(const std::string &s,
                       const std::string &defaultValue) const;
    void resolve(cb::JSON::Value &value) const;
  };
}
