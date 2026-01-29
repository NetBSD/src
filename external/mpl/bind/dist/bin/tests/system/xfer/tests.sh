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
NS_PARAMS="-m record -c named.conf -d 99 -g -T maxcachesize=2097152"

dig_with_opts() (
  "$DIG" -p "$PORT" "$@"
)

status=0
n=0

n=$((n + 1))
echo_i "testing basic zone transfer functionality (from primary) ($n)"
tmp=0
$DIG $DIGOPTS example. @10.53.0.2 axfr >dig.out.ns2.test$n || tmp=1
grep "^;" dig.out.ns2.test$n | cat_i
digcomp dig1.good dig.out.ns2.test$n || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

n=$((n + 1))
echo_i "testing basic zone transfer functionality (from secondary) ($n)"
tmp=0
#
# Spin to allow the zone to transfer.
#
wait_for_xfer() {
  ZONE=$1
  SERVER=$2
  $DIG $DIGOPTS $ZONE @$SERVER axfr >dig.out.test$n || return 1
  grep "^;" dig.out.test$n >/dev/null && return 1
  return 0
}
retry_quiet 25 wait_for_xfer example. 10.53.0.3 || tmp=1
grep "^;" dig.out.test$n | cat_i
digcomp dig1.good dig.out.test$n || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

n=$((n + 1))
echo_i "testing zone transfer functionality (fallback to DNS after DoT failed) ($n)"
tmp=0
retry_quiet 25 wait_for_xfer dot-fallback. 10.53.0.2 || tmp=1
grep "^;" dig.out.test$n | cat_i
digcomp dig3.good dig.out.test$n || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

n=$((n + 1))
echo_i "testing TSIG signed zone transfers ($n)"
tmp=0
$DIG $DIGOPTS tsigzone. @10.53.0.2 axfr -y "${DEFAULT_HMAC}:tsigzone.:1234abcd8765" >dig.out.ns2.test$n || tmp=1
grep "^;" dig.out.ns2.test$n | cat_i

#
# Spin to allow the zone to transfer.
#
wait_for_xfer_tsig() {
  $DIG $DIGOPTS tsigzone. @10.53.0.3 axfr -y "${DEFAULT_HMAC}:tsigzone.:1234abcd8765" >dig.out.ns3.test$n || return 1
  grep "^;" dig.out.ns3.test$n >/dev/null && return 1
  return 0
}
retry_quiet 25 wait_for_xfer_tsig || tmp=1
grep "^;" dig.out.ns3.test$n | cat_i
digcomp dig.out.ns2.test$n dig.out.ns3.test$n || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

echo_i "reload servers for in preparation for ixfr-from-differences tests"

rndc_reload ns1 10.53.0.1
rndc_reload ns2 10.53.0.2
rndc_reload ns3 10.53.0.3
rndc_reload ns6 10.53.0.6
rndc_reload ns7 10.53.0.7

sleep 2

echo_i "updating primary zones for ixfr-from-differences tests"

$PERL -i -p -e '
	s/0\.0\.0\.0/0.0.0.1/;
	s/1397051952/1397051953/
' ns1/sec.db

rndc_reload ns1 10.53.0.1

$PERL -i -p -e '
	s/0\.0\.0\.0/0.0.0.1/;
	s/1397051952/1397051953/
' ns2/example.db

rndc_reload ns2 10.53.0.2

$PERL -i -p -e '
	s/0\.0\.0\.0/0.0.0.1/;
	s/1397051952/1397051953/
' ns6/primary.db

rndc_reload ns6 10.53.0.6

$PERL -i -p -e '
	s/0\.0\.0\.0/0.0.0.1/;
	s/1397051952/1397051953/
' ns7/primary2.db

rndc_reload ns7 10.53.0.7

sleep 3

n=$((n + 1))
echo_i "testing zone is dumped after successful transfer ($n)"
tmp=0
$DIG $DIGOPTS +noall +answer +multi @10.53.0.2 \
  secondary. soa >dig.out.ns2.test$n || tmp=1
grep "1397051952 ; serial" dig.out.ns2.test$n >/dev/null 2>&1 || tmp=1
grep "1397051952 ; serial" ns2/sec.db >/dev/null 2>&1 || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

n=$((n + 1))
echo_i "testing ixfr-from-differences yes; ($n)"
tmp=0

