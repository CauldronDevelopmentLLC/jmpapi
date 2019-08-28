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

#include "Template.h"

#include "ContextTmpl.h"
#include "LiteralTmpl.h"
#include "DictTmpl.h"
#include "ListTmpl.h"
#include "ProxyTmpl.h"
#include "ConditionTmpl.h"

using namespace std;
using namespace cb;
using namespace JmpAPI;


SmartPointer<Template> Template::parse(const JSON::ValuePtr &tmpl) {
  if (tmpl.isNull()) return 0;
  if (tmpl->isString()) return new ContextTmpl(tmpl->getString(), 0);

  SmartPointer<Template> child;
  if (tmpl->has("literal"))       child = new LiteralTmpl(tmpl->get("literal"));
  else if (tmpl->hasDict("dict")) child = new DictTmpl(tmpl->get("dict"));
  else if (tmpl->hasDict("list")) child = new ListTmpl(tmpl->get("list"));
  else if (tmpl->has("url"))      child = new ProxyTmpl(tmpl);

  if (tmpl->hasString("context"))
    child = new ContextTmpl(tmpl->getString("context"), child);

  if (tmpl->has("condition"))
    child = new ConditionTmpl(tmpl->get("condition"), child);

  if (!child.isSet()) THROW("Invalid template: " << *tmpl);

  return child;
}
