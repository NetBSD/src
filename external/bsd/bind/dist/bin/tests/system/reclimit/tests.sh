#!/bin/sh
#
# Copyright (C) 2014-2017  Internet Systems Consortium, Inc. ("ISC")
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

DIGOPTS="-p 5300"

status=0
n=0

ns3_reset() {
	cp $1 ns3/named.conf
	$RNDC -c ../common/rndc.conf -s 10.53.0.3 -p 9953 reconfig 2>&1 | sed 's/^/I:ns3 /'
	$RNDC -c ../common/rndc.conf -s 10.53.0.3 -p 9953 flush | sed 's/^/I: ns3 /'
}

ns3_sends_aaaa_queries() {
	if grep "started AAAA fetch" ns3/named.run >/dev/null; then
		return 0
	else
		return 1
	fi
}

# Check whether the number of queries ans2 received from ns3 (this value is
# read from dig output stored in file $1) is as expected.  The expected query
# count is variable:
#   - if ns3 sends AAAA queries, the query count should equal $2,
#   - if ns3 does not send AAAA queries, the query count should equal $3.
check_query_count() {
	count=`sed 's/[^0-9]//g;' $1`
	expected_count_with_aaaa=$2
	expected_count_without_aaaa=$3

	if ns3_sends_aaaa_queries; then
		expected_count=$expected_count_with_aaaa
	else
		expected_count=$expected_count_without_aaaa
	fi

	if [ $count -ne $expected_count ]; then
		echo "I: count ($count) != $expected_count"
		ret=1
	fi
}

echo "I: set max-recursion-depth=12"

n=`expr $n + 1`
echo "I:  attempt excessive-depth lookup ($n)"
ret=0
echo "1000" > ans2/ans.limit
$DIG $DIGOPTS @10.53.0.2 reset > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 indirect1.example.org > dig.out.1.test$n || ret=1
grep "status: SERVFAIL" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +short @10.53.0.2 count txt > dig.out.2.test$n || ret=1
check_query_count dig.out.2.test$n 26 14
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:  attempt permissible lookup ($n)"
ret=0
echo "12" > ans2/ans.limit
ns3_reset ns3/named1.conf
$DIG $DIGOPTS @10.53.0.2 reset > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 indirect2.example.org > dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +short @10.53.0.2 count txt > dig.out.2.test$n || ret=1
check_query_count dig.out.2.test$n 49 26
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I: set max-recursion-depth=5"

n=`expr $n + 1`
echo "I:  attempt excessive-depth lookup ($n)"
ret=0
echo "12" > ans2/ans.limit
ns3_reset ns3/named2.conf
$DIG $DIGOPTS @10.53.0.2 reset > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 indirect3.example.org > dig.out.1.test$n || ret=1
grep "status: SERVFAIL" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +short @10.53.0.2 count txt > dig.out.2.test$n || ret=1
check_query_count dig.out.2.test$n 12 7
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:  attempt permissible lookup ($n)"
ret=0
echo "5" > ans2/ans.limit
ns3_reset ns3/named2.conf
$DIG $DIGOPTS @10.53.0.2 reset > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 indirect4.example.org > dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +short @10.53.0.2 count txt > dig.out.2.test$n || ret=1
check_query_count dig.out.2.test$n 21 12
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I: set max-recursion-depth=100, max-recursion-queries=50"

n=`expr $n + 1`
echo "I:  attempt excessive-queries lookup ($n)"
ret=0
echo "13" > ans2/ans.limit
ns3_reset ns3/named3.conf
$DIG $DIGOPTS @10.53.0.2 reset > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 indirect5.example.org > dig.out.1.test$n || ret=1
if ns3_sends_aaaa_queries; then
  grep "status: SERVFAIL" dig.out.1.test$n > /dev/null || ret=1
fi
$DIG $DIGOPTS +short @10.53.0.2 count txt > dig.out.2.test$n || ret=1
eval count=`cat dig.out.2.test$n`
[ $count -le 50 ] || { ret=1; echo "I: count ($count) !<= 50"; }
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:  attempt permissible lookup ($n)"
ret=0
echo "12" > ans2/ans.limit
ns3_reset ns3/named3.conf
$DIG $DIGOPTS @10.53.0.2 reset > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 indirect6.example.org > dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +short @10.53.0.2 count txt > dig.out.2.test$n || ret=1
eval count=`cat dig.out.2.test$n`
[ $count -le 50 ] || { ret=1; echo "I: count ($count) !<= 50"; }
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I: set max-recursion-depth=100, max-recursion-queries=40"

n=`expr $n + 1`
echo "I:  attempt excessive-queries lookup ($n)"
ret=0
echo "10" > ans2/ans.limit
ns3_reset ns3/named4.conf
$DIG $DIGOPTS @10.53.0.2 reset > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 indirect7.example.org > dig.out.1.test$n || ret=1
if ns3_sends_aaaa_queries; then
  grep "status: SERVFAIL" dig.out.1.test$n > /dev/null || ret=1
fi
$DIG $DIGOPTS +short @10.53.0.2 count txt > dig.out.2.test$n || ret=1
eval count=`cat dig.out.2.test$n`
[ $count -le 40 ] || { ret=1; echo "I: count ($count) !<= 40"; }
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:  attempt permissible lookup ($n)"
ret=0
echo "9" > ans2/ans.limit
ns3_reset ns3/named4.conf
$DIG $DIGOPTS @10.53.0.2 reset > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 indirect8.example.org > dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +short @10.53.0.2 count txt > dig.out.2.test$n || ret=1
eval count=`cat dig.out.2.test$n`
[ $count -le 40 ] || { ret=1; echo "I: count ($count) !<= 40"; }
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: attempting NS explosion ($n)"
ret=0
ns3_reset ns3/named4.conf
$DIG $DIGOPTS @10.53.0.2 reset > /dev/null || ret=1
$DIG $DIGOPTS +short @10.53.0.3 ns1.1.example.net > dig.out.1.test$n || ret=1
$DIG $DIGOPTS +short @10.53.0.2 count txt > dig.out.2.test$n || ret=1
eval count=`cat dig.out.2.test$n`
[ $count -lt 50 ] || ret=1
$DIG $DIGOPTS +short @10.53.0.7 count txt > dig.out.3.test$n || ret=1
eval count=`cat dig.out.3.test$n`
[ $count -lt 50 ] || { ret=1; echo "I: count ($count) !<= 50";  }
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:exit status: $status"
[ $status -eq 0 ] || exit 1
