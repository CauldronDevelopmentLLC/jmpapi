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
  template: '#admin-group-template',
  props: ['group'],


  data: function () {
    return {
      users: []
    }
  },


  mounted: function () {
    this.$root.api({url: '/groups/' + this.group + '/users'})
      .done(function (users) {this.users = users}.bind(this))
  },


  methods: {
    back: function () {this.$parent.page = 'main'},


    remove_user: function (user) {
      this.$root.api({
        method: 'DELETE',
        url: '/users/' + user.id + '/groups/' + this.group,
        msgs: {
          success:
          'Removed user ' + user.name + ' from group ' + this.group + '.'
        }

      }).done(function () {
        this.users.splice(this.users.indexOf(user), 1);
      }.bind(this))
    }
  }
}
