#!/bin/sh
#
# Copyright (C) 2016  Internet Systems Consortium, Inc. ("ISC")
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

RNDCCMD="$RNDC -p 9953 -c ../common/rndc.conf"

status=0

echo "I:initialize counters"
$RNDCCMD -s 10.53.0.1 stats > /dev/null 2>&1
$RNDCCMD -s 10.53.0.2 stats > /dev/null 2>&1
ntcp10=`grep "TCP requests received" ns1/named.stats | tail -1 | awk '{print $1}'`
ntcp20=`grep "TCP requests received" ns2/named.stats | tail -1 | awk '{print $1}'`
#echo ntcp10 ':' "$ntcp10"
#echo ntcp20 ':' "$ntcp20"

echo "I:check TCP transport"
ret=0
$DIG -p 5300 @10.53.0.3 txt.example. > dig.out.3
$RNDCCMD -s 10.53.0.1 stats > /dev/null 2>&1
$RNDCCMD -s 10.53.0.2 stats > /dev/null 2>&1
ntcp11=`grep "TCP requests received" ns1/named.stats | tail -1 | awk '{print $1}'`
ntcp21=`grep "TCP requests received" ns2/named.stats | tail -1 | awk '{print $1}'`
#echo ntcp11 ':' "$ntcp11"
#echo ntcp21 ':' "$ntcp21"
if [ "$ntcp10" -ge "$ntcp11" ]; then ret=1; fi
if [ "$ntcp20" -ne "$ntcp21" ]; then ret=1; fi
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:check TCP forwarder"
ret=0
$DIG -p 5300 @10.53.0.4 txt.example. > dig.out.4
$RNDCCMD -s 10.53.0.1 stats > /dev/null 2>&1
$RNDCCMD -s 10.53.0.2 stats > /dev/null 2>&1
ntcp12=`grep "TCP requests received" ns1/named.stats | tail -1 | awk '{print $1}'`
ntcp22=`grep "TCP requests received" ns2/named.stats | tail -1 | awk '{print $1}'`
#echo ntcp12 ':' "$ntcp12"
#echo ntcp22 ':' "$ntcp22"
if [ "$ntcp11" -ne "$ntcp12" ]; then ret=1; fi
if [ "$ntcp21" -ge "$ntcp22" ];then ret=1; fi
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:exit status: $status"
exit $status
