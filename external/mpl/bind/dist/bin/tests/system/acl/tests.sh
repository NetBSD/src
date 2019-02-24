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

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0
t=0

echo_i "testing basic ACL processing"
# key "one" should fail
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 10.53.0.1 axfr -y one:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo_i "test $t failed" ; status=1; }


# any other key should be fine
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 10.53.0.1 axfr -y two:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo_i "test $t failed" ; status=1; }

copy_setports ns2/named2.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2
sleep 5

# prefix 10/8 should fail
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 10.53.0.1 axfr -y one:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo_i "test $t failed" ; status=1; }

# any other address should work, as long as it sends key "one"
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 127.0.0.1 axfr -y two:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo_i "test $t failed" ; status=1; }

t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 127.0.0.1 axfr -y one:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo_i "test $t failed" ; status=1; }

echo_i "testing nested ACL processing"
# all combinations of 10.53.0.{1|2} with key {one|two}, should succeed
copy_setports ns2/named3.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2
sleep 5

# should succeed
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 10.53.0.2 axfr -y two:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo_i "test $t failed" ; status=1; }

# should succeed
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 10.53.0.2 axfr -y one:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo_i "test $t failed" ; status=1; }

# should succeed
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 10.53.0.1 axfr -y two:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo_i "test $t failed" ; status=1; }

# should succeed
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 10.53.0.1 axfr -y two:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo_i "test $t failed" ; status=1; }

# but only one or the other should fail
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 127.0.0.1 axfr -y one:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo_i "test $t failed" ; status=1; }

t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 10.53.0.2 axfr > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo_i "test $tt failed" ; status=1; }

# and other values? right out
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 127.0.0.1 axfr -y three:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo_i "test $t failed" ; status=1; }

# now we only allow 10.53.0.1 *and* key one, or 10.53.0.2 *and* key two
copy_setports ns2/named4.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2
sleep 5

# should succeed
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 10.53.0.2 axfr -y two:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo_i "test $t failed" ; status=1; }

# should succeed
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 10.53.0.1 axfr -y one:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 && { echo_i "test $t failed" ; status=1; }

# should fail
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 10.53.0.2 axfr -y one:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo_i "test $t failed" ; status=1; }

# should fail
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 10.53.0.1 axfr -y two:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo_i "test $t failed" ; status=1; }

# should fail
t=`expr $t + 1`
$DIG $DIGOPTS tsigzone. \
	@10.53.0.2 -b 10.53.0.3 axfr -y one:1234abcd8765 > dig.out.${t}
grep "^;" dig.out.${t} > /dev/null 2>&1 || { echo_i "test $t failed" ; status=1; }

echo_i "testing allow-query-on ACL processing"
copy_setports ns2/named5.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2
sleep 5
t=`expr $t + 1`
$DIG -p ${PORT} +tcp soa example. \
	@10.53.0.2 -b 10.53.0.3 > dig.out.${t}
grep "status: NOERROR" dig.out.${t} > /dev/null 2>&1 || { echo_i "test $t failed" ; status=1; }

# AXFR tests against ns3

echo_i "testing allow-transfer ACLs against ns3 (no existing zones)"

echo_i "calling addzone example.com on ns3"
$RNDCCMD 10.53.0.3 addzone 'example.com {type master; file "example.db"; }; '
sleep 1

t=`expr $t + 1`
ret=0
echo_i "checking AXFR of example.com from ns3 with ACL allow-transfer { none; }; (${t})"
$DIG -p ${PORT} @10.53.0.3 example.com axfr > dig.out.${t} 2>&1
grep "Transfer failed." dig.out.${t} >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "calling rndc reconfig"
rndc_reconfig ns3 10.53.0.3

sleep 1

t=`expr $t + 1`
ret=0
echo_i "re-checking AXFR of example.com from ns3 with ACL allow-transfer { none; }; (${t})"
$DIG -p ${PORT} @10.53.0.3 example.com axfr > dig.out.${t} 2>&1
grep "Transfer failed." dig.out.${t} >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

# AXFR tests against ns4

echo_i "testing allow-transfer ACLs against ns4 (1 pre-existing zone)"

echo_i "calling addzone example.com on ns4"
$RNDCCMD 10.53.0.4 addzone 'example.com {type master; file "example.db"; }; '
sleep 1

t=`expr $t + 1`
ret=0
echo_i "checking AXFR of example.com from ns4 with ACL allow-transfer { none; }; (${t})"
$DIG -p ${PORT} @10.53.0.4 example.com axfr > dig.out.${t} 2>&1
grep "Transfer failed." dig.out.${t} >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "calling rndc reconfig"
rndc_reconfig ns4 10.53.0.4

sleep 1

t=`expr $t + 1`
ret=0
echo_i "re-checking AXFR of example.com from ns4 with ACL allow-transfer { none; }; (${t})"
$DIG -p ${PORT} @10.53.0.4 example.com axfr > dig.out.${t} 2>&1
grep "Transfer failed." dig.out.${t} >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
