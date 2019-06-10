# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

DIGOPTS="+tcp +noadd +nosea +nostat +noquest -p ${PORT}"

status=0

#
# 10.53.0.1 10.53.0.2 10.53.0.3 have a global dscp setting;
# 10.53.0.4 10.53.0.5 10.53.0.6 have dscp set in option *-source clauses;
# 10.53.0.7 has dscp set in zone *-source clauses;
#
for server in 10.53.0.1 10.53.0.2 10.53.0.3 10.53.0.4 10.53.0.5 \
	      10.53.0.6 10.53.0.7
do
	echo_i "testing root SOA lookup at $server"
	for i in 0 1 2 3 4 5 6 7 8 9
	do
		ret=0
		$DIG $DIGOPTS @$server soa . > dig.out.$server
		grep "status: NOERROR" dig.out.$server > /dev/null || ret=1
		test $ret = 0 && break
		sleep 1
	done
	test $ret = 0 || { echo_i "failed"; status=`expr $status + $ret`; }
done

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
