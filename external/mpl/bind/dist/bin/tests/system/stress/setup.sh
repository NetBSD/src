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

# shellcheck source=conf.sh
. ../conf.sh

cp ns2/zone.template.db ns2/zone000000.example.db
cp ns2/zone.template.db ns2/zone000001.example.db
cp ns2/zone.template.db ns2/zone000002.example.db
cp ns2/zone.template.db ns2/zone000003.example.db
cp ns2/zone.template.db ns2/zone000004.example.db
