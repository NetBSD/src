#!/bin/sh
#
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
n=0

rm -f dig.out.*

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd +dnssec -p ${PORT}"

newtest() {
	n=`expr $n + 1`
	case $# in
	1)
		echo_i "$1 ($n)"
		;;
	2)
		echo_i "$1"
		echo_ic "$2 ($n)"
		;;
	esac
	ret=0
}

newtest "get test ids"
$DIG $DIGOPTS . dnskey +short +rrcomm @10.53.0.1 > dig.out.ns1.test$n || ret=1
oldid=`sed -n 's/.*key id = //p' < dig.out.ns1.test$n`
oldid=`expr "0000${oldid}" : '.*\(.....\)$'`
newid=`expr \( ${oldid} + 1000 \) % 65536`
newid=`expr "0000${newid}" : '.*\(.....\)$'`
badid=`expr \( ${oldid} + 7777 \) % 65536`
badid=`expr "0000${badid}" : '.*\(.....\)$'`
echo_i "test id: oldid=${oldid} (configured)"
echo_i "test id: newid=${newid} (not configured)"
echo_i "test id: badid=${badid}"
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check authoritative server (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.2 example SOA > dig.out.ns2.test$n
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check test zone resolves with 'root-key-sentinel yes;'" " (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.3 example SOA > dig.out.ns3.test$n
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-is-ta with old ta and" " 'root-key-sentinel yes;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.3 root-key-sentinel-is-ta-${oldid}.example A > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-not-ta with old ta and" " 'root-key-sentinel yes;' (expect SERVFAIL)"
$DIG $DIGOPTS @10.53.0.3 root-key-sentinel-not-ta-${oldid}.example A > dig.out.ns3.test$n || ret=1
grep "status: SERVFAIL" dig.out.ns3.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-not-ta with old ta, CD=1 and" " 'root-key-sentinel yes;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.3 +cd root-key-sentinel-not-ta-${oldid}.example A > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-is-ta with new ta and" " 'root-key-sentinel yes;' (expect SERVFAIL)"
$DIG $DIGOPTS @10.53.0.3 root-key-sentinel-is-ta-${newid}.example A > dig.out.ns3.test$n || ret=1
grep "status: SERVFAIL" dig.out.ns3.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-is-ta with new ta, CD=1 and" " 'root-key-sentinel yes;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.3 +cd root-key-sentinel-is-ta-${newid}.example A > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-not-ta with new ta and" " 'root-key-sentinel yes;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.3 root-key-sentinel-not-ta-${newid}.example A > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi

newtest "check root-key-sentinel-is-ta with bad ta and" " 'root-key-sentinel yes;' (expect SERVFAIL)"
$DIG $DIGOPTS @10.53.0.3 root-key-sentinel-is-ta-${badid}.example A > dig.out.ns3.test$n || ret=1
grep "status: SERVFAIL" dig.out.ns3.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-is-ta with bad ta, CD=1 and" " 'root-key-sentinel yes;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.3 +cd root-key-sentinel-is-ta-${badid}.example A > dig.out.ns3.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-not-ta with bad ta and" " 'root-key-sentinel yes;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.3 root-key-sentinel-not-ta-${bad}.example A > dig.out.ns3.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi

newtest "check root-key-sentinel-is-ta with out-of-range ta and" " 'root-key-sentinel yes;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.3 root-key-sentinel-is-ta-72345.example A > dig.out.ns3.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-not-ta with out-of-range ta and" " 'root-key-sentinel yes;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.3 root-key-sentinel-not-ta-72345.example A > dig.out.ns3.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi

newtest "check root-key-sentinel-is-ta with no-zero-pad ta and" " 'root-key-sentinel yes;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.3 root-key-sentinel-is-ta-1234.example A > dig.out.ns3.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-not-ta with no-zero-pad ta and" " 'root-key-sentinel yes;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.3 root-key-sentinel-not-ta-1234.example A > dig.out.ns3.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi

newtest "check CNAME to root-key-sentinel-is-ta with old ta and" " 'root-key-sentinel yes;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.3 old-is-ta.example A > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "old-is-ta.*CNAME.root-key-sentinel-is-ta-${oldid}.example." dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check CNAME to root-key-sentinel-not-ta with old ta and" " 'root-key-sentinel yes;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.3 old-not-ta.example A > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "old-not-ta.*CNAME.root-key-sentinel-not-ta-${oldid}.example." dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check CNAME to root-key-sentinel-is-ta with new ta and" " 'root-key-sentinel yes;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.3 new-is-ta.example A > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "new-is-ta.*CNAME.root-key-sentinel-is-ta-${newid}.example." dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check CNAME to root-key-sentinel-not-ta with new ta and" " 'root-key-sentinel yes;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.3 new-not-ta.example A > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "new-not-ta.*CNAME.root-key-sentinel-not-ta-${newid}.example." dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check CNAME to root-key-sentinel-is-ta with bad ta and" " 'root-key-sentinel yes;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.3 bad-is-ta.example A > dig.out.ns3.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns3.test$n > /dev/null || ret=1
grep "bad-is-ta.*CNAME.root-key-sentinel-is-ta-${badid}.example" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check CNAME to root-key-sentinel-not-ta with bad ta and" " 'root-key-sentinel yes;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.3 bad-not-ta.example A > dig.out.ns3.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns3.test$n > /dev/null || ret=1
grep "bad-not-ta.*CNAME.root-key-sentinel-not-ta-${badid}.example." dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check test zone resolves with 'root-key-sentinel no;'" " (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.4 example SOA > dig.out.ns4.test$n
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-is-ta with old ta and" " 'root-key-sentinel no;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.4 root-key-sentinel-is-ta-${oldid}.example A > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-not-ta with old ta and" " 'root-key-sentinel no;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.4 root-key-sentinel-not-ta-${oldid}.example A > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-is-ta with new ta and" " 'root-key-sentinel no;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.4 root-key-sentinel-is-ta-${newid}.example A > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-not-ta with new ta and" " 'root-key-sentinel no;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.4 root-key-sentinel-not-ta-${newid}.example A > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi

newtest "check root-key-sentinel-is-ta with bad ta and" " 'root-key-sentinel no;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.4 root-key-sentinel-is-ta-${badid}.example A > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-not-ta with bad ta and" " 'root-key-sentinel no;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.4 root-key-sentinel-not-ta-${bad}.example A > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi

newtest "check root-key-sentinel-is-ta with out-of-range ta and" " 'root-key-sentinel no;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.4 root-key-sentinel-is-ta-72345.example A > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-not-ta with out-of-range ta and" " 'root-key-sentinel no;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.4 root-key-sentinel-not-ta-72345.example A > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi

newtest "check root-key-sentinel-is-ta with no-zero-pad ta and" " 'root-key-sentinel no;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.4 root-key-sentinel-is-ta-1234.example A > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check root-key-sentinel-not-ta with no-zero-pad ta and" " 'root-key-sentinel no;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.4 root-key-sentinel-not-ta-1234.example A > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi

newtest "check CNAME to root-key-sentinel-is-ta with old ta and" " 'root-key-sentinel no;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.4 old-is-ta.example A > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "old-is-ta.*CNAME.root-key-sentinel-is-ta-${oldid}.example." dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check CNAME to root-key-sentinel-not-ta with old ta and" " 'root-key-sentinel no;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.4 old-not-ta.example A > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "old-not-ta.*CNAME.root-key-sentinel-not-ta-${oldid}.example." dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check CNAME to root-key-sentinel-is-ta with new ta and" " 'root-key-sentinel no;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.4 new-is-ta.example A > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "new-is-ta.*CNAME.root-key-sentinel-is-ta-${newid}.example." dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check CNAME to root-key-sentinel-not-ta with new ta and" " 'root-key-sentinel no;' (expect NOERROR)"
$DIG $DIGOPTS @10.53.0.4 new-not-ta.example A > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "new-not-ta.*CNAME.root-key-sentinel-not-ta-${newid}.example." dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check CNAME to root-key-sentinel-is-ta with bad ta and" " 'root-key-sentinel no;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.4 bad-is-ta.example A > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
grep "bad-is-ta.*CNAME.root-key-sentinel-is-ta-${badid}.example" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "check CNAME to root-key-sentinel-not-ta with bad ta and" " 'root-key-sentinel no;' (expect NXDOMAIN)"
$DIG $DIGOPTS @10.53.0.4 bad-not-ta.example A > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
grep "bad-not-ta.*CNAME.root-key-sentinel-not-ta-${badid}.example." dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
