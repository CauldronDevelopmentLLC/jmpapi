/******************************************************************************\

                          This file is part of JmpAPI.

               Copyright (c) 2014-2018, Cauldron Development LLC
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

       In addition, BSD licensing may be granted on a case by case basis
       by written permission from at least one of the copyright holders.
          You may request written permission by emailing the authors.

                 For information regarding this software email:
                                Joseph Coffland
                         joseph@cauldrondevelopment.com

\******************************************************************************/

#include "ArgValidator.h"

#include "ArgPattern.h"
#include "ArgEnum.h"
#include "ArgNumber.h"


using namespace JmpAPI;
using namespace cb;
using namespace std;


ArgValidator::ArgValidator(const JSON::ValuePtr &config) :
  optional(config->getBoolean("optional", false)) {

  if (config->has("default")) defaultValue = config->get("default");

  string type = config->getString("type", "");

  // Implicit type
  if (type.empty()) {
    if (config->has("enum")) type = "enum";
    else if (config->has("min") || config->has("max")) type = "number";
    else type = "string";
  }

  if (type == "enum") constraints.push_back(new ArgEnum(config->get("enum")));
  else if (type == "number") constraints.push_back(new ArgNumber(config));

  if (config->has("pattern"))
    constraints.push_back(new ArgPattern(config->getString("pattern")));
}


void ArgValidator::operator()(const string &value) const {
  for (unsigned i = 0; i < constraints.size(); i++)
    (*constraints[i])(value);
}
