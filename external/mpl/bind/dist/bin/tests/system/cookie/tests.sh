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

DIGOPTS="-p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0
n=0

getcookie() {
	awk '$2 == "COOKIE:" {
		print $3;
	}' < $1 | tr -d '\r'
}

fullcookie() {
	awk 'BEGIN { n = 0 }
	     // { v[n++] = length(); }
	     END { print (v[1] == v[2]); }'
}

havetc() {
	grep 'flags:.* tc[^;]*;' $1 > /dev/null
}

for bad in bad*.conf
do
	n=`expr $n + 1`
	echo_i "checking that named-checkconf detects error in $bad ($n)"
	ret=0
	$CHECKCONF $bad > /dev/null 2>&1 && ret=1
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`
done

for good in good*.conf
do
	n=`expr $n + 1`
	echo_i "checking that named-checkconf detects accepts $good ($n)"
	ret=0
	$CHECKCONF $good > /dev/null 2>&1 || ret=1
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`
done

n=`expr $n + 1`
echo_i "checking RCODE=FORMERR to query without question section and without COOKIE option ($n)"
ret=0
$DIG $DIGOPTS +qr +header-only +nocookie version.bind txt ch @10.53.0.1 > dig.out.test$n
grep COOKIE: dig.out.test$n > /dev/null && ret=1
grep "status: FORMERR" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking RCODE=NOERROR to query without question section and with COOKIE option ($n)"
ret=0
$DIG $DIGOPTS +qr +header-only +cookie version.bind txt ch @10.53.0.1 > dig.out.test$n
grep COOKIE: dig.out.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking COOKIE token is returned to empty COOKIE option ($n)"
ret=0
$DIG $DIGOPTS +cookie version.bind txt ch @10.53.0.1 > dig.out.test$n
grep COOKIE: dig.out.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking COOKIE is not returned when answer-cookie is false ($n)"
ret=0
$DIG $DIGOPTS +cookie version.bind txt ch @10.53.0.7 > dig.out.test$n
grep COOKIE: dig.out.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking response size without COOKIE ($n)"
ret=0
$DIG $DIGOPTS large.example txt @10.53.0.1 +ignore > dig.out.test$n
havetc dig.out.test$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking response size without valid COOKIE ($n)"
ret=0
$DIG $DIGOPTS +cookie large.example txt @10.53.0.1 +ignore > dig.out.test$n
havetc dig.out.test$n || ret=1
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking response size with COOKIE ($n)"
ret=0
$DIG $DIGOPTS +cookie large.example txt @10.53.0.1 > dig.out.test$n.l
cookie=`getcookie dig.out.test$n.l`
$DIG $DIGOPTS +qr +cookie=$cookie large.example txt @10.53.0.1 +ignore > dig.out.test$n
havetc dig.out.test$n && ret=1
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking response size with COOKIE recursive ($n)"
ret=0
$DIG $DIGOPTS +qr +cookie=$cookie large.xxx txt @10.53.0.1 +ignore > dig.out.test$n
havetc dig.out.test$n && ret=1
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking COOKIE is learnt for TCP retry ($n)"
ret=0
$DIG $DIGOPTS +qr +cookie large.example txt @10.53.0.1 > dig.out.test$n
linecount=`getcookie dig.out.test$n | wc -l`
if [ $linecount != 3 ]; then ret=1; fi
checkfull=`getcookie dig.out.test$n | fullcookie`
if [ $checkfull != 1 ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking for COOKIE value in adb ($n)"
ret=0
$RNDCCMD 10.53.0.1 dumpdb
for i in 1 2 3 4 5 6 7 8 9 10
do
  sleep 1
  grep "10.53.0.2.*\[cookie=" ns1/named_dump.db > /dev/null && break
done
grep "10.53.0.2.*\[cookie=" ns1/named_dump.db > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking require-server-cookie default (no) ($n)"
ret=0
$DIG $DIGOPTS +qr +cookie +nobadcookie soa @10.53.0.1 > dig.out.test$n
grep BADCOOKIE dig.out.test$n > /dev/null && ret=1
linecount=`getcookie dig.out.test$n | wc -l`
if [ $linecount != 2 ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking require-server-cookie yes ($n)"
ret=0
$DIG $DIGOPTS +qr +cookie +nobadcookie soa @10.53.0.3 > dig.out.test$n
grep "flags: qr[^;]* aa[ ;]" dig.out.test$n > /dev/null && ret=1
grep "flags: qr[^;]* ad[ ;]" dig.out.test$n > /dev/null && ret=1
grep BADCOOKIE dig.out.test$n > /dev/null || ret=1
linecount=`getcookie dig.out.test$n | wc -l`
if [ $linecount != 2 ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking require-server-cookie yes with rate-limit ($n)"
ret=0
$DIG $DIGOPTS +qr +cookie +nobadcookie soa example @10.53.0.8 > dig.out.test$n
grep "flags: qr[^;]* ad[ ;]" dig.out.test$n > /dev/null && ret=1
grep BADCOOKIE dig.out.test$n > /dev/null || ret=1
linecount=`getcookie dig.out.test$n | wc -l`
if [ $linecount != 2 ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "send undersized cookie ($n)"
ret=0
$DIG $DIGOPTS +qr +cookie=000000 soa @10.53.0.1 > dig.out.test$n || ret=1
grep "status: FORMERR" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "send oversized for named cookie ($n)"
ret=0
$DIG $DIGOPTS +qr +cookie=${cookie}00 soa @10.53.0.1 > dig.out.test$n || ret=1
grep "COOKIE: [a-f0-9]* (good)" dig.out.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "send oversized for named cookie with server requiring a good cookie ($n)"
ret=0
$DIG $DIGOPTS +qr +cookie=${cookie}00 soa @10.53.0.3 > dig.out.test$n || ret=1
grep "COOKIE: [a-f0-9]* (good)" dig.out.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

#
# Test shared cookie-secret support.
#
# NS4 has cookie-secret "569d36a6cc27d6bf55502183302ba352745255a2";
#
# NS5 has cookie-secret "569d36a6cc27d6bf55502183302ba352745255a2";
# NS5 has cookie-secret "6b300e27a0db46d4b046e4189790fa7db3c1ffb3"; (alternate)
#
# NS6 has cookie-secret "6b300e27a0db46d4b046e4189790fa7db3c1ffb3";
#
# Server cookies from NS4 are accepted by NS5 and not NS6
# Server cookies from NS5 are accepted by NS4 and not NS6
# Server cookies from NS6 are accepted by NS5 and not NS4
#
# Force local address so that the client's address is the same to all servers.
#

n=`expr $n + 1`
echo_i "get NS4 cookie for cross server checking ($n)"
ret=0
$DIG $DIGOPTS +cookie -b 10.53.0.4 soa . @10.53.0.4 > dig.out.test$n
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
ns4cookie=`getcookie dig.out.test$n`
test -n "$ns4cookie" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "get NS5 cookie for cross server checking ($n)"
ret=0
$DIG $DIGOPTS +cookie -b 10.53.0.4 soa . @10.53.0.5 > dig.out.test$n
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
ns5cookie=`getcookie dig.out.test$n`
test -n "$ns5cookie" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "get NS6 cookie for cross server checking ($n)"
ret=0
$DIG $DIGOPTS +cookie -b 10.53.0.4 soa . @10.53.0.6 > dig.out.test$n
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
ns6cookie=`getcookie dig.out.test$n`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test NS4 cookie on NS5 (expect success) ($n)"
ret=0
$DIG $DIGOPTS +cookie=$ns4cookie -b 10.53.0.4 +nobadcookie soa . @10.53.0.5 > dig.out.test$n
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
grep "status: NOERROR," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test NS4 cookie on NS6 (expect badcookie) ($n)"
ret=0
$DIG $DIGOPTS +cookie=$ns4cookie -b 10.53.0.4 +nobadcookie soa . @10.53.0.6 > dig.out.test$n
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
grep "status: BADCOOKIE," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test NS5 cookie on NS4 (expect success) ($n)"
ret=0
$DIG $DIGOPTS +cookie=$ns5cookie -b 10.53.0.4 +nobadcookie soa . @10.53.0.4 > dig.out.test$n
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
grep "status: NOERROR," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test NS5 cookie on NS6 (expect badcookie) ($n)"
ret=0
$DIG $DIGOPTS +cookie=$ns5cookie -b 10.53.0.4 +nobadcookie soa . @10.53.0.6 > dig.out.test$n
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
grep "status: BADCOOKIE," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test NS6 cookie on NS4 (expect badcookie) ($n)"
ret=0
$DIG $DIGOPTS +cookie=$ns6cookie -b 10.53.0.4 +nobadcookie soa . @10.53.0.4 > dig.out.test$n
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
grep "status: BADCOOKIE," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test NS6 cookie on NS5 (expect success) ($n)"
ret=0
$DIG $DIGOPTS +cookie=$ns6cookie -b 10.53.0.4 +nobadcookie soa . @10.53.0.5 > dig.out.test$n
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
grep "status: NOERROR," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
