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
#include <cbang/json/String.h>

#include <set>

using namespace std;
using namespace cb;
using namespace JmpAPI;


Resolver::Resolver(const JSON::ValuePtr &ctx, const ResolverPtr &parent) :
  req(parent->req), ctx(ctx), parent(parent) {}


Resolver::Resolver(const RequestPtr &req) : req(req) {}


Resolver &Resolver::getRoot() {
  return parent.isSet() ? parent->getRoot() : *this;
}


const JSON::ValuePtr &Resolver::getArgs() const {
  if (req.isNull()) THROW("No request");
  auto &args = req->getArgs();
  if (args->empty()) req->parseArgs();
  return args;
}


ResolverPtr Resolver::makeChild(const JSON::ValuePtr &ctx) {
  return new Resolver(ctx, this);
}


JSON::ValuePtr Resolver::select(const string &name) const {
  if (name.empty()) return 0;
  if (name == ".") return ctx;

  if (name[0] == '~') {
    auto result = select(name.substr(1));
    if (result.isNull()) return JSON::Null::instancePtr();
    return result;
  }

  if (name == "..") return parent->getContext();
  if (String::startsWith(name, "../")) {
    if (parent.isSet()) return parent->select(name.substr(3));
    return 0;
  }

  if (req.isSet()) {
    if (name == "args") return getArgs();
    if (String::startsWith(name, "args."))
      return getArgs()->select(name.substr(5), 0);

    auto &session = req->getSession();
    if (session.isSet()) {
      if (name == "session") return session;
      if (String::startsWith(name, "session."))
        return session->select(name.substr(8), 0);

      if (name == "group") return session->get("group");
      if (String::startsWith(name, "group."))
        return session->get("group")->select(name.substr(6), 0);
    }
  }

  if (ctx.isSet()) {
    if (String::startsWith(name, "./")) return ctx->select(name.substr(2), 0);
    return ctx->select(name, 0);
  }

  return 0;
}


string Resolver::format(const string &s,
                        cb::String::format_cb_t default_cb) const {
  String::format_cb_t cb =
    [&] (char type, int index, const string &name, bool &matched) {
      // Do not allow recursive refs, user input could contain var refs
      auto value = select(name);
      if (value.isSet()) return value->format(type);

      // TODO Is there something wrong with matching bools?: %(test)b

      if (default_cb) return default_cb(type, index, name, matched);

      matched = false;
      return string();
    };

  return String(s).format(cb);
}


string Resolver::format(const string &s, const string &defaultValue) const {
  auto cb = [&] (char, int, const string &, bool &) {return defaultValue;};
  return format(s, cb);
}


void Resolver::resolve(JSON::Value &value) const {
  auto cb =
    [&] (JSON::Value &value, JSON::Value *parent, unsigned index) {
      if (!value.isString()) return;

      string s = value.getString();
      if (s.find('%') == string::npos) return;

      value.cast<JSON::String>().getValue() = format(s);
    };

  value.visit(cb);
}
