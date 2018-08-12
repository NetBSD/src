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

status=0
start=`date +%s`
end=`expr $start + 1200`
now=$start
while test $now -lt $end
do
	et=`expr $now - $start`
	echo "=============== $et ============"
	$JOURNALPRINT ns1/signing.test.db.signed.jnl | $PERL check_journal.pl
	$DIG axfr signing.test -p 5300 @10.53.0.1 > dig.out.at$et
	awk '$4 == "RRSIG" { print $11 }' dig.out.at$et | sort | uniq -c
	lines=`awk '$4 == "RRSIG" { print}' dig.out.at$et | wc -l`
	if [ ${et} -ne 0 -a ${lines} -ne 4009 ]
	then
		echo_i "failed"
		status=`expr $status + 1`
	fi
	sleep 20
	now=`date +%s`
done

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
