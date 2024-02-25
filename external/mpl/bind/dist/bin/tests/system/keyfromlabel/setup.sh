#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# shellcheck source=conf.sh
. ../conf.sh

set -e

softhsm2-util --init-token --free --pin 1234 --so-pin 1234 --label "softhsm2-keyfromlabel" | awk '/^The token has been initialized and is reassigned to slot/ { print $NF }'

printf '%s' "${HSMPIN:-1234}" >pin
PWD=$(pwd)