echo_i "wait for reloads..."
wait_for_reloads() (
  $DIG $DIGOPTS @10.53.0.6 +noall +answer soa primary >dig.out.soa1.ns6.test$n
  grep "1397051953" dig.out.soa1.ns6.test$n >/dev/null || return 1
  $DIG $DIGOPTS @10.53.0.1 +noall +answer soa secondary >dig.out.soa2.ns1.test$n
  grep "1397051953" dig.out.soa2.ns1.test$n >/dev/null || return 1
  $DIG $DIGOPTS @10.53.0.2 +noall +answer soa example >dig.out.soa3.ns2.test$n
  grep "1397051953" dig.out.soa3.ns2.test$n >/dev/null || return 1
  return 0
)
retry_quiet 20 wait_for_reloads || tmp=1

echo_i "wait for transfers..."
wait_for_transfers() (
  a=0 b=0 c=0 d=0
  $DIG $DIGOPTS @10.53.0.3 +noall +answer soa example >dig.out.soa1.ns3.test$n
  grep "1397051953" dig.out.soa1.ns3.test$n >/dev/null && a=1
  $DIG $DIGOPTS @10.53.0.3 +noall +answer soa primary >dig.out.soa2.ns3.test$n
  grep "1397051953" dig.out.soa2.ns3.test$n >/dev/null && b=1
  $DIG $DIGOPTS @10.53.0.6 +noall +answer soa secondary >dig.out.soa3.ns6.test$n
  grep "1397051953" dig.out.soa3.ns6.test$n >/dev/null && c=1
  [ $a -eq 1 -a $b -eq 1 -a $c -eq 1 ] && return 0

  # re-notify if necessary
  $RNDCCMD 10.53.0.6 notify primary 2>&1 | sed 's/^/ns6 /' | cat_i
  $RNDCCMD 10.53.0.1 notify secondary 2>&1 | sed 's/^/ns1 /' | cat_i
  $RNDCCMD 10.53.0.2 notify example 2>&1 | sed 's/^/ns2 /' | cat_i
  return 1
)
retry_quiet 20 wait_for_transfers || tmp=1

$DIG $DIGOPTS example. \
  @10.53.0.3 axfr >dig.out.ns3.test$n || tmp=1
grep "^;" dig.out.ns3.test$n | cat_i

digcomp dig2.good dig.out.ns3.test$n || tmp=1

# ns3 has a journal iff it received an IXFR.
test -f ns3/example.bk || tmp=1
test -f ns3/example.bk.jnl || tmp=1

if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

n=$((n + 1))
echo_i "testing ixfr-from-differences primary; (primary zone) ($n)"
tmp=0

$DIG $DIGOPTS primary. \
  @10.53.0.6 axfr >dig.out.ns6.test$n || tmp=1
grep "^;" dig.out.ns6.test$n | cat_i

$DIG $DIGOPTS primary. \
  @10.53.0.3 axfr >dig.out.ns3.test$n || tmp=1
grep "^;" dig.out.ns3.test$n >/dev/null && cat_i <dig.out.ns3.test$n

digcomp dig.out.ns6.test$n dig.out.ns3.test$n || tmp=1

# ns3 has a journal iff it received an IXFR.
test -f ns3/primary.bk || tmp=1
test -f ns3/primary.bk.jnl || tmp=1

if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

n=$((n + 1))
echo_i "testing ixfr-from-differences primary; (secondary zone) ($n)"
tmp=0

$DIG $DIGOPTS secondary. \
  @10.53.0.6 axfr >dig.out.ns6.test$n || tmp=1
grep "^;" dig.out.ns6.test$n | cat_i

$DIG $DIGOPTS secondary. \
  @10.53.0.1 axfr >dig.out.ns1.test$n || tmp=1
grep "^;" dig.out.ns1.test$n | cat_i

digcomp dig.out.ns6.test$n dig.out.ns1.test$n || tmp=1

# ns6 has a journal iff it received an IXFR.
test -f ns6/sec.bk || tmp=1
test -f ns6/sec.bk.jnl && tmp=1

if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

n=$((n + 1))
echo_i "testing ixfr-from-differences secondary; (secondary zone) ($n)"
tmp=0

# ns7 has a journal iff it generates an IXFR.
test -f ns7/primary2.db || tmp=1
test -f ns7/primary2.db.jnl && tmp=1

if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

n=$((n + 1))
echo_i "testing ixfr-from-differences secondary; (secondary zone) ($n)"
tmp=0

$DIG $DIGOPTS secondary. \
  @10.53.0.1 axfr >dig.out.ns1.test$n || tmp=1
