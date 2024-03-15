/******************************************************************************\

                           This file is part of JmpAPI.

                Copyright (c) 2014-2023, Cauldron Development LLC
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

#include "ArgFilterProcess.h"

#include <cbang/Catch.h>
#include <cbang/event/Base.h>
#include <cbang/http/Request.h>
#include <cbang/http/RequestErrorHandler.h>
#include <cbang/http/Enum.h>
#include <cbang/log/Logger.h>
#include <cbang/json/Reader.h>

using namespace std;
using namespace cb;
using namespace cb::Event;
using namespace JmpAPI;


void ArgFilterProcess::exec() {
  Subprocess::exec(cmd, Subprocess::SHELL | Subprocess::REDIR_STDERR |
                   Subprocess::REDIR_STDOUT | Subprocess::REDIR_STDIN);

  // Make pipes non-blocking
  for (unsigned i = 0; i < 3; i++)
    getPipe(i).setBlocking(false);

  inStr  = new StreamEventBuffer(base, getPipeIn(), Enum::EVENT_WRITE);
  outStr = new StreamEventBuffer(base, getPipeOut(), Enum::EVENT_READ);
  errStr = new StreamLogger(
    base, getPipeErr(), SSTR("PID:" << getPID() << ':'),
    CBANG_LOG_DOMAIN, CBANG_LOG_WARNING_LEVEL);

  inStr->add(req.getArgs()->toString());
  inStr->write();
}


void ArgFilterProcess::done() {
  try {
    outStr->read();
    errStr->flush();

    string output = outStr->toString();
    if (output.empty()) THROW("No output from arg-filter");

    auto results  = JSON::Reader::parseString(output);
    unsigned code = results->getU32("code", 200);

    if (code == 200) {
      if (results->hasDict("data"))
        req.getArgs()->merge(results->getDict("data"));

      (HTTP::RequestErrorHandler(*child))(req);
      return;
    }

    if (results->hasString("error")) LOG_WARNING(results->getString("error"));
    req.sendError((HTTP::Status::enum_t)code);
    return;

  } CATCH_ERROR;

  req.sendError(HTTP::Enum::HTTP_INTERNAL_SERVER_ERROR);
}
