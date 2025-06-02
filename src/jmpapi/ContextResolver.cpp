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

#include "ContextResolver.h"

using namespace std;
using namespace cb;
using namespace JmpAPI;

ContextResolver::ContextResolver(
  const API::ResolverPtr &parent, const JSON::ValuePtr &ctx) :
  parent(parent), ctx(ctx) {

  if (parent.isInstance<ContextResolver>())
    root = parent.cast<ContextResolver>()->root;
  else root = parent;
}


JSON::ValuePtr ContextResolver::select(const string &name) const {
  if (name == ".") return ctx;

  if (name == "..") return parent->select(".");
  if (String::startsWith(name, "../")) {
    if (parent.isSet()) return parent->select(name.substr(3));
    return 0;
  }

  if (String::startsWith(name, "./")) return ctx->select(name.substr(2), 0);

  auto result = root->select(name);
  return result.isSet() ? result : ctx->select(name, 0);
}
