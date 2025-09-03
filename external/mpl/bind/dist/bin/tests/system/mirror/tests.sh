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

DIGOPTS="-p ${PORT} -b 10.53.0.1 +dnssec +time=2 +tries=1 +multi"
RNDCCMD="$RNDC -c ../_common/rndc.conf -p ${CONTROLPORT} -s"

# Wait until the transfer of the given zone to ns3 either completes
# successfully or is aborted by a verification failure or a REFUSED response
# from the primary.  Note that matching on any transfer status is deliberately
# avoided because some checks performed by this test cause transfer attempts to
# end with the "IXFR failed" status, which is followed by an AXFR retry and
# this test needs to check what the result of the latter transfer attempt is.
wait_for_transfer() {
  zone=$1
  for i in 1 2 3 4 5 6 7 8 9 10; do
    # Wait until a "freeing transfer context" message is logged
    # after one of the transfer results we are looking for is
    # logged.  This is needed to prevent races when checking for
    # "mirror zone is now in use" messages.
    nextpartpeek ns3/named.run \
      | awk "matched; /'$zone\/IN'.*Transfer status: (success|verify failure|REFUSED)/ {matched=1}" \
      | grep "'$zone/IN'.*freeing transfer context" >/dev/null && return
    sleep 1
  done
  echo_i "exceeded time limit waiting for proof of '$zone' being transferred to appear in ns3/named.run"
  ret=1
}

# Wait until loading the given zone on the given server either completes
# successfully for the specified serial number or fails.
wait_for_load() {
  zone=$1
  serial=$2
  log=$3
  for i in 1 2 3 4 5 6 7 8 9 10; do
    # Wait until a "zone_postload: (...): done" message is logged
    # after one of the loading-related messages we are looking for
    # is logged.  This is needed to prevent races when checking for
    # "mirror zone is now in use" messages.
    nextpartpeek $log \
      | awk "matched; /$zone.*(loaded serial $serial|unable to load)/ {matched=1}" \
      | grep "zone_postload: zone $zone/IN: done" >/dev/null && return
    sleep 1
  done
  echo_i "exceeded time limit waiting for proof of '$zone' being loaded to appear in $log"
  ret=1
}

# Trigger a reload of ns2 and wait until loading the given zone completes.
reload_zone() {
  zone=$1
  serial=$2
  rndc_reload ns2 10.53.0.2
  wait_for_load $zone $serial ns2/named.run
}

status=0
n=0

ORIGINAL_SERIAL=$(awk '$2 == "SOA" {print $5}' ns2/verify.db.in)
UPDATED_SERIAL_BAD=$((ORIGINAL_SERIAL + 1))
UPDATED_SERIAL_GOOD=$((ORIGINAL_SERIAL + 2))

