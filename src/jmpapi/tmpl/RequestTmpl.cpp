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

#include "RequestTmpl.h"

#include <jmpapi/App.h>
#include <jmpapi/Transaction.h>

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


RequestTmpl::RequestTmpl(const JSON::ValuePtr &config) :
  url(config->getString("url")),
  method(HTTP::Method::parse(config->getString("method", "UNKNOWN"))),
  headers(config->get("request-headers", 0)),
  dataTmpl(Template::parse(config->get("data", 0))),
  child(Template::parse(config)) {}


void RequestTmpl::apply(const ResolverPtr &resolver, cb_t done) {
  auto error_cb =
    [done] (const string &msg, const JSON::ValuePtr &data) {
      JSON::ValuePtr err = new JSON::Dict;

      err->insert("error", msg);
      if (data.isSet()) err->insert("data", data);

      done(HTTP_INTERNAL_SERVER_ERROR, err);
    };

  auto cb =
    [this, done, error_cb, resolver] (HTTP::Request &outReq) {
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
        if (child.isSet()) child->apply(resolver->makeChild(data), done);
        else done(outReq.getResponseCode(), data);

      } catch (Exception &e) {
        LOG_DEBUG(3, e);
        error_cb(e.getMessage(), data);

      } catch (std::exception &e) {error_cb(e.what(), data);}
    };

  // Use config method if specified otherwise use method caller used
  auto req = resolver->getRequest();
  HTTP::Method method = this->method ? this->method : req->getMethod();

  auto &client = req->cast<Transaction>().getApp().getEventClient();
  auto outReq = client.call(resolver->format(url), method, cb);
  headers.set(*outReq);

  if (dataTmpl.isSet()) {
    if (!outReq->outHas("Content-Type"))
      outReq->outSet("Content-Type", "application/json");

    auto cb =
      [outReq, done] (HTTP::Status status, const JSON::ValuePtr &data) {
        if (status == HTTP_OK) {
          LOG_DEBUG(3, "Sending: " << *data);
          outReq->getOutputBuffer().add(data->toString());
          outReq->send();

        } else done(status, data);
      };

    dataTmpl->apply(resolver, cb);

  } else outReq->send();
}
