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
n=0

RNDCOPTS="-c ../common/rndc.conf -s 10.53.0.2 -p ${CONTROLPORT}"
DIGOPTS="+nosea +nocomm +nocmd +noquest +noadd +noauth +nocomm \
         +nostat @10.53.0.2 -p ${PORT}"

# fill the cache with nodes from flushtest.example zone
load_cache () {
        # empty all existing cache data
        $RNDC $RNDCOPTS flush

	# load the positive cache entries
	$DIG $DIGOPTS -f - << EOF > /dev/null 2>&1
txt top1.flushtest.example
txt second1.top1.flushtest.example
txt third1.second1.top1.flushtest.example
txt third2.second1.top1.flushtest.example
txt second2.top1.flushtest.example
txt second3.top1.flushtest.example
txt second1.top2.flushtest.example
txt second2.top2.flushtest.example
txt second3.top2.flushtest.example
txt top3.flushtest.example
txt second1.top3.flushtest.example
txt third1.second1.top3.flushtest.example
txt third2.second1.top3.flushtest.example
txt third1.second2.top3.flushtest.example
txt third2.second2.top3.flushtest.example
txt second3.top3.flushtest.example
EOF

	# load the negative cache entries
        # nxrrset:
	$DIG $DIGOPTS a third1.second1.top1.flushtest.example > /dev/null
        # nxdomain:
	$DIG $DIGOPTS txt top4.flushtest.example > /dev/null
        # empty nonterminal:
	$DIG $DIGOPTS txt second2.top3.flushtest.example > /dev/null

	# sleep 2 seconds ensure the TTLs will be lower on cached data
	sleep 2
}

dump_cache () {
        rndc_dumpdb ns2 -cache _default
}

clear_cache () {
        $RNDC $RNDCOPTS flush
}

in_cache () {
        ttl=`$DIG $DIGOPTS "$@" | awk '{print $2}'`
        [ -z "$ttl" ] && {
                ttl=`$DIG $DIGOPTS +noanswer +auth "$@" | awk '{print $2}'`
                [ "$ttl" -ge 3599 ] && return 1
                return 0
        }
        [ "$ttl" -ge 3599 ] && return 1
        return 0
}

# Extract records at and below name "$1" from the cache dump in file "$2".
filter_tree () {
	tree="$1"
	file="$2"
	perl -n -e '
		next if /^;/;
		if (/'"$tree"'/ || (/^\t/ && $print)) {
			$print = 1;
		} else {
			$print = 0;
		}
		print if $print;
	' "$file"
}

n=`expr $n + 1`
echo_i "check correctness of routine cache cleaning ($n)"
$DIG $DIGOPTS +tcp +keepopen -b 10.53.0.7 -f dig.batch > dig.out.ns2 || status=1

digcomp --lc dig.out.ns2 knowngood.dig.out || status=1

n=`expr $n + 1`
echo_i "only one tcp socket was used ($n)"
tcpclients=`awk '$3 == "client" && $5 ~ /10.53.0.7#[0-9]*:/ {print $5}' ns2/named.run | sort | uniq -c | wc -l`

test $tcpclients -eq 1 || { status=1; echo_i "failed"; }

