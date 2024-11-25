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

#include "API.h"
#include "ProxyHandler.h"

using namespace std;
using namespace cb;
using namespace JmpAPI;


string JmpAPI::API::getEndpointType(const JSON::ValuePtr &config) const {
  string type = config->getString("handler", "");
  if (type.empty() && config->has("url")) return "proxy";
  return cb::API::API::getEndpointType(config);
}


HTTP::RequestHandlerPtr JmpAPI::API::createEndpointHandler(
  const JSON::ValuePtr &types, const JSON::ValuePtr &config) {
  if (types->asString() == "proxy") return new ProxyHandler(*this, config);
  return cb::API::API::createEndpointHandler(types, config);
}
