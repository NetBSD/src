# Copyright Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.

# Copyright (C) Stichting NLnet, Netherlands, stichting@nlnet.nl.
# Copyright (C) John Eaglesham
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

# For building the dlz_perl_driver driver we don't use
# the bind9 build structure as the aim is to provide an
# perl_driver that is separable from the bind9 source tree

CFLAGS += -fPIC -O2 -I../include
FLAGS_PERL ?= perl
LIBNAME = dlz_perl_driver.so

all: $(LIBNAME)

dlz_perl_driver.o: dlz_perl_driver.c
	$(CC) $(CFLAGS) `${FLAGS_PERL} -MExtUtils::Embed -e ccopts` -c -o dlz_perl_driver.o dlz_perl_driver.c


dlz_perl_callback_clientinfo.c: dlz_perl_callback_clientinfo.xs
	${FLAGS_PERL} `${FLAGS_PERL} -MConfig -le 'print $$Config{privlibexp}'`/ExtUtils/xsubpp -prototypes -typemap `${FLAGS_PERL} -MConfig -le 'print $$Config{privlibexp}'`/ExtUtils/typemap dlz_perl_callback_clientinfo.xs > dlz_perl_callback_clientinfo.c

dlz_perl_callback_clientinfo.o: dlz_perl_callback_clientinfo.c
	$(CC) $(CFLAGS) `${FLAGS_PERL} -MExtUtils::Embed -e ccopts` -c -o dlz_perl_callback_clientinfo.o dlz_perl_callback_clientinfo.c


dlz_perl_callback.c: dlz_perl_callback.xs
	${FLAGS_PERL} `${FLAGS_PERL} -MConfig -le 'print $$Config{privlibexp}'`/ExtUtils/xsubpp -prototypes -typemap `${FLAGS_PERL} -MConfig -le 'print $$Config{privlibexp}'`/ExtUtils/typemap dlz_perl_callback.xs > dlz_perl_callback.c

dlz_perl_callback.o: dlz_perl_callback.c
	$(CC) $(CFLAGS) `${FLAGS_PERL} -MExtUtils::Embed -e ccopts` -c -o dlz_perl_callback.o dlz_perl_callback.c


$(LIBNAME): dlz_perl_driver.o dlz_perl_callback_clientinfo.o dlz_perl_callback.o
	$(CC) $(LDFLAGS) -shared -o $(LIBNAME) dlz_perl_driver.o dlz_perl_callback_clientinfo.o dlz_perl_callback.o `${FLAGS_PERL} -MExtUtils::Embed -e ldopts`

clean:
	rm -f dlz_perl_driver.o dlz_perl_driver.so dlz_perl_callback_clientinfo.c dlz_perl_callback_clientinfo.o dlz_perl_callback.c dlz_perl_callback.o

install: dlz_perl_driver.so
	mkdir -p $(DESTDIR)$(libdir)
	install dlz_perl_driver.so $(DESTDIR)$(libdir)
