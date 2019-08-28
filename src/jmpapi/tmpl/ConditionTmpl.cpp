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

#include "ConditionTmpl.h"

#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


ConditionTmpl::ConditionTmpl(const JSON::ValuePtr &config,
                             const SmartPointer<Template> child) :
  child(child) {
  if (config->isString()) ctx = config->getString();
  else if (config->hasString("context")) {
    ctx = config->getString("context");
    if (config->has("value")) value = config->get("value");
    // TODO support comparison operators: != < <= >= contains

  } else THROW("Invalid condition template: " << *config);
}


void ConditionTmpl::apply(const ResolverPtr &resolver, cb_t done) {
  JSON::ValuePtr result = resolver->select(ctx);
  bool match = false;

  if (result.isSet()) {
    // TODO improve comparison
    if (value.isSet()) match = value->asString() == result->asString();
    else if (result->toBoolean()) match = true;
  }

  if (!match) return done(true, resolver->getContext()->createUndefined());

  if (child.isSet()) child->apply(resolver, done);
  else done(true, resolver->getContext());
}