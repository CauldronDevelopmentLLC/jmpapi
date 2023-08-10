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

#pragma once

#include "Server.h"

#include <cbang/ServerApplication.h>
#include <cbang/db/maria/EventDB.h>

#include <cbang/net/IPAddress.h>
#include <cbang/net/SessionManager.h>

#include <cbang/auth/GoogleOAuth2.h>
#include <cbang/auth/GitHubOAuth2.h>
#include <cbang/auth/FacebookOAuth2.h>

#include <cbang/event/Base.h>
#include <cbang/event/DNSBase.h>
#include <cbang/event/Client.h>
#include <cbang/event/SubprocessPool.h>

namespace cb {
  namespace Event {class Event;}
  namespace MariaDB {class EventDB;}
}


namespace JmpAPI {
  class App : public cb::ServerApplication {
    cb::Event::Base base;
    cb::Event::DNSBase dns;
    cb::Event::Client client;
    cb::Event::SubprocessPool procPool;

    cb::GoogleOAuth2 googleAuth;
    cb::GitHubOAuth2 githubAuth;
    cb::FacebookOAuth2 facebookAuth;

    std::string dbHost;
    std::string dbUser;
    std::string dbPass;
    std::string dbName;
    uint32_t dbPort;
    unsigned dbTimeout;

    Server server;
    cb::SessionManager sessionManager;
    cb::JSON::ValuePtr config;

  public:
    App();

    static bool _hasFeature(int feature);

    cb::Event::Base &getEventBase() {return base;}
    cb::Event::DNSBase &getEventDNS() {return dns;}
    cb::Event::Client &getEventClient() {return client;}
    cb::Event::SubprocessPool &getProcPool() {return procPool;}

    cb::GoogleOAuth2 &getGoogleAuth() {return googleAuth;}
    cb::GitHubOAuth2 &getGitHubAuth() {return githubAuth;}
    cb::FacebookOAuth2 &getFacebookAuth() {return facebookAuth;}

    Server &getServer() {return server;}
    cb::SessionManager &getSessionManager() {return sessionManager;}
    std::string getSessionCookieName() const;
    cb::JSON::ValuePtr getConfig() const {return config;}

    cb::SmartPointer<cb::MariaDB::EventDB>
    getDBConnection(bool blocking = false);

    // From cb::ServerApplication
    void beforeDroppingPrivileges();

    // From cb::Application
    void afterCommandLineParse();
    void run();

    void signalEvent(cb::Event::Event &e, int signal, unsigned flags);
  };
}
