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

DIGOPTS="-p ${PORT}"
RNDCCMD="$RNDC -p ${CONTROLPORT} -c ../_common/rndc.conf"

status=0

# dnstap_data_ready <fstrm_capture_PID> <capture_file> <min_file_size>
# Flushes capture_file and checks wheter its size is >= min_file_size.
dnstap_data_ready() {
  # Process id of running fstrm_capture.
  fstrm_capture_pid=$1
  # Output file provided to fstrm_capture via -w switch.
  capture_file=$2
  # Minimum expected file size.
  min_size_expected=$3

  kill -HUP $fstrm_capture_pid
  file_size=$(wc -c <"$capture_file" | tr -d ' ')
  if [ $file_size -lt $min_size_expected ]; then
    return 1
  fi
}

check_count() {
  [ $2 -eq $3 ] || {
    echo_i "$1 $2 expected $3"
    ret=1
  }
}

for bad in bad-*.conf; do
  ret=0
  echo_i "checking that named-checkconf detects error in $bad"
  {
    $CHECKCONF $bad >/dev/null 2>&1
    rc=$?
  } || true
  if [ $rc != 1 ]; then
    echo_i "failed"
    ret=1
  fi
  status=$((status + ret))
done

for good in good-*.conf; do
  ret=0
  echo_i "checking that named-checkconf detects no error in $good"
  {
    $CHECKCONF $good >/dev/null 2>&1
    rc=$?
  } || true
  if [ $rc != 0 ]; then
    echo_i "failed"
    ret=1
  fi
  status=$((status + ret))
done

echo_i "wait for servers to finish loading"
ret=0
wait_for_log 20 "all zones loaded" ns1/named.run || ret=1
wait_for_log 20 "all zones loaded" ns2/named.run || ret=1
wait_for_log 20 "all zones loaded" ns3/named.run || ret=1
wait_for_log 20 "all zones loaded" ns4/named.run || ret=1
wait_for_log 20 "all zones loaded" ns5/named.run || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# both the 'a.example/A' lookup and the './NS' lookup to ns1
# need to complete before reopening/rolling for the counts to
# be correct.

echo_i "prime cache"
ret=0
$DIG $DIGOPTS @10.53.0.3 a.example >dig.out || true
wait_for_log 20 "(.): reset client" ns1/named.run || true
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# check three different dnstap reopen/roll methods:
# ns1: dnstap-reopen; ns2: dnstap -reopen; ns3: dnstap -roll
mv ns1/dnstap.out ns1/dnstap.out.save
mv ns2/dnstap.out ns2/dnstap.out.save
mv ns5/dnstap.out ns5/dnstap.out.save

if [ -n "$FSTRM_CAPTURE" ]; then
  ret=0
  echo_i "starting fstrm_capture"
  $FSTRM_CAPTURE -t protobuf:dnstap.Dnstap -u ns4/dnstap.out \
    -w dnstap.out >fstrm_capture.out.1 2>&1 &
  fstrm_capture_pid=$!
  wait_for_log 10 "socket path ns4/dnstap.out" fstrm_capture.out.1 || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

echo_i "reopen/roll capture streams"
ret=0
$RNDCCMD -s 10.53.0.1 dnstap-reopen | sed 's/^/ns1 /' | cat_i
$RNDCCMD -s 10.53.0.2 dnstap -reopen | sed 's/^/ns2 /' | cat_i
$RNDCCMD -s 10.53.0.3 dnstap -roll | sed 's/^/ns3 /' | cat_i
$RNDCCMD -s 10.53.0.4 dnstap -reopen | sed 's/^/ns4 /' | cat_i
$RNDCCMD -s 10.53.0.5 dnstap -reopen | sed 's/^/ns5 /' | cat_i

echo_i "send test traffic"
ret=0
$DIG $DIGOPTS @10.53.0.5 a.example >dig.out || ret=1

# send an UPDATE to ns2
$NSUPDATE <<-EOF
server 10.53.0.2 ${PORT}
zone example
update add b.example 3600 in a 10.10.10.10
send
EOF

