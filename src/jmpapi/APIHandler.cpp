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

#include <cbang/String.h>
#include <cbang/event/Request.h>
#include <cbang/event/HTTPURLPatternMatcher.h>
#include <cbang/event/RequestMethod.h>
#include <cbang/json/KeywordFilter.h>
#include <cbang/json/KeywordsFilter.h>

using namespace JmpAPI;
using namespace cb;
using namespace std;


namespace {
  void copyExistingKey(const string &key, const JSON::Value &src,
                       JSON::Value &dst) {
    if (src.has(key)) dst.insert(key, src.get(key));
  }


  void copyExistingKeys(const char *keys[], const JSON::Value &src,
                       JSON::Value &dst) {
    for (int i = 0; keys[i]; i++) copyExistingKey(keys[i], src, dst);
  }


  bool isMethod(const std::string &s) {
    vector<string> tokens;

    String::tokenize(String::toUpper(s), tokens, "| \n\r\t");
    for (unsigned i = 0; i < tokens.size(); i++)
      if (Event::RequestMethod::parse
          (tokens[i], Event::RequestMethod::HTTP_UNKNOWN) ==
          Event::RequestMethod::HTTP_UNKNOWN) return false;

    return !tokens.empty();
  }
}


APIHandler::APIHandler(const JSON::Value &config, const JSON::Value &_api) :
  api(new JSON::Dict) {

  api->insert("title", config.getString("title", "JmpAPI Docs"));
  if (config.hasString("doc-help"))
    api->insert("help", config.getString("doc-help"));

  api->insert("categories", loadCategories(_api));
}


bool APIHandler::operator()(Event::Request &req) {
  SmartPointer<JSON::Writer> writer = req.getJSONWriter();
  api->write(*writer);
  writer->close();
  req.reply();
  return true;
}


JSON::ValuePtr APIHandler::loadCategories(const JSON::Value &_cats) {
  JSON::ValuePtr cats = new JSON::Dict;

  for (unsigned i = 0; i < _cats.size(); i++) {
    JSON::Value &cat = *_cats.get(i);
    if (!cat.getBoolean("hide", false))
      cats->insert(_cats.keyAt(i), loadCategory(cat));
  }

  return cats;
}


JSON::ValuePtr APIHandler::loadCategory(const JSON::Value &_cat) {
  JSON::ValuePtr cat = new JSON::Dict;

  string base = _cat.getString("base", "");
  const char *keys[] = {"base", "title", "help", "allow", "deny", 0};
  copyExistingKeys(keys, _cat, *cat);

  if (_cat.hasDict("endpoints")) {
    const JSON::Value &_endpoints = *_cat.get("endpoints");
    JSON::ValuePtr endpoints = new JSON::Dict;

    cat->insert("endpoints", endpoints);

    for (unsigned i = 0; i < _endpoints.size(); i++) {
      string pattern = base + _endpoints.keyAt(i);
      const JSON::Value &_endpoint = *_endpoints.get(i);
      if (_endpoint.getBoolean("hide", false)) continue;

      endpoints->insert(pattern, loadEndpoint(pattern, _endpoint));
    }
  }

  return cat;
}


JSON::ValuePtr APIHandler::loadEndpoint(const string &pattern,
                                        const JSON::Value &_endpoint) {
  JSON::ValuePtr endpoint = new JSON::Dict;

  // Copy keys
  const char *keys[] = {"help", "allow", "deny", 0};
  copyExistingKeys(keys, _endpoint, *endpoint);

  // Get URL args from endpoint pattern
  Event::HTTPURLPatternMatcher matcher(pattern, 0);
  const set<string> &urlArgs = matcher.getArgs();
  JSON::ValuePtr endpointArgs = _endpoint.get("args", new JSON::Dict);

  // Load methods
  for (unsigned i = 0; i < _endpoint.size(); i++) {
    const string &key = _endpoint.keyAt(i);

    // Ignore non-methods
    if (!isMethod(key)) continue;

    const JSON::Value &_method = *_endpoint.get(i);
    if (_method.getBoolean("hide", false) ||
        _method.getString("handler", "") == "pass") continue;

    endpoint->insert(key, loadMethod(_method, urlArgs, *endpointArgs));
  }

  return endpoint;
}


JSON::ValuePtr APIHandler::loadMethod(const JSON::Value &_method,
                                      const set<string> &urlArgs,
                                      const JSON::Value &endpointArgs) {
  JSON::ValuePtr method = new JSON::Dict;

  // Load args
  JSON::ValuePtr args = new JSON::Dict;
  args->merge(endpointArgs);
  if (_method.has("args")) args->merge(*_method.get("args"));
  if (args->size()) {
    // Mark URL args
    for (unsigned i = 0; i < args->size(); i++)
      if (urlArgs.find(args->keyAt(i)) != urlArgs.end())
        args->get(i)->insertBoolean("url", true);

    method->insert("args", args);
  }

  // Copy other keys
  const char *keys[] = {"help", "handler", "allow", "deny", 0};
  copyExistingKeys(keys, _method, *method);

  return method;
}
