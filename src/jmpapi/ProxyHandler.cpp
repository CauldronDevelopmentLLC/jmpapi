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

#include "ProxyHandler.h"
#include "API.h"

#include <jmpapi/tmpl/RequestTmpl.h>
#include <jmpapi/tmpl/Template.h>

#include <cbang/api/Resolver.h>
#include <cbang/json/Value.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


ProxyHandler::ProxyHandler(API &api, const JSON::ValuePtr &config) :
  api(api), tmpl(new RequestTmpl(api, config)) {}


bool ProxyHandler::operator()(HTTP::Request &req) {
  auto cb =
    [&req] (HTTP::Status status, const JSON::ValuePtr &data) {
      if (data.isSet()) {
        if (!req.outHas("Content-Type"))
          req.outSet("Content-Type", "application/json");
        req.send(data->toString());
      }

      req.reply(status);
    };

  tmpl->apply(new cb::API::Resolver(api, req), cb);

  return true;
}
