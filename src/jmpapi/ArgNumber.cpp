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

#include "ArgNumber.h"

#include <cbang/Math.h>


using namespace JmpAPI;
using namespace cb;
using namespace std;


ArgNumber::ArgNumber(const JSON::ValuePtr &config) :
  min(config->getNumber("min", NAN)), max(config->getNumber("max", NAN)) {}


void ArgNumber::operator()(const string &value) const {
  double n = String::parseDouble(value);
  if (!isnan(min) && n < min) THROWS("Must be greater than " << min);
  if (!isnan(max) && max < n) THROWS("Must be less than " << max);
}