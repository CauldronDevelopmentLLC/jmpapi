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

var base = location.pathname.substring(0, location.pathname.lastIndexOf('/'));


var __app = {
  el: '#content',


  data: function () {
    return {
      loggedin: false,
      showConfirm: false,
      confirmMsg: '',
      msg: '',
      err: false,
      page: 'main',
      user: {},
      group: ''
    }
  },


  components: {
    'user-login': require('./modules/user-login.js'),
    'admin-main': require('./modules/admin-main.js'),
    'admin-user': require('./modules/admin-user.js'),
    'admin-group': require('./modules/admin-group.js')
  },


  methods: {
    message: function () {
      this.msg = '';
      this.err = false;
      for (var i = 0; i < arguments.length; i++) {
        var arg = arguments[i];
        if (typeof arg != 'string') arg = JSON.stringify(arg);
        this.msg += arg;
      }
    },


    error: function () {
      Array.prototype.unshift.call(arguments, 'ERROR: ')
      this.message.apply(this, arguments);
      this.err = true;
    },


    confirm: function (response) {
      this.showConfirm = false;
      this.confirmCB(response);
    },


    login: function (loggedin, user) {
      if (!loggedin) this.message('Login for access.');
      else if (!user.has_group('admin')) this.message('Admin group required.')
      else this.loggedin = true;
    },


    logout: function () {
      this.message('Logged out.')
      this.loggedin = false;
    },


    _api: function (config) {
      config.url = base + config.url;

      var d = $.ajax(config);

      if (typeof config.msgs == 'undefined') return d;

      d.done(function () {
        if (config.msgs.success) this.message(config.msgs.success);

      }.bind(this))

      d.fail(function (xhr, status, error) {
        var msg;

        function has(msg) {
          return typeof config.msgs[msg] != 'undefined';
        }

        if (xhr.status == 409 && has('conflict')) msg = config.msgs.conflict;
        else if (xhr.status == 401 && has('unauthorized'))
          msg = config.msgs.unauthorized;
        else if (has('fail')) msg = config.msgs.fail;
        else msg = xhr.responseJSON.error || status;

        if (msg) this.error(msg);
      }.bind(this))

      return d;
    },


    api: function (config) {
      if (typeof config.msgs != 'undefined' &&
          typeof config.msgs.confirm != 'undefined') {
        this.confirmMsg = config.msgs.confirm;
        this.showConfirm = true;

        var d = $.Deferred();

        this.confirmCB = function (response) {
          if (response == 'accept') {
            this._api(config).done(function (data) {
              d.resolve(data);

            }).fail(function (xhr, status, error) {
              d.reject(xhr, status, error);
            })

          } else d.reject();
        }

        return d.promise();

      } else return this._api(config);
    },


    edit_user: function (user) {
      this.page = 'user';
      this.user = user;
    },


    edit_group: function (group) {
      this.page = 'group';
      this.group = group;
    }
  }
}


$(function () {
  // Detect incompatible browsers
  if (!Object.defineProperty) {
    $('#incompatible-browser')
      .show()
      .find('.page-content')
      .append(
        $('<button>')
          .addClass('success')
          .text('Update')
          .click(function () {window.location = 'http://whatbrowser.org/'})
      )

    return
  }

  // Vue debugging
  Vue.config.debug = true;

  Vue.component('modal', require('./modules/modal.js'))

  // Vue app
  new Vue(__app);
})
