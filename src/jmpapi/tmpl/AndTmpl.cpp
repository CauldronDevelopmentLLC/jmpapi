/******************************************************************************\

                           This file is part of JmpAPI.

                 Copyright (c) 2014-2026, Cauldron Development Oy
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

#include "AndTmpl.h"

#include <cbang/json/True.h>
#include <cbang/json/False.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


AndTmpl::AndTmpl(API &api, const JSON::ValuePtr &config) : Template(api) {
  if (!config->isList() || !config->size())
    THROW("Invalid 'and' template, must be an non-empty list");

  for (auto item: *config)
    children.push_back(parse(item));
}


void AndTmpl::apply(const cb::API::ResolverPtr &resolver, cb_t done) {
  SmartPointer<size_t> count = new size_t(children.size());

  auto cb =
    [this, resolver, done, count] (
      HTTP::Status status, const JSON::ValuePtr &data) {
      if (!*count) return;

      if (status != HTTP_OK || !data.isSet() || !data->toBoolean()) {
        done(HTTP_OK, JSON::False::instancePtr());
        *count = 0;

      } else if (!--*count) done(HTTP_OK, JSON::True::instancePtr());
    };

  for (unsigned i = 0; i < children.size(); i++)
    children[i]->apply(resolver, cb);
}
