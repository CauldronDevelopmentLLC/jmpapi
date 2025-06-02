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

#include "Template.h"

#include "SimpleWithTmpl.h"
#include "WithTmpl.h"
#include "LiteralTmpl.h"
#include "DictTmpl.h"
#include "EachTmpl.h"
#include "RequestTmpl.h"
#include "IfTmpl.h"
#include "StatusTmpl.h"
#include "DebugTmpl.h"
#include "OnTmpl.h"
#include "AndTmpl.h"
#include "OrTmpl.h"
#include "NotTmpl.h"
#include "EqualTmpl.h"

using namespace std;
using namespace cb;
using namespace JmpAPI;


SmartPointer<Template> Template::parse(const JSON::ValuePtr &tmpl) {
  if (tmpl.isNull()) return 0;
  if (!tmpl->isDict() && !tmpl->isList())
    return new SimpleWithTmpl(api, tmpl->asString(), 0);

  SmartPointer<Template> child;
  auto set =
    [&] (const SmartPointer<Template> &c) {
      if (child.isSet())
        THROW("Invalid template, can only have one of 'literal', 'dict', "
              "'list', 'request', 'on', 'and', 'or', 'not', 'equal' or 'if': "
              << *tmpl);
      child = c;
    };

  if (tmpl->has("literal"))  set(new LiteralTmpl(api, tmpl->get("literal")));
  if (tmpl->hasDict("dict")) set(new DictTmpl   (api, tmpl->get("dict")));
  if (tmpl->hasDict("each")) set(new EachTmpl   (api, tmpl->get("each")));
  if (tmpl->has("request"))  set(new RequestTmpl(api, tmpl->get("request")));
  if (tmpl->has("and"))      set(new AndTmpl    (api, tmpl->get("and")));
  if (tmpl->has("or"))       set(new OrTmpl     (api, tmpl->get("or")));
  if (tmpl->has("not"))      set(new NotTmpl    (api, tmpl->get("not")));
  if (tmpl->has("equal"))    set(new EqualTmpl  (api, tmpl->get("equal")));
  if (tmpl->has("if"))
    set(new IfTmpl(api, parse(tmpl->get("if")), parse(tmpl->get("then", 0)),
                   parse(tmpl->get("else", 0))));

  if (tmpl->has("debug"))
    child = new DebugTmpl (api, tmpl->get("debug"),  child);
  if (tmpl->has("with"))
    child = new WithTmpl  (api, tmpl->get("with"),   child);
  if (tmpl->has("status"))
    child = new StatusTmpl(api, tmpl->get("status"), child);
  if (tmpl->has("on"))
    child = new OnTmpl    (api, tmpl->get("on"),     child);

  return child;
}
