# Copyright Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.

# Copyright (C) Stichting NLnet, Netherlands, stichting@nlnet.nl.
#
# The development of Dynamically Loadable Zones (DLZ) for Bind 9 was
# conceived and contributed by Rob Butler.
#
# SPDX-License-Identifier: ISC and MPL-2.0
#
# Permission to use, copy, modify, and distribute this software for any purpose
# with or without fee is hereby granted, provided that the above copyright
# notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND STICHTING NLNET DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL STICHTING NLNET BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
# OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

prefix = /usr
libdir = $(prefix)/lib/bind9

CFLAGS += -fPIC -g -I../include
BDB_LIBS=-ldb

all: dlz_bdbhpt_dynamic.so

dlz_bdbhpt_dynamic.so: dlz_bdbhpt_dynamic.c
	$(CC) $(CFLAGS) -shared -o dlz_bdbhpt_dynamic.so \
		dlz_bdbhpt_dynamic.c $(BDB_LIBS)

clean:
	rm -f dlz_bdbhpt_dynamic.so

install: dlz_bdbhpt_dynamic.so
	mkdir -p $(DESTDIR)$(libdir)
	install dlz_bdbhpt_dynamic.so $(DESTDIR)$(libdir)
