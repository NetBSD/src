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

status=0

# Wait for the zone to be fully signed before beginning test
#
# We expect the zone to have the following:
#
# - 5 signatures for signing.test.
# - 3 signatures for ns.signing.test.
# - 2 x 500 signatures for a{0000-0499}.signing.test.
#
# for a total of 1008.
fully_signed () {
        $DIG axfr signing.test -p ${PORT} @10.53.0.1 |
                awk 'BEGIN { lines = 0 }
                     $4 == "RRSIG" {lines++}
                     END { if (lines != 1008) exit(1) }'
}
retry_quiet 30 fully_signed || ret=1

start=`date +%s`
now=$start
end=$((start + 140))

while [ $now -lt $end ]; do
        et=$((now - start))
	echo "=============== $et ============"
	$JOURNALPRINT ns1/signing.test.db.signed.jnl | $PERL check_journal.pl
	$DIG axfr signing.test -p ${PORT} @10.53.0.1 > dig.out.at$et
	awk '$4 == "RRSIG" { print $11 }' dig.out.at$et | sort | uniq -c
	lines=`awk '$4 == "RRSIG" { print}' dig.out.at$et | wc -l`
	if [ ${et} -ne 0 -a ${lines} -ne 1009 ]
	then
		echo_i "failed"
                status=$((status + 1))
	fi
	sleep 5
	now=`date +%s`
done

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
