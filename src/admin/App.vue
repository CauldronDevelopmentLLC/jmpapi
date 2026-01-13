<!--

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

-->

<script>
import Modal     from './Modal.vue'
import UserLogin from './UserLogin.vue'


export default {
  components: {Modal, UserLogin},


  data() {
    return {
      loading:     true,
      is_admin:    false,
      loggedin:    false,
      providers:   [],
      msg:         '',
      err:         false,
      uid:         undefined,
      group:       ''
    }
  },


  async mounted() {
    this.$api.set_dialog_handler(this.dialog)

    // Get list of login providers
    this.providers = await this.$api.get('/login/providers')
  },


  methods: {
    async dialog(type, msg) {
      switch (type) {
      case 'confirm': return this.confirm(msg)
      case 'error':   return this.error(msg)
      default:        return this.message(msg)
      }
    },


    clear() {
      this.msg = ''
      this.err = false
    },


    async confirm(msg) {
      let response = await this.$refs.modal.exec(msg)
      return response == 'accept'
    },


    message() {
      this.clear()

      for (let i = 0; i < arguments.length; i++) {
        let arg = arguments[i]
        if (typeof arg != 'string') arg = JSON.stringify(arg)
        this.msg += arg
      }

      setTimeout(this.clear, 5000)
    },


    error() {
      Array.prototype.unshift.call(arguments, 'ERROR: ')
      this.message.apply(this, arguments)
      this.err = true
    },


    login(loggedin, user) {
      console.debug('user', user)
      this.loading  = false
      this.loggedin = loggedin
      this.is_admin = user && user.group.admin
      this.user     = user
    },


    logout() {
      this.message('Logged out.')
      this.loggedin = false
      this.is_admin = false
      delete this.user
      this.$router.push('/')
    }
  }
}
</script>

<template lang="pug">
#content
  user-login(@login="login", @logout="logout")

  Teleport(to="body")
    modal(ref="modal", header="Please confirm")

  .message(:class="{error: err, success: msg && !err}") {{msg}}

  router-view(v-slot="{Component}", v-if="!loading")
    KeepAlive
      component(:is="Component")
</template>

<style lang="stylus">
.button
  display inline-block
  width 32px
  height 32px
  line-height 32px
  font-size 24px
  text-align center
  cursor pointer

  &:hover
    color #888

#content
  margin auto
  max-width 700px

  .message
    min-height 2em
    font-size 120%
    padding 0.5em
    border-radius 3px
    border 2px solid transparent

    &.error
      border 2px solid red

    &.success
      border 2px solid #666

  table
    border-collapse collapse
    width 100%

    tr.disabled
      background #efefef

    tr:nth-child(even)
      background #fafafa

      &.disabled
        background #eaeaea

    td, th
      border 1px solid #eee
      padding 4px
      white-space nowrap
      overflow hidden
      text-overflow ellipsis
      text-align left

    tbody tr:nth-child(even)
      background #f7f7f7
</style>
