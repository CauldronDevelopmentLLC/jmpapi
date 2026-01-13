/******************************************************************************\

                           This file is part of JmpAPI.

                 Copyright (c) 2014-2026, Cauldron Development Oy
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

function trimif(s) {
  s = s == undefined ? '' : s.trim()
  return s.length ? s : undefined
}


class API {
  constructor() {
    this.dialog = async (type, msg) => {console.log(type, msg)}
  }


  set_dialog_handler(cb) {this.dialog = cb}


  async api(method, path, data, msgs = {}) {
    if (msgs.confirm && !await this.dialog('confirm', msgs.confirm)) return

    let opts = {method}

    if (data) {
      if (method == 'PUT' || method == 'POST') {
        opts['headers'] = {'Content-Type': 'application/json; charset=utf-8'}
        opts['body']    = JSON.stringify(data)

      } else path += '?' + new URLSearchParams(data)
    }

    let r = await fetch(path, opts)

    if (!r.ok) {
      let msg
      if (r.status == 409 && msgs.conflict) msg = msgs.conflict
      else if (r.status == 401 && msgs.unauthorized) msg = msgs.unauthorized
      else if (msgs.fail) msg = msgs.fail
      if (msg) this.dialog('error', msg)

      return Promise.reject(r.statusText)

    } else if (msgs.success) this.dialog('success', msgs.success)

    if (method == 'GET') return r.json()
  }


  async get()    {return this.api('GET',    ...arguments)}
  async put()    {return this.api('PUT',    ...arguments)}
  async post()   {return this.api('POST',   ...arguments)}
  async delete() {return this.api('DELETE', ...arguments)}


  async config_set(entry) {
    let value = entry.type == 'bool' ? (entry.value ? 1 : 0) : entry.value

    return this.put(
      '/config/' + entry.name, {value: value},
      {success: 'Set "' + entry.name + '".'}
    )
  }


  async user_add(provider, email, name) {
    return this.post(
      '/users',
      {
        provider: provider,
        email: trimif(email),
        name: trimif(name)
      },
      {
        success: 'Added user ' + email + '.',
        conflict: 'User ' + email + ' already exists.'
      }
    )
  }


  async user_save(uid, user) {
    return this.put(
      '/users/' + uid,
      {
        email: trimif(user.email),
        name: trimif(user.name),
        avatar: trimif(user.avatar),
        enabled: !!user.enabled
      },
      {success: 'Updated user ' + uid + '.'}
    )
  }


  async user_delete(uid, name) {
    return this.delete(
      '/users/' + uid, undefined,
      {
        confirm: 'Are you sure you want to delete user ' + name + '?',
        success: 'Deleted user ' + name + '.'
      }
    )
  }


  async user_association_set(uid, name, association) {
    return this.put(
      '/users/' + uid + '/associations/' + association.provider,
      {
        email: trimif(association.email),
        name: trimif(association.name),
        avatar: trimif(association.avatar)
      },
      {
        success: 'Set user ' + name + ' association with ' +
          association.provider + '.'
      }
    )
  }


  async user_association_remove(uid, name, association) {
    return this.delete(
      '/users/' + uid + '/associations/' + association.provider,
      undefined,
      {
        confirm: 'Are you sure you want to delete user ' + name +
          ' association with ' + association.provider + '?',
        success: 'Deleted user ' + name + ' association with ' +
          association.provider + '.'
      }
    )
  }


  async user_group_remove(uid, name, group) {
    return this.delete(
      '/users/' + uid + '/groups/' + group,
      undefined,
      {success: 'Removed user ' + name + ' from group ' + group + '.'}
    )
  }


  async user_group_add(uid, name, group) {
    return this.put(
      '/users/' + uid + '/groups/' + group, undefined,
      {
        success: 'Added user ' + name + ' to group ' + group + '.',
        conflict: 'User ' + name + ' already in group ' + group + '.'
      }
    )
  }


  async group_add(group) {
    return this.put(
      '/groups/' + group, undefined,
      {
        success: 'Added group ' + group + '.',
        conflict: 'Group ' + group + ' already exists.'
      }
    )
  }


  async group_delete(group) {
    return this.delete(
      '/groups/' + group, undefined,
      {
        confirm: 'Are you sure you want to delete group ' + group + '?',
        success: 'Deleted group ' + group + '.'
      }
    )
  }
}


export default API;
