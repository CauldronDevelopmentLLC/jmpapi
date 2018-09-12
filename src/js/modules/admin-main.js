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
  template: '#admin-main-template',


  data: function () {
    return {
      users: {},
      groups: [],
      providers: [],
      new_user_email: '',
      new_user_provider: '',
      new_group: ''
    }
  },


  mounted: function () {this.update()},


  methods: {
    update: function () {
      this.$root.api({url: '/groups'})
        .done(function (groups) {this.groups = groups}.bind(this))
      this.$root.api({url: '/users'})
        .done(function (users) {this.users = users}.bind(this))

      this.$root.api({url: '/login/providers'})
        .done(function (providers) {
          this.providers = providers
          this.new_user_provider = providers[0]
        }.bind(this))
    },


    edit_user: function (user) {this.$root.edit_user(user)},
    edit_group: function (group) {this.$root.edit_group(group)},


    dragstart: function (event, user) {
      event.dataTransfer.setData('text', JSON.stringify(user));
    },


    dragover: function (event) {event.preventDefault()},


    drop: function (event, group) {
      event.preventDefault();

      var user = JSON.parse(event.dataTransfer.getData('text'));

      this.$root.api({
        method: 'PUT',
        url: '/users/' + user.id + '/groups/' + group,
        msgs: {
          success: 'Added user ' + user.name + ' to group ' + group + '.',
          conflict: 'User ' + user.name + ' already in group ' + group + '.'
        }
      })
    },


    add_user: function () {
      this.$root.api({
        method: 'POST',
        url: '/users',
        data: JSON.stringify({
          provider: this.new_user_provider,
          email: this.new_user_email,
          name: this.new_user_email.replace(/@.*$/, '')
        }),
        contentType: 'application/json',
        msgs: {
          success: 'Added user ' + this.new_user_email + '.',
          conflict: 'User ' + this.new_user_email + ' already exists.'
        }

      }).done(function () {
        this.new_user_email = '';
        this.update();
      })
    },


    delete_user: function (user) {
      this.$root.api({
        method: 'DELETE',
        url: '/users/' + user.id,
        msgs: {
          confirm: 'Are you sure you want to delete user ' + user.name + '?',
          success: 'Deleted user ' + user.name + '.'
        }

      }).done(this.update);
    },


    add_group: function () {
      this.$root.api({
        method: 'PUT',
        url: '/groups/' + this.new_group,
        msgs: {
          success: 'Added group ' + this.new_group + '.',
          conflict: 'Group ' + this.new_group + ' already exists.'
        }

      }).done(this.update)
    },


    delete_group: function (group) {
      this.$root.api({
        method: 'DELETE',
        url: '/groups/' + group,
        msgs: {
          confirm: 'Are you sure you want to delete group ' + group + '?',
          success: 'Deleted group ' + group + '.'
        }

      }).done(this.update);
    }
  }
}
