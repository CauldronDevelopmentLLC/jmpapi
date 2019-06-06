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

#include "Server.h"
#include "App.h"
#include "Transaction.h"

#include <jmpapi/handler/EndpointHandler.h>
#include <jmpapi/handler/StatusHandler.h>
#include <jmpapi/handler/QueryHandler.h>
#include <jmpapi/handler/APIHandler.h>
#include <jmpapi/handler/ArgsHandler.h>
#include <jmpapi/handler/PassHandler.h>
#include <jmpapi/handler/HeadersHandler.h>
#include <jmpapi/handler/RedirectHandler.h>
#include <jmpapi/handler/SessionHandler.h>
#include <jmpapi/handler/CORSHandler.h>

#include <cbang/event/Request.h>
#include <cbang/event/ACLHandler.h>
#include <cbang/event/HTTPHandlerGroup.h>
#include <cbang/event/HTTPURLPatternMatcher.h>
#include <cbang/event/HTTPMethodMatcher.h>
#include <cbang/event/HTTPAccessHandler.h>
#include <cbang/event/FileHandler.h>
#include <cbang/event/IndexHTMLHandler.h>

#include <cbang/openssl/SSLContext.h>
#include <cbang/json/JSON.h>
#include <cbang/log/Logger.h>

using namespace cb;
using namespace std;
using namespace JmpAPI;


namespace {
  unsigned parseMethods(const std::string &s) {
    unsigned methods = Event::RequestMethod::HTTP_UNKNOWN;
    vector<string> tokens;

    String::tokenize(String::toUpper(s), tokens, "| \n\r\t");
    for (unsigned i = 0; i < tokens.size(); i++)
      methods |= HTTP::RequestMethod::parse
        (tokens[i], Event::RequestMethod::HTTP_UNKNOWN);

    return methods;
  }
}


Server::Server(App &app) :
  Event::WebServer(app.getOptions(), app.getEventBase(), new SSLContext,
                   SmartPointer<HTTPHandlerFactory>::Phony(this)),
  Event::HTTPHandlerFactory(false), app(app) {
  app.getOptions()["http-addresses"].setDefault("");
  app.getOptions()["https-addresses"].setDefault("");
  app.getOptions()["certificate-file"].clearDefault();
  app.getOptions()["private-key-file"].clearDefault();
}


SmartPointer<Event::HTTPRequestHandler>
Server::createAccessHandler(const JSON::Value &config) {
  SmartPointer<Event::HTTPAccessHandler> handler =
    new Event::HTTPAccessHandler;

  if (config.has("allow")) {
    JSON::ValuePtr list = config.get("allow");
    for (unsigned i = 0; i < list->size(); i++) {
      string name = list->getAsString(i);
      if (name.empty()) continue;
      if (name[0] == '$') handler->allowGroup(name.substr(1));
      else handler->allowUser(name);
    }
  }

  if (config.has("deny")) {
    JSON::ValuePtr list = config.get("deny");
    for (unsigned i = 0; i < list->size(); i++) {
      string name = list->getAsString(i);
      if (name.empty()) continue;
      if (name[0] == '$') handler->denyGroup(name.substr(1));
      else handler->denyUser(name);
    }
  }

  return handler;
}


SmartPointer<Event::HTTPRequestHandler>
Server::createEndpoint(const JSON::ValuePtr &config) {
  string type = config->getString("handler", "");

  if (type.empty() && config->has("sql")) type = "query";
  if (type.empty()) type == "pass";

  if (type == "pass")    return new PassHandler;
  if (type == "cors")    return new CORSHandler(*config);
  if (type == "session") return new SessionHandler(*config);
  if (type == "query")   return new QueryHandler(*config);

  if (type == "login")
    return new EndpointHandler(&Transaction::apiLogin, config);

  if (type == "logout")
    return new EndpointHandler(&Transaction::apiLogout, config);

  if (type == "status") {
    if (config->hasNumber("code"))
      return new StatusHandler
        ((Event::HTTPStatus::enum_t)config->getU8("code"));
    return new StatusHandler(config->getString("code"));
  }

  if (type == "redirect")
    return new RedirectHandler((Event::HTTPStatus::enum_t)
                               config->getU32("code", 301),
                               config->getString("location"));

  if (type == "api")
    return new APIHandler(*app.getConfig(), *app.getConfig()->get("api"));

  THROW("Unsupported handler '" << type << "'");
}


