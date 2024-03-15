/******************************************************************************\

                           This file is part of JmpAPI.

                Copyright (c) 2014-2023, Cauldron Development LLC
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

#include "ArgValidator.h"

#include <cbang/http/Enum.h>

#include <map>

namespace JmpAPI {
  class ArgDict : public ArgConstraint, public cb::HTTP::Enum {
    std::map<std::string, cb::SmartPointer<ArgValidator> > validators;

  public:
    ArgDict(const cb::JSON::ValuePtr &args);

    // From ArgConstraint
    void operator()(cb::HTTP::Request &req, cb::JSON::Value &value) const;
  };
}
