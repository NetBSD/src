#!/bin/sh
#
# Copyright (C) 2008, 2012-2014, 2016, 2017  Internet Systems Consortium, Inc. ("ISC")
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

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd"

status=0
t=0

echo "I:testing basic ACL processing"
# key "one" should fail
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 10.53.0.1 axfr -y one:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo "I:test $t failed" ; status=1; }


# any other key should be fine
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 10.53.0.1 axfr -y two:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo "I:test $t failed" ; status=1; }

cp -f ns2/named2.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 5

# prefix 10/8 should fail
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 10.53.0.1 axfr -y one:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo "I:test $t failed" ; status=1; }

# any other address should work, as long as it sends key "one"
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 127.0.0.1 axfr -y two:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo "I:test $t failed" ; status=1; }

t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 127.0.0.1 axfr -y one:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo "I:test $t failed" ; status=1; }

echo "I:testing nested ACL processing"
# all combinations of 10.53.0.{1|2} with key {one|two}, should succeed
cp -f ns2/named3.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 5

# should succeed
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 10.53.0.2 axfr -y two:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo "I:test $t failed" ; status=1; }

# should succeed
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 10.53.0.2 axfr -y one:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo "I:test $t failed" ; status=1; }

# should succeed
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 10.53.0.1 axfr -y two:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo "I:test $t failed" ; status=1; }

# should succeed
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 10.53.0.1 axfr -y two:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo "I:test $t failed" ; status=1; }

# but only one or the other should fail
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 127.0.0.1 axfr -y one:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo "I:test $t failed" ; status=1; }

t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 10.53.0.2 axfr -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo "I:test $tt failed" ; status=1; }

# and other values? right out
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 127.0.0.1 axfr -y three:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo "I:test $t failed" ; status=1; }

# now we only allow 10.53.0.1 *and* key one, or 10.53.0.2 *and* key two
cp -f ns2/named4.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 5

# should succeed
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 10.53.0.2 axfr -y two:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo "I:test $t failed" ; status=1; }

# should succeed
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 10.53.0.1 axfr -y one:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo "I:test $t failed" ; status=1; }

# should fail
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 10.53.0.2 axfr -y one:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo "I:test $t failed" ; status=1; }

# should fail
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 10.53.0.1 axfr -y two:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo "I:test $t failed" ; status=1; }

# should fail
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
    	@10.53.0.2 -b 10.53.0.3 axfr -y one:1234abcd8765 -p 5300 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo "I:test $t failed" ; status=1; }

echo "I:testing allow-query-on ACL processing"
cp -f ns2/named5.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 5
t=`expr $t + 1`
$DIG +tcp soa example. \
    	@10.53.0.2 -b 10.53.0.3 -p 5300 > dig.out.${t}
grep "status: NOERROR" dig.out.${t} > /dev/null 2>&1 || { echo "I:test $t failed" ; status=1; }

# AXFR tests against ns3

echo "I:testing allow-transfer ACLs against ns3 (no existing zones)"

echo "I:calling addzone example.com on ns3"
$RNDC -c ../common/rndc.conf -s 10.53.0.3 -p 9953 addzone 'example.com {type master; file "example.db"; }; '

sleep 1

t=`expr $t + 1`
ret=0
echo "I:checking AXFR of example.com from ns3 with ACL allow-transfer { none; }; (${t})"
$DIG @10.53.0.3 -p 5300 example.com axfr > dig.out.${t} 2>&1
grep "Transfer failed." dig.out.${t} >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:calling rndc reconfig"
$RNDC -c ../common/rndc.conf -s 10.53.0.3 -p 9953 reconfig

sleep 1

t=`expr $t + 1`
ret=0
echo "I:re-checking AXFR of example.com from ns3 with ACL allow-transfer { none; }; (${t})"
$DIG @10.53.0.3 -p 5300 example.com axfr > dig.out.${t} 2>&1
grep "Transfer failed." dig.out.${t} >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

# AXFR tests against ns4

echo "I:testing allow-transfer ACLs against ns4 (1 pre-existing zone)"

echo "I:calling addzone example.com on ns4"
$RNDC -c ../common/rndc.conf -s 10.53.0.4 -p 9953 addzone 'example.com {type master; file "example.db"; }; '

sleep 1

t=`expr $t + 1`
ret=0
echo "I:checking AXFR of example.com from ns4 with ACL allow-transfer { none; }; (${t})"
$DIG @10.53.0.4 -p 5300 example.com axfr > dig.out.${t} 2>&1
grep "Transfer failed." dig.out.${t} >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:calling rndc reconfig"
$RNDC -c ../common/rndc.conf -s 10.53.0.4 -p 9953 reconfig

sleep 1

t=`expr $t + 1`
ret=0
echo "I:re-checking AXFR of example.com from ns4 with ACL allow-transfer { none; }; (${t})"
$DIG @10.53.0.4 -p 5300 example.com axfr > dig.out.${t} 2>&1
grep "Transfer failed." dig.out.${t} >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:exit status: $status"
[ $status -eq 0 ] || exit 1
