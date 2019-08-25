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

#include "Resolver.h"

#include <cbang/log/Logger.h>
#include <cbang/json/Null.h>

#include <set>

using namespace std;
using namespace cb;
using namespace JmpAPI;


Resolver::Resolver(const string &key, const JSON::ValuePtr &value) {
  insert(key, value);
}


Resolver::Resolver(Event::Request &req) : req(&req) {}


Resolver::Resolver(const ResolverPtr &parent, const JSON::ValuePtr &ctx) :
  parent(parent), req(parent->req), ctx(ctx) {}


ResolverPtr Resolver::makeChild(const JSON::ValuePtr &ctx) {
  return new Resolver(this, ctx);
}


JSON::ValuePtr Resolver::select(const string &name) {
  if (name.empty()) return 0;

  if (String::startsWith(name, "../")) {
    if (parent.isSet()) return parent->select(name.substr(3));
    return 0;
  }

  if (name[0] == '~') {
    auto result = select(name.substr(1));
    if (result.isNull()) return JSON::Null::instancePtr();
    return result;
  }

  if (req) {
    if (String::startsWith(name, "args.")) {
      auto args = req->getArgs();
      if (args->empty()) req->parseArgs();

      return args->select(name.substr(5), 0);
    }

    auto session = req->getSession();
    if (session.isSet()) {
      if (String::startsWith(name, "session."))
        return session->select(name.substr(8), 0);

      if (String::startsWith(name, "group."))
        return session->get("group")->select(name.substr(6), 0);
    }
  }

  auto result = JSON::Value::select(name, 0);

  if (!result.isSet() && ctx.isSet()) result = ctx->select(name, 0);

  if (!result.isSet() && req) {
    auto args = req->getArgs();
    if (args->empty()) req->parseArgs();

    result = args->select(name, 0);
  }

  if (!result.isSet() && hasDict("global") && name[0] == '$')
    result = get("global")->select(name.substr(1), 0);

  return result;
}


string Resolver::format(const string &s, cb::String::format_cb_t default_cb) {
  std::set<string> exclude;

  String::format_cb_t cb =
    [&] (char type, int index, const string &name, bool &matched) {
      if (exclude.find(name) == exclude.end()) {

        auto value = select(name);
        if (value.isSet())  {
          // Allow recursive refs but avoid infinite loops
          exclude.insert(name);
          string s = String(value->format(type)).format(cb);
          exclude.erase(name);

          return s;
        }
      }

      if (default_cb) return default_cb(type, index, name, matched);

      matched = false;
      return string();
    };

  return String(s).format(cb);
}


string Resolver::format(const string &s, const string &defaultValue) {
  auto cb = [&] (char, int, const string &, bool &) {return defaultValue;};
  return format(s, cb);
}


void Resolver::resolve(JSON::Value &value) {
  auto cb =
    [&] (JSON::Value &value, JSON::Value *parent, unsigned index) {
      if (!value.isString() || !parent) return;

      string s = value.getString();
      if (s.find('%') == string::npos) return;

      if (parent->isList()) parent->set(index, format(s));
      else parent->insert(parent->keyAt(index), format(s));
    };

  value.visit(cb);
}
