#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

if [ -n "${SOFTHSM2_CONF}" ] && command -v softhsm2-util >/dev/null; then
    SOFTHSM2_DIR=$(dirname "$SOFTHSM2_CONF")
    mkdir -p "${SOFTHSM2_DIR}/tokens"
    echo "directories.tokendir = ${SOFTHSM2_DIR}/tokens" > "${SOFTHSM2_CONF}"
    echo "objectstore.backend = file" >> "${SOFTHSM2_CONF}"
    echo "log.level = DEBUG" >> "${SOFTHSM2_CONF}"
    softhsm2-util --init-token --free --pin 1234 --so-pin 1234 --label "softhsm2" | awk '/^The token has been initialized and is reassigned to slot/ { print $NF }'
fi
exit 0
