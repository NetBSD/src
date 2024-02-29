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

. ../conf.sh

copy_setports bad-kasp-keydir1.conf.in bad-kasp-keydir1.conf
copy_setports bad-kasp-keydir2.conf.in bad-kasp-keydir2.conf
copy_setports bad-kasp-keydir3.conf.in bad-kasp-keydir3.conf
copy_setports bad-kasp-keydir4.conf.in bad-kasp-keydir4.conf
copy_setports bad-kasp-keydir5.conf.in bad-kasp-keydir5.conf
cp -f good-server-christmas-tree.conf.in good-server-christmas-tree.conf
