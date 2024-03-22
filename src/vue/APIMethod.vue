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
import APIArg from './APIArg.vue'


function get_arg_example(arg) {
  if (arg.example != undefined) return arg.example
  if (arg.default != undefined) return arg.default
  if (arg.enum    != undefined) return arg.enum[0]

  let type = arg.type || 'string'
  return '<' + type + '>'
}


export default {
  props: ['method', 'path', 'config'],
  components: {'api-arg': APIArg},

  data() {
    return {
      open:     false,
      selected: false
    }
  },


  computed: {
    name() {return this.method + '-' + this.path},


    klass() {
      let klass = this.open ? 'opened' : 'closed'
      if (this.selected) klass += ' highlight'
      return klass
    }
  },


  mounted() {
    window.addEventListener('hashchange', this.update)
    this.update()

    // Scroll into view if open
    if (this.open) Vue.nextTick(() => this.$el.scrollIntoView())
  },


  methods: {
    update() {
      this.selected = location.hash.substr(1) == this.name
      if (this.selected) this.open = true
    },


    api_path(path) {
      let base = location.protocol + '//' + location.hostname
      if (location.port) base += ':' + location.port
      return base + path
    },


    example(method, path, entry) {
      if (method.indexOf('|') != -1)
        return method.split('|')
        .map(method => this.example(method, path, entry))
        .join('\n')

      else {
        let s = ''
        s += 'curl'
        if (method != 'GET') s += ' -X ' + method
        else if (entry.args) s += ' -G'

        if (entry.allow || entry.deny) s += ' -H "Authorization: $TOKEN"'

        s += ' ' + this.api_path(path)

        if (entry.args)
          for (let name in entry.args)
            if (!entry.args[name].url)
              s += ' -d ' + name + '=' + get_arg_example(entry.args[name])

        return s
      }
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
.api-method(:class="klass")
  a(:name="name")
  .api-path(@click="open = !open",
    :title="'Click to ' + (open ? 'collapase.' : 'expand.')")
    a.api-link(:href="'#' + name", title="Direct link to endpoint.",
      @click.stop="")
      .fa.fa-link
    .fa(:class="'fa-caret-' + (open ? 'down' : 'right')")
    | {{method}} {{path}}

  template(v-if="open")
    table.api-args(v-if="config.args")
      tr
        th Argument
        th Type
        th Default
        th Description

      api-arg(v-for="config, name in config.args", :name="name",
        :config="config")

    .api-help(v-if="config.help"): span(v-html="config.help")
    .api-allow(v-if="config.allow")
      | #[b Allow:] {{auth_list(config.allow)}}
    .api-deny(v-if="config.deny") #[b Deny:] {{auth_list(config.deny)}}
    .api-example: pre {{example(method, path, config)}}
</template>

<style lang="stylus">
</style>
