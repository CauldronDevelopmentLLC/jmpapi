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

#include "QueryHandler.h"

using namespace std;
using namespace cb;
using namespace JmpAPI;


QueryHandler::QueryHandler(const JSON::Value &config) :
  sql(config.getString("sql")), pass(config.getBoolean("pass", false)) {

  if (config.hasList("fields")) fields = config.get("fields");

  string ret = config.getString("return", fields.isNull() ? "ok" : "fields");

  if      (ret == "ok")     replyCB = &Transaction::returnOk;
  else if (ret == "hlist")  replyCB = &Transaction::returnHeadList;
  else if (ret == "list")   replyCB = &Transaction::returnList;
  else if (ret == "fields") replyCB = &Transaction::returnFields;
  else if (ret == "dict")   replyCB = &Transaction::returnDict;
  else if (ret == "one")    replyCB = &Transaction::returnOne;
  else if (ret == "bool")   replyCB = &Transaction::returnBool;
  else if (ret == "u64")    replyCB = &Transaction::returnU64;
  else if (ret == "s64")    replyCB = &Transaction::returnS64;
  else THROW("Unsupported query return type '" << ret << "'");
}


bool QueryHandler::operator()(Event::Request &req) {
  req.cast<Transaction>().setFields(fields);
  req.cast<Transaction>().query(replyCB, sql);

  return !pass;
}
