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
  template: '#user-login-template',


  data: function () {
    return {
      providers: [],
      user: {},
      loggedin: false
    }
  },


  mounted: function () {
    // Get list of login providers
    this.$root.api({url: '/login/list'}).done(function (providers) {
      this.providers = providers;
    }.bind(this))

    // Check if we are already logged in
    this.$root.api({
      url: '/login/',
      msgs: {unauthorized: ''}

    }).done(function (user) {
      user.has_group = function () {
        return function (group) {
          return typeof user.groups != 'undefined' &&
            user.groups.indexOf(group) != -1
        }
      }();

      this.loggedin = true;
      this.$emit('login', true, this.user = user);

    }.bind(this)).fail(function (xhr, status, error) {
      if (xhr.status == 401) this.$emit('login', false);

    }.bind(this))
  },


  methods: {
    login: function (provider) {
      this.$root.api({url: '/login/' + provider})
        .done(function (data) {location.href = data.redirect})
    },


    logout: function () {
      this.$root.api({
        method: 'PUT',
        url: '/logout'

      }).done(function () {
        this.user = {}
        this.loggedin = false;
        this.$emit('logout')
      }.bind(this))
    }
  }
}