# XXX: file output should be flushed once a second according
# to the libfstrm source, but it doesn't seem to happen until
# enough data has accumulated. to get all the output, we stop
# the name servers, forcing a flush on shutdown. it would be
# nice to find a better way to do this.
$RNDCCMD -s 10.53.0.1 stop | sed 's/^/ns1 /' | cat_i
$RNDCCMD -s 10.53.0.2 stop | sed 's/^/ns2 /' | cat_i
$RNDCCMD -s 10.53.0.3 stop | sed 's/^/ns3 /' | cat_i
$RNDCCMD -s 10.53.0.5 stop | sed 's/^/ns5 /' | cat_i

sleep 1

echo_i "checking initial message counts"

udp1=$($DNSTAPREAD ns1/dnstap.out.save | grep "UDP " | wc -l)
tcp1=$($DNSTAPREAD ns1/dnstap.out.save | grep "TCP " | wc -l)
aq1=$($DNSTAPREAD ns1/dnstap.out.save | grep "AQ " | wc -l)
ar1=$($DNSTAPREAD ns1/dnstap.out.save | grep "AR " | wc -l)
cq1=$($DNSTAPREAD ns1/dnstap.out.save | grep "CQ " | wc -l)
cr1=$($DNSTAPREAD ns1/dnstap.out.save | grep "CR " | wc -l)
fq1=$($DNSTAPREAD ns1/dnstap.out.save | grep "FQ " | wc -l)
fr1=$($DNSTAPREAD ns1/dnstap.out.save | grep "FR " | wc -l)
rq1=$($DNSTAPREAD ns1/dnstap.out.save | grep "RQ " | wc -l)
rr1=$($DNSTAPREAD ns1/dnstap.out.save | grep "RR " | wc -l)
uq1=$($DNSTAPREAD ns1/dnstap.out.save | grep "UQ " | wc -l)
ur1=$($DNSTAPREAD ns1/dnstap.out.save | grep "UR " | wc -l)

udp2=$($DNSTAPREAD ns2/dnstap.out.save | grep "UDP " | wc -l)
tcp2=$($DNSTAPREAD ns2/dnstap.out.save | grep "TCP " | wc -l)
aq2=$($DNSTAPREAD ns2/dnstap.out.save | grep "AQ " | wc -l)
ar2=$($DNSTAPREAD ns2/dnstap.out.save | grep "AR " | wc -l)
cq2=$($DNSTAPREAD ns2/dnstap.out.save | grep "CQ " | wc -l)
cr2=$($DNSTAPREAD ns2/dnstap.out.save | grep "CR " | wc -l)
fq2=$($DNSTAPREAD ns2/dnstap.out.save | grep "FQ " | wc -l)
fr2=$($DNSTAPREAD ns2/dnstap.out.save | grep "FR " | wc -l)
rq2=$($DNSTAPREAD ns2/dnstap.out.save | grep "RQ " | wc -l)
rr2=$($DNSTAPREAD ns2/dnstap.out.save | grep "RR " | wc -l)
uq2=$($DNSTAPREAD ns2/dnstap.out.save | grep "UQ " | wc -l)
ur2=$($DNSTAPREAD ns2/dnstap.out.save | grep "UR " | wc -l)

mv ns3/dnstap.out.0 ns3/dnstap.out.save
udp3=$($DNSTAPREAD ns3/dnstap.out.save | grep "UDP " | wc -l)
tcp3=$($DNSTAPREAD ns3/dnstap.out.save | grep "TCP " | wc -l)
aq3=$($DNSTAPREAD ns3/dnstap.out.save | grep "AQ " | wc -l)
ar3=$($DNSTAPREAD ns3/dnstap.out.save | grep "AR " | wc -l)
cq3=$($DNSTAPREAD ns3/dnstap.out.save | grep "CQ " | wc -l)
cr3=$($DNSTAPREAD ns3/dnstap.out.save | grep "CR " | wc -l)
fq3=$($DNSTAPREAD ns3/dnstap.out.save | grep "FQ " | wc -l)
fr3=$($DNSTAPREAD ns3/dnstap.out.save | grep "FR " | wc -l)
rq3=$($DNSTAPREAD ns3/dnstap.out.save | grep "RQ " | wc -l)
rr3=$($DNSTAPREAD ns3/dnstap.out.save | grep "RR " | wc -l)
uq3=$($DNSTAPREAD ns3/dnstap.out.save | grep "UQ " | wc -l)
ur3=$($DNSTAPREAD ns3/dnstap.out.save | grep "UR " | wc -l)