grep "^;" dig.out.ns1.test$n | cat_i

$DIG $DIGOPTS secondary. \
  @10.53.0.7 axfr >dig.out.ns7.test$n || tmp=1
grep "^;" dig.out.ns7.test$n | cat_i

digcomp dig.out.ns7.test$n dig.out.ns1.test$n || tmp=1

# ns7 has a journal iff it generates an IXFR.
test -f ns7/sec.bk || tmp=1
test -f ns7/sec.bk.jnl || tmp=1

if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

n=$((n + 1))
echo_i "check that a multi-message uncompressable zone transfers ($n)"
$DIG axfr . -p ${PORT} @10.53.0.4 | grep SOA >axfr.out || tmp=1
if test $(wc -l <axfr.out) != 2; then
  echo_i "failed"
  status=$((status + 1))
fi

# now we test transfers with assorted TSIG glitches
DIGCMD="$DIG $DIGOPTS @10.53.0.4"

sendcmd() {
  send 10.53.0.5 "$EXTRAPORT1"
}

echo_i "testing that incorrectly signed transfers will fail..."
n=$((n + 1))
echo_i "initial correctly-signed transfer should succeed ($n)"

sendcmd <ans5/goodaxfr

# Initially, ns4 is not authoritative for anything.
# Now that ans is up and running with the right data, we make ns4
# a secondary for nil.

cat <<EOF >>ns4/named.conf
zone "nil" {
	type secondary;
	file "nil.db";
	primaries { 10.53.0.5 key tsig_key; };
};
EOF

nextpart ns4/named.run >/dev/null

rndc_reload ns4 10.53.0.4

wait_for_soa() (
  $DIGCMD nil. SOA >dig.out.ns4.test$n
  grep SOA dig.out.ns4.test$n >/dev/null
)
retry_quiet 10 wait_for_soa

nextpart ns4/named.run | grep "Transfer status: success" >/dev/null || {
  echo_i "failed: expected status was not logged"
  status=$((status + 1))
}

