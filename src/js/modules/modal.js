/******************************************************************************\

                          This file is part of JmpAPI.

               Copyright (c) 2014-2019, Cauldron Development LLC
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

'use strict'


module.exports = {
  template: '#modal-template',
  props: ['buttons', 'show'],


  data: function () {
    return {
      bnts: [
        {label: 'Cancel', response: 'cancel', icon: 'ban'},
        {label: 'Ok', response: 'accept', icon: 'check'}
      ]
    }
  },


  mounted: function () {
    if (typeof this.buttons == 'string') {
      var labels = this.buttons.split(' ');
      this.bnts = [];

      for (var i = 0; i < labels.length; i++)
        this.bnts.push({label: labels[i], response: labels[i]})

    } else if (typeof this.buttons != 'undefined')
      this.bnts = this.buttons;
  },


  beforeDestroy: function () {this.removeOverlay()},


  watch: {
    show: function (show) {
      if (show) this.addOverlay();
      else this.removeOverlay();
    }
  },


  methods: {
    overlayClick: function () {this.$emit('response', 'cancle')},


    addOverlay: function () {
      if (typeof this.overlay != 'undefined') return;
      this.overlay = $('<div>')
        .addClass('modal-overlay')
        .click(this.overlayClick)
        .appendTo('body');
    },


    removeOverlay: function () {
      if (typeof this.overlay == 'undefined') return;
      this.overlay.remove();
      delete this.overlay;
    },


    click: function (button) {
      if (typeof button.cb != 'undefined') button.cb(button.response);
      this.$emit('response', button.response);
    }
  }
}