udp5=$($DNSTAPREAD ns5/dnstap.out.save | grep "UDP " | wc -l)
tcp5=$($DNSTAPREAD ns5/dnstap.out.save | grep "TCP " | wc -l)
aq5=$($DNSTAPREAD ns5/dnstap.out.save | grep "AQ " | wc -l)
ar5=$($DNSTAPREAD ns5/dnstap.out.save | grep "AR " | wc -l)
cq5=$($DNSTAPREAD ns5/dnstap.out.save | grep "CQ " | wc -l)
cr5=$($DNSTAPREAD ns5/dnstap.out.save | grep "CR " | wc -l)
fq5=$($DNSTAPREAD ns5/dnstap.out.save | grep "FQ " | wc -l)
fr5=$($DNSTAPREAD ns5/dnstap.out.save | grep "FR " | wc -l)
rq5=$($DNSTAPREAD ns5/dnstap.out.save | grep "RQ " | wc -l)
rr5=$($DNSTAPREAD ns5/dnstap.out.save | grep "RR " | wc -l)
uq5=$($DNSTAPREAD ns5/dnstap.out.save | grep "UQ " | wc -l)
ur5=$($DNSTAPREAD ns5/dnstap.out.save | grep "UR " | wc -l)

echo_i "checking UDP message counts"
ret=0
check_count ns1 $udp1 0
check_count ns2 $udp2 2
check_count ns3 $udp3 4
check_count ns5 $udp5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TCP message counts"
ret=0
check_count ns1 $tcp1 6
check_count ns2 $tcp2 2
check_count ns3 $tcp3 6
check_count ns5 $tcp5 2
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AUTH_QUERY message counts"
ret=0
check_count ns1 $aq1 3
check_count ns2 $aq2 2
check_count ns3 $aq3 1
check_count ns5 $aq5 1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AUTH_RESPONSE message counts"
ret=0
check_count ns1 $ar1 2
check_count ns2 $ar2 1
check_count ns3 $ar3 0
check_count ns5 $ar5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking CLIENT_QUERY message counts"
ret=0
check_count ns1 $cq1 0
check_count ns2 $cq2 0
check_count ns3 $cq3 1
check_count ns5 $cq5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking CLIENT_RESPONSE message counts"
ret=0
check_count ns1 $cr1 1
check_count ns2 $cr2 1
check_count ns3 $cr3 2
check_count ns5 $cr5 1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking RESOLVER_QUERY message counts"
ret=0
check_count ns1 $rq1 0
check_count ns2 $rq2 0
check_count ns3 $rq3 3
check_count ns5 $rq5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking RESOLVER_RESPONSE message counts"
ret=0
check_count ns1 $rr1 0
check_count ns2 $rr2 0
check_count ns3 $rr3 3
check_count ns5 $rr5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking FORWARD_QUERY message counts"
ret=0
check_count ns1 $fq1 0
check_count ns2 $fq2 0
check_count ns3 $fq3 0
check_count ns5 $fq5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking FORWARD_RESPONSE message counts"
ret=0
check_count ns1 $fr1 0
check_count ns2 $fr2 0
check_count ns3 $fr3 0
check_count ns5 $fr5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking UPDATE_QUERY message counts"
ret=0
check_count ns1 $uq1 0
check_count ns2 $uq2 0
check_count ns3 $uq3 0
check_count ns5 $uq5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking UPDATE_RESPONSE message counts"
ret=0
check_count ns1 $ur1 0
check_count ns2 $ur2 0
check_count ns3 $ur3 0
check_count ns5 $ur5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking reopened message counts"

