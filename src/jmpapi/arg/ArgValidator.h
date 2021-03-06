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

#pragma once

#include "ArgConstraint.h"

#include <cbang/Math.h>
#include <cbang/json/Value.h>

namespace cb {namespace Event {class Request;}}


namespace JmpAPI {
  class ArgValidator {
    bool optional;
    cb::JSON::ValuePtr defaultValue;
    std::vector<cb::SmartPointer<ArgConstraint> > constraints;

  public:
    ArgValidator() : optional(false) {}
    ArgValidator(const cb::JSON::ValuePtr &config);

    bool isOptional() const {return optional;}
    bool hasDefault() const {return !defaultValue.isNull();}
    const cb::JSON::ValuePtr &getDefault() const {return defaultValue;}

    void add(const cb::SmartPointer<ArgConstraint> &constraint);

    void operator()(cb::Event::Request &req,
                    const cb::JSON::Value &value) const;
  };
}
