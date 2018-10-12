/******************************************************************************\

                          This file is part of JmpAPI.

               Copyright (c) 2014-2018, Cauldron Development LLC
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

       In addition, BSD licensing may be granted on a case by case basis
       by written permission from at least one of the copyright holders.
          You may request written permission by emailing the authors.

                 For information regarding this software email:
                                Joseph Coffland
                         joseph@cauldrondevelopment.com

\******************************************************************************/

#pragma once

#include "Transaction.h"

#include <cbang/event/HTTPHandler.h>
#include <cbang/db/maria/EventDBCallback.h>


namespace JmpAPI {
  class QueryHandler : public cb::Event::HTTPHandler {
    std::string sql;
    bool pass;
    Transaction::event_db_member_functor_t replyCB;

  public:
    QueryHandler(const cb::JSON::Value &config);

    // From HTTPHandler
    bool operator()(cb::Event::Request &req);
  };
}
