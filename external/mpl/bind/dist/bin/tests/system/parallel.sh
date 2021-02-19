#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

. ./conf.sh

PARALLELS=`echo $PARALLELDIRS | sed "s|\([^ ][^ ]*\)|test-\1|g;" | tr _ -`

echo ".PHONY: $PARALLELS"
echo
echo "check_interfaces:"
echo "	@${PERL} testsock.pl > /dev/null 2>&1 || { \\"
echo "		echo \"I:NOTE: System tests were skipped because they require the\"; \\"
echo "		echo \"I:      test IP addresses 10.53.0.* to be configured as alias\"; \\"
echo "		echo \"I:      addresses on the loopback interface.  Please run\"; \\"
echo "		echo \"I:      \"bin/tests/system/ifconfig.sh up\" as root to configure them.\"; \\"
echo "		exit 1; \\"
echo "	}"
echo
echo "test check: $PARALLELS"
port=${STARTPORT:-5000}
for directory in $PARALLELDIRS ; do
        echo
        echo "test-`echo $directory | tr _ -`: check_interfaces"
        echo "	@${SHELL} ./run.sh -p $port $directory 2>&1 | tee test.output.$directory"
        port=`expr $port + 100`
done
