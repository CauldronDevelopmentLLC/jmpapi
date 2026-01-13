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

#include "StatusTmpl.h"

using namespace std;
using namespace cb;
using namespace JmpAPI;


StatusTmpl::StatusTmpl(API &api, const JSON::ValuePtr &config,
  const SmartPointer<Template> child) : Template(api), child(child) {
  if (config->isString())
    status = HTTP::Status::parse(config->getString());

  else if (config->isNumber())
    status = (HTTP::Status::enum_t)config->getNumber();

  else THROW("Invalid template: " << *config);
}


void StatusTmpl::apply(const cb::API::ResolverPtr &resolver, cb_t done) {
  auto cb =
    [this, done] (HTTP::Status, const JSON::ValuePtr &data) {
      done(status, data);
    };

  child->apply(resolver, cb);
}
