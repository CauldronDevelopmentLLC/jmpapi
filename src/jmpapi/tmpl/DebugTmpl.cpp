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

#include "DebugTmpl.h"

#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


namespace {
  void log(const string &prefix, Event::HTTPStatus status,
           const JSON::ValuePtr &data) {
    if (data.isSet())
      LOG_DEBUG(3, prefix << "STATUS:" << status << " DATA:" << *data);
    else LOG_DEBUG(3, prefix << "STATUS:" << status << " DATA: <null>");
  }
}


DebugTmpl::DebugTmpl(const JSON::ValuePtr &config,
                     const SmartPointer<Template> child) :
  ctx(Template::parse(config)), child(child) {}


void DebugTmpl::apply(const ResolverPtr &resolver, cb_t _done) {
  auto done =
    [this, resolver, _done] (Event::HTTPStatus status,
                            const JSON::ValuePtr &data) {
      log("< ", status, data);
      _done(status, data);
    };

  auto cb =
    [this, resolver, done] (Event::HTTPStatus status,
                            const JSON::ValuePtr &data) {
      log("> ", status, data);

      if (child.isSet()) child->apply(resolver, done);
      else done(status, resolver->getContext());
    };

  ctx->apply(resolver, cb);
}
