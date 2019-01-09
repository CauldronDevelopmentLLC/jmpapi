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

var base = location.pathname.substring(0, location.pathname.lastIndexOf('/'));


function trimif(s) {
  s = typeof s == 'undefined' ? '' : s.trim();
  return s.length ? s : undefined;
}


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
      uid: undefined,
      group: ''
    }
  },


  components: {
    'user-login': require('./modules/user-login.js'),
    'admin-main': require('./modules/admin-main.js'),
    'admin-user': require('./modules/admin-user.js'),
    'admin-group': require('./modules/admin-group.js')
  },


  created: function () {
    this.providers_promise = this.api({url: '/login/providers'});
  },


  methods: {
    clear: function () {
      this.msg = '';
      this.err = false;
    },


    message: function () {
      this.clear();

      for (var i = 0; i < arguments.length; i++) {
        var arg = arguments[i];
        if (typeof arg != 'string') arg = JSON.stringify(arg);
        this.msg += arg;
      }

      setTimeout(this.clear, 5000);
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

      if (typeof config.data != 'undefined') {
        config.data = JSON.stringify(config.data);
        config.contentType = 'application/json';
      }

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


    get_providers: function (cb) {this.providers_promise.done(cb)},


    config_set: function (entry) {
      var value = entry.type == 'bool' ? (entry.value ? 1 : 0) : entry.value;

      this.api({
        method: 'PUT',
        url: '/config/' + entry.name,
        data: {value: value},
        msgs: {success: 'Set "' + entry.name + '".'}
      })
    },


    user_edit: function (user) {
      this.page = 'user';
      this.uid = user.id;
    },


    group_edit: function (group) {
      this.page = 'group';
      this.group = group;
    },


    user_add: function (provider, email, name) {
      return this.api({
        method: 'POST',
        url: '/users',
        data: {
          provider: provider,
          email: trimif(email),
          name: trimif(name)
        },
        msgs: {
          success: 'Added user ' + email + '.',
          conflict: 'User ' + email + ' already exists.'
        }
      })
    },


    user_save: function (uid, user) {
      return this.api({
        method: 'PUT',
        url: '/users/' + uid,
        data: {
          email: trimif(user.email),
          name: trimif(user.name),
          avatar: trimif(user.avatar),
          enabled: !!user.enabled
        },
        msgs: {success: 'Updated user ' + uid + '.'}
      })
    },


    user_delete: function (uid, name) {
      return this.api({
        method: 'DELETE',
        url: '/users/' + uid,
        msgs: {
          confirm: 'Are you sure you want to delete user ' + name + '?',
          success: 'Deleted user ' + name + '.'
        }
      })
    },


    user_association_set: function (uid, name, association) {
      return this.api({
        method: 'PUT',
        url: '/users/' + uid + '/associations/' + association.provider,
        data: {
          email: trimif(association.email),
          name: trimif(association.name),
          avatar: trimif(association.avatar)
        },
        msgs: {
          success: 'Set user ' + name + ' association with ' +
            association.provider + '.'
        }
      })
    },


    user_association_remove: function (uid, name, association) {
      return this.api({
        method: 'DELETE',
        url: '/users/' + uid + '/associations/' + association.provider,
        msgs: {
          confirm: 'Are you sure you want to delete user ' + name +
            ' association with ' + association.provider + '?',
          success: 'Deleted user ' + name + ' association with ' +
            association.provider + '.'
        }
      })
    },


    user_group_remove: function (uid, name, group) {
      return this.api({
        method: 'DELETE',
        url: '/users/' + uid + '/groups/' + group,
        msgs: {
          success: 'Removed user ' + name + ' from group ' + group + '.'
        }
      })
    },


    user_group_add: function (uid, name, group) {
      return this.api({
        method: 'PUT',
        url: '/users/' + uid + '/groups/' + group,
        msgs: {
          success: 'Added user ' + name + ' to group ' + group + '.',
          conflict: 'User ' + name + ' already in group ' + group + '.'
        }
      })
    },


    group_add: function (group) {
      return this.api({
        method: 'PUT',
        url: '/groups/' + group,
        msgs: {
          success: 'Added group ' + group + '.',
          conflict: 'Group ' + group + ' already exists.'
        }
      })
    },


    group_delete: function (group) {
      return this.api({
        method: 'DELETE',
        url: '/groups/' + group,
        msgs: {
          confirm: 'Are you sure you want to delete group ' + group + '?',
          success: 'Deleted group ' + group + '.'
        }
      })
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
