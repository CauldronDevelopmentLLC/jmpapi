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


module.exports = {
  template: '#api-category-template',
  props: ['name', 'config'],


  data: function () {
    return {
      open: true,
      expand: true
    }
  },


  components: {
    'api-method': require('./api-method')
  },


  methods: {
    toggle: function () {
      for (var i = 0; i < this.$children.length; i++)
        this.$children[i].open = this.expand;

      this.expand = !this.expand;
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
