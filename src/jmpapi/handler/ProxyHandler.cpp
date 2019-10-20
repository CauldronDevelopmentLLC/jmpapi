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

#include "ProxyHandler.h"

#include <jmpapi/Resolver.h>
#include <jmpapi/tmpl/RequestTmpl.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


ProxyHandler::ProxyHandler(const JSON::ValuePtr &config) :
  tmpl(new RequestTmpl(config)) {}


bool ProxyHandler::operator()(Event::Request &req) {
  auto cb =
    [&req] (Event::HTTPStatus status, const JSON::ValuePtr &data) {
      if (data.isSet()) {
        if (!req.outHas("Content-Type"))
          req.outSet("Content-Type", "application/json");
        req.send(data->toString());
      }

      req.reply(status);
    };

  tmpl->apply(new Resolver(&req), cb);

  return true;
}
