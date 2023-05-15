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

#pragma once

#include <cbang/event/AsyncSubprocess.h>
#include <cbang/event/StreamEventBuffer.h>
#include <cbang/event/StreamLogger.h>
#include <cbang/event/HTTPRequestHandler.h>

#include <vector>

namespace cb {
  namespace Event {
    class Base;
    class Request;
  }
}


namespace JmpAPI {
  class ArgFilterProcess : public cb::Event::AsyncSubprocess {
    cb::Event::Base &base;
    cb::SmartPointer<cb::Event::HTTPRequestHandler> child;
    std::vector<std::string> cmd;
    cb::Event::Request &req;

    cb::SmartPointer<cb::Event::StreamEventBuffer> inStr;
    cb::SmartPointer<cb::Event::StreamEventBuffer> outStr;
    cb::SmartPointer<cb::Event::StreamLogger>      errStr;

  public:
    ArgFilterProcess(
      cb::Event::Base &base,
      cb::SmartPointer<cb::Event::HTTPRequestHandler> child,
      const std::vector<std::string> &cmd, cb::Event::Request &req) :
      base(base), child(child), cmd(cmd), req(req) {}

    // From AsyncSubprocess
    void exec();
    void done();
  };
}
