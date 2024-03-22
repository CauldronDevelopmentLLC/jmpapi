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
import APIMethod from './APIMethod.vue'


export default {
  props: ['name', 'config'],
  components: {'api-method': APIMethod},

  data() {
    return {
      selected: false,
      open: false,
      expand: true
    }
  },


  computed: {
    klass() {
      let klass = this.open ? 'opened' : 'closed'
      if (this.selected) klass += ' highlight'
      return klass
    }
  },


  mounted() {
    window.addEventListener('hashchange', this.update)
    this.update()

    // Open if an endpoint method is selected
    for (let path in this.config.endpoints) {
      let endpoint = this.config.endpoints[path]

      for (let method in endpoint) {
        let config = endpoint[method]

        if (method + '-' + path == location.hash.substr(1)) {
          this.open = true
          return
        }
      }
    }
  },


  methods: {
    update() {
      this.selected = location.hash.substr(1) == this.name
      if (this.selected) this.open = true
    },


    toggle() {
      let methods = this.$refs.methods
      for (let i = 0; i < methods.length; i++)
        methods[i].open = this.expand

      this.expand = !this.expand
    },


    auth_list(list) {
      function auth_entry(entry) {
        if (entry[0] == '$') return 'group:' + entry.substr(1)
        return 'user:' + entry
      }

      return list.map(auth_entry).join(', ')
    }
  }
}
</script>

<template lang="pug">
.api-category(:class="klass")
  a(:name="name")

  .api-category-title(@click="open = !open",
    :title="'Click to ' + (open ? 'collapase.' : 'expand.')")

    a.api-link(:href="'#' + name", title="Direct link to API category.",
      @click.stop="select")
      .fa.fa-link

    .fa(@click.stop="toggle", :class="expand ? 'fa-expand' : 'fa-compress'",
      :title="(expand ? 'Expand' : 'Collapse') + ' all endpoints.'")

    .fa(:class="'fa-caret-' + (open ? 'down' : 'right')")
    span(v-html="config.title || name")

  template(v-if="open")
    p.api-help(v-if="config.help"): span(v-html="config.help")

    .api-allow(v-if="config.allow") #[b Allow:] {{auth_list(config.allow)}}
    .api-deny(v-if="config.deny") #[b Deny:] {{auth_list(config.deny)}}

    .api-endpoint(v-for="endpoint, path in config.endpoints")
      api-method(v-for="_config, method in endpoint", :method="method",
        :path="path", :config="_config", ref="methods")
</template>

<style lang="stylus">
.api-link
  float right
  display block
  height 32px
  width 32px
  text-align center

  &:visited, &:link
    color #aaa

.api-category .api-category-title, .api-endpoint .api-method .api-path
  cursor pointer
  font-weight bold

  &:hover
    background #fafafa
    color #000

.api-category
  border-radius 4px
  border 3px solid #eee

  &.opened
    margin-bottom 2em

  &.closed
    margin-bottom 4px

  > div, > p
    margin-left 4px
    margin-right 4px

  .fa
    color #aaa
    margin-right 0.5em
    font-size 12pt
    font-weight normal
    width 1em

  .api-category-title
    font-size 150%
    margin 0
    padding 4px
    background #eee
    color #444

    .fa-expand, .fa-compress
      float right
      cursor pointer
      text-align center
      line-height 32px
      width 32px
      margin-right 0

  &.closed .api-category-title
    border 0

    .fa-expand, .fa-compress
      display none

.api-endpoint
  .api-method
    color #444
    border 3px solid #eee
    background #fff
    border-radius 4px
    margin-bottom 2px

    &.opened
      margin-bottom 1.5em

    > div
      padding 0.5em

    .api-path
      background #f5f5f5
      font-size 110%

    &.closed .api-path
      border-radius 4px

    .api-args
      width 100%
      border-collapse collapse

      th, td
        padding 0.5em
        border 1px solid #ddd
        white-space nowrap
        text-align left
        vertical-align top

      td.description
        white-space normal

      th
        font-size 90%

      td:first-child, th:first-child
        border-left 0

      td:last-child, th:last-child
        border-right 0

      .description
        width 100%

    .api-example
      padding 0

      pre
        line-height 1.5em
        background #f5f5f5
        padding 0.5em
        margin 0
        font-family mono
        border-bottom-left-radius 4px
        border-bottom-right-radius 4px
        width 100%
        overflow-x auto
        box-sizing border-box


.api-category, .api-method
  &.highlight
    border 3px solid #666
    padding 0
</style>
