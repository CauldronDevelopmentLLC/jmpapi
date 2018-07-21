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

#include "StatusHandler.h"

#include <cbang/event/Request.h>
#include <cbang/event/RequestMethod.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


StatusHandler::StatusHandler(int code, const JSON::ValuePtr &config) :
  code(code) {
  if (config.isNull()) return;

  if (config->hasNumber("code")) code = config->getU8("code");
  else if (config->hasString("code"))
    code = HTTPStatus::parse(config->getString("code"));
  else if (!code) THROW("Missing HTTP reponse code");

  addHeaders(config);
}


void StatusHandler::addHeaders(const JSON::ValuePtr &config) {
  if (config->hasDict("headers")) {
    JSON::ValuePtr dict = config->get("headers");
    for (unsigned i = 0; i < dict->size(); i++)
      addHeader(dict->keyAt(i), dict->getString(i));
  }
}


void StatusHandler::addHeader(const string &name, const string &value) {
  headers.push_back(pair<string, string>(name, value));
}


bool StatusHandler::operator()(Event::Request &req) {
  for (unsigned i = 0; i < headers.size(); i++)
    req.outSet(headers[i].first, headers[i].second);

  req.send(Event::HTTPStatus((HTTPStatus::enum_t)code).toString());
  req.reply(code);

  return true;
}
