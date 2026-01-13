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
  props: ['header'],


  data() {
    return {
      show: false,
      msg: '',
      bnts: [
        {label: 'Cancel', response: 'cancel', icon: 'ban'},
        {label: 'Ok',     response: 'accept', icon: 'check'}
      ]
    }
  },


  methods: {
    async exec(body) {
      this.show = true
      this.body = body
      return new Promise(resolve => {this.resolve = resolve})
    },


    respond(response) {
      this.show = false
      if (this.resolve) this.resolve(response)
    },


    cancel() {this.respond('cancel')}
  }
}
</script>

<template lang="pug">
.modal-overlay(@click="cancel", v-if="show")
  .modal(@click.stop="false")
    .modal-wrapper
      .modal-header(v-html="header")
      .modal-body(v-html="body")
      .modal-footer
        button(v-for="b in bnts", @click="respond(b.response)")
          label {{b.label}}
          .fa(v-if="b.icon != undefined", :class="'fa-' + b.icon")
</template>

<style lang="stylus">
.modal
  width 500px
  margin 10% auto
  background #fff
  border-radius 3px
  border 2px solid #888
  z-index 110
  text-align left
  font-size 12pt
  white-space normal

  .modal-header, .modal-body, .modal-footer
    padding 1em

  .modal-header, .modal-footer
    white-space nowrap

  .modal-header
    font-weight bold
    background-color #f3f3f3
    border-bottom 1px solid #eee
    border-top-left-radius 3px
    border-top-right-radius 3px

  .modal-footer
    display flex
    flex-direction row
    justify-content end
    gap 0.5em

    button
      display flex
      flex-direction row
      align-items center
      gap 0.5em
      cursor pointer

      label
        text-transform capitalize
        cursor pointer

.modal-overlay
  background-color rgba(0, 0, 0, 0.3)
  z-index 100
  position fixed
  top 0
  left 0
  width 100%
  height 100%
</style>
