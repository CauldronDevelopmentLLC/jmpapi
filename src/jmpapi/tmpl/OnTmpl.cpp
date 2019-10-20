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

#include "OnTmpl.h"

#include <cbang/json/True.h>
#include <cbang/json/False.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


OnTmpl::OnTmpl(const JSON::ValuePtr &config,
               const SmartPointer<Template> &child) : child(child) {
  if (config->isString() || config->isNumber()) add(*config);

  else if (config->isList()) {
    for (unsigned i = 0; i < config->size(); i++)
      add(*config->get(i));

  } else THROW("Invalid template: " << *config);
}


void OnTmpl::add(const JSON::Value &status) {
  if (status.isString())
    on.insert(Event::HTTPStatus::parse(status.getString()));

  else if (status.isNumber())
    on.insert((Event::HTTPStatus::enum_t)status.getNumber());

  else THROW("Invalid 'on' status: " << status);
}


void OnTmpl::apply(const ResolverPtr &resolver, cb_t done) {
  auto cb =
    [this, done] (Event::HTTPStatus status, const JSON::ValuePtr &data) {
      if (on.find(status) == on.end())
        done(HTTP_OK, JSON::False::instancePtr());
      else done(HTTP_OK, JSON::True::instancePtr());
    };

  child->apply(resolver, cb);
}
