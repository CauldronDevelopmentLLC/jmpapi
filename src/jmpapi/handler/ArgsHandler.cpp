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

#include "ArgsHandler.h"

#include <cbang/event/Request.h>

using namespace std;
using namespace cb;
using namespace JmpAPI;


ArgsHandler::ArgsHandler(const JSON::ValuePtr &args) {
  for (unsigned i = 0; i < args->size(); i++)
    validators[args->keyAt(i)] = new ArgValidator(args->get(i));
}


bool ArgsHandler::operator()(Event::Request &req) {
  auto &args = *req.parseArgs();
  set<string> found;

  for (unsigned i = 0; i < args.size(); i++) {
    const string &name = args.keyAt(i);

    found.insert(name);

    auto it = validators.find(name);
    if (it == validators.end()) continue; // Ignore unrecognized args

    try {
      (*it->second)(req, *args.get(i));

    } catch (const Exception &e) {
      if (e.getCode() == HTTP_UNAUTHORIZED)
        THROWX("Access denied", HTTP_UNAUTHORIZED);

      THROWX("Invalid argument '" << name << "=" << args.getAsString(i)
              << "': " << e.getMessage(), HTTP_BAD_REQUEST);
    }
  }

  // Make sure all required arguments were found and handle defaults
  vector<string> missing;
  for (auto it = validators.begin(); it != validators.end(); it++)
    if (found.find(it->first) == found.end()) {
      const ArgValidator &av = *it->second;

      if (av.hasDefault()) args.insert(it->first, av.getDefault());
      else if (!av.isOptional()) missing.push_back(it->first);
    }

  if (!missing.empty())
    THROWX("Missing argument" << (1 < missing.size() ? "s" : "") << ": "
            << String::join(missing, ", "), HTTP_BAD_REQUEST);

  return false;
}
