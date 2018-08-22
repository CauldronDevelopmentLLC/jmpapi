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

#pragma once

#include "ArgConstraint.h"

#include <cbang/json/Value.h>

#include <string>


namespace JmpAPI {
  class ArgValidator {
    bool optional;
    bool defaultSet;
    std::string defaultValue;
    std::vector<cb::SmartPointer<ArgConstraint> > constraints;

  public:
    ArgValidator() : optional(false), defaultSet(false) {}
    ArgValidator(const cb::JSON::ValuePtr &config);

    bool isOptional() const {return optional;}
    bool hasDefault() const {return defaultSet;}
    const std::string &getDefault() const {return defaultValue;}

    void operator()(const std::string &value) const;
  };
}
