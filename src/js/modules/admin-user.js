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
  template: '#admin-user-template',
  props: ['uid'],


  data: function () {
    return {
      user: {},
      groups: [],
      providers: [],
      association_new: {
        provider: '',
        email: '',
        name: ''
      }
    }
  },


  mounted: function () {
    this.update_user();
    this.update_groups();

    this.$root.get_providers(function (providers) {
      this.providers = providers
      this.association_new.provider = providers[0]
    }.bind(this))
  },


  methods: {
    update_user: function () {
      this.$root.api({url: '/users/' + this.uid})
        .done(function (user) {this.user = user}.bind(this))
    },


    update_groups: function () {
      this.$root.api({url: '/users/' + this.uid + '/groups'})
        .done(function (groups) {this.groups = groups}.bind(this))
    },


    back: function () {this.$parent.page = 'main'},


    user_save: function () {
      this.$root.user_save(this.uid, this.user).done(this.back)
    },


    user_delete: function () {
      this.$root.user_delete(this.uid, this.user.name).done(this.back)
    },


    user_association_set: function () {
      var assoc = this.association_new;

      this.$root.user_association_set(this.uid, this.user.name, assoc)
        .done(function () {
          assoc.email = '';
          this.update_user()
        }.bind(this));
    },


    user_association_remove: function (association) {
      this.$root.user_association_remove(this.uid, this.user.name, association)
        .done(this.update_user)
    },


    user_group_remove: function (group) {
      this.$root.user_group_remove(this.uid, this.user.name, group.name)
        .done(function () {group.member = 0})
    },


    user_group_add: function (group) {
      this.$root.user_group_add(this.uid, this.user.name, group.name)
        .done(function () {group.member = 1})
    }
  }
}
