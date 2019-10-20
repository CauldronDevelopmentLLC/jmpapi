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

'use strict'


function get_arg_example(arg) {
  if (typeof arg.example != 'undefined') return arg.example;
  if (typeof arg.default != 'undefined') return arg.default;
  if (typeof arg.enum != 'undefined') return arg.enum[0];

  var type = arg.type || 'string';
  return '<' + type + '>';
}


module.exports = {
  template: '#api-method-template',
  props: ['method', 'path', 'config'],


  data: function () {
    return {
      open: false,
      selected: false
    }
  },


  components: {
    'api-arg': require('./api-arg')
  },


  computed: {
    name: function () {return this.method + '-' + this.path},


    klass: function () {
      var klass = this.open ? 'opened' : 'closed';
      if (this.selected) klass += ' highlight';
      return klass;
    }
  },


  mounted: function () {
    window.addEventListener('hashchange', this.update);
    this.update();

    // Scroll into view if open
    if (this.open)
      Vue.nextTick(function (el) {
        return function () {el.scrollIntoView()}
      }(this.$el))
  },


  methods: {
    update: function () {
      this.selected = location.hash.substr(1) == this.name;
      if (this.selected) this.open = true;
    },


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

        if (entry.allow || entry.deny) s += ' -H "Authorization: $TOKEN"'

        s += ' ' + this.api_path(path);

        if (entry.args)
          for (var name in entry.args)
            if (!entry.args[name].url)
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