udp1=$($DNSTAPREAD ns1/dnstap.out | grep "UDP " | wc -l)
tcp1=$($DNSTAPREAD ns1/dnstap.out | grep "TCP " | wc -l)
aq1=$($DNSTAPREAD ns1/dnstap.out | grep "AQ " | wc -l)
ar1=$($DNSTAPREAD ns1/dnstap.out | grep "AR " | wc -l)
cq1=$($DNSTAPREAD ns1/dnstap.out | grep "CQ " | wc -l)
cr1=$($DNSTAPREAD ns1/dnstap.out | grep "CR " | wc -l)
fq1=$($DNSTAPREAD ns1/dnstap.out | grep "FQ " | wc -l)
fr1=$($DNSTAPREAD ns1/dnstap.out | grep "FR " | wc -l)
rq1=$($DNSTAPREAD ns1/dnstap.out | grep "RQ " | wc -l)
rr1=$($DNSTAPREAD ns1/dnstap.out | grep "RR " | wc -l)
uq1=$($DNSTAPREAD ns1/dnstap.out | grep "UQ " | wc -l)
ur1=$($DNSTAPREAD ns1/dnstap.out | grep "UR " | wc -l)

udp2=$($DNSTAPREAD ns2/dnstap.out | grep "UDP " | wc -l)
tcp2=$($DNSTAPREAD ns2/dnstap.out | grep "TCP " | wc -l)
aq2=$($DNSTAPREAD ns2/dnstap.out | grep "AQ " | wc -l)
ar2=$($DNSTAPREAD ns2/dnstap.out | grep "AR " | wc -l)
cq2=$($DNSTAPREAD ns2/dnstap.out | grep "CQ " | wc -l)
cr2=$($DNSTAPREAD ns2/dnstap.out | grep "CR " | wc -l)
fq2=$($DNSTAPREAD ns2/dnstap.out | grep "FQ " | wc -l)
fr2=$($DNSTAPREAD ns2/dnstap.out | grep "FR " | wc -l)
rq2=$($DNSTAPREAD ns2/dnstap.out | grep "RQ " | wc -l)
rr2=$($DNSTAPREAD ns2/dnstap.out | grep "RR " | wc -l)
uq2=$($DNSTAPREAD ns2/dnstap.out | grep "UQ " | wc -l)
ur2=$($DNSTAPREAD ns2/dnstap.out | grep "UR " | wc -l)

udp3=$($DNSTAPREAD ns3/dnstap.out | grep "UDP " | wc -l)
tcp3=$($DNSTAPREAD ns3/dnstap.out | grep "TCP " | wc -l)
aq3=$($DNSTAPREAD ns3/dnstap.out | grep "AQ " | wc -l)
ar3=$($DNSTAPREAD ns3/dnstap.out | grep "AR " | wc -l)
cq3=$($DNSTAPREAD ns3/dnstap.out | grep "CQ " | wc -l)
cr3=$($DNSTAPREAD ns3/dnstap.out | grep "CR " | wc -l)
fq3=$($DNSTAPREAD ns3/dnstap.out | grep "FQ " | wc -l)
fr3=$($DNSTAPREAD ns3/dnstap.out | grep "FR " | wc -l)
rq3=$($DNSTAPREAD ns3/dnstap.out | grep "RQ " | wc -l)
rr3=$($DNSTAPREAD ns3/dnstap.out | grep "RR " | wc -l)
uq3=$($DNSTAPREAD ns3/dnstap.out | grep "UQ " | wc -l)
ur3=$($DNSTAPREAD ns3/dnstap.out | grep "UR " | wc -l)