n=$((n + 1))
echo_i "checking that an unsigned mirror zone is rejected ($n)"
ret=0
wait_for_transfer verify-unsigned
$DIG $DIGOPTS @10.53.0.3 +norec verify-unsigned SOA >dig.out.ns3.test$n 2>&1 || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n >/dev/null || ret=1
grep "${ORIGINAL_SERIAL}.*; serial" dig.out.ns3.test$n >/dev/null && ret=1
nextpartpeek ns3/named.run | grep "verify-unsigned.*Zone contains no DNSSEC keys" >/dev/null || ret=1
nextpartpeek ns3/named.run | grep "verify-unsigned.*mirror zone is now in use" >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that a mirror zone signed using an untrusted key is rejected ($n)"
ret=0
nextpartreset ns3/named.run
wait_for_transfer verify-untrusted
$DIG $DIGOPTS @10.53.0.3 +norec verify-untrusted SOA >dig.out.ns3.test$n 2>&1 || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n >/dev/null || ret=1
grep "${ORIGINAL_SERIAL}.*; serial" dig.out.ns3.test$n >/dev/null && ret=1
nextpartpeek ns3/named.run | grep "verify-untrusted.*No trusted DNSKEY found" >/dev/null || ret=1
nextpartpeek ns3/named.run | grep "verify-untrusted.*mirror zone is now in use" >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that a mirror zone signed using a CSK without the SEP bit set is accepted ($n)"
ret=0
nextpartreset ns3/named.run
wait_for_transfer verify-csk
$DIG $DIGOPTS @10.53.0.3 +norec verify-csk SOA >dig.out.ns3.test$n 2>&1 || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n >/dev/null && ret=1
grep "${ORIGINAL_SERIAL}.*; serial" dig.out.ns3.test$n >/dev/null || ret=1
nextpartpeek ns3/named.run | grep "verify-csk.*mirror zone is now in use" >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that an AXFR of an incorrectly signed mirror zone is rejected ($n)"
ret=0
nextpartreset ns3/named.run
wait_for_transfer verify-axfr
$DIG $DIGOPTS @10.53.0.3 +norec verify-axfr SOA >dig.out.ns3.test$n 2>&1 || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n >/dev/null || ret=1
grep "${UPDATED_SERIAL_BAD}.*; serial" dig.out.ns3.test$n >/dev/null && ret=1
nextpartpeek ns3/named.run | grep "No correct ${DEFAULT_ALGORITHM} signature for verify-axfr SOA" >/dev/null || ret=1
nextpartpeek ns3/named.run | grep "verify-axfr.*mirror zone is now in use" >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that an AXFR of an updated, correctly signed mirror zone is accepted ($n)"
ret=0
nextpart ns3/named.run >/dev/null
sleep 1
cat ns2/verify-axfr.db.good.signed >ns2/verify-axfr.db.signed
reload_zone verify-axfr ${UPDATED_SERIAL_GOOD}
$RNDCCMD 10.53.0.3 retransfer verify-axfr >/dev/null 2>&1
wait_for_transfer verify-axfr
$DIG $DIGOPTS @10.53.0.3 +norec verify-axfr SOA >dig.out.ns3.test$n 2>&1 || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n >/dev/null && ret=1
grep "${UPDATED_SERIAL_GOOD}.*; serial" dig.out.ns3.test$n >/dev/null || ret=1
nextpartpeek ns3/named.run | grep "verify-axfr.*mirror zone is now in use" >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that an IXFR of an incorrectly signed mirror zone is rejected ($n)"
nextpartreset ns3/named.run
ret=0
wait_for_transfer verify-ixfr
# Sanity check: the initial, properly signed version of the zone should have
# been announced as coming into effect.
nextpart ns3/named.run | grep "verify-ixfr.*mirror zone is now in use" >/dev/null || ret=1
# Make a copy of the original zone file for reuse in journal tests below.
cp ns2/verify-ixfr.db.signed ns3/verify-journal.db.mirror
# Wait 1 second so that the zone file timestamp changes and the subsequent
# invocation of "rndc reload" triggers a zone reload.
sleep 1
cat ns2/verify-ixfr.db.bad.signed >ns2/verify-ixfr.db.signed
reload_zone verify-ixfr ${UPDATED_SERIAL_BAD}
# Make a copy of the bad zone journal for reuse in journal tests below.
cp ns2/verify-ixfr.db.signed.jnl ns3/verify-journal.db.bad.mirror.jnl
# Trigger IXFR.
$RNDCCMD 10.53.0.3 refresh verify-ixfr >/dev/null 2>&1
wait_for_transfer verify-ixfr
# Ensure the transfer was incremental as expected.
if [ $(nextpartpeek ns3/named.run | grep "verify-ixfr.*got incremental response" | wc -l) -eq 0 ]; then
  echo_i "failed: did not get an incremental response"
  ret=1
