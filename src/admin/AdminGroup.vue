<!--

                           This file is part of JmpAPI.

                Copyright (c) 2014-2024, Cauldron Development LLC
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
  props: ['group'],


  data() {
    return {
      loading:    true,
      members:    [],
      nonmembers: []
    }
  },


  async activated() {
    this.loading    = true
    this.members    = []
    this.nonmembers = []

    try {
      let url = '/groups/' + this.group
      this.members    = await this.$api.get(url + '/members')
      this.nonmembers = await this.$api.get(url + '/nonmembers')

    } catch (e) {
      this.$root.error('Failed to load group ' + this.group + '.')

    } finally {this.loading = false}
  },


  methods: {
    back() {this.$router.back()},


    async group_user_remove(user) {
      await this.$api.user_group_remove(user.id, user.name, this.group)
      this.members.splice(this.members.indexOf(user), 1)
      this.nonmembers.push(user)
    },


    async group_user_add(user) {
      await this.$api.user_group_add(user.id, user.name, this.group)
      this.nonmembers.splice(this.nonmembers.indexOf(user), 1)
      this.members.push(user)
    }
  }
}
</script>

<template lang="pug">
.admin-group
  h2
    .back
      .button.fa.fa-arrow-left(@click="back", title="Return to main page")
    | Editing Group {{group}}

  h2(v-if="loading") Loading...

  template(v-if="!loading")
    h2(v-if="!members.length") No members.
    .members(v-if="members.length")
      h2 Members
      table
        thead
          tr
            th Avatar
            th User
            th Email
            th Actions

        tbody
          tr(v-for="user in members")
            td.avatar: img(:src="user.avatar")
            td.name {{user.name}}
            td.email {{user.email}}
            td.actions
              .button.fa.fa-minus(@click="group_user_remove(user)",
                title="Remove user from group.")

    h2(v-if="!nonmembers.length") No nonmembers.
    .nonmembers(v-if="nonmembers.length")
      h2 Nonmembers
      table
        thead
          tr
            th Avatar
            th User
            th Email
            th Actions

        tbody
          tr(v-for="user in nonmembers")
            td.avatar: img(:src="user.avatar")
            td.name {{user.name}}
            td.email {{user.email}}
            td.actions
              .button.fa.fa-plus(@click="group_user_add(user)",
                title="Add user to group.")
</template>

<style lang="stylus">
</style>