udp5=$($DNSTAPREAD ns5/dnstap.out | grep "UDP " | wc -l)
tcp5=$($DNSTAPREAD ns5/dnstap.out | grep "TCP " | wc -l)
aq5=$($DNSTAPREAD ns5/dnstap.out | grep "AQ " | wc -l)
ar5=$($DNSTAPREAD ns5/dnstap.out | grep "AR " | wc -l)
cq5=$($DNSTAPREAD ns5/dnstap.out | grep "CQ " | wc -l)
cr5=$($DNSTAPREAD ns5/dnstap.out | grep "CR " | wc -l)
fq5=$($DNSTAPREAD ns5/dnstap.out | grep "FQ " | wc -l)
fr5=$($DNSTAPREAD ns5/dnstap.out | grep "FR " | wc -l)
rq5=$($DNSTAPREAD ns5/dnstap.out | grep "RQ " | wc -l)
rr5=$($DNSTAPREAD ns5/dnstap.out | grep "RR " | wc -l)
uq5=$($DNSTAPREAD ns5/dnstap.out | grep "UQ " | wc -l)
ur5=$($DNSTAPREAD ns5/dnstap.out | grep "UR " | wc -l)

echo_i "checking UDP message counts"
ret=0
check_count ns1 $udp1 0
check_count ns2 $udp2 2
check_count ns3 $udp3 2
check_count ns5 $udp5 4
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TCP message counts"
ret=0
check_count ns1 $tcp1 0
check_count ns2 $tcp2 0
check_count ns3 $tcp3 0
check_count ns5 $tcp5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AUTH_QUERY message counts"
ret=0
check_count ns1 $aq1 0
check_count ns2 $aq2 0
check_count ns3 $aq3 0
check_count ns5 $aq5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AUTH_RESPONSE message counts"
ret=0
check_count ns1 $ar1 0
check_count ns2 $ar2 0
check_count ns3 $ar3 0
check_count ns5 $ar5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking CLIENT_QUERY message counts"
ret=0
check_count ns1 $cq1 0
check_count ns2 $cq2 0
check_count ns3 $cq3 1
check_count ns5 $cq5 1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking CLIENT_RESPONSE message counts"
ret=0
check_count ns1 $cr1 0
check_count ns2 $cr2 0
check_count ns3 $cr3 1
check_count ns5 $cr5 1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking RESOLVER_QUERY message counts"
ret=0
check_count ns1 $rq1 0
check_count ns2 $rq2 0
check_count ns3 $rq3 0
check_count ns5 $rq5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking RESOLVER_RESPONSE message counts"
ret=0
check_count ns1 $rr1 0
check_count ns2 $rr2 0
check_count ns3 $rr3 0
check_count ns5 $rr5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking FORWARD_QUERY message counts"
ret=0
check_count ns1 $fq1 0
check_count ns2 $fq2 0
check_count ns3 $fq3 0
check_count ns5 $fq5 1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking FORWARD_RESPONSE message counts"
ret=0
check_count ns1 $fr1 0
check_count ns2 $fr2 0
check_count ns3 $fr3 0
check_count ns5 $fr5 1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking UPDATE_QUERY message counts"
ret=0
check_count ns1 $uq1 0
check_count ns2 $uq2 1
check_count ns3 $uq3 0
check_count ns5 $uq5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking UPDATE_RESPONSE message counts"
ret=0
check_count ns1 $ur1 0
check_count ns2 $ur2 1
check_count ns3 $ur3 0
check_count ns5 $ur5 0
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking whether destination UDP port is logged for client queries"
ret=0
$DNSTAPREAD ns3/dnstap.out.save | grep -Eq "CQ [0-9:.]+ -> 10.53.0.3:${PORT} UDP" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

HAS_PYYAML=0
$PYTHON -c "import yaml" 2>/dev/null && HAS_PYYAML=1

if [ $HAS_PYYAML -ne 0 ]; then
  echo_i "checking dnstap-read YAML output"
  ret=0
  {
    $PYTHON ydump.py "$DNSTAPREAD" "ns3/dnstap.out.save" >ydump.out || ret=1
  } | cat_i
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

