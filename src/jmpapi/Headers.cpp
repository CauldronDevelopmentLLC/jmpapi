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

#include "Headers.h"

#include <cbang/String.h>
#include <cbang/event/Request.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


Headers::Headers(const JSON::ValuePtr &hdrs) {
  if (hdrs.isSet())
    for (unsigned i = 0; i < hdrs->size(); i++)
      add(hdrs->keyAt(i), hdrs->getString(i));
}


void Headers::add(const string &key, const string &value) {
  push_back(pair<string, string>(String::trim(key), String::trim(value)));
}


void Headers::set(Event::Request &req) {
  for (auto it = begin(); it != end(); it++)
    req.outSet(it->first, it->second);
}
