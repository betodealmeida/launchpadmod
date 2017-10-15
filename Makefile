#!/usr/bin/make -f
# Makefile for launchpadmod.lv2 #
# ----------------------- #
# Created by falkTX
#

include Makefile.mk

# --------------------------------------------------------------

PREFIX  ?= /usr/local
DESTDIR ?=

# --------------------------------------------------------------
# Default target is to build all plugins

all: build
build: launchpadmod

# --------------------------------------------------------------
# launchpadmod build rules

launchpadmod: launchpadmod.lv2/launchpadmod$(LIB_EXT) launchpadmod.lv2/manifest.ttl

launchpadmod.lv2/launchpadmod$(LIB_EXT): launchpadmod.c
	$(CC) $^ $(BUILD_C_FLAGS) $(LINK_FLAGS) -lm $(SHARED) -o $@

launchpadmod.lv2/manifest.ttl: launchpadmod.lv2/manifest.ttl.in
	sed -e "s|@LIB_EXT@|$(LIB_EXT)|" $< > $@

# --------------------------------------------------------------

clean:
	rm -f launchpadmod.lv2/launchpadmod$(LIB_EXT) launchpadmod.lv2/manifest.ttl

# --------------------------------------------------------------

install: build
	install -d $(DESTDIR)$(PREFIX)/lib/lv2/launchpadmod.lv2

	install -m 644 launchpadmod.lv2/*.so  $(DESTDIR)$(PREFIX)/lib/lv2/launchpadmod.lv2/
	install -m 644 launchpadmod.lv2/*.ttl $(DESTDIR)$(PREFIX)/lib/lv2/launchpadmod.lv2/

# --------------------------------------------------------------
