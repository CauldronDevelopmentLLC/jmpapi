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

#include "EachTmpl.h"

#include <cbang/json/List.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


EachTmpl::EachTmpl(const JSON::ValuePtr &tmpl) :
  child(Template::parse(tmpl)) {
  if (tmpl->has("flatten")) flatten = tmpl->get("flatten")->toBoolean();
}


void EachTmpl::apply(const API::ResolverPtr &resolver, cb_t done) {
  auto ctx = resolver->getContext();
  if (ctx.isNull() || !ctx->size()) return done(HTTP_OK, 0);

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
    });

  // Populate list
  for (unsigned i = 0; i < ctx->size(); i++)
    result->data->appendUndefined();

  for (unsigned i = 0; i < ctx->size(); i++) {
    auto cb =
      [this, result, done, i] (HTTP::Status status,
                               const JSON::ValuePtr &data) {
        if (!result->pass) return;

        if (status != HTTP_NOT_FOUND) {
          if (status != HTTP_OK) {
            result->pass = false;
            done(status, data);
            return;
          }

          if (data.isSet()) result->data->set(i, data);
        }

        if (!--result->count) {
          JSON::ValuePtr list = new JSON::List;

          for (unsigned j = 0; j < result->data->size(); j++) {
            auto item = result->data->get(j);

            if (flatten && item->isList())
              for (unsigned k = 0; k < item->size(); k++)
                list->append(item->get(k));

            else if (!item->isUndefined()) list->append(item);
          }

          done(HTTP_OK, list->empty() ? 0 : list);
        }
      };

    child->apply(resolver->makeChild(ctx->get(i)), cb);
  }
}
