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

#include "App.h"

#include <cbang/Catch.h>

#include <cbang/os/SystemUtilities.h>

#include <cbang/openssl/SSLContext.h>
#include <cbang/time/Timer.h>
#include <cbang/time/Time.h>
#include <cbang/log/Logger.h>
#include <cbang/event/Event.h>
#include <cbang/db/maria/EventDB.h>
#include <cbang/json/JSON.h>

#include <algorithm>

#include <stdlib.h>
#include <unistd.h>

using namespace JmpAPI;
using namespace cb;
using namespace std;


App::App() :
  ServerApplication("JmpAPI", App::_hasFeature), base(true), dns(base),
  client(base, dns, new SSLContext), procPool(base), googleAuth(options),
  githubAuth(options), facebookAuth(options), dbHost("localhost"),
  dbUser("jmpapi"), dbName("jmpapi"), dbPort(3306), dbTimeout(5), server(*this),
  config(new JSON::Dict) {

  cmdLine.setAllowPositionalArgs(true);

  options.pushCategory("JmpAPI");
  options.add("http-root", "Root directory for static files."
              )->setDefault("/usr/share/jmpapi/http");
  options.add("jsonp", "Respond with JSONP format data if this argument is "
              "present in an API call.");
  options.add("session-cookie", "Session cookie name")->setDefault("sid");
  options.popCategory();

  options.pushCategory("SSL");
  options.add("ssl-ca-certificates", "Path to trusted SSL CA certificates file"
    )->setDefault("/etc/ssl/certs/ca-certificates.crt");
  options.add("ssl-cipher-list", "Allowed OpenSSL ciphers"
              )->setDefault("HIGH:!aNULL:!PSK:!SRP:!MD5:!RC4");
  options.popCategory();

  options.pushCategory("Debugging");
  options.add("debug-libevent", "Enable verbose libevent debugging"
              )->setDefault(false);
  options.popCategory();

  options.pushCategory("Database");
  options.addTarget("db-host", dbHost, "DB host name");
  options.addTarget("db-user", dbUser, "DB user name");
  options.addTarget("db-pass", dbPass, "DB password")->setObscured();;
  options.addTarget("db-name", dbName, "DB name");
  options.addTarget("db-port", dbPort, "DB port");
  options.addTarget("db-timeout", dbTimeout, "DB timeout");
  options.popCategory();

  // Seed random number generator
  srand48(Time::now());

  // Enable libevent logging
  Event::Event::enableLogging(3);

  // Handle exit signal
  base.newSignal(SIGINT, this, &App::signalEvent)->add();
  base.newSignal(SIGTERM, this, &App::signalEvent)->add();
}


bool App::_hasFeature(int feature) {
  if (feature == FEATURE_SIGNAL_HANDLER) return false;
  return ServerApplication::_hasFeature(feature);
}


string App::getSessionCookieName() const {
  return options["session-cookie"].toString();
}


SmartPointer<MariaDB::EventDB> App::getDBConnection(bool blocking) {
  // TODO Limit the total number of active connections

  SmartPointer<MariaDB::EventDB> db = new MariaDB::EventDB(base);

  // Configure
  db->setConnectTimeout(dbTimeout);
  db->setReadTimeout(dbTimeout);
  db->setWriteTimeout(dbTimeout);
  db->setReconnect(true);
  if (!blocking) db->enableNonBlocking();
  db->setCharacterSet("utf8");

  // Connect
  if (blocking) db->connect(dbHost, dbUser, dbPass, dbName, dbPort);
  else db->connectNB(dbHost, dbUser, dbPass, dbName, dbPort);

  return db;
}


void App::beforeDroppingPrivileges() {
  // Libevent debugging
  if (options["debug-libevent"].toBoolean()) Event::Event::enableDebugLogging();

  LOG_DEBUG(3, "MySQL client version: " << MariaDB::DB::getClientInfo());
  MariaDB::DB::libraryInit();
  MariaDB::DB::threadInit();

  server.init();
}


void App::afterCommandLineParse() {
  // Load config
  const vector<string> &configs = cmdLine.getPositionalArgs();
  for (unsigned i = 0; i < configs.size(); i++) {
    LOG_INFO(1, "Loading config " << configs[i]);
    config->merge(*JSON::YAMLReader::parse(configs[i]));
  }

  // Apply options
  if (config->hasDict("options")) options.read(*config->get("options"));

  // Configure SSL
  SSLContext &sslCtx = *client.getSSLContext();

  string caCertsFile = options["ssl-ca-certificates"];
  if (!caCertsFile.empty())
    sslCtx.loadVerifyLocationsFile(caCertsFile);

  string sslCipherList = options["ssl-cipher-list"];
  if (!sslCipherList.empty()) {
    sslCtx.setCipherList(sslCipherList);
    server.getSSLContext()->setCipherList(sslCipherList);
  }

  ServerApplication::afterCommandLineParse();
}


void App::run() {
  try {
    base.dispatch();
    procPool.shutdown();
    LOG_INFO(1, "Clean exit");
  } CATCH_ERROR;
}


void App::signalEvent(Event::Event &e, int signal, unsigned flags) {
  base.loopExit();
}
