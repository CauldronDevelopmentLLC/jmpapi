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

#include "ListTmpl.h"

#include <cbang/json/List.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


ListTmpl::ListTmpl(const JSON::ValuePtr &tmpl) :
  child(Template::parse(tmpl)) {}


void ListTmpl::apply(const ResolverPtr &resolver, cb_t done) {
  auto ctx = resolver->getContext();
  if (!ctx->isList() || !ctx->size()) return done(true, new JSON::List);

  struct Result {
    unsigned count;
    JSON::ValuePtr data;
    bool pass;
    cb_t done;
  };

  SmartPointer<Result> result = new Result({
      .count = ctx->size(),
      .data = new JSON::List,
      .pass = true,
      .done = done,
    });

  // Populate list
  for (unsigned i = 0; i < ctx->size(); i++)
    result->data->appendUndefined();

  for (unsigned i = 0; i < ctx->size(); i++) {
    auto child_cb =
      [result, i] (bool ok, const JSON::ValuePtr &data) {
        if (!ok) result->pass = false;
        if (data.isSet()) result->data->set(i, data);
        if (!--result->count) result->done(result->pass, result->data);
      };

    child->apply(resolver->makeChild(ctx->get(i)), child_cb);
  }
}
