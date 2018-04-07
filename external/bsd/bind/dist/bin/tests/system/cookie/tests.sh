#!/bin/sh
#
# Copyright (C) 2017  Internet Systems Consortium, Inc. ("ISC")
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

status=0
n=0

getcookie() {
	awk '$2 == "COOKIE:" {
		print $3;
	}' < $1
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
        ret=0
        echo "I:checking that named-checkconf detects error in $bad"
        $CHECKCONF $bad > /dev/null 2>&1
        if [ $? != 1 ]; then echo "I:failed"; ret=1; fi
        status=`expr $status + $ret`
done

n=`expr $n + 1`
echo "I:checking COOKIE token returned to empty COOKIE option ($n)"
ret=0
$DIG +qr +cookie version.bind txt ch @10.53.0.1 -p 5300 > dig.out.test$n
grep COOKIE: dig.out.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:checking COOKIE token returned to empty COOKIE option (+sit) ($n)"
ret=0
$DIG +qr +sit version.bind txt ch @10.53.0.1 -p 5300 > dig.out.test$n
grep COOKIE: dig.out.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:checking response size without COOKIE ($n)"
ret=0
$DIG large.example txt @10.53.0.1 -p 5300 +ignore > dig.out.test$n
havetc dig.out.test$n || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:checking response size without valid COOKIE ($n)"
ret=0
$DIG +cookie large.example txt @10.53.0.1 -p 5300 +ignore > dig.out.test$n
havetc dig.out.test$n || ret=1
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:checking response size without valid COOKIE (+sit) ($n)"
ret=0
$DIG +sit large.example txt @10.53.0.1 -p 5300 +ignore > dig.out.test$n
havetc dig.out.test$n || ret=1
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:checking response size with COOKIE ($n)"
ret=0
$DIG +cookie large.example txt @10.53.0.1 -p 5300 > dig.out.test$n.l
cookie=`getcookie dig.out.test$n.l`
$DIG +qr +cookie=$cookie large.example txt @10.53.0.1 -p 5300 +ignore > dig.out.test$n
havetc dig.out.test$n && ret=1
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:checking response size with COOKIE (+sit) ($n)"
ret=0
$DIG +sit large.example txt @10.53.0.1 -p 5300 > dig.out.test$n.l
cookie=`getcookie dig.out.test$n.l`
$DIG +qr +cookie=$cookie large.example txt @10.53.0.1 -p 5300 +ignore > dig.out.test$n
havetc dig.out.test$n && ret=1
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:checking response size with COOKIE recursive ($n)"
ret=0
$DIG +qr +cookie=$cookie large.xxx txt @10.53.0.1 -p 5300 +ignore > dig.out.test$n
havetc dig.out.test$n && ret=1
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:checking response size with COOKIE recursive (+sit) ($n)"
ret=0
$DIG +qr +sit=$cookie large.xxx txt @10.53.0.1 -p 5300 +ignore > dig.out.test$n
havetc dig.out.test$n && ret=1
grep "; COOKIE:.*(good)" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:checking COOKIE is learnt for TCP retry ($n)"
ret=0
$DIG +qr +cookie large.example txt @10.53.0.1 -p 5300 > dig.out.test$n
linecount=`getcookie dig.out.test$n | wc -l`
if [ $linecount != 3 ]; then ret=1; fi
checkfull=`getcookie dig.out.test$n | fullcookie`
if [ $checkfull != 1 ]; then ret=1; fi
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:checking COOKIE is learnt for TCP retry (+sit) ($n)"
ret=0
$DIG +qr +sit large.example txt @10.53.0.1 -p 5300 > dig.out.test$n
linecount=`getcookie dig.out.test$n | wc -l`
if [ $linecount != 3 ]; then ret=1; fi
checkfull=`getcookie dig.out.test$n | fullcookie`
if [ $checkfull != 1 ]; then ret=1; fi
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:checking for COOKIE value in adb ($n)"
ret=0
$RNDC -c ../common/rndc.conf -s 10.53.0.1 -p 9953 dumpdb
sleep 1
grep "10.53.0.2.*\[sit=" ns1/named_dump.db > /dev/null|| ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:exit status: $status"
[ $status -eq 0 ] || exit 1
