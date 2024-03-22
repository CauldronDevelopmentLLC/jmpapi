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

#include "EqualTmpl.h"

#include <cbang/json/True.h>
#include <cbang/json/False.h>

#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


EqualTmpl::EqualTmpl(const JSON::ValuePtr &config) {
  if (!config->isList() || !config->size())
    THROW("Invalid 'equal' template, must be an non-empty list");

  for (unsigned i = 0; i < config->size(); i++) {
    auto child = parse(config->get(i));
    if (child.isNull()) THROW("Empty child in 'equal': " << *config);
    children.push_back(child);
  }
}


void EqualTmpl::apply(const API::ResolverPtr &resolver, cb_t done) {
  struct Result {
    size_t total;
    size_t count;
    bool pass;
    string value;
  };

  SmartPointer<Result> result = new Result({
      .total = children.size(),
      .count = children.size(),
      .pass = true,
    });

  auto cb =
    [this, resolver, done, result] (HTTP::Status status,
                                    const JSON::ValuePtr &data) {
      if (!result->pass) return;

      string value = data.isSet() ? data->asString() : string();

      if (result->count == result->total) result->value = value;
      else if (result->value != value) {
        result->pass = false;
        return done(HTTP_OK, JSON::False::instancePtr());
      }

      if (!--result->count)
        done(HTTP_OK, JSON::True::instancePtr());
    };

  for (unsigned i = 0; i < children.size(); i++)
    children[i]->apply(resolver, cb);
}
