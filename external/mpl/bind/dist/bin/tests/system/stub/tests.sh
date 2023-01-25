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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

DIGOPTS="+tcp -p ${PORT}"

status=0
echo_i "check that the stub zone has been saved to disk"
for i in 1 2 3 4 5 6 7 8 9 20
do
	[ -f ns3/child.example.st ] && break
	sleep 1
done
[ -f ns3/child.example.st ] || { status=1;  echo_i "failed"; }

for pass in 1 2
do

echo_i "trying an axfr that should be denied (NOTAUTH) (pass=$pass)"
ret=0
$DIG $DIGOPTS child.example. @10.53.0.3 axfr > dig.out.ns3 || ret=1
grep "; Transfer failed." dig.out.ns3 > /dev/null || ret=1
[ $ret = 0 ] || { status=1;  echo_i "failed"; }

echo_i "look for stub zone data without recursion (should not be found) (pass=$pass)"
for i in 1 2 3 4 5 6 7 8 9
do
	ret=0
	$DIG $DIGOPTS +norec data.child.example. \
		@10.53.0.3 txt > dig.out.ns3 || ret=1
	grep "status: NOERROR" dig.out.ns3 > /dev/null || ret=1
	[ $ret = 0 ] && break
	sleep 1
done
digcomp knowngood.dig.out.norec dig.out.ns3 || ret=1
[ $ret = 0 ] || { status=1;  echo_i "failed"; }

echo_i "look for stub zone data with recursion (should be found) (pass=$pass)"
ret=0
$DIG $DIGOPTS +noauth +noadd data.child.example. @10.53.0.3 txt > dig.out.ns3 || ret=1
digcomp knowngood.dig.out.rec dig.out.ns3 || ret=1
[ $ret = 0 ] || { status=1;  echo_i "failed"; }

[ $pass = 1 ] && {
	echo_i "stopping stub server"
	stop_server ns3

	echo_i "re-starting stub server"
	start_server --noclean --restart --port ${PORT} ns3
}
done

echo_i "check that glue record is correctly transferred from master when minimal-responses is on"
ret=0
# First ensure that zone data was transfered.
for i in 1 2 3 4 5 6 7; do
    [ -f ns5/example.db ] && break
    sleep 1
done

if [ -f ns5/example.db ]; then
    # If NS glue wasn't transferred,  this query would fail.
    $DIG $DIGOPTS +nodnssec @10.53.0.5 target.example. txt > dig.out.ns5 || ret=1
    grep  'target\.example.*TXT.*"test"' dig.out.ns5 > /dev/null || ret=1
    # Ensure both ipv4 and ipv6 glue records were transferred.
    grep -E 'ns4[[:space:]]+A[[:space:]]+10.53.0.4' ns5/example.db > /dev/null || ret=1
    grep -E 'AAAA[[:space:]]+fd92:7065:b8e:ffff::4' ns5/example.db > /dev/null || ret=1
    [ $ret = 0 ] || { status=1;  echo_i "failed"; }
else
    status=1
    echo_i "failed: stub zone transfer failed ns4(master) <---> ns5/example.db"
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
