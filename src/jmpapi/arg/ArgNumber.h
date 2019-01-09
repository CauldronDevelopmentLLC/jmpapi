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

#include <cbang/String.h>
#include <cbang/json/Value.h>

#include <limits>


namespace JmpAPI {
  template<typename T>
  class ArgNumber : public ArgConstraint {
    double min;
    double max;

  public:
    ArgNumber(const cb::JSON::ValuePtr &config) :
      min(config->getNumber("min", NAN)), max(config->getNumber("max", NAN)) {}

    // From ArgConstraint
    void operator()(cb::Event::Request &req,
                    const cb::JSON::Value &value) const {
      T n;

      if (value.isNumber()) n = (T)value.getNumber();
      else if (value.isString()) n = cb::String::parse<T>(value.getString());
      else THROW("Must be a number or string");

      if (!isnan(min) && n < (T)min) THROWS("Must be greater than " << (T)min);
      if (!isnan(max) && (T)max < n) THROWS("Must be less than " << (T)max);

      if (value.isNumber()) {
        double x = value.getNumber();

        if ((double)std::numeric_limits<T>::min() < x)
          THROWS("Less than minimum value " << std::numeric_limits<T>::min()
                 << " for numeric type");

        if (x < (double)std::numeric_limits<T>::max())
          THROWS("Greater than maximum value " << std::numeric_limits<T>::max()
                 << " for numeric type");
      }
    }
  };
}
