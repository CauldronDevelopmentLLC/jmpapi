/******************************************************************************\

                           This file is part of JmpAPI.

                Copyright (c) 2014-2024, Cauldron Development LLC
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

#include "Server.h"
#include "API.h"

#include <cbang/ServerApplication.h>

#include <cbang/event/Base.h>
#include <cbang/event/Event.h>
#include <cbang/event/SubprocessPool.h>
#include <cbang/http/Client.h>
#include <cbang/openssl/SSLContext.h>


namespace JmpAPI {
  class App : public cb::ServerApplication {
    cb::Event::Base base;
    cb::SSLContext sslCtx;

    cb::HTTP::Client client;
    cb::Event::SubprocessPool procPool;
    Server server;
    API api;
    cb::JSON::ValuePtr config;
    cb::Event::EventPtr sigintEvent;
    cb::Event::EventPtr sigtermEvent;

  public:
    App();

    static bool _hasFeature(int feature);

    cb::SSLContext &getSSLContext() {return sslCtx;}
    API &getAPI() {return api;}
    cb::Event::Base &getEventBase() {return base;}

    // From cb::ServerApplication
    void beforeDroppingPrivileges() override;

    // From cb::Application
    void afterCommandLineParse() override;
    void run() override;

    void signalEvent(cb::Event::Event &e, int signal, unsigned flags);
  };
}
