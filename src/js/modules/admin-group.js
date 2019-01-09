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


    group_user_remove: function (user) {
      this.$root.user_group_remove(user.id, user.name, this.group)
        .done(function () {
          this.members.splice(this.members.indexOf(user), 1);
          this.nonmembers.push(user);
        }.bind(this))
    },


    group_user_add: function (user) {
      this.$root.user_group_add(user.id, user.name, this.group)
        .done(function () {
          this.nonmembers.splice(this.nonmembers.indexOf(user), 1);
          this.members.push(user);
        }.bind(this))
    }
  }
}
