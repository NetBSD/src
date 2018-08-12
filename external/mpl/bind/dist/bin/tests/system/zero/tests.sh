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

DIGOPTS="-p ${PORT}"

status=0
n=0

n=`expr $n + 1`
echo_i "check lookups against TTL=0 records ($n)"
i=0
passes=10
$DIG $DIGOPTS @10.53.0.2 axfr example | grep -v "^ds0" |
awk '$2 == "0" { print "-q", $1, $4; print "-q", "zzz"$1, $4;}' > query.list
while [ $i -lt $passes ]
do
	ret=0
	$DIG $DIGOPTS @10.53.0.3 -f query.list > dig.out$i.1.test$n &
	$DIG $DIGOPTS @10.53.0.3 -f query.list > dig.out$i.2.test$n &
	$DIG $DIGOPTS @10.53.0.3 -f query.list > dig.out$i.3.test$n &
	$DIG $DIGOPTS @10.53.0.3 -f query.list > dig.out$i.4.test$n &
	$DIG $DIGOPTS @10.53.0.3 -f query.list > dig.out$i.5.test$n &
	$DIG $DIGOPTS @10.53.0.3 -f query.list > dig.out$i.6.test$n &
	wait
	grep "status: SERVFAIL" dig.out$i.1.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.2.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.3.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.4.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.5.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.6.test$n && ret=1
	[ $ret = 1 ] && break
	i=`expr $i + 1`
	echo_i "successfully completed pass $i of $passes"
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check repeated recursive lookups of non recurring TTL=0 responses get new values ($n)"
count=`(
$DIG $DIGOPTS +short @10.53.0.3 foo.increment
$DIG $DIGOPTS +short @10.53.0.3 foo.increment
$DIG $DIGOPTS +short @10.53.0.3 foo.increment
$DIG $DIGOPTS +short @10.53.0.3 foo.increment
$DIG $DIGOPTS +short @10.53.0.3 foo.increment
$DIG $DIGOPTS +short @10.53.0.3 foo.increment
$DIG $DIGOPTS +short @10.53.0.3 foo.increment
) | sort -u | wc -l `
if [ $count -ne 7 ] ; then echo_i "failed (count=$count)"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check lookups against TTL=1 records ($n)"
i=0
passes=10
while [ $i -lt $passes ]
do
	ret=0
	$DIG $DIGOPTS @10.53.0.3 www.one.tld > dig.out$i.1.test$n
	$DIG $DIGOPTS @10.53.0.3 www.one.tld > dig.out$i.2.test$n
	$DIG $DIGOPTS @10.53.0.3 www.one.tld > dig.out$i.3.test$n
	$DIG $DIGOPTS @10.53.0.3 www.one.tld > dig.out$i.4.test$n
	$DIG $DIGOPTS @10.53.0.3 www.one.tld > dig.out$i.5.test$n
	$DIG $DIGOPTS @10.53.0.3 www.one.tld > dig.out$i.6.test$n
	grep "status: SERVFAIL" dig.out$i.1.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.2.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.3.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.4.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.5.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.6.test$n && ret=1
	[ $ret = 1 ] && break
	i=`expr $i + 1`
	echo_i "successfully completed pass $i of $passes"
	$PERL -e 'select(undef, undef, undef, 0.3);'
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
