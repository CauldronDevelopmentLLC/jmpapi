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
    this.$root.get_providers(function (providers) {
      this.providers = providers;
    }.bind(this))

    // Check if we are already logged in
    this.$root.api({
      url: '/login/',
      msgs: {unauthorized: ''}

    }).done(function (user) {
      user.has_group = function () {
        return function (group) {return user.group[group]}
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
