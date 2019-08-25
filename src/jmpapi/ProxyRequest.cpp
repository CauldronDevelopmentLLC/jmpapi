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

#include "ProxyRequest.h"
#include "Resolver.h"

#include <jmpapi/App.h>
#include <jmpapi/Transaction.h>

#include <cbang/event/Client.h>
#include <cbang/json/JSON.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


namespace {
  Event::HTTPStatus conErrToStatus(Event::ConnectionError err) {
    switch (err) {
    case Event::Enum::CONN_ERR_TIMEOUT:
      return Event::Enum::HTTP_GATEWAY_TIME_OUT;

    case Event::Enum::CONN_ERR_CONNECT:
      return Event::Enum::HTTP_SERVICE_UNAVAILABLE;

    default: return Event::Enum::HTTP_BAD_GATEWAY;
    }
  }
}


ProxyRequest::ProxyRequest(const JSON::Value &config) :
  url(config.getString("url")),
  method(Event::RequestMethod::parse(config.getString("method", "UNKNOWN"))),
  headers(config.get("request-headers", 0)),
  data(Template::parse(config.get("data", 0))),
  tmpl(Template::parse(config.get("template", 0))) {}


void ProxyRequest::request(const ResolverPtr &resolver, done_cb_t done) {
  auto error_cb =
    [done] (const string &msg, const JSON::ValuePtr &data) {
      JSON::ValuePtr err = new JSON::Dict;

      err->insert("error", msg);
      if (data.isSet()) err->insert("data", data);

      done(HTTP_INTERNAL_SERVER_ERROR, err);
    };

  auto cb =
    [this, done, error_cb, resolver] (Event::Request &outReq) {
      JSON::ValuePtr data;

      try {
        auto err = outReq.getConnectionError();
        if (err) return done(conErrToStatus(err), 0);

        if (!outReq.getInputBuffer().getLength())
          return done(HTTP_BAD_GATEWAY, 0);

        // Parse JSON
        string response = outReq.getInput();
        try {
          data = JSON::Reader::parseString(response);

        } catch (Exception &e) {
          JSON::ValuePtr err = new JSON::Dict;
          err->insert("error", e.getMessage());
          err->insert("response", response);

          return done(HTTP_BAD_GATEWAY, err);
        }

        // Template response
        if (tmpl.isSet()) {
          Event::HTTPStatus::enum_t status = outReq.getResponseCode();

          auto tmpl_cb =
            [done, status] (bool ok, const JSON::ValuePtr &data) {
              done(ok ? status : HTTP_BAD_GATEWAY, data);
            };

          tmpl->apply(resolver->makeChild(data), tmpl_cb);

        } else done(outReq.getResponseCode(), data);

      } catch (Exception &e) {error_cb(e.getMessage(), data);}
      catch (std::exception &e) {error_cb(e.what(), data);}
    };

  // Use config method if specified otherwise use method caller used
  auto req = resolver->getRequest();
  Event::RequestMethod method = this->method ? this->method : req->getMethod();

  auto &client = req->cast<Transaction>().getApp().getEventClient();
  auto outReq = client.call(resolver->format(url), method, cb);
  headers.set(*outReq);

  if (data.isSet()) {
    if (!outReq->outHas("Content-Type"))
      outReq->outSet("Content-Type", "application/json");

    auto cb =
      [outReq, done] (bool ok, const JSON::ValuePtr &data) {
        if (ok) {
          outReq->getOutputBuffer().add(data->toString());
          outReq->send();

        } else done(HTTP_INTERNAL_SERVER_ERROR, data);
      };

    data->apply(resolver, cb);

  } else outReq->send();
}