echo_i "checking dnstap-read hex output"
ret=0
hex=$($DNSTAPREAD -x ns3/dnstap.out | tail -1)
echo $hex | $WIRETEST >dnstap.hex
grep 'status: NOERROR' dnstap.hex >/dev/null 2>&1 || ret=1
grep 'ANSWER: 3, AUTHORITY: 1' dnstap.hex >/dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

if [ -n "$FSTRM_CAPTURE" ]; then
  $DIG $DIGOPTS @10.53.0.4 a.example >dig.out || ret=1

  # send an UPDATE to ns4
  $NSUPDATE <<-EOF >nsupdate.out 2>&1 && ret=1
	server 10.53.0.4 ${PORT}
	zone example
	update add b.example 3600 in a 10.10.10.10
	send
EOF
  grep "update failed: NOTAUTH" nsupdate.out >/dev/null || ret=1

  echo_i "checking unix socket message counts"
  sleep 2
  retry_quiet 5 dnstap_data_ready $fstrm_capture_pid dnstap.out 450 || {
    echo_i "dnstap output file smaller than expected"
    ret=1
  }
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
  kill $fstrm_capture_pid
  wait
  udp4=$($DNSTAPREAD dnstap.out | grep "UDP " | wc -l)
  tcp4=$($DNSTAPREAD dnstap.out | grep "TCP " | wc -l)
  aq4=$($DNSTAPREAD dnstap.out | grep "AQ " | wc -l)
  ar4=$($DNSTAPREAD dnstap.out | grep "AR " | wc -l)
  cq4=$($DNSTAPREAD dnstap.out | grep "CQ " | wc -l)
  cr4=$($DNSTAPREAD dnstap.out | grep "CR " | wc -l)
  fq4=$($DNSTAPREAD dnstap.out | grep "FQ " | wc -l)
  fr4=$($DNSTAPREAD dnstap.out | grep "FR " | wc -l)
  rq4=$($DNSTAPREAD dnstap.out | grep "RQ " | wc -l)
  rr4=$($DNSTAPREAD dnstap.out | grep "RR " | wc -l)
  uq4=$($DNSTAPREAD dnstap.out | grep "UQ " | wc -l)
  ur4=$($DNSTAPREAD dnstap.out | grep "UR " | wc -l)

  echo_i "checking UDP message counts"
  ret=0
  check_count ns4 $udp4 4
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking TCP message counts"
  ret=0
  check_count ns4 $tcp4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking AUTH_QUERY message counts"
  ret=0
  check_count ns4 $aq4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking AUTH_RESPONSE message counts"
  ret=0
  check_count ns4 $ar4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking CLIENT_QUERY message counts"
  ret=0
  check_count ns4 $cq4 1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking CLIENT_RESPONSE message counts"
  ret=0
  check_count ns4 $cr4 1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking RESOLVER_QUERY message counts"
  ret=0
  check_count ns4 $rq4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking RESOLVER_RESPONSE message counts"
  ret=0
  check_count ns4 $rr4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking FORWARDER_QUERY message counts"
  ret=0
  check_count ns4 $fq4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking FORWARDER_RESPONSE message counts"
  ret=0
  check_count ns4 $fr4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking UPDATE_QUERY message counts"
  ret=0
  check_count ns4 $uq4 1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking UPDATE_RESPONSE message counts"
  ret=0
  check_count ns4 $ur4 1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  mv dnstap.out dnstap.out.save

  echo_i "restarting fstrm_capture"
  $FSTRM_CAPTURE -t protobuf:dnstap.Dnstap -u ns4/dnstap.out \
    -w dnstap.out >fstrm_capture.out.2 2>&1 &
  fstrm_capture_pid=$!
  wait_for_log 10 "socket path ns4/dnstap.out" fstrm_capture.out.2 || {
    echo_i "failed"
    ret=1
  }
  $RNDCCMD -s 10.53.0.4 dnstap -reopen | sed 's/^/ns4 /' | cat_i
  $DIG $DIGOPTS @10.53.0.4 a.example >dig.out || ret=1

  echo_i "checking reopened unix socket message counts"
  sleep 2
  retry_quiet 5 dnstap_data_ready $fstrm_capture_pid dnstap.out 270 || {
    echo_i "dnstap output file smaller than expected"
    ret=1
  }
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
  kill $fstrm_capture_pid
  wait
  udp4=$($DNSTAPREAD dnstap.out | grep "UDP " | wc -l)
  tcp4=$($DNSTAPREAD dnstap.out | grep "TCP " | wc -l)
  aq4=$($DNSTAPREAD dnstap.out | grep "AQ " | wc -l)
  ar4=$($DNSTAPREAD dnstap.out | grep "AR " | wc -l)
  cq4=$($DNSTAPREAD dnstap.out | grep "CQ " | wc -l)
  cr4=$($DNSTAPREAD dnstap.out | grep "CR " | wc -l)
  fq4=$($DNSTAPREAD dnstap.out | grep "FQ " | wc -l)
  fr4=$($DNSTAPREAD dnstap.out | grep "FR " | wc -l)
  rq4=$($DNSTAPREAD dnstap.out | grep "RQ " | wc -l)
  rr4=$($DNSTAPREAD dnstap.out | grep "RR " | wc -l)
  uq4=$($DNSTAPREAD dnstap.out | grep "UQ " | wc -l)
  ur4=$($DNSTAPREAD dnstap.out | grep "UR " | wc -l)

  echo_i "checking UDP message counts"
  ret=0
  check_count ns4 $udp4 2
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking TCP message counts"
  ret=0
  check_count ns4 $tcp4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking AUTH_QUERY message counts"
  ret=0
  check_count ns4 $aq4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking AUTH_RESPONSE message counts"
  ret=0
  check_count ns4 $ar4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking CLIENT_QUERY message counts"
  ret=0
  check_count ns4 $cq4 1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking CLIENT_RESPONSE message counts"
  ret=0
  check_count ns4 $cr4 1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking RESOLVER_QUERY message counts"
  ret=0
  check_count ns4 $rq4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking RESOLVER_RESPONSE message counts"
  ret=0
  check_count ns4 $rr4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking FORWARDER_QUERY message counts"
  ret=0
  check_count ns4 $fq4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking FORWARDER_RESPONSE message counts"
  ret=0
  check_count ns4 $fr4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking UPDATE_QUERY message counts"
  ret=0
  check_count ns4 $uq4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "checking UPDATE_RESPONSE message counts"
  ret=0
  check_count ns4 $ur4 0
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

