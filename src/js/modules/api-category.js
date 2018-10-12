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


module.exports = {
  template: '#api-category-template',
  props: ['name', 'config'],


  data: function () {
    return {
      open: true
    }
  },


  components: {
    'api-method': require('./api-method')
  },


  methods: {
    expand: function () {
      for (var i = 0; i < this.$children.length; i++)
        this.$children[i].open = true;
    },


    collapse: function () {
      for (var i = 0; i < this.$children.length; i++)
        this.$children[i].open = false;
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
