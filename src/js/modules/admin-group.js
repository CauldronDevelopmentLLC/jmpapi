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
      loading: true,
      members: [],
      nonmembers: []
    }
  },


  mounted: function () {
    this.loading = true;

    var d1 = this.$root.api({url: '/groups/' + this.group + '/members'});
    var d2 = this.$root.api({url: '/groups/' + this.group + '/nonmembers'});

    $.when(d1, d2).done(function (members, nonmembers) {
      this.members = members[0];
      this.nonmembers = nonmembers[0];

    }.bind(this)).fail(function () {
      this.$root.error('Failed to load group ' + this.group + '.');

    }.bind(this)).always(function () {this.loading = false}.bind(this));
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

        this.members.splice(this.members.indexOf(user), 1);
        this.nonmembers.push(user);
      }.bind(this))
    },


    add_user: function (user) {
      this.$root.api({
        method: 'PUT',
        url: '/users/' + user.id + '/groups/' + this.group,
        msgs: {
          success:
          'Added user ' + user.name + ' to group ' + this.group + '.'
        }

      }).done(function () {

        this.nonmembers.splice(this.nonmembers.indexOf(user), 1);
        this.members.push(user);
      }.bind(this))
    }
  }
}
