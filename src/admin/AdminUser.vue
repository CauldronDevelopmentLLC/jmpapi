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
  props: ['uid'],


  data() {
    return {
      user: {},
      groups: [],
      association_new: {
        provider: '',
        email: '',
        name: ''
      }
    }
  },


  activated() {
    this.user = {}
    this.groups = []
    this.update_user()
    this.update_groups()
    this.association_new.provider = this.$root.providers[0]
  },


  methods: {
    async update_user() {this.user = await this.$api.get('/users/' + this.uid)},


    async update_groups() {
      this.groups = await this.$api.get('/users/' + this.uid + '/groups')
    },


    back() {this.$router.back()},


    async user_save() {
      await this.$api.user_save(this.uid, this.user)
      this.back()
    },


    async user_delete() {
      await this.$api.user_delete(this.uid, this.user.email)
      this.back()
    },


    async user_association_set() {
      let assoc = this.association_new

      await this.$api.user_association_set(this.uid, this.user.email, assoc)
      assoc.email = ''
      this.update_user()
    },


    async user_association_remove(association) {
      await this.$api.user_association_remove(
        this.uid, this.user.email, association)
      this.update_user()
    },


    async user_group_remove(group) {
      await this.$api.user_group_remove(this.uid, this.user.email, group.name)
      group.member = 0
    },


    async user_group_add(group) {
      await this.$api.user_group_add(this.uid, this.user.email, group.name)
      group.member = 1
    }
  }
}
</script>

<template lang="pug">
.admin-user
  h2
    .back
      .button.fa.fa-arrow-left(@click="back", title="Return to main page")
    | Editing User {{uid}}

  .avatar
    img(:src="user.avatar")

  table.user
    tr
      th Email
      td.email {{user.email}}
    tr
      th Name
      td.name: input(v-model="user.name")
    tr
      th Avatar
      td.avatar: input(v-model="user.avatar")
    tr
      th Enabled
      td.enabled: input(type="checkbox", v-model="user.enabled")
    tr
      th Created
      td.created {{user.created}}
    tr
      th Last Seen
      td.last_Seen {{user.last_used}}
    tr
      th Actions
      td.actions
        .button.fa.fa-trash(@click="user_delete", title="Delete user")
        .button.fa.fa-save(@click="user_save", title="Save changes")

  h2 Login Associations
  table.associations
    thead
      tr
        th Email
        td.input: input(v-model="association_new.email")
        th Provider
        td.input
          select(v-model="association_new.provider")
            option(v-for="provider in $root.providers", :value="provider")
              | {{provider}}
        td.actions
          .button.fa.fa-plus(@click="user_association_set",
            title="Set user login provider association.")

      tr
        th Provider
        th Email
        th Name
        th Avatar
        th Actions

    tbody
      tr(v-for="association in user.associations")
        td.provider {{association.provider}}
        td.email(@click="user.email = association.email")
          | {{association.email}}
        td.name(@click="user.name = association.name") {{association.name}}
        td.avatar: img(:src="association.avatar",
          @click="user.avatar = association.avatar")
        td.actions
          .button.fa.fa-trash(@click="user_association_remove(association)",
            title="Remove login association.")

  h2 Groups
  table.groups
    thead
      tr
        th Name
        th Status
        th Actions

    tbody
      tr(v-for="group in groups")
        td.group {{group.name}}
        td.status {{group.member ? 'Member' : 'Nonmember'}}
        td.actions
          .button.fa.fa-minus(v-if="group.member",
            @click="user_group_remove(group)", title="Remove user from group")
          .button.fa.fa-plus(v-if="!group.member",
            @click="user_group_add(group)", title="Add user to group")
</template>

<style lang="stylus">
</style>