$DIGCMD nil. TXT | grep 'initial AXFR' >/dev/null || {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "handle IXFR NOTIMP ($n)"

sendcmd <ans5/ixfrnotimp

$RNDCCMD 10.53.0.4 refresh nil | sed 's/^/ns4 /' | cat_i

sleep 2

nextpart ns4/named.run | grep "zone nil/IN: requesting IXFR from 10.53.0.5" >/dev/null || {
  echo_i "failed: expected status was not logged"
  status=$((status + 1))
}

$DIGCMD nil. TXT | grep 'IXFR NOTIMP' >/dev/null || {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "unsigned transfer ($n)"

sendcmd <ans5/unsigned
sleep 1

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

nextpart ns4/named.run | grep "Transfer status: expected a TSIG or SIG(0)" >/dev/null || {
  echo_i "failed: expected status was not logged"
  status=$((status + 1))
}

$DIGCMD nil. TXT | grep 'unsigned AXFR' >/dev/null && {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "bad keydata ($n)"

sendcmd <ans5/badkeydata

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

nextpart ns4/named.run | grep "Transfer status: tsig verify failure" >/dev/null || {
  echo_i "failed: expected status was not logged"
  status=$((status + 1))
}

$DIGCMD nil. TXT | grep 'bad keydata AXFR' >/dev/null && {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "partially-signed transfer ($n)"

sendcmd <ans5/partial

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

nextpart ns4/named.run | grep "Transfer status: expected a TSIG or SIG(0)" >/dev/null || {
  echo_i "failed: expected status was not logged"
  status=$((status + 1))
}

$DIGCMD nil. TXT | grep 'partially signed AXFR' >/dev/null && {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "unknown key ($n)"

sendcmd <ans5/unknownkey

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

nextpart ns4/named.run | grep "tsig key 'tsig_key': key name and algorithm do not match" >/dev/null || {
  echo_i "failed: expected status was not logged"
  status=$((status + 1))
}

$DIGCMD nil. TXT | grep 'unknown key AXFR' >/dev/null && {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "incorrect key ($n)"

sendcmd <ans5/wrongkey

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

nextpart ns4/named.run | grep "tsig key 'tsig_key': key name and algorithm do not match" >/dev/null || {
  echo_i "failed: expected status was not logged"
  status=$((status + 1))
}

$DIGCMD nil. TXT | grep 'incorrect key AXFR' >/dev/null && {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "bad question section ($n)"

sendcmd <ans5/wrongname

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

nextpart ns4/named.run | grep "question name mismatch" >/dev/null || {
  echo_i "failed: expected status was not logged"
  status=$((status + 1))
}

$DIGCMD nil. TXT | grep 'wrong question AXFR' >/dev/null && {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "bad message id ($n)"

sendcmd <ans5/badmessageid

# Uncomment to see AXFR stream with mismatching IDs.
# $DIG $DIGOPTS @10.53.0.5 -y "${DEFAULT_HMAC}:tsig_key:LSAnCU+Z" nil. AXFR +all

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

nextpart ns4/named.run | grep "Transfer status: unexpected error" >/dev/null || {
  echo_i "failed: expected status was not logged"
  status=$((status + 1))
}

$DIGCMD nil. TXT | grep 'bad message id' >/dev/null && {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "mismatched SOA ($n)"

sendcmd <ans5/soamismatch

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

nextpart ns4/named.run | grep "Transfer status: FORMERR" >/dev/null || {
  echo_i "failed: expected status was not logged"
  status=$((status + 1))
}

$DIGCMD nil. TXT | grep 'SOA mismatch AXFR' >/dev/null && {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "handle EDNS NOTIMP ($n)"

$RNDCCMD 10.53.0.4 null testing EDNS NOTIMP | sed 's/^/ns4 /' | cat_i

sendcmd <ans5/ednsnotimp

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

nextpart ns4/named.run | grep "Transfer status: NOTIMP" >/dev/null || {
  echo_i "failed: expected status was not logged"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "handle EDNS FORMERR ($n)"

$RNDCCMD 10.53.0.4 null testing EDNS FORMERR | sed 's/^/ns4 /' | cat_i

sendcmd <ans5/ednsformerr

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 10

$DIGCMD nil. TXT | grep 'EDNS FORMERR' >/dev/null || {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "check that we ask for and got a EDNS EXPIRE response when transfering from a secondary ($n)"
tmp=0
msg="zone edns-expire/IN: zone transfer finished: success, expire=1814[0-4][0-9][0-9]"
grep "$msg" ns7/named.run >/dev/null || tmp=1
[ "$tmp" -ne 0 ] && echo_i "failed"
status=$((status + tmp))

n=$((n + 1))
echo_i "check that we ask for and get a EDNS EXPIRE response when refreshing ($n)"
# force a refresh query
$RNDCCMD 10.53.0.7 refresh edns-expire 2>&1 | sed 's/^/ns7 /' | cat_i
sleep 10

# there may be multiple log entries so get the last one.
expire=$(awk '/edns-expire\/IN: got EDNS EXPIRE of/ { x=$9 } END { print x }' ns7/named.run)
test ${expire:-0} -gt 0 -a ${expire:-0} -lt 1814400 || {
  echo_i "failed (expire=${expire:-0})"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "test smaller transfer TCP message size ($n)"
$DIG $DIGOPTS example. @10.53.0.8 axfr \
  -y "${DEFAULT_HMAC}:key1.:1234abcd8765" >dig.out.msgsize.test$n || status=1

bytes=$(wc -c <dig.out.msgsize.test$n)
if [ $bytes -ne 459357 ]; then
  echo_i "failed axfr size check"
  status=$((status + 1))
fi

num_messages=$(cat ns8/named.run | grep "sending TCP message of" | wc -l)
if [ $num_messages -le 300 ]; then
  echo_i "failed transfer message count check"
  status=$((status + 1))
fi

n=$((n + 1))
echo_i "test mapped zone with out of zone data ($n)"
tmp=0
$DIG -p ${PORT} txt mapped @10.53.0.3 >dig.out.1.test$n
grep "status: NOERROR," dig.out.1.test$n >/dev/null || tmp=1
stop_server ns3
start_server --noclean --restart --port ${PORT} ns3
check_mapped() {
  $DIG -p ${PORT} txt mapped @10.53.0.3 >dig.out.2.test$n
  grep "status: NOERROR," dig.out.2.test$n >/dev/null || return 1
  $DIG -p ${PORT} axfr mapped @10.53.0.3 >dig.out.3.test$n
  digcomp knowngood.mapped dig.out.3.test$n || return 1
  return 0
}
retry_quiet 10 check_mapped || tmp=1
[ "$tmp" -ne 0 ] && echo_i "failed"
status=$((status + tmp))

n=$((n + 1))
echo_i "test that a zone with too many records is rejected (AXFR) ($n)"
tmp=0
grep "'axfr-too-big/IN'.*: too many records" ns6/named.run >/dev/null || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

n=$((n + 1))
echo_i "test that a zone with too many records is rejected (IXFR) ($n)"
tmp=0
nextpart ns6/named.run >/dev/null
$NSUPDATE <<EOF
zone ixfr-too-big
server 10.53.0.1 ${PORT}
update add the-31st-record.ixfr-too-big 0 TXT this is it
send
EOF
msg="'ixfr-too-big/IN' from 10.53.0.1#${PORT}: Transfer status: too many records"
wait_for_log 10 "$msg" ns6/named.run || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

n=$((n + 1))
echo_i "checking whether dig calculates AXFR statistics correctly ($n)"
tmp=0
# Loop until the secondary server manages to transfer the "xfer-stats" zone so
# that we can both check dig output and immediately proceed with the next test.
# Use -b so that we can discern between incoming and outgoing transfers in ns3
# logs later on.
wait_for_xfer() (
  $DIG $DIGOPTS +edns +nocookie +noexpire +stat -b 10.53.0.2 @10.53.0.3 xfer-stats. AXFR >dig.out.ns3.test$n
  grep "; Transfer failed" dig.out.ns3.test$n >/dev/null || return 0
  return 1
)
if retry_quiet 10 wait_for_xfer; then
  get_dig_xfer_stats dig.out.ns3.test$n >stats.dig
  diff axfr-stats.good stats.dig || tmp=1
else
  echo_i "timed out waiting for zone transfer"
fi
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

# Note: in the next two tests, we use ns3 logs for checking both incoming and
# outgoing transfer statistics as ns3 is both a secondary server (for ns1) and a
# primary server (for dig queries from the previous test) for "xfer-stats".
n=$((n + 1))
echo_i "checking whether named calculates incoming AXFR statistics correctly ($n)"
tmp=0
get_named_xfer_stats ns3/named.run 10.53.0.1 xfer-stats "Transfer completed" >stats.incoming
diff axfr-stats.good stats.incoming || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

n=$((n + 1))
echo_i "checking whether named calculates outgoing AXFR statistics correctly ($n)"
tmp=0
check_xfer_stats() {
  get_named_xfer_stats ns3/named.run 10.53.0.2 xfer-stats "AXFR ended" >stats.outgoing
  diff axfr-stats.good stats.outgoing >/dev/null
}
retry_quiet 10 check_xfer_stats || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

n=$((n + 1))
echo_i "test that transfer-source uses port option correctly ($n)"
tmp=0
grep "10.53.0.3#${EXTRAPORT1} (primary): query 'primary/SOA/IN' approved" ns6/named.run >/dev/null || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

wait_for_message() (
  nextpartpeek ns6/named.run >wait_for_message.$n
  grep -F "$1" wait_for_message.$n >/dev/null
)

nextpart ns6/named.run >/dev/null

n=$((n + 1))
echo_i "test that named tries the next primary in the list when the first one fails (XoT -> Do53) ($n)"
tmp=0
$RNDCCMD 10.53.0.6 retransfer xot-primary-try-next 2>&1 | sed 's/^/ns6 /' | cat_i
msg="'xot-primary-try-next/IN' from 10.53.0.1#${PORT}: Transfer status: success"
retry_quiet 60 wait_for_message "$msg" || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

nextpart ns6/named.run >/dev/null

n=$((n + 1))
echo_i "test that named tries the next primary in the list when the first one is already marked as unreachable (XoT -> Do53) ($n)"
tmp=0
$RNDCCMD 10.53.0.6 retransfer xot-primary-try-next 2>&1 | sed 's/^/ns6 /' | cat_i
msg="'xot-primary-try-next/IN' from 10.53.0.1#${PORT}: Transfer status: success"
retry_quiet 60 wait_for_message "$msg" || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

# Restart ns1 with -T transferslowly
stop_server ns1
cp ns1/named2.conf ns1/named.conf
start_server --noclean --restart --port ${PORT} ns1 -- "-D xfer-ns1 $NS_PARAMS -T transferinsecs -T transferslowly"
sleep 1

nextpart ns6/named.run >/dev/null

n=$((n + 1))
echo_i "test rndc retransfer -force ($n)"
tmp=0
$RNDCCMD 10.53.0.6 retransfer axfr-rndc-retransfer-force 2>&1 | sed 's/^/ns6 /' | cat_i
# Wait for at least one message
msg="'axfr-rndc-retransfer-force/IN' from 10.53.0.1#${PORT}: received"
retry_quiet 5 wait_for_message "$msg" || tmp=1
# Issue a retransfer-force command which should cancel the ongoing transfer and start a new one
$RNDCCMD 10.53.0.6 retransfer -force axfr-rndc-retransfer-force 2>&1 | sed 's/^/ns6 /' | cat_i
msg="'axfr-rndc-retransfer-force/IN' from 10.53.0.1#${PORT}: Transfer status: shutting down"
retry_quiet 5 wait_for_message "$msg" || tmp=1
# Wait for the new transfer to complete successfully
msg="'axfr-rndc-retransfer-force/IN' from 10.53.0.1#${PORT}: Transfer status: success"
retry_quiet 30 wait_for_message "$msg" || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

nextpart ns6/named.run >/dev/null

n=$((n + 1))
echo_i "test min-transfer-rate-in with 5 seconds timeout ($n)"
$RNDCCMD 10.53.0.6 retransfer axfr-min-transfer-rate 2>&1 | sed 's/^/ns6 /' | cat_i
tmp=0
retry_quiet 10 wait_for_message "minimum transfer rate reached: timed out" || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

nextpart ns6/named.run >/dev/null

n=$((n + 1))
echo_i "test max-transfer-time-in with 1 second timeout ($n)"
$RNDCCMD 10.53.0.6 retransfer axfr-max-transfer-time 2>&1 | sed 's/^/ns6 /' | cat_i
tmp=0
retry_quiet 10 wait_for_message "maximum transfer time exceeded: timed out" || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

# Restart ns1 with -T transferstuck
stop_server ns1
cp ns1/named3.conf ns1/named.conf
start_server --noclean --restart --port ${PORT} ns1 -- "-D xfer-ns1 $NS_PARAMS -T transferinsecs -T transferstuck"
sleep 1

nextpart ns6/named.run >/dev/null

n=$((n + 1))
echo_i "test max-transfer-idle-in with 50 seconds timeout ($n)"
start=$(date +%s)
$RNDCCMD 10.53.0.6 retransfer axfr-max-idle-time 2>&1 | sed 's/^/ns6 /' | cat_i
tmp=0
retry_quiet 60 wait_for_message "maximum idle time exceeded: timed out" || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
if [ $tmp -eq 0 ]; then
  now=$(date +%s)
  diff=$((now - start))
  # we expect a timeout in 50 seconds
  test $diff -lt 50 && tmp=1
  test $diff -ge 59 && tmp=1
  if test $tmp != 0; then echo_i "unexpected diff value: ${diff}"; fi
fi
status=$((status + tmp))

nextpart ns6/named.run >/dev/null

sendcmd() (
  dig_with_opts "@${1}" "${2}._control." TXT +time=5 +tries=1 +tcp >/dev/null 2>&1
)

# See #5307#note_558185
n=$((n + 1))
echo_i "test reconfiguration when zone transfer is in the middle of a SOA query (part 1) ($n)"
tmp=0
# Check that xfr-and-reconfig has been successfully transferred by the secondary.
grep -F 'zone xfr-and-reconfig/IN: zone transfer finished: success' ns6/named.run 2>&1 >/dev/null || tmp=0
# Make ans6 receive queries without responding to them.
sendcmd 10.53.0.9 "disable.send-responses"
sleep 1
# Try to reload the zone from an unresponsive primary.
$RNDCCMD 10.53.0.6 reload xfr-and-reconfig 2>&1 | sed 's/^/ns6 /' | cat_i
sleep 1
# Reconfigure named while zone transfer attempt is in progress.
$RNDCCMD 10.53.0.6 reconfig 2>&1 | sed 's/^/ns6 /' | cat_i
# Confirm that the ongoing SOA request was canceled, caused by the reconfiguratoin.
retry_quiet 60 wait_for_message "refresh: request result: operation canceled" || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

nextpart ns6/named.run >/dev/null

n=$((n + 1))
echo_i "test reconfiguration when zone transfer is in the middle of a SOA query (part 2) ($n)"
tmp=0
# Make ans6 receive queries and respond to them.
sendcmd 10.53.0.9 "enable.send-responses"
sleep 1
# Try to reload the zone from the primary.
$RNDCCMD 10.53.0.6 reload xfr-and-reconfig 2>&1 | sed 's/^/ns6 /' | cat_i
retry_quiet 60 wait_for_message "zone xfr-and-reconfig/IN: Transfer started." || tmp=1
if test $tmp != 0; then echo_i "failed"; fi
status=$((status + tmp))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
