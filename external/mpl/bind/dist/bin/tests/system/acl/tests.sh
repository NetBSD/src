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

set -e

. ../conf.sh

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"
RNDCCMD="$RNDC -c ../_common/rndc.conf -p ${CONTROLPORT} -s"

status=0
t=0

echo_i "testing basic ACL processing"
# key "one" should fail
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 10.53.0.1 axfr -y "${DEFAULT_HMAC}:one:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 || {
  echo_i "test $t failed"
  status=1
}

# any other key should be fine
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 10.53.0.1 axfr -y "${DEFAULT_HMAC}:two:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 && {
  echo_i "test $t failed"
  status=1
}

cp ns2/named2.conf ns2/named.conf
rndc_reload ns2 10.53.0.2
sleep 5

# prefix 10/8 should fail
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 10.53.0.1 axfr -y "${DEFAULT_HMAC}:one:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 || {
  echo_i "test $t failed"
  status=1
}

# any other address should work, as long as it sends key "one"
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 127.0.0.1 axfr -y "${DEFAULT_HMAC}:two:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 || {
  echo_i "test $t failed"
  status=1
}

t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 127.0.0.1 axfr -y "${DEFAULT_HMAC}:one:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 && {
  echo_i "test $t failed"
  status=1
}

echo_i "testing nested ACL processing"
# all combinations of 10.53.0.{1|2} with key {one|two}, should succeed
cp ns2/named3.conf ns2/named.conf
rndc_reload ns2 10.53.0.2
sleep 5

# should succeed
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 10.53.0.2 axfr -y "${DEFAULT_HMAC}:two:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 && {
  echo_i "test $t failed"
  status=1
}

# should succeed
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 10.53.0.2 axfr -y "${DEFAULT_HMAC}:one:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 && {
  echo_i "test $t failed"
  status=1
}

# should succeed
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 10.53.0.1 axfr -y "${DEFAULT_HMAC}:two:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 && {
  echo_i "test $t failed"
  status=1
}

# should succeed
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 10.53.0.1 axfr -y "${DEFAULT_HMAC}:two:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 && {
  echo_i "test $t failed"
  status=1
}

# but only one or the other should fail
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 127.0.0.1 axfr -y "${DEFAULT_HMAC}:one:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 || {
  echo_i "test $t failed"
  status=1
}

t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 10.53.0.2 axfr >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 || {
  echo_i "test $tt failed"
  status=1
}

# and other values? right out
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 127.0.0.1 axfr -y "${DEFAULT_HMAC}:three:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 || {
  echo_i "test $t failed"
  status=1
}

# now we only allow 10.53.0.1 *and* key one, or 10.53.0.2 *and* key two
cp ns2/named4.conf ns2/named.conf
rndc_reload ns2 10.53.0.2
sleep 5

# should succeed
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 10.53.0.2 axfr -y "${DEFAULT_HMAC}:two:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 && {
  echo_i "test $t failed"
  status=1
}

# should succeed
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 10.53.0.1 axfr -y "${DEFAULT_HMAC}:one:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 && {
  echo_i "test $t failed"
  status=1
}

# should fail
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 10.53.0.2 axfr -y "${DEFAULT_HMAC}:one:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 || {
  echo_i "test $t failed"
  status=1
}

# should fail
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 10.53.0.1 axfr -y "${DEFAULT_HMAC}:two:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 || {
  echo_i "test $t failed"
  status=1
}

# should fail
t=$((t + 1))
$DIG $DIGOPTS tsigzone. \
  @10.53.0.2 -b 10.53.0.3 axfr -y "${DEFAULT_HMAC}:one:1234abcd8765" >dig.out.${t}
grep "^;" dig.out.${t} >/dev/null 2>&1 || {
  echo_i "test $t failed"
  status=1
}

echo_i "testing allow-query-on ACL processing"
cp ns2/named5.conf ns2/named.conf
rndc_reload ns2 10.53.0.2
sleep 5
t=$((t + 1))
$DIG -p ${PORT} +tcp soa example. \
  @10.53.0.2 -b 10.53.0.3 >dig.out.${t}
grep "status: NOERROR" dig.out.${t} >/dev/null 2>&1 || {
  echo_i "test $t failed"
  status=1
}

echo_i "testing blackhole ACL processing"
t=$((t + 1))
ret=0
$DIG -p ${PORT} +tcp soa example. \
  @10.53.0.2 -b 10.53.0.3 >dig.out.1.${t}
grep "status: NOERROR" dig.out.1.${t} >/dev/null 2>&1 || ret=1
$DIG -p ${PORT} +tcp soa example. \
  @10.53.0.2 -b 10.53.0.8 >dig.out.2.${t} && ret=1
grep "status: NOERROR" dig.out.2.${t} >/dev/null 2>&1 && ret=1
grep "communications error" dig.out.2.${t} >/dev/null 2>&1 || ret=1
$DIG -p ${PORT} soa example. \
  @10.53.0.2 -b 10.53.0.3 >dig.out.3.${t}
grep "status: NOERROR" dig.out.3.${t} >/dev/null 2>&1 || ret=1
$DIG -p ${PORT} soa example. \
  @10.53.0.2 -b 10.53.0.8 >dig.out.4.${t} && ret=1
grep "status: NOERROR" dig.out.4.${t} >/dev/null 2>&1 && ret=1
grep "timed out" dig.out.4.${t} >/dev/null 2>&1 || ret=1
grep ";; no servers could be reached" dig.out.4.${t} >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

# AXFR tests against ns3

echo_i "testing allow-transfer ACLs against ns3 (no existing zones)"

echo_i "calling addzone example.com on ns3"
$RNDCCMD 10.53.0.3 addzone 'example.com {type primary; file "example.db"; }; '
sleep 1

t=$((t + 1))
ret=0
echo_i "checking AXFR of example.com from ns3 with ACL allow-transfer { none; }; (${t})"
$DIG -p ${PORT} @10.53.0.3 example.com axfr >dig.out.${t} 2>&1
grep "Transfer failed." dig.out.${t} >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

echo_i "calling rndc reconfig"
rndc_reconfig ns3 10.53.0.3

sleep 1

t=$((t + 1))
ret=0
echo_i "re-checking AXFR of example.com from ns3 with ACL allow-transfer { none; }; (${t})"
$DIG -p ${PORT} @10.53.0.3 example.com axfr >dig.out.${t} 2>&1
grep "Transfer failed." dig.out.${t} >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

# AXFR tests against ns4

echo_i "testing allow-transfer ACLs against ns4 (1 pre-existing zone)"

echo_i "calling addzone example.com on ns4"
$RNDCCMD 10.53.0.4 addzone 'example.com {type primary; file "example.db"; }; '
sleep 1

t=$((t + 1))
ret=0
echo_i "checking AXFR of example.com from ns4 with ACL allow-transfer { none; }; (${t})"
$DIG -p ${PORT} @10.53.0.4 example.com axfr >dig.out.${t} 2>&1
grep "Transfer failed." dig.out.${t} >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

echo_i "calling rndc reconfig"
rndc_reconfig ns4 10.53.0.4

sleep 1

t=$((t + 1))
ret=0
echo_i "re-checking AXFR of example.com from ns4 with ACL allow-transfer { none; }; (${t})"
$DIG -p ${PORT} @10.53.0.4 example.com axfr >dig.out.${t} 2>&1
grep "Transfer failed." dig.out.${t} >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
