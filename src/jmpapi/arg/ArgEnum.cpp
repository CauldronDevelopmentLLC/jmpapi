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

#include "ArgEnum.h"

using namespace JmpAPI;
using namespace cb;
using namespace std;


ArgEnum::ArgEnum(const JSON::ValuePtr &config) :
  caseSensitive(config->getBoolean("case-sensitive", false)) {
  auto list = config->get("enum");

  for (unsigned i = 0; i < list->size(); i++) {
    const string &value = list->getString(i);
    values.insert(caseSensitive ? value : String::toLower(value));
  }
}


void ArgEnum::operator()(Event::Request &req, JSON::Value &_value) const {
  if (!_value.isString()) THROW("Enum argument must be string");

  string value = _value.getString();
  if (caseSensitive) value = String::toLower(value);

  if (values.find(value) == values.end())
    THROW("Must be one of: " << String::join(values, ", "));
}
