/******************************************************************************\

                           This file is part of JmpAPI.

                 Copyright (c) 2014-2026, Cauldron Development Oy
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

#include "RequestTmpl.h"

#include <jmpapi/API.h>
#include <jmpapi/ContextResolver.h>

#include <cbang/api/API.h>
#include <cbang/http/Client.h>
#include <cbang/json/JSON.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


namespace {
  HTTP::Status conErrToStatus(Event::ConnectionError err) {
    switch (err) {
    case HTTP::Enum::CONN_ERR_TIMEOUT:
      return HTTP::Enum::HTTP_GATEWAY_TIME_OUT;

    case HTTP::Enum::CONN_ERR_CONNECT:
      return HTTP::Enum::HTTP_SERVICE_UNAVAILABLE;

    default: return HTTP::Enum::HTTP_BAD_GATEWAY;
    }
  }
}


RequestTmpl::RequestTmpl(API &api, const JSON::ValuePtr &config) :
  Template(api), url(config->getString("url")),
  method(HTTP::Method::parse(config->getString("method", "GET"))),
  headers(config->get("request-headers", 0)),
  dataTmpl(Template::parse(config->get("data", 0))),
  child(Template::parse(config)) {}


void RequestTmpl::apply(const cb::API::ResolverPtr &resolver, cb_t done) {
  auto error_cb =
    [done] (const string &msg, const JSON::ValuePtr &data) {
      JSON::ValuePtr err = new JSON::Dict;

      err->insert("error", msg);
      if (data.isSet()) err->insert("data", data);

      done(HTTP_INTERNAL_SERVER_ERROR, err);
    };

  auto cb =
    [this, done, error_cb, resolver] (HTTP::Request &outReq) {
      activeRequests.erase(&outReq);
      JSON::ValuePtr data;

      try {
        auto err = outReq.getConnectionError();
        if (err) return done(conErrToStatus(err), 0);

        // Parse JSON
        if (outReq.getInputBuffer().getLength()) {
          string response = outReq.getInput();

          try {
            data = JSON::Reader::parse(response);

          } catch (Exception &e) {
            JSON::ValuePtr err = new JSON::Dict;
            err->insert("error", e.getMessage());
            err->insert("response", response);

            return done(HTTP_BAD_GATEWAY, err);
          }
        }

        // Template response
        if (child.isSet())
          child->apply(new ContextResolver(resolver, data), done);
        else done(outReq.getResponseCode(), data);

      } catch (Exception &e) {
        LOG_DEBUG(3, e);
        error_cb(e.getMessage(), data);

      } catch (std::exception &e) {error_cb(e.what(), data);}
    };

  auto pr = getAPI().getClient().call(resolver->resolve(url), method, cb);
  activeRequests[pr->getRequest().get()] = pr;
  headers.set(*pr->getRequest());

  if (dataTmpl.isSet()) {
    if (!pr->getRequest()->outHas("Content-Type"))
      pr->getRequest()->outSet("Content-Type", "application/json");

    auto cb =
      [pr, done] (HTTP::Status status, const JSON::ValuePtr &data) {
        if (status == HTTP_OK) {
          LOG_DEBUG(3, "Sending: " << *data);
          pr->getRequest()->getOutputBuffer().add(data->toString());
          pr->send();

        } else done(status, data);
      };

    dataTmpl->apply(resolver, cb);

  } else pr->send();
}
