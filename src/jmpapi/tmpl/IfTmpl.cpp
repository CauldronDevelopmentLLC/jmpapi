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

#include "IfTmpl.h"

using namespace std;
using namespace cb;
using namespace JmpAPI;


IfTmpl::IfTmpl(const SmartPointer<Template> &ifTmpl,
               const SmartPointer<Template> &thenTmpl,
               const SmartPointer<Template> &elseTmpl) :
  ifTmpl(ifTmpl), thenTmpl(thenTmpl), elseTmpl(elseTmpl) {}


void IfTmpl::apply(const API::ResolverPtr &resolver, cb_t done) {
  auto cb =
    [this, resolver, done] (HTTP::Status status, const JSON::ValuePtr &data) {
      if (status == HTTP_OK && data.isSet() && data->toBoolean()) {
        if (thenTmpl.isSet()) thenTmpl->apply(resolver, done);
        else done(HTTP_OK, resolver->getContext());

      } else {
        if (elseTmpl.isSet()) elseTmpl->apply(resolver, done);
        else done(HTTP_NOT_FOUND, 0);
      }
    };

  ifTmpl->apply(resolver, cb);
}
