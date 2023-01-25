#!/bin/sh

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

rm -f bad-kasp-keydir1.conf
rm -f bad-kasp-keydir2.conf
rm -f bad-kasp-keydir3.conf
rm -f bad-kasp-keydir4.conf
rm -f bad-kasp-keydir5.conf
rm -f checkconf.out*
rm -f diff.out*
rm -f good-kasp.conf.in
rm -f good-server-christmas-tree.conf
rm -f good.conf.in good.conf.out badzero.conf *.out
rm -f ns*/named.lock
rm -rf test.keydir
