# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# for building the dlz_example driver we don't use
# the bind9 build structure as the aim is to provide an
# example that is separable from the bind9 source tree

# this means this Makefile is not portable, so the testsuite
# skips this test on platforms where it doesn't build

CFLAGS += -Wall -fPIC -g

all: dlz_example.so

dlz_example.so: dlz_example.o
	$(CC) $(CFLAGS) -shared -o dlz_example.so dlz_example.o

clean:
	rm -f dlz_example.o dlz_example.so