n=`expr $n + 1`
echo_i "reset and check that records are correctly cached initially ($n)"
ret=0
load_cache
dump_cache
nrecords=`filter_tree flushtest.example ns2/named_dump.db.test$n | grep -E '(TXT|ANY)' | wc -l`
[ $nrecords -eq 18 ] || { ret=1; echo_i "found $nrecords records expected 18"; }
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check flushing of the full cache ($n)"
ret=0
clear_cache
dump_cache
nrecords=`filter_tree flushtest.example ns2/named_dump.db.test$n | wc -l`
[ $nrecords -eq 0 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check flushing of individual nodes (interior node) ($n)"
ret=0
clear_cache
load_cache
# interior node
in_cache txt top1.flushtest.example || ret=1
$RNDC $RNDCOPTS flushname top1.flushtest.example
in_cache txt top1.flushtest.example && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check flushing of individual nodes (leaf node, under the interior node) ($n)"
ret=0
# leaf node, under the interior node (should still exist)
in_cache txt third2.second1.top1.flushtest.example || ret=1
$RNDC $RNDCOPTS flushname third2.second1.top1.flushtest.example
in_cache txt third2.second1.top1.flushtest.example && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check flushing of individual nodes (another leaf node, with both positive and negative cache entries) ($n)"
ret=0
# another leaf node, with both positive and negative cache entries
in_cache a third1.second1.top1.flushtest.example || ret=1
in_cache txt third1.second1.top1.flushtest.example || ret=1
$RNDC $RNDCOPTS flushname third1.second1.top1.flushtest.example
in_cache a third1.second1.top1.flushtest.example && ret=1
in_cache txt third1.second1.top1.flushtest.example && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check flushing a nonexistent name ($n)"
ret=0
$RNDC $RNDCOPTS flushname fake.flushtest.example || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check flushing of namespaces ($n)"
ret=0
clear_cache
load_cache
# flushing leaf node should leave the interior node:
in_cache txt third1.second1.top1.flushtest.example || ret=1
in_cache txt top1.flushtest.example || ret=1
$RNDC $RNDCOPTS flushtree third1.second1.top1.flushtest.example
in_cache txt third1.second1.top1.flushtest.example && ret=1
in_cache txt top1.flushtest.example || ret=1
in_cache txt second1.top1.flushtest.example || ret=1
in_cache txt third2.second1.top1.flushtest.example || ret=1
$RNDC $RNDCOPTS flushtree second1.top1.flushtest.example
in_cache txt top1.flushtest.example || ret=1
in_cache txt second1.top1.flushtest.example && ret=1
in_cache txt third2.second1.top1.flushtest.example && ret=1

# flushing from an empty node should still remove all its children
in_cache txt second1.top2.flushtest.example || ret=1
$RNDC $RNDCOPTS flushtree top2.flushtest.example
in_cache txt second1.top2.flushtest.example && ret=1
in_cache txt second2.top2.flushtest.example && ret=1
in_cache txt second3.top2.flushtest.example && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check flushing a nonexistent namespace ($n)"
ret=0
$RNDC $RNDCOPTS flushtree fake.flushtest.example || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check the number of cached records remaining ($n)"
ret=0
dump_cache
nrecords=`filter_tree flushtest.example ns2/named_dump.db.test$n | grep -v '^;' | grep -E '(TXT|ANY)' | wc -l`
[ $nrecords -eq 17 ] || { ret=1; echo_i "found $nrecords records expected 17"; }
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check the check that flushname of a partial match works ($n)"
ret=0
in_cache txt second2.top1.flushtest.example || ret=1
$RNDC $RNDCOPTS flushtree example
in_cache txt second2.top1.flushtest.example && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check the number of cached records remaining ($n)"
ret=0
dump_cache
nrecords=`filter_tree flushtest.example ns2/named_dump.db.test$n | grep -E '(TXT|ANY)' | wc -l`
[ $nrecords -eq 1 ] || { ret=1; echo_i "found $nrecords records expected 1"; }
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check flushtree clears adb correctly ($n)"
ret=0
load_cache
dump_cache
mv ns2/named_dump.db.test$n ns2/named_dump.db.test$n.a
sed -n '/plain success\/timeout/,/Unassociated entries/p' \
	ns2/named_dump.db.test$n.a > sed.out.$n.a
grep 'plain success/timeout' sed.out.$n.a > /dev/null 2>&1 || ret=1
grep 'Unassociated entries' sed.out.$n.a > /dev/null 2>&1 || ret=1
grep 'ns.flushtest.example' sed.out.$n.a > /dev/null 2>&1 || ret=1
$RNDC $RNDCOPTS flushtree flushtest.example || ret=1
dump_cache
mv ns2/named_dump.db.test$n ns2/named_dump.db.test$n.b
sed -n '/plain success\/timeout/,/Unassociated entries/p' \
	ns2/named_dump.db.test$n.b > sed.out.$n.b
grep 'plain success/timeout' sed.out.$n.b > /dev/null 2>&1 || ret=1
grep 'Unassociated entries' sed.out.$n.b > /dev/null 2>&1 || ret=1
grep 'ns.flushtest.example' sed.out.$n.b > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check expire option returned from primary zone ($n)"
ret=0
$DIG @10.53.0.1 -p ${PORT} +expire soa expire-test > dig.out.expire
grep EXPIRE: dig.out.expire > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check expire option returned from secondary zone ($n)"
ret=0
$DIG @10.53.0.2 -p ${PORT} +expire soa expire-test > dig.out.expire
grep EXPIRE: dig.out.expire > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
