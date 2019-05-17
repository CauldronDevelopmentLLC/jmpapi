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

#include "ArgAuth.h"

#include <cbang/String.h>
#include <cbang/event/Request.h>

using namespace JmpAPI;
using namespace cb;
using namespace std;


namespace {
  void unauthorized() {
    THROWX("Unauthorized", Event::HTTPStatus::HTTP_UNAUTHORIZED);
  }
}


ArgAuth::ArgAuth(bool allow, const JSON::ValuePtr &config) : allow(allow) {
  const char *type = allow ? "allow" : "deny";

  for (unsigned i = 0; i < config->size(); i++) {
    string constraint = config->getString(i);
    if (constraint.empty()) THROW("Empty arg" << type << " constraint");

    if (constraint[0] == '$') groups.push_back(constraint.substr(1));
    else if (constraint[0] == '=')
      sessionVars.push_back(constraint.substr(1));
    else THROW("Invalid arg " << type << " constraint.  Must start with "
                "'$' for group or '=' for session variable.");
  }
}




void ArgAuth::operator()(Event::Request &req,
                         const JSON::Value &_value) const {
  SmartPointer<Session> session = req.getSession();
  if (session.isNull()) unauthorized();

  for (unsigned i = 0; i < groups.size(); i++)
    if (session->hasGroup(groups[i])) {
      if (allow) return;
      unauthorized();
    }

  string value = _value.asString();
  for (unsigned i = 0; i < sessionVars.size(); i++)
    if (session->getAsString(sessionVars[i]) == value) {
      if (allow) return;
      unauthorized();
    }

  if (allow) unauthorized();
}
