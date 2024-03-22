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
import APICategory from './APICategory.vue'


export default {
  components: {'api-category': APICategory},

  data() {
    return {
      data: {}
    }
  },


  async mounted() {
    this.data = await this.$api.get('/api')
  },


  methods: {
  }
}
</script>

<template lang="pug">
.api-docs
  h1.api-title: span(v-html="data.header || data.title")
  p.api-help(v-if="data.help"): span(v-html="data.help")

  api-category(v-for="(config, name) in data.categories", :name="name",
    :config="config")
</template>

<style lang="stylus">
</style>
