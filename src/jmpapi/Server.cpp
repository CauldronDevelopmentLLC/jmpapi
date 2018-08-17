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

#include "Server.h"
#include "App.h"
#include "Transaction.h"
#include "EndpointHandler.h"
#include "StatusHandler.h"
#include "QueryHandler.h"
#include "APIHandler.h"
#include "ArgsHandler.h"

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
    unsigned methods = 0;
    vector<string> tokens;

    String::tokenize(String::toUpper(s), tokens, "| \n\r\t");
    for (unsigned i = 0; i < tokens.size(); i++)
      methods |= HTTP::RequestMethod::parse(tokens[i]);

    return methods;
  }
}


Server::Server(App &app) :
  Event::WebServer(app.getOptions(), app.getEventBase(), new SSLContext,
                   SmartPointer<HTTPHandlerFactory>::Phony(this)),
  Event::HTTPHandlerFactory(false), app(app) {
  app.getOptions()["https-addresses"].setDefault("");
  app.getOptions()["certificate-file"].clearDefault();
  app.getOptions()["private-key-file"].clearDefault();
}


SmartPointer<Event::HTTPHandler>
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


SmartPointer<Event::HTTPHandler>
Server::createEndpoint(const SmartPointer<JSON::Value> &config) {
  string type = config->getString("handler", "");

  if (type.empty() && config->has("sql")) type = "query";

  if (type.empty()) return 0;

  if (type == "query") return new QueryHandler(config);

  if (type == "login")
    return new EndpointHandler(&Transaction::apiLogin, config);

  if (type == "logout")
    return new EndpointHandler(&Transaction::apiLogout, config);

  if (type == "status") return new StatusHandler(0, config);

  if (type == "redirect") {
    SmartPointer<StatusHandler> handler = new StatusHandler(301, config);
    handler->addHeader("Location", config->getString("location"));
    return handler;
  }

  if (type == "api") return new APIHandler(app.getConfig()->get("api"));

  THROWS("Unsupported handler '" << type << "'");
}


SmartPointer<Event::HTTPHandler>
Server::createAPIHandler(const string &pattern, const JSON::Value &api) {
  LOG_INFO(1, "Adding endpoint " << pattern);

  // Group
  SmartPointer<Event::HTTPHandlerGroup> group = new Event::HTTPHandlerGroup;
  SmartPointer<Event::HTTPURLPatternMatcher> matcher =
    new Event::HTTPURLPatternMatcher(pattern, group);

  // Methods
  for (unsigned i = 0; i < api.size(); i++) {
    unsigned methods = parseMethods(api.keyAt(i));
    JSON::ValuePtr config = api.get(i);
    SmartPointer<Event::HTTPHandler> handler = createEndpoint(config);

    if (handler.isNull()) continue;

    SmartPointer<Event::HTTPHandlerGroup> methodGroup =
      new Event::HTTPHandlerGroup;

    // Auth
    if (config->has("allow") || config->has("deny"))
      methodGroup->addHandler(createAccessHandler(*config));

    // Args
    methodGroup->addHandler
      (new ArgsHandler(matcher->getArgs(), api.get("args", new JSON::Dict),
                       config->get("args", new JSON::Dict)));

    methodGroup->addHandler(handler);
    group->addHandler(new Event::HTTPMethodMatcher(methods, methodGroup));
  }

  return matcher;
}


void Server::init() {
  Event::WebServer::init();

  JSON::ValuePtr config = app.getConfig();

  // Load Sessions
  addMember<Transaction>(&Transaction::lookupSession);

  // API
  JSON::ValuePtr api = config->get("api");
  for (unsigned i = 0; i < api->size(); i++)
    addHandler(createAPIHandler(api->keyAt(i), *api->get(i)));

  // Root
  string root = app.getOptions()["http-root"].toString("");
  if (!root.empty()) {
    LOG_INFO(1, "Adding file handler at " << root);
    addHandler(new Event::IndexHTMLHandler(new Event::FileHandler(root)));
  }

  // Not found
  addHandler(new StatusHandler(HTTP_NOT_FOUND));
}


Event::Request *Server::createRequest(evhttp_request *req) {
  return new Transaction(app, req);
}
