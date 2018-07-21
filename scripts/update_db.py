#!/usr/bin/env python3

################################################################################
#                                                                              #
#                         This file is part of JmpAPI.                         #
#                                                                              #
#              Copyright (c) 2014-2018, Cauldron Development LLC               #
#                             All rights reserved.                             #
#                                                                              #
#         The JmpAPI Webserver is free software: you can redistribute          #
#        it and/or modify it under the terms of the GNU General Public         #
#         License as published by the Free Software Foundation, either         #
#       version 2 of the License, or (at your option) any later version.       #
#                                                                              #
#         The JmpAPI Webserver is distributed in the hope that it will         #
#        be useful, but WITHOUT ANY WARRANTY; without even the implied         #
#           warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR            #
#        PURPOSE.  See the GNU General Public License for more details.        #
#                                                                              #
#      You should have received a copy of the GNU General Public License       #
#                    along with this software.  If not, see                    #
#                       <http://www.gnu.org/licenses/>.                        #
#                                                                              #
#      In addition, BSD licensing may be granted on a case by case basis       #
#      by written permission from at least one of the copyright holders.       #
#         You may request written permission by emailing the authors.          #
#                                                                              #
#                For information regarding this software email:                #
#                               Joseph Coffland                                #
#                        joseph@cauldrondevelopment.com                        #
#                                                                              #
################################################################################

# Process options
from optparse import OptionParser

parser = OptionParser()
parser.add_option('-u', '--user', dest = 'user', help = 'DB user name',
                  default = 'root')
parser.add_option('', '--host', dest = 'host', help = 'DB host name',
                  default = 'localhost')
parser.add_option('-d', '--db', dest = 'db', help = 'DB name',
                  default = 'jmpapi')
parser.add_option('-r', '--reset', dest = 'reset', help = 'Reset DB',
                  default = False, action = 'store_true')
parser.add_option('-p', '--path', dest = 'path', help = 'Path to DB schema',
                  default = 'src/sql')


(options, args) = parser.parse_args()


# Ask pass
from getpass import getpass
db_pass = getpass('Password: ')


# Connect to DB
from mysql.connector import connect
from mysql.connector.errors import Error as MySQLError

db = connect(host = options.host, user = options.user, password = db_pass,
             database = options.db)
db.start_transaction()
cur = db.cursor()


# Get schema version
def version_to_str(v):
    return '.'.join(map(str, v))

def version_from_str(s):
    return tuple(map(int, s.split('.')))

version = None
try:
    cur.execute('SELECT value FROM config WHERE name = "version"')

    if cur.with_rows:
        version = version_from_str(cur.fetchone()[0])
        print('DB version', version_to_str(version))

except: pass


# Read updates
from glob import glob
import re

updateRE = re.compile(r'^.*-(\d+\.\d+\.\d+)\.sql')
updates = []

for path in glob(options.path + '/update-*.sql'):
    m = updateRE.match(path)
    v = version_from_str(m.group(1))
    updates.append((v, path))

updates = sorted(updates)


# Latest version
if len(updates): latest = updates[-1]
else: latest = (0, 0, 0)


# Update
def exec_file(filename):
    sql = open(filename, 'r').read()

    results = cur.execute(sql, multi = True)
    if results:
        for result in results:
            if result.with_rows:
                print(result.fetchall())


if version is None or options.reset:
    # Init DB
    exec_file(options.path + '/schema.sql')
    exec_file(options.path + '/triggers.sql')
    exec_file(options.path + '/procedures.sql')

else:
    # Apply DB updates
    for v, path in updates:
        if version < v:
            print('Applying update', version_to_str(v))
            exec_file(path)


# Update version
if version is None or version < latest or options.reset:
    cur.execute('REPLACE INTO config (name, value) VALUES ("version", "%s")' %
                version_to_str(latest))


# Commit
cur.close()
db.commit()
db.close()
