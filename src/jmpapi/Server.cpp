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

#include <jmpapi/handler/ArgsParser.h>
#include <jmpapi/handler/EndpointHandler.h>
#include <jmpapi/handler/StatusHandler.h>
#include <jmpapi/handler/QueryHandler.h>
#include <jmpapi/handler/ProxyHandler.h>
#include <jmpapi/handler/APIHandler.h>
#include <jmpapi/handler/ArgsHandler.h>
#include <jmpapi/handler/ArgFilterHandler.h>
#include <jmpapi/handler/PassHandler.h>
#include <jmpapi/handler/HeadersHandler.h>
#include <jmpapi/handler/RedirectHandler.h>
#include <jmpapi/handler/SessionHandler.h>
#include <jmpapi/handler/CORSHandler.h>

#include <cbang/http/Request.h>
#include <cbang/http/HandlerGroup.h>
#include <cbang/http/URLPatternMatcher.h>
#include <cbang/http/MethodMatcher.h>
#include <cbang/http/AccessHandler.h>
#include <cbang/http/FileHandler.h>
#include <cbang/http/IndexHandler.h>

#include <cbang/openssl/SSLContext.h>
#include <cbang/json/Value.h>
#include <cbang/json/String.h>
#include <cbang/log/Logger.h>

using namespace cb;
using namespace std;
using namespace JmpAPI;


namespace {
  unsigned parseMethods(const std::string &s) {
    unsigned methods = HTTP::Method::HTTP_UNKNOWN;
    vector<string> tokens;

    String::tokenize(String::toUpper(s), tokens, "| \n\r\t");
    for (unsigned i = 0; i < tokens.size(); i++)
      methods |= HTTP::Method::parse
        (tokens[i], HTTP::Method::HTTP_UNKNOWN);

    return methods;
  }
}


Server::Server(App &app) :
  HTTP::WebServer(app.getOptions(), app.getEventBase(), new SSLContext,
                  SmartPointer<HTTP::HandlerFactory>::Phony(this)),
  HTTP::HandlerFactory(false), app(app) {
  app.getOptions()["http-addresses"].setDefault("");
  app.getOptions()["https-addresses"].setDefault("");
  app.getOptions()["certificate-file"].clearDefault();
  app.getOptions()["private-key-file"].clearDefault();
}


SmartPointer<HTTP::RequestHandler>
Server::createAccessHandler(const JSON::Value &config) {
  SmartPointer<HTTP::AccessHandler> handler =
    new HTTP::AccessHandler;

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


SmartPointer<HTTP::RequestHandler>
Server::createEndpoint(const JSON::ValuePtr &config) {
  string type = config->getString("handler", "");

  if (type.empty() && config->has("sql")) type = "query";
  if (type.empty() && config->has("url")) type = "proxy";
  if (type.empty()) type == "pass";

  if (type == "pass")    return new PassHandler;
  if (type == "cors")    return new CORSHandler(*config);
  if (type == "session") return new SessionHandler(*config);
  if (type == "query")   return new QueryHandler(*config);
  if (type == "proxy")   return new ProxyHandler(config);

  if (type == "login")
    return new EndpointHandler(&Transaction::apiLogin, config);

  if (type == "logout")
    return new EndpointHandler(&Transaction::apiLogout, config);

  if (type == "status") {
    if (config->hasNumber("code"))
      return new StatusHandler
        ((HTTP::Status::enum_t)config->getU8("code"));
    return new StatusHandler(config->getString("code"));
  }

  if (type == "redirect")
    return new RedirectHandler((HTTP::Status::enum_t)
                               config->getU32("code", 301),
                               config->getString("location"));

  if (type == "api") return new APIHandler(*app.getConfig());

  THROW("Unsupported handler '" << type << "'");
}


SmartPointer<HTTP::RequestHandler>
Server::createValidationHandler(const JSON::Value &config) {
  SmartPointer<HTTP::HandlerGroup> group = new HTTP::HandlerGroup;

  // Endpoint Auth
  if (config.has("allow") || config.has("deny"))
    group->addHandler(createAccessHandler(config));

  // Args
  if (config.has("args"))
    group->addHandler(new ArgsHandler(config.get("args")));

  return group;
}


SmartPointer<HTTP::RequestHandler>
Server::createAPIHandler(const string &pattern, const JSON::Value &api,
                         const RequestHandlerPtr &parentValidation) {
  LOG_INFO(1, "Adding endpoint " << pattern);

  // Patterns
  SmartPointer<HTTP::HandlerGroup> patterns = new HTTP::HandlerGroup;

  // Group
  SmartPointer<HTTP::HandlerGroup> group = new HTTP::HandlerGroup;
  SmartPointer<HTTP::URLPatternMatcher> root =
    new HTTP::URLPatternMatcher(pattern, group);
  patterns->addHandler(root);

  // Request validation
  auto validation = group->addGroup();
  if (parentValidation.isSet()) group->addHandler(parentValidation);
  validation->addHandler(createValidationHandler(api));

  // Children
  for (unsigned i = 0; i < api.size(); i++) {
    auto &key    = api.keyAt(i);
    auto &config = api.get(i);

    // Child
    if (!key.empty() && key[0] == '/') {
      auto child = createAPIHandler(pattern + key, *config, validation);
      patterns->addHandler(child);
      continue;
    }

    // Methods
    unsigned methods = parseMethods(key);
    if (!methods) continue; // Ignore non-methods

    auto handler = createEndpoint(config);
    if (handler.isNull()) continue;

    SmartPointer<HTTP::HandlerGroup> methodGroup =
      new HTTP::HandlerGroup;
    methodGroup->addHandler(createValidationHandler(*config));

    // Headers
    if (config->has("headers") && !handler.isInstance<HeadersHandler>())
      methodGroup->addHandler(new HeadersHandler(config->get("headers")));

    // Arg filter
    if (config->has("arg-filter"))
      handler = new ArgFilterHandler(app, *config->get("arg-filter"), handler);

    // Method(s)
    methodGroup->addHandler(handler);
    group->addHandler(new HTTP::MethodMatcher(methods, methodGroup));
  }

  return patterns;
}


void Server::loadCategory(const string &name, const JSON::Value &cat) {
  SmartPointer<HTTP::HandlerGroup> group = new HTTP::HandlerGroup;
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
  HTTP::WebServer::init();

  // Always parse args
  addHandler(new ArgsParser);

  // Load API
  auto &config = *app.getConfig();
  if (config.has("api")) loadCategories(*config.get("api"));

  // Root
  string root = app.getOptions()["http-root"].toString("");
  if (!root.empty()) {
    LOG_INFO(1, "Adding file handler at " << root);
    addHandler(new HTTP::IndexHandler(new HTTP::FileHandler(root)));
  }

  // Not found
  addHandler(new StatusHandler(HTTP_NOT_FOUND));
}


SmartPointer<HTTP::Request> Server::createRequest(
  const SmartPointer<HTTP::Conn> &connection, HTTP::Method method,
  const URI &uri, const Version &version) {
  return new Transaction(app, connection, method, uri, version);
}
