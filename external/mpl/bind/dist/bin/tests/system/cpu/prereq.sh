#!/bin/sh -e
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

set -e

# shellcheck source=conf.sh
. ../conf.sh

case $(uname) in
	Linux*)
		;;
	*)
		echo_i "cpu test only runs on Linux"
		exit 255
		;;
esac

# TASKSET will be an empty string if no taskset program was found.
TASKSET=$(command -v "taskset" || true)
if ! test -x "$TASKSET" ; then
	exit 255
fi

if ! $TASKSET fff0 true > /dev/null 2>&1; then
        echo_i "taskset failed"
        exit 255
fi
