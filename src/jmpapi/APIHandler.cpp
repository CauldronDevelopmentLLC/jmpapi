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

#include "APIHandler.h"

#include <cbang/event/Request.h>
#include <cbang/event/HTTPURLPatternMatcher.h>
#include <cbang/json/KeywordFilter.h>
#include <cbang/json/KeywordsFilter.h>

using namespace JmpAPI;
using namespace cb;
using namespace std;


APIHandler::APIHandler(const string &title, const JSON::ValuePtr &_api) :
  api(new JSON::Dict) {

  api->insert("title", title);
  JSON::ValuePtr entrypoints = new JSON::Dict;
  api->insert("api", entrypoints);

  // Load entrypoints
  for (unsigned i = 0; i < _api->size(); i++) {
    const string &pattern = _api->keyAt(i);
    JSON::ValuePtr _entrypoint = _api->get(i);
    if (_entrypoint->getBoolean("hide", false)) continue;

    JSON::ValuePtr entrypoint = new JSON::Dict;
    entrypoints->insert(pattern, entrypoint);

    // Get URL args from entrypoint pattern
    Event::HTTPURLPatternMatcher matcher(pattern, 0);
    const set<string> &urlArgs = matcher.getArgs();

    JSON::ValuePtr entrypointArgs = _entrypoint->get("args", new JSON::Dict);

    // Load methods
    for (unsigned j = 0; j < _entrypoint->size(); j++) {
      if (_entrypoint->keyAt(j) == "args") continue;
      JSON::ValuePtr _method = _entrypoint->get(j);
      if (_method->getBoolean("hide", false)) continue;

      JSON::ValuePtr method = new JSON::Dict;
      entrypoint->insert(_entrypoint->keyAt(j), method);

      // Load args
      JSON::ValuePtr args = new JSON::Dict;
      args->merge(*entrypointArgs);
      if (_method->has("args")) args->merge(*_method->get("args"));
      if (args->size()) {
        // Mark URL args
        for (unsigned i = 0; i < args->size(); i++)
          if (urlArgs.find(args->keyAt(i)) != urlArgs.end())
            args->get(i)->insertBoolean("url", true);

        method->insert("args", args);
      }

      // Copy other keys
      const char *keys[] = {"allow", "deny", "handler", "help", 0};
      for (unsigned k = 0; keys[k]; k++)
        if (_method->has(keys[k]))
          method->insert(keys[k], _method->get(keys[k]));
    }
  }
}


bool APIHandler::operator()(Event::Request &req) {
  SmartPointer<JSON::Writer> writer = req.getJSONWriter();
  api->write(*writer);
  writer->close();
  req.reply();
  return true;
}
