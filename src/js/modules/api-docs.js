/******************************************************************************\

                    Copyright 2018. Cauldron Development LLC
                              All Rights Reserved.

                  For information regarding this software email:
                                 Joseph Coffland
                          joseph@cauldrondevelopment.com

        This software is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
        as published by the Free Software Foundation, either version 2.1 of
        the License, or (at your option) any later version.

        This software is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
        License along with the C! library.  If not, see
        <http://www.gnu.org/licenses/>.

\******************************************************************************/

'use strict'


function get_arg_example(arg) {
  if (typeof arg.example != 'undefined') return arg.example;
  if (typeof arg.default != 'undefined') return arg.default;
  if (typeof arg.enum != 'undefined') return arg.enum[0];

  var type = arg.type || 'string';
  return '<' + type + '>';
}


module.exports = {
  template: '#api-docs-template',


  data: function () {
    return {
      api: {}
    }
  },


  components: {
    'api-arg': require('./api-arg')
  },


  mounted: function () {
    $.getJSON(location.pathname + 'api',
              function (api) {this.api = api}.bind(this))
  },


  methods: {
    api_path: function (path) {
      var base = location.protocol + '//' + location.hostname;
      if (location.port) base += ':' + location.port;
      return base + path;
    },


    example: function (method, path, entry) {
      if (method.indexOf('|') != -1)
        return method.split('|').map(function (method) {
          return this.example(method, path, entry);
        }.bind(this)).join('\n');

      else {
        var s = '';
        s += 'curl';
        if (method != 'GET') s += ' -X ' + method;
        else if (entry.args) s += ' -G'

        if (entry.allow || entry.deny) s += ' -H "Authorization: <token>"'

        s += ' ' + this.api_path(path);

        if (entry.args)
          for (var name in entry.args)
            s += ' -d ' + name + '=' + get_arg_example(entry.args[name]);

        return s;
      }
    },


    auth_list: function (list) {
      function auth_entry(entry) {
        if (entry[0] == '$') return 'group:' + entry.substr(1);
        return 'user:' + entry;
      }

      return list.map(auth_entry).join(', ');
    }
  }
}
