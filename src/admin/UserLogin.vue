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
export default {
  data() {
    return {
      user: {},
      loggedin: false
    }
  },


  async mounted() {
    // Process login
    const params = new URLSearchParams(location.search)
    let state    = params.get('state')
    let code     = params.get('code')
    let provider = localStorage.getItem('oauth2-provider')
    if (state && code && provider) {
      let redirect_uri = location.origin + location.pathname
      let config = {state, code, redirect_uri}
      await this.$api.get('/login/' + provider, config)
      location.search = ''
    }

    // Check login
    try {
      this.user = await this.$api.get('/login/', state ? {state} : undefined)
      this.loggedin = true
      this.$emit('login', true, this.user)

    } catch (e) {
      this.$emit('login', false)
    }
  },


  methods: {
    async login(provider) {
      localStorage.setItem('oauth2-provider', provider)
      let config = {redirect_uri: location.href}
      let data = await this.$api.get('/login/' + provider, config)
      if (data.redirect) location.href = data.redirect
    },


    async logout() {
      await this.$api.put('/logout')
      this.user = {}
      this.loggedin = false
      this.$emit('logout')
    },

    toggle() {
      if (this.$router.currentRoute.value.path != '/admin')
        this.$router.push('/admin')
      else this.$router.push('/')
    }
  }
}
</script>

<template lang="pug">
.user-login
  .login(v-if="!loggedin && $root.providers.length")
    | Admin login
    .button.fa(v-for="provider in $root.providers", :class="'fa-' + provider",
      @click="login(provider)", :title="'Login with ' + provider + '.'")

  .login-menu(v-if="loggedin")
    img.button.avatar(@click="toggle",
      :src="user.avatar", :title="'Logged in as ' + user.name + '.'")
    .button.fa.fa-sign-out(@click="logout", title="Sign out.")
</template>

<style lang="stylus">
.user-login
  position fixed
  top 4px
  right 4px
  font-weight bold
  line-height 32px

  .avatar
    vertical-align top
    width 32px
    height 32px
    border-radius 5px
    margin 1px
</style>
