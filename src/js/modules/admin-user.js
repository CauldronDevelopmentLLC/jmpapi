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
  template: '#admin-user-template',
  props: ['user'],


  data: function () {
    return {
      groups: []
    }
  },


  mounted: function () {
    this.$root.api({
      url: '/users/' + this.user.id + '/groups'
    }).done(function (groups) {this.groups = groups}.bind(this))
  },


  methods: {
    back: function () {this.$parent.page = 'main'},


    remove_group: function (group) {
      this.$root.api({
        method: 'DELETE',
        url: '/users/' + this.user.id + '/groups/' + group.name,
        msgs: {
          success:
          'Removed user ' + this.user.name + ' from group ' + group.name + '.'
        }
      }).done(function () {group.member = 0})
    },


    add_group: function (group) {
      this.$root.api({
        method: 'PUT',
        url: '/users/' + this.user.id + '/groups/' + group.name,
        msgs: {
          success:
          'Added user ' + this.user.name + ' to group ' + group.name + '.'
        }
      }).done(function () {group.member = 1})
    }
  }
}
