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

#include "ArgFilterHandler.h"

#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/json/Reader.h>
#include <cbang/event/BufferDevice.h>

#include <poll.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


ArgFilterHandler::ArgFilterHandler(const JSON::Value &config) :
  errLog("arg-filter", CBANG_LOG_DOMAIN, CBANG_LOG_WARNING_LEVEL) {

  if (config.isString()) Subprocess::parse(config.toString(), cmd);
  else {
    if (!config.isList()) THROW("Invalid arg-filter config");

    for (unsigned i = 0; i < config.size(); i++)
      cmd.push_back(config.getAsString(i));
  }
}


bool ArgFilterHandler::operator()(Event::Request &req) {
  try {
    auto &args = *req.parseArgs();

    exec(cmd, Subprocess::SHELL | Subprocess::REDIR_STDERR |
         Subprocess::REDIR_STDOUT | Subprocess::REDIR_STDIN);

    struct pollfd fds[2];
    auto &in  = getStdIn();
    fds[0].fd = getPipeHandle(1, false);
    fds[1].fd = getPipeHandle(2, false);
    Event::Buffer outBuf;

    in << args;

    fds[0].events = fds[1].events = POLLIN;

    while (0 < poll(fds, 2, -1)) {
      if (fds[0].revents & POLLIN) outBuf.read(fds[0].fd, 4096);
      if (fds[1].revents & POLLIN) errLog.read(fds[1].fd);
    }

    int ret = wait();

    if (ret == 0) {
      Event::BufferStream<char> outStr(outBuf);
      args.merge(*JSON::Reader::parse(outStr));
      return false;
    }

    if (99 < ret && ret < 600) {
      req.send(outBuf);
      req.reply((Event::HTTPStatus::enum_t)ret);
      return true;
    }

  } CATCH_ERROR;

  THROWX("Internal server configuration error", HTTP_INTERNAL_SERVER_ERROR);
}
