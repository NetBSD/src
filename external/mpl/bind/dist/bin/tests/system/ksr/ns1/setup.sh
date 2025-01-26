#!/bin/sh -e

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

# shellcheck source=conf.sh
. ../../conf.sh

# Key directories
mkdir keydir
mkdir offline

# Zone files
cp template.db.in common.test.db
cp template.db.in past.test.db
cp template.db.in future.test.db
cp template.db.in last-bundle.test.db
cp template.db.in in-the-middle.test.db
cp template.db.in unlimited.test.db
cp template.db.in two-tone.test.db
cp template.db.in ksk-roll.test.db