echo_i "checking large packet printing"
ret=0
# Expect one occurrence of "opcode: QUERY" below "reponse_message_data" and
# another one below "response_message".
lines=$($DNSTAPREAD -y large-answer.fstrm | grep -c "opcode: QUERY")
[ $lines -eq 2 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

_test_dnstap_roll() (
  ip="$1"
  ns="$2"
  n="$3"

  $RNDCCMD -s "${ip}" dnstap -roll "${n}" | sed "s/^/${ns} /" | cat_i \
    && files=$(find "$ns" -name "dnstap.out.[0-9]" | wc -l) \
    && test "$files" -eq "${n}" && test "$files" -ge "1" || return 1
)

test_dnstap_roll() {
  echo_i "checking 'rndc -roll $4' ($1)"
  ret=0

  try=0
  while test $try -lt 12; do
    touch "$3/dnstap.out.$try"
    try=$((try + 1))
  done

  _repeat 10 _test_dnstap_roll $2 $3 $4 || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
}

start_server --noclean --restart --port "${PORT}" ns3
test_dnstap_roll "no versions" 10.53.0.3 ns3 6
test_dnstap_roll "no versions" 10.53.0.3 ns3 3
test_dnstap_roll "no versions" 10.53.0.3 ns3 1

start_server --noclean --restart --port "${PORT}" ns2
test_dnstap_roll "versions" 10.53.0.2 ns2 6
test_dnstap_roll "versions" 10.53.0.2 ns2 3
test_dnstap_roll "versions" 10.53.0.2 ns2 1

echo_i "exit status: $status"
[ "$status" -eq 0 ] || exit 1