fi
# Ensure the new, bad version of the zone was not accepted.
$DIG $DIGOPTS @10.53.0.3 +norec verify-ixfr SOA >dig.out.ns3.test$n 2>&1 || ret=1
# A positive answer is expected as the original version of the "verify-ixfr"
# zone should have been successfully verified.
grep "ANSWER: 0" dig.out.ns3.test$n >/dev/null && ret=1
grep "${UPDATED_SERIAL_BAD}.*; serial" dig.out.ns3.test$n >/dev/null && ret=1
nextpartpeek ns3/named.run | grep "No correct ${DEFAULT_ALGORITHM} signature for verify-ixfr SOA" >/dev/null || ret=1
# Despite the verification failure for this IXFR, this mirror zone should still
# be in use as its previous version should have been verified successfully.
nextpartpeek ns3/named.run | grep "verify-ixfr.*mirror zone is no longer in use" >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that an IXFR of an updated, correctly signed mirror zone is accepted after AXFR failover ($n)"
ret=0
nextpart ns3/named.run >/dev/null
# Wait 1 second so that the zone file timestamp changes and the subsequent
# invocation of "rndc reload" triggers a zone reload.
sleep 1
cat ns2/verify-ixfr.db.good.signed >ns2/verify-ixfr.db.signed
reload_zone verify-ixfr ${UPDATED_SERIAL_GOOD}
# Make a copy of the good zone journal for reuse in journal tests below.
cp ns2/verify-ixfr.db.signed.jnl ns3/verify-journal.db.good.mirror.jnl
# Trigger IXFR.
$RNDCCMD 10.53.0.3 refresh verify-ixfr >/dev/null 2>&1
wait_for_transfer verify-ixfr
# Ensure the new, good version of the zone was accepted.
$DIG $DIGOPTS @10.53.0.3 +norec verify-ixfr SOA >dig.out.ns3.test$n 2>&1 || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n >/dev/null && ret=1
grep "${UPDATED_SERIAL_GOOD}.*; serial" dig.out.ns3.test$n >/dev/null || ret=1
# The log message announcing the mirror zone coming into effect should not have
# been logged this time since the mirror zone in question is expected to
# already be in use before this test case is checked.
nextpartpeek ns3/named.run | grep "verify-ixfr.*mirror zone is now in use" >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that loading an incorrectly signed mirror zone from disk fails ($n)"
ret=0
nextpartreset ns3/named.run
wait_for_load verify-load ${UPDATED_SERIAL_BAD} ns3/named.run
$DIG $DIGOPTS @10.53.0.3 +norec verify-load SOA >dig.out.ns3.test$n 2>&1 || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n >/dev/null || ret=1
grep "${UPDATED_SERIAL_BAD}.*; serial" dig.out.ns3.test$n >/dev/null && ret=1
nextpartpeek ns3/named.run | grep "No correct ${DEFAULT_ALGORITHM} signature for verify-load SOA" >/dev/null || ret=1
nextpartpeek ns3/named.run | grep "verify-load.*mirror zone is now in use" >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "ensuring trust anchor telemetry queries are sent upstream for a mirror zone ($n)"
ret=0
# ns3 is started with "-T tat=3", so TAT queries should have already been sent.
wait_for_log_re 3 "_ta-[-0-9a-f]*/NULL" ns1/named.run || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that loading a correctly signed mirror zone from disk succeeds ($n)"
ret=0
stop_server --use-rndc --port ${CONTROLPORT} ns3
cat ns2/verify-load.db.good.signed >ns3/verify-load.db.mirror
nextpart ns3/named.run >/dev/null
start_server --noclean --restart --port ${PORT} ns3
wait_for_load verify-load ${UPDATED_SERIAL_GOOD} ns3/named.run
$DIG $DIGOPTS @10.53.0.3 +norec verify-load SOA >dig.out.ns3.test$n 2>&1 || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n >/dev/null && ret=1
grep "${UPDATED_SERIAL_GOOD}.*; serial" dig.out.ns3.test$n >/dev/null || ret=1
nextpartpeek ns3/named.run | grep "verify-load.*mirror zone is now in use" >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that loading a journal for an incorrectly signed mirror zone fails ($n)"
ret=0
stop_server --use-rndc --port ${CONTROLPORT} ns3
cp ns3/verify-journal.db.mirror ns3/verify-ixfr.db.mirror
cp ns3/verify-journal.db.bad.mirror.jnl ns3/verify-ixfr.db.mirror.jnl
# Temporarily disable transfers of the "verify-ixfr" zone on ns2.  This is
# required to reliably test whether the message announcing the mirror zone
# coming into effect is not logged after a failed journal verification since
# otherwise a corrected version of the zone may be transferred after
# verification fails but before we look for the aforementioned log message.
# (NOTE: Keep the embedded newline in the sed function list below.)
sed '/^zone "verify-ixfr" {$/,/^};$/ {
	s/10.53.0.3/10.53.0.254/
}' ns2/named.conf >ns2/named.conf.modified
mv ns2/named.conf.modified ns2/named.conf
rndc_reconfig ns2 10.53.0.2
nextpart ns3/named.run >/dev/null
start_server --noclean --restart --port ${PORT} ns3
wait_for_load verify-ixfr ${UPDATED_SERIAL_BAD} ns3/named.run
$DIG $DIGOPTS @10.53.0.3 +norec verify-ixfr SOA >dig.out.ns3.test$n 2>&1 || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n >/dev/null || ret=1
grep "${UPDATED_SERIAL_BAD}.*; serial" dig.out.ns3.test$n >/dev/null && ret=1
nextpartpeek ns3/named.run | grep "No correct ${DEFAULT_ALGORITHM} signature for verify-ixfr SOA" >/dev/null || ret=1
nextpartpeek ns3/named.run | grep "verify-ixfr.*mirror zone is now in use" >/dev/null && ret=1
# Restore transfers for the "verify-ixfr" zone on ns2.
# (NOTE: Keep the embedded newline in the sed function list below.)
sed '/^zone "verify-ixfr" {$/,/^};$/ {
	s/10.53.0.254/10.53.0.3/
}' ns2/named.conf >ns2/named.conf.modified
mv ns2/named.conf.modified ns2/named.conf
rndc_reconfig ns2 10.53.0.2
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that loading a journal for a correctly signed mirror zone succeeds ($n)"
ret=0
stop_server --use-rndc --port ${CONTROLPORT} ns3
cp ns3/verify-journal.db.mirror ns3/verify-ixfr.db.mirror
cp ns3/verify-journal.db.good.mirror.jnl ns3/verify-ixfr.db.mirror.jnl
nextpart ns3/named.run >/dev/null
start_server --noclean --restart --port ${PORT} ns3
wait_for_load verify-ixfr ${UPDATED_SERIAL_GOOD} ns3/named.run
$DIG $DIGOPTS @10.53.0.3 +norec verify-ixfr SOA >dig.out.ns3.test$n 2>&1 || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n >/dev/null && ret=1
grep "${UPDATED_SERIAL_GOOD}.*; serial" dig.out.ns3.test$n >/dev/null || ret=1
nextpartpeek ns3/named.run | grep "verify-ixfr.*mirror zone is now in use" >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking delegations sourced from a mirror zone ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 foo.example A +norec >dig.out.ns3.test$n 2>&1 || ret=1
# Check response code and flags in the answer.
grep "NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
grep "flags:.* ad" dig.out.ns3.test$n >/dev/null && ret=1
# Check that a delegation containing a DS RRset and glue is present.
grep "ANSWER: 0" dig.out.ns3.test$n >/dev/null || ret=1
grep "example.*IN.*NS" dig.out.ns3.test$n >/dev/null || ret=1
grep "example.*IN.*DS" dig.out.ns3.test$n >/dev/null || ret=1
grep "ns2.example.*A.*10.53.0.2" dig.out.ns3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that resolution involving a mirror zone works as expected ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 foo.example A >dig.out.ns3.test$n 2>&1 || ret=1
# Check response code and flags in the answer.
grep "NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
grep "flags:.* ad" dig.out.ns3.test$n >/dev/null || ret=1
# Ensure ns1 was not queried.
grep "query 'foo.example/A/IN'" ns1/named.run >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that non-recursive queries for names below mirror zone get responded from cache ($n)"
ret=0
# Issue a non-recursive query for an RRset which is expected to be in cache.
$DIG $DIGOPTS @10.53.0.3 +norec foo.example. A >dig.out.ns3.test$n 2>&1 || ret=1
# Check response code and flags in the answer.
grep "NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
grep "flags:.* ad" dig.out.ns3.test$n >/dev/null || ret=1
# Ensure the response is not a delegation.
grep "ANSWER: 0" dig.out.ns3.test$n >/dev/null && ret=1
grep "foo.example.*IN.*A.*127.0.0.1" dig.out.ns3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that delegations from cache which improve mirror zone delegations are properly handled ($n)"
ret=0
# First, issue a recursive query in order to cache an RRset which is not within
# the mirror zone's bailiwick.
$DIG $DIGOPTS @10.53.0.3 sub.example. NS >dig.out.ns3.test$n.1 2>&1 || ret=1
# Ensure the child-side NS RRset is returned.
grep "NOERROR" dig.out.ns3.test$n.1 >/dev/null || ret=1
grep "ANSWER: 2" dig.out.ns3.test$n.1 >/dev/null || ret=1
grep "sub.example.*IN.*NS" dig.out.ns3.test$n.1 >/dev/null || ret=1
# Issue a non-recursive query for something below the cached zone cut.
$DIG $DIGOPTS @10.53.0.3 +norec foo.sub.example. A >dig.out.ns3.test$n.2 2>&1 || ret=1
# Ensure the cached NS RRset is returned in a delegation, along with the
# parent-side DS RRset.
grep "NOERROR" dig.out.ns3.test$n.2 >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n.2 >/dev/null || ret=1
grep "sub.example.*IN.*NS" dig.out.ns3.test$n.2 >/dev/null || ret=1
grep "sub.example.*IN.*DS" dig.out.ns3.test$n.2 >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking flags set in a DNSKEY response sourced from a mirror zone ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 . DNSKEY >dig.out.ns3.test$n 2>&1 || ret=1
# Check response code and flags in the answer.
grep "NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
grep "flags:.* aa" dig.out.ns3.test$n >/dev/null && ret=1
grep "flags:.* ad" dig.out.ns3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking flags set in a SOA response sourced from a mirror zone ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 . SOA >dig.out.ns3.test$n 2>&1 || ret=1
# Check response code and flags in the answer.
grep "NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
grep "flags:.* aa" dig.out.ns3.test$n >/dev/null && ret=1
grep "flags:.* ad" dig.out.ns3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that resolution succeeds with unavailable mirror zone data ($n)"
ret=0
wait_for_transfer initially-unavailable
# Query for a record in a zone that is set up to be mirrored, but
# untransferrable from the configured primary.  Resolution should still succeed.
$DIG $DIGOPTS @10.53.0.3 foo.initially-unavailable. A >dig.out.ns3.test$n.1 2>&1 || ret=1
# Check response code and flags in the answer.
grep "NOERROR" dig.out.ns3.test$n.1 >/dev/null || ret=1
grep "flags:.* ad" dig.out.ns3.test$n.1 >/dev/null || ret=1
# Sanity check: the authoritative server should have been queried.
nextpart ns2/named.run | grep "query 'foo.initially-unavailable/A/IN'" >/dev/null || ret=1
# Reconfigure ns2 so that the zone can be mirrored on ns3.
sed '/^zone "initially-unavailable" {$/,/^};$/ {
	s/10.53.0.254/10.53.0.3/
}' ns2/named.conf >ns2/named.conf.modified
mv ns2/named.conf.modified ns2/named.conf
rndc_reconfig ns2 10.53.0.2
# Flush the cache on ns3 and retransfer the mirror zone.
$RNDCCMD 10.53.0.3 flush >/dev/null 2>&1
nextpart ns3/named.run >/dev/null
$RNDCCMD 10.53.0.3 retransfer initially-unavailable >/dev/null 2>&1
wait_for_transfer initially-unavailable
# Query for the same record again.  Resolution should still succeed.
$DIG $DIGOPTS @10.53.0.3 foo.initially-unavailable. A >dig.out.ns3.test$n.2 2>&1 || ret=1
# Check response code and flags in the answer.
grep "NOERROR" dig.out.ns3.test$n.2 >/dev/null || ret=1
grep "flags:.* ad" dig.out.ns3.test$n.2 >/dev/null || ret=1
# Ensure the authoritative server was not queried.
nextpart ns2/named.run | grep "query 'foo.initially-unavailable/A/IN'" >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that resolution succeeds with expired mirror zone data ($n)"
ret=0
# Reconfigure ns2 so that the zone from the previous test can no longer be
# mirrored on ns3.
sed '/^zone "initially-unavailable" {$/,/^};$/ {
	s/10.53.0.3/10.53.0.254/
}' ns2/named.conf >ns2/named.conf.modified
mv ns2/named.conf.modified ns2/named.conf
rndc_reconfig ns2 10.53.0.2
# Stop ns3, update the timestamp of the zone file to one far in the past, then
# restart ns3.
stop_server --use-rndc --port ${CONTROLPORT} ns3
touch -t 200001010000 ns3/initially-unavailable.db.mirror
nextpart ns3/named.run >/dev/null
start_server --noclean --restart --port ${PORT} ns3
# Ensure named attempts to retransfer the zone due to its expiry.
wait_for_transfer initially-unavailable
# Ensure the expected messages were logged.
nextpartpeek ns3/named.run | grep "initially-unavailable.*expired" >/dev/null || ret=1
nextpartpeek ns3/named.run | grep "initially-unavailable.*mirror zone is no longer in use" >/dev/null || ret=1
# Query for a record in the expired zone.  Resolution should still succeed.
$DIG $DIGOPTS @10.53.0.3 foo.initially-unavailable. A >dig.out.ns3.test$n 2>&1 || ret=1
# Check response code and flags in the answer.
grep "NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
grep "flags:.* ad" dig.out.ns3.test$n >/dev/null || ret=1
# Sanity check: the authoritative server should have been queried.
nextpart ns2/named.run | grep "query 'foo.initially-unavailable/A/IN'" >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that clients without cache access cannot retrieve mirror zone data ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 -b 10.53.0.3 +norec . SOA >dig.out.ns3.test$n 2>&1 || ret=1
# Check response code and flags in the answer.
grep "REFUSED" dig.out.ns3.test$n >/dev/null || ret=1
grep "flags:.* ad" dig.out.ns3.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that outgoing transfers of mirror zones are disabled by default ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 . AXFR >dig.out.ns3.test$n 2>&1 || ret=1
grep "; Transfer failed" dig.out.ns3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that notifies are disabled by default for mirror zones ($n)"
ret=0
grep "initially-unavailable.*sending notifies" ns3/named.run >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking output of \"rndc zonestatus\" for a mirror zone ($n)"
ret=0
$RNDCCMD 10.53.0.3 zonestatus . >rndc.out.ns3.test$n 2>&1
grep "type: mirror" rndc.out.ns3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that \"rndc reconfig\" properly handles a mirror -> secondary zone type change ($n)"
ret=0
# Sanity check before we start.
$DIG $DIGOPTS @10.53.0.3 +norec verify-reconfig SOA >dig.out.ns3.test$n.1 2>&1 || ret=1
grep "NOERROR" dig.out.ns3.test$n.1 >/dev/null || ret=1
grep "flags:.* aa" dig.out.ns3.test$n.1 >/dev/null && ret=1
grep "flags:.* ad" dig.out.ns3.test$n.1 >/dev/null || ret=1
# Reconfigure the zone so that it is no longer a mirror zone.
# (NOTE: Keep the embedded newline in the sed function list below.)
sed '/^zone "verify-reconfig" {$/,/^};$/ {
	s/type mirror;/type secondary;/
}' ns3/named.conf >ns3/named.conf.modified
mv ns3/named.conf.modified ns3/named.conf
nextpart ns3/named.run >/dev/null
rndc_reconfig ns3 10.53.0.3
# Zones whose type was changed should not be reusable, which means the tested
# zone should have been reloaded from disk.
wait_for_load verify-reconfig ${ORIGINAL_SERIAL} ns3/named.run
# Ensure responses sourced from the reconfigured zone have AA=1 and AD=0.
$DIG $DIGOPTS @10.53.0.3 +norec verify-reconfig SOA >dig.out.ns3.test$n.2 2>&1 || ret=1
grep "NOERROR" dig.out.ns3.test$n.2 >/dev/null || ret=1
grep "flags:.* aa" dig.out.ns3.test$n.2 >/dev/null || ret=1
grep "flags:.* ad" dig.out.ns3.test$n.2 >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that \"rndc reconfig\" properly handles a secondary -> mirror zone type change ($n)"
ret=0
# Put an incorrectly signed version of the zone in the zone file used by ns3.
nextpart ns3/named.run >/dev/null
cat ns2/verify-reconfig.db.bad.signed >ns3/verify-reconfig.db.mirror
# Reconfigure the zone so that it is a mirror zone again.
# (NOTE: Keep the embedded newline in the sed function list below.)
sed '/^zone "verify-reconfig" {$/,/^};$/ {
	s/type secondary;/type mirror;/
}' ns3/named.conf >ns3/named.conf.modified
mv ns3/named.conf.modified ns3/named.conf
rndc_reconfig ns3 10.53.0.3
# The reconfigured zone should fail verification.
wait_for_load verify-reconfig ${UPDATED_SERIAL_BAD} ns3/named.run
$DIG $DIGOPTS @10.53.0.3 +norec verify-reconfig SOA >dig.out.ns3.test$n 2>&1 || ret=1
grep "${UPDATED_SERIAL_BAD}.*; serial" dig.out.ns3.test$n >/dev/null && ret=1
nextpart ns3/named.run | grep "No correct ${DEFAULT_ALGORITHM} signature for verify-reconfig SOA" >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that a mirror zone can be added using rndc ($n)"
ret=0
# Sanity check: the zone should not exist in the root zone.
$DIG $DIGOPTS @10.53.0.3 +norec verify-addzone SOA >dig.out.ns3.test$n.1 2>&1 || ret=1
grep "NXDOMAIN" dig.out.ns3.test$n.1 >/dev/null || ret=1
grep "flags:.* aa" dig.out.ns3.test$n.1 >/dev/null && ret=1
grep "flags:.* ad" dig.out.ns3.test$n.1 >/dev/null || ret=1
# Mirror a zone which does not exist in the root zone.
nextpart ns3/named.run >/dev/null
$RNDCCMD 10.53.0.3 addzone verify-addzone '{ type mirror; primaries { 10.53.0.2; }; };' >rndc.out.ns3.test$n 2>&1 || ret=1
wait_for_transfer verify-addzone
# Check whether the mirror zone was added and whether it behaves as expected.
$DIG $DIGOPTS @10.53.0.3 +norec verify-addzone SOA >dig.out.ns3.test$n.2 2>&1 || ret=1
grep "NOERROR" dig.out.ns3.test$n.2 >/dev/null || ret=1
grep "flags:.* aa" dig.out.ns3.test$n.2 >/dev/null && ret=1
grep "flags:.* ad" dig.out.ns3.test$n.2 >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that a mirror zone can be deleted using rndc ($n)"
ret=0
# Remove the mirror zone added in the previous test.
nextpart ns3/named.run >/dev/null
$RNDCCMD 10.53.0.3 delzone verify-addzone >rndc.out.ns3.test$n 2>&1 || ret=1
wait_for_log 20 "zone verify-addzone/IN: mirror zone is no longer in use; reverting to normal recursion" ns3/named.run || ret=1
# Check whether the mirror zone was removed.
$DIG $DIGOPTS @10.53.0.3 +norec verify-addzone SOA >dig.out.ns3.test$n 2>&1 || ret=1
grep "NXDOMAIN" dig.out.ns3.test$n >/dev/null || ret=1
grep "flags:.* aa" dig.out.ns3.test$n >/dev/null && ret=1
grep "flags:.* ad" dig.out.ns3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
