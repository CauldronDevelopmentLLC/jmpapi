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

#include "CORSHandler.h"

#include <cbang/String.h>
#include <cbang/event/Request.h>
#include <cbang/event/RequestMethod.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


CORSHandler::CORSHandler(const JSON::Value &config) :
  HeadersHandler(config.has("headers") ?
                 config.get("headers") : new JSON::Dict) {
  // Explicit origins
  if (config.has("origins")) {
    auto &l = *config.get("origins");
    for (unsigned i = 0; i < l.size(); i++)
      origins.insert(String::toLower(l.getString(i)));
  }

  // Origin regex patterns
  if (config.has("patterns")) {
    auto &l = *config.get("patterns");
    for (unsigned i = 0; i < l.size(); i++)
      patterns.push_back(l.getString(i));
  }

  // Add defaults
  auto hdrs = config.get("headers", new JSON::Dict);

  const char *name = "Access-Control-Allow-Origin";
  if (config.has("origin") && !hdrs->has(name))
    add(name, config.getAsString("origin"));

  name = "Access-Control-Allow-Methods";
  if (!hdrs->has(name))
    add(name, config.getString("methods", "POST,PUT,GET,OPTIONS,DELETE"));

  name = "Access-Control-Allow-Headers";
  if (!hdrs->has(name))
    add(name, "DNT,User-Agent,X-Requested-With,If-Modified-Since,"
        "Cache-Control,Content-Type,Range,Set-Cookie,Authorization");

  name = "Access-Control-Allow-Credentials";
  if (config.getBoolean("credentials", false) && !hdrs->has(name))
    add(name, "true");

  name = "Access-Control-Max-Age";
  if (config.has("max-age") && !hdrs->has(name))
    add(name, config.getAsString("max-age"));
}


bool CORSHandler::operator()(Event::Request &req) {
  // Add headers
  HeadersHandler::operator()(req);

  // Copy Origin if it matches
  if (req.inHas("Origin")) {
    string origin = String::toLower(req.inGet("Origin"));

    bool match = origins.find(origin) != origins.end();
    for (unsigned i = 0; i < patterns.size() && !match; i++)
      match = patterns[i].match(origin);

    if (match) req.outSet("Access-Control-Allow-Origin", req.inGet("Origin"));
  }

  // OPTIONS are done here
  if (req.getMethod() == HTTP_OPTIONS) {
    req.reply();
    return true;
  }

  return false;
}
