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
