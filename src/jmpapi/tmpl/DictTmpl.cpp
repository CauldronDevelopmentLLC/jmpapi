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

#include "DictTmpl.h"

#include <cbang/json/Dict.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


DictTmpl::DictTmpl(const JSON::ValuePtr &tmpl) {
  for (unsigned i = 0; i < tmpl->size(); i++)
    fields.push_back(field_t(tmpl->keyAt(i), Template::parse(tmpl->get(i))));
}


void DictTmpl::apply(const ResolverPtr &resolver, cb_t done) {
  // Handle empty template
  if (fields.empty()) return done(true, new JSON::Dict);

  struct Result {
    unsigned long count;
    JSON::ValuePtr data;
    bool pass;
    cb_t done;
  };

  SmartPointer<Result> result = new Result({
      .count = fields.size(),
      .data = new JSON::Dict,
      .pass = true,
      .done = done,
    });

  // Establish correct sort order
  for (unsigned i = 0; i < fields.size(); i++)
    result->data->insertUndefined(fields[i].first);

  for (unsigned i = 0; i < fields.size(); i++) {
    string key = fields[i].first;

    auto child_cb =
      [result, key] (bool ok, const JSON::ValuePtr &data) {
        if (!ok) result->pass = false;
        result->data->insert(key, data);
        if (!--result->count) result->done(result->pass, result->data);
      };

    fields[i].second->apply(resolver, child_cb);
  }
}
