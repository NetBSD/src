#! /bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

for QDIR in `echo "$PATH" | tr : ' '` ../../../../contrib/queryperf; do
    QPERF=$QDIR/queryperf
    if test -f $QPERF -a -x $QPERF; then
	echo $QPERF
	exit 0
    fi
done

exit 0