SmartPointer<Event::HTTPRequestHandler>
Server::createAPIHandler(const string &pattern, const JSON::Value &api) {
  LOG_INFO(1, "Adding endpoint " << pattern);

  // Group
  SmartPointer<Event::HTTPHandlerGroup> group = new Event::HTTPHandlerGroup;
  SmartPointer<Event::HTTPURLPatternMatcher> matcher =
    new Event::HTTPURLPatternMatcher(pattern, group);

  // Endpoint Auth
  if (api.has("allow") || api.has("deny"))
    group->addHandler(createAccessHandler(api));

  // Methods
  unsigned endpoints = 0;
  for (unsigned i = 0; i < api.size(); i++) {
    const string &key = api.keyAt(i);
    unsigned methods = parseMethods(key);
    if (!methods) continue; // Ignore non-methods

    const JSON::ValuePtr config = api.get(i);
    SmartPointer<Event::HTTPRequestHandler> handler = createEndpoint(config);

    if (handler.isNull()) continue;
    endpoints++;

    SmartPointer<Event::HTTPHandlerGroup> methodGroup =
      new Event::HTTPHandlerGroup;

    // Auth
    if (config->has("allow") || config->has("deny"))
      methodGroup->addHandler(createAccessHandler(*config));

    // Args
    JSON::ValuePtr args = new JSON::Dict;
    if (api.has("args")) args->merge(*api.get("args"));
    if (config->has("args")) args->merge(*config->get("args"));

    const set<string> &implicitArgs = matcher->getArgs();
    for (auto it = implicitArgs.begin(); it != implicitArgs.end(); it++)
      if (!args->has(*it)) args->insertDict(*it);

    if (args->size()) methodGroup->addHandler(new ArgsHandler(args));

    // Headers
    if (config->has("headers") && !handler.isInstance<HeadersHandler>())
      methodGroup->addHandler(new HeadersHandler(config->get("headers")));

    // Method(s)
    methodGroup->addHandler(handler);
    group->addHandler(new Event::HTTPMethodMatcher(methods, methodGroup));
  }

  // Handle arg constraints when there are no endpoints
  if (!endpoints && api.has("args"))
    group->addHandler(new ArgsHandler(api.get("args")));

  return matcher;
}


void Server::loadCategory(const string &name, const JSON::Value &cat) {
  SmartPointer<Event::HTTPHandlerGroup> group = new Event::HTTPHandlerGroup;
  string base = cat.getString("base", "");

  // Category Auth
  if (cat.has("allow") || cat.has("deny"))
    group->addHandler(createAccessHandler(cat));

  // Endpoints
  if (cat.hasDict("endpoints")) {
    const JSON::Value &endpoints = *cat.get("endpoints");

    for (unsigned i = 0; i < endpoints.size(); i++)
      group->addHandler
        (createAPIHandler(base + endpoints.keyAt(i), *endpoints.get(i)));
  }

  addHandler(group);
}


void Server::loadCategories(const JSON::Value &cats) {
  for (unsigned i = 0; i < cats.size(); i++)
    loadCategory(cats.keyAt(i), *cats.get(i));
}


void Server::init() {
  Event::WebServer::init();

  // API Categories
  auto &config = *app.getConfig();
  if (config.has("api")) loadCategories(*config.get("api"));

  // Root
  string root = app.getOptions()["http-root"].toString("");
  if (!root.empty()) {
    LOG_INFO(1, "Adding file handler at " << root);
    addHandler(new Event::IndexHTMLHandler(new Event::FileHandler(root)));
  }

  // Not found
  addHandler(new StatusHandler(HTTP_NOT_FOUND));
}


SmartPointer<Event::Request> Server::createRequest
(Event::RequestMethod method, const URI &uri, const Version &version) {
  return new Transaction(app, method, uri, version);
}
