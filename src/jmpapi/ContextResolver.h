/******************************************************************************\

                           This file is part of JmpAPI.

                 Copyright (c) 2014-2025, Cauldron Development Oy
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


namespace JmpAPI {
  class ContextResolver : public cb::API::Resolver {
    cb::API::ResolverPtr root;
    cb::API::ResolverPtr parent;
    cb::JSON::ValuePtr ctx;

  public:
    ContextResolver(
      const cb::API::ResolverPtr &parent, const cb::JSON::ValuePtr &ctx);

    // From cb::API::Resolver
    cb::JSON::ValuePtr select(const std::string &name) const override;
  };
}
