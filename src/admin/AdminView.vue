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
  data() {
    return {
      config: [],
      users: {},
      groups: [],
      user_new: {
        email: '',
        provider: '',
      },
      group_new: ''
    }
  },


  activated() {
    if (!this.$root.loggedin) this.$router.push('/')
    else if (!this.$root.is_admin) this.$root.message('Admin group required.')
    else this.update()
  },


  async mounted() {
    this.config = await this.$api.get('/config')
    await this.update()
    this.user_new.provider = this.$root.providers[0]
  },


  methods: {
    async update_users()  {this.users  = await this.$api.get('/users')},
    async update_groups() {this.groups = await this.$api.get('/groups')},


    async update() {
      await this.update_groups()
      await this.update_users()
    },


    async config_set(entry) {return this.$api.config_set(entry)},
    user_edit(user)   {this.$router.push('/user/'  + user.id)},
    group_edit(group) {this.$router.push('/group/' + group)},


    dragstart(event, user) {
      event.dataTransfer.setData('text', JSON.stringify(user))
    },


    dragover(event) {event.preventDefault()},


    drop(event, group) {
      event.preventDefault()
      let user = JSON.parse(event.dataTransfer.getData('text'))
      this.$api.user_group_add(user.id, user.name, group)
    },


    async user_add() {
      await this.$api.user_add(this.user_new.provider, this.user_new.email)
      this.user_new.email = ''
      return this.update()
    },


    async user_delete(user) {
      await this.$api.user_delete(user.id, user.email)
      return this.update()
    },


    async group_add() {
      await this.$api.group_add(this.group_new)
      return this.update()
    },


    async group_delete(group) {
      await this.$api.group_delete(group)
      return this.update()
    }
  }
}
</script>

<template lang="pug">
.admin-page(v-if="$root.is_admin")
  h2 Users
  .admin-users
    table
      thead
        tr
          th Email
          td.input: input(v-model="user_new.email")
          th Provider
          td.input
            select(v-model="user_new.provider")
              option(v-for="provider in $root.providers", :value="provider")
                | {{provider}}

          td.actions
            .button.fa.fa-plus(@click="user_add", title="Create new user.")

        tr.head
          th Avatar
          th Name
          th Created
          th Last Seen
          th Actions

      tbody
        tr(v-for="user in users", :class="{disabled: !user.enabled}")
          td.avatar
            img(:src="user.avatar", draggable="true",
              @dragstart="dragstart($event, user)")
          td.name(:title="user.name") {{user.name || user.email}}
          td.created {{user.created}}
          td.last {{user.last_used}}
          td.actions
            .button.fa.fa-cog(@click="user_edit(user)", title="Edit user.")
            .button.fa.fa-trash(
              @click="user_delete(user)", title="Delete user.")

  h2 Groups
  .admin-groups
    table
      thead
        tr
          th.name Name
          td.input: input(v-model="group_new", @keyup.enter="group_add")
          td.actions
            .button.fa.fa-plus(@click="group_add", title="Create new group.")

        tr.head
          th(colspan=2) Name
          th Actions

      tbody
        tr(v-for="group in groups", @drop="drop($event, group)",
          @dragover="dragover")
          td.name(colspan=2) {{group}}
          td.actions
            .button.fa.fa-cog(@click="group_edit(group)", title="Edit group.")
            .button.fa.fa-trash(@click="group_delete(group)",
              v-if="group != 'admin'", title="Delete group.")

  h2 Settings
  table(v-if="config.length")
    thead
      tr
        th Name
        td Value
        td Actions
    tbody
      template(v-for="entry in config")
        tr(v-if="entry.writable", :title="entry.help")
          th.name {{entry.name}}
          td.value(v-if="entry.writable")
            input(v-if="entry.type == 'bool'", type="checkbox",
              v-model="entry.value", :true-value="1", :false-value="0")
            input(v-else, type="number", v-model="entry.value")
          td.actions
            .button.fa.fa-save(
              v-if="entry.writable", @click="config_set(entry)")
</template>

<style lang="stylus">
#content
  .admin-page, .admin-group, .admin-user
    padding 1em
    border-radius 3px
    border 1px solid #ddd
    background #fff

  .admin-page
    h2:first-child
      margin-top 0

  table
    tr.head
      background #ddd

    .actions, .groups, .users, .avatar
      text-align center

    .avatar img
      width 32px
      vertical-align middle
      border-radius 3px

  .back
    display inline-block
    margin-right 0.5em

  .admin-users, .admin-groups
    max-height 400px
    overflow-y auto

  .admin-users
    .name, .email
      max-width 12em

    .created, .last
      font-size 80%

  .admin-groups
    td.input
      width 99%

      input
        width 31em

  .admin-users
    input
      max-width 13em

  .admin-user
    > h2 img
      margin 0 0.5em
      width 40px
      vertical-align middle
      border-radius 3px

    div.avatar img
      max-width 128px
      border-radius 3px

    .user
      margin-bottom 1em

      td, th
        border 0

      th
        text-align right

      td
        width 100%
        text-align left

      input:not([type])
        width 34em

    td.status
      width 100%

    .associations
      .name, .email
        cursor pointer

        &:hover
          text-decoration underline

      .avatar img
        border-radius 5px
        cursor pointer
        border 2px solid transparent

        &:hover
          border 2px solid #666

  .admin-group
    td.name
      width 100%
</style>
