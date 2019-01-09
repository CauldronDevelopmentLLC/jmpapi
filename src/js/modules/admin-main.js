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
  template: '#admin-main-template',


  data: function () {
    return {
      config: [],
      users: {},
      groups: [],
      providers: [],
      user_new: {
        email: '',
        provider: '',
      },
      group_new: ''
    }
  },


  mounted: function () {
    this.update()

    this.$root.get_providers(function (providers) {
      this.providers = providers
      this.user_new.provider = providers[0]
    }.bind(this))

    this.$root.api({url: '/config'})
      .done(function (config) {this.config = config}.bind(this))
  },


  methods: {
    update: function () {
      this.$root.api({url: '/groups'})
        .done(function (groups) {this.groups = groups}.bind(this))

      this.$root.api({url: '/users'})
        .done(function (users) {this.users = users}.bind(this))
    },


    config_set: function (entry) {this.$root.config_set(entry)},
    user_edit: function (user) {this.$root.user_edit(user)},
    group_edit: function (group) {this.$root.group_edit(group)},


    dragstart: function (event, user) {
      event.dataTransfer.setData('text', JSON.stringify(user));
    },


    dragover: function (event) {event.preventDefault()},


    drop: function (event, group) {
      event.preventDefault();
      var user = JSON.parse(event.dataTransfer.getData('text'));
      this.$root.user_group_add(user.id, user.name, group)
    },


    user_add: function () {
      this.$root.user_add(
        this.user_new.provider,
        this.user_new.email

      ).done(function () {
        this.user_new.email = '';
        this.update();
      }.bind(this))
    },


    user_delete: function (user) {
      this.$root.user_delete(user.id, user.name).done(this.update)
    },


    group_add: function () {
      this.$root.group_add(this.group_new).done(this.update)
    },


    group_delete: function (group) {
      this.$root.group_delete(group).done(this.update)
    }
  }
}
