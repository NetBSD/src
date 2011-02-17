#!/bin/sh
#
# Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
# Copyright (C) 2000, 2001  Internet Software Consortium.
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

# Id: tests.sh,v 1.31 2007-06-19 23:47:07 tbox Exp

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd"

status=0

echo "I:testing basic zone transfer functionality"
$DIG $DIGOPTS example. \
	@10.53.0.2 axfr -p 5300 > dig.out.ns2 || status=1
grep ";" dig.out.ns2

#
# Spin to allow the zone to tranfer.
#
for i in 1 2 3 4 5
do
tmp=0
$DIG $DIGOPTS example. \
	@10.53.0.3 axfr -p 5300 > dig.out.ns3 || tmp=1
	grep ";" dig.out.ns3 > /dev/null
	if test $? -ne 0 ; then break; fi
	echo "I: plain zone re-transfer"
	sleep 5
done
if test $tmp -eq 1 ; then status=1; fi
grep ";" dig.out.ns3

$PERL ../digcomp.pl dig1.good dig.out.ns2 || status=1

$PERL ../digcomp.pl dig1.good dig.out.ns3 || status=1

echo "I:testing TSIG signed zone transfers"
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 axfr -y tsigzone.:1234abcd8765 -p 5300 \
	> dig.out.ns2 || status=1
grep ";" dig.out.ns2

#
# Spin to allow the zone to tranfer.
#
for i in 1 2 3 4 5
do
tmp=0
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.3 axfr -y tsigzone.:1234abcd8765 -p 5300 \
	> dig.out.ns3 || tmp=1
	grep ";" dig.out.ns3 > /dev/null
	if test $? -ne 0 ; then break; fi
	echo "I: plain zone re-transfer"
	sleep 5
done
if test $tmp -eq 1 ; then status=1; fi
grep ";" dig.out.ns3

$PERL ../digcomp.pl dig.out.ns2 dig.out.ns3 || status=1

echo "I:testing ixfr-from-differences"

$PERL -i -p -e '
	s/0\.0\.0\.0/0.0.0.1/;
	s/1397051952/1397051953/
' ns2/example.db

$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'

sleep 5

$RNDC -c ../common/rndc.conf -s 10.53.0.3 -p 9953 reload 2>&1 | sed 's/^/I:ns3 /'

sleep 5

$DIG $DIGOPTS example. \
	@10.53.0.3 axfr -p 5300 > dig.out.ns3 || status=1
grep ";" dig.out.ns3

$PERL ../digcomp.pl dig2.good dig.out.ns3 || status=1

# ns3 has a journal iff it received an IXFR.
test -f ns3/example.bk.jnl || status=1 

echo "I:exit status: $status"
exit $status
