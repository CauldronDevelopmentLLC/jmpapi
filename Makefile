################################################################################
#                                                                              #
#                 Copyright (c) 2018, Cauldron Development LLC                 #
#                             All rights reserved.                             #
#                                                                              #
#     This file ("the software") is free software: you can redistribute it     #
#     and/or modify it under the terms of the GNU General Public License,      #
#      version 2 as published by the Free Software Foundation. You should      #
#      have received a copy of the GNU General Public License, version 2       #
#     along with the software. If not, see <http: #www.gnu.org/licenses/>.     #
#                                                                              #
#     The software is distributed in the hope that it will be useful, but      #
#          WITHOUT ANY WARRANTY; without even the implied warranty of          #
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       #
#               Lesser General Public License for more details.                #
#                                                                              #
#       You should have received a copy of the GNU Lesser General Public       #
#                License along with the software.  If not, see                 #
#                       <http: #www.gnu.org/licenses/>.                        #
#                                                                              #
#                For information regarding this software email:                #
#              "Joseph Coffland" <joseph@cauldrondevelopment.com>              #
#                                                                              #
################################################################################

DEST := johndoe@example.org:

DIR := $(dir $(lastword $(MAKEFILE_LIST)))
NODE_MODS  := $(DIR)node_modules
PUG        := $(NODE_MODS)/.bin/pug
JSHINT     := $(NODE_MODS)/.bin/jshint

HTML     := $(wildcard src/pug/*.pug)
HTML     := $(patsubst src/pug/%.pug,http/%.html,$(HTML))
STATIC   := $(shell find src/static -type f -not \( -name \*~ -o -name \\\#* \))
STATIC   := $(patsubst src/static/%,http/%,$(STATIC))

all: node_modules $(HTML) $(STATIC) lint

publish: all
	rsync -rv http/ $(DEST)

http/%: src/static/%
	mkdir -p $(dir $@)
	install -D $< $@

http/%.html: src/pug/%.pug
	$(PUG) -O pug-opts.js $< --out $(dir $@) || (rm -f $@; exit 1)
	@mkdir -p build/dep
	@echo -n "$@: " > build/dep/$(shell basename $<)
	@./pug-deps $< >> build/dep/$(shell basename $<)

lint: node_modules
	$(JSHINT) --config jshint.json src/js/*.js src/js/modules/*.js

node_modules:
	npm install

watch:
	@clear
	$(MAKE)
	@while sleep 1; do \
	  inotifywait -qr -e modify -e create -e delete \
		--exclude .*~ --exclude \#.* $(shell find src -type f); \
	  clear; \
	  $(MAKE); \
	done

tidy:
	rm -rf $(shell find . -name \*~ -o -name \#\*)

clean: tidy
	rm -rf http build

dist-clean:
	git clean -fxd -e /private.pem -e /certificate.pem

.PHONY: all publish lint watch tidy clean dist-clean

# Dependencies
-include $(shell mkdir -p build/dep) $(wildcard build/dep/*)
