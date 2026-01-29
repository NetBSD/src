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

status=0
n=0

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd +dnssec -p ${PORT}"
RNDCCMD="$RNDC -c ../_common/rndc.conf -p ${CONTROLPORT} -s"

# convert private-type records to readable form
# $1 is the zone
# $2 is the server
# $3 is ignored
# $4 is the alternate type
showprivate() {
  echo "-- $@ --"
  $DIG $DIGOPTS +nodnssec +short @$2 -t ${4:-type65534} $1 | cut -f3 -d' ' \
    | while read record; do
      $PERL -e 'my $rdata = pack("H*", @ARGV[0]);
                die "invalid record" unless length($rdata) == 5;
                my ($alg, $key, $remove, $complete) = unpack("CnCC", $rdata);
                my $action = "signing";
                $action = "removing" if $remove;
                my $state = " (incomplete)";
                $state = " (complete)" if $complete;
                print ("$action: alg: $alg, key: $key$state\n");' $record
    done
}

# check that signing records are marked as complete
# if $3 is 1 then we are expecting "(incomplete)"
# if $3 is 2 then we are not expecting either "(complete)" or "(incomplete)"
# if $4 is present then that specifies any alternate type to check
checkprivate() {
  _ret=0
  expected="${3:-0}"
  x=$(showprivate "$@")
  echo $x | grep "(complete)" >/dev/null || _ret=2
  echo $x | grep "(incomplete)" >/dev/null && _ret=1

  if [ $_ret = $expected ]; then
    return 0
  fi

  echo "$x"
  echo_i "failed"
  return 1
}

# wait until notifies for zone $1 are sent by server $2. This is an indication
# that the zone is signed with the active keys, and the changes have been
# committed.
wait_for_notifies() {
  wait_for_log 10 "zone ${1}/IN: sending notifies" "${2}/named.run" || return 1
}

freq() {
  _file=$1
  # remove first and last line that has incomplete set and skews the distribution
  awk '$4 == "RRSIG" {print substr($9,1,8)}' <"$_file" | sort | uniq -c | sed '1d;$d'
}
# Check the signatures expiration times.  First check how many signatures
# there are in total ($rrsigs).  Then see what the distribution of signature
# expiration times is ($expiretimes).  Ignore the time part for a better
# modelled distribution.
checkjitter() {
  _file=$1
  _ret=0

  if ! command -v bc >/dev/null 2>&1; then
    echo_i "skip: bc not available"
    return 0
  fi

  freq "$_file" | cat_i
  _expiretimes=$(freq "$_file" | awk '{print $1}')

  _count=0
  # Check if we have at least 4 days
  # This number has been tuned for `signatures-validity 10d; signatures-jitter 8d`, as
  # 1. signature expiration dates should be spread out across at most 8 days
  # 2. we remove first and last day to remove frequency outlier, we are left with 6 days
  # 3. we subtract two more days to allow test pass on day boundaries, etc. leaving us with 4 days
  for _num in $_expiretimes; do
    _count=$((_count + 1))
  done
  if [ "$_count" -lt 4 ]; then
    echo_i "error: not enough categories"
    return 1
  fi

  # Calculate mean
  _total=0
  for _num in $_expiretimes; do
    _total=$((_total + _num))
  done
  _mean=$(($_total / $_count))

  # Calculate stddev
  _stddev=0
  for _num in $_expiretimes; do
    _stddev=$(echo "$_stddev + (($_num - $_mean) * ($_num - $_mean))" | bc)
  done
  _stddev=$(echo "sqrt($_stddev/$_count)" | bc)

  # We expect the number of signatures not to exceed the mean +- 3 * stddev.
  _limit=$((_stddev * 3))
  _low=$((_mean - _limit))
  _high=$((_mean + _limit))
  # Find outliers.
  echo_i "checking whether all frequencies fall into <$_low;$_high> range"
  for _num in $_expiretimes; do
    if [ $_num -gt $_high ]; then
      echo_i "error: too many RRSIG records ($_num) in expiration bucket"
      _ret=1
    fi
    if [ $_num -lt $_low ]; then
      echo_i "error: too few RRSIG records ($_num) in expiration bucket"
      _ret=1
    fi
  done

  return $_ret
}

#
#  The NSEC record at the apex of the zone and its RRSIG records are
#  added as part of the last step in signing a zone.  We wait for the
#  NSEC records to appear before proceeding with a counter to prevent
#  infinite loops if there is a error.
#
echo_i "waiting for autosign changes to take effect"
i=0
while [ $i -lt 30 ]; do
  ret=0
  #
  # Wait for the root DNSKEY RRset to be fully signed.
  #
  $DIG $DIGOPTS . @10.53.0.1 dnskey >dig.out.ns1.test$n.dnskey || ret=1
  grep "ANSWER: 11," dig.out.ns1.test$n.dnskey >/dev/null || ret=1
  for z in .; do
    $DIG $DIGOPTS $z @10.53.0.1 nsec >dig.out.ns1.test$n.nsec || ret=1
    grep "NS SOA" dig.out.ns1.test$n.nsec >/dev/null || ret=1
  done
  for z in bar. example.; do # skip NSEC3 signed zones
    $DIG $DIGOPTS $z @10.53.0.2 nsec >dig.out.ns2.test$n.$z || ret=1
    grep "NS SOA" dig.out.ns2.test$n.$z >/dev/null || ret=1
  done
  for z in bar. example. inaczsk2.example. noksk.example nozsk.example; do
    $DIG $DIGOPTS $z @10.53.0.3 nsec >dig.out.ns3.test$n.$z || ret=1
    grep "NS SOA" dig.out.ns3.test$n.$z >/dev/null || ret=1
  done
  i=$((i + 1))
  if [ $ret = 0 ]; then break; fi
  echo_i "waiting ... ($i)"
  sleep 2
done
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Initial counts of RRSIG expiry fields values for auto signed zones"
for z in .; do
  echo_i zone $z
  $DIG $DIGOPTS $z @10.53.0.1 axfr | awk '$4 == "RRSIG" {print $9}' | sort | uniq -c | cat_i
done
for z in bar. example. private.secure.example.; do
  echo_i zone $z
  $DIG $DIGOPTS $z @10.53.0.2 axfr | awk '$4 == "RRSIG" {print $9}' | sort | uniq -c | cat_i
done
for z in inaczsk2.example.; do
  echo_i zone $z
  $DIG $DIGOPTS $z @10.53.0.3 axfr | awk '$4 == "RRSIG" {print $9}' | sort | uniq -c | cat_i
done

# Set logfile offset for wait_for_log usage.
nextpartreset ns3/named.run

echo_i "signing preset nsec3 zone"
zsk=$(cat autozsk.key)
ksk=$(cat autoksk.key)
$SETTIME -K ns3 -P now -A now $zsk >settime.out.test$n.zsk || ret=1
$SETTIME -K ns3 -P now -A now $ksk >settime.out.test$n.ksk || ret=1
($RNDCCMD 10.53.0.3 loadkeys autonsec3.example. 2>&1 | sed 's/^/ns3 /' | cat_i) || ret=1

echo_i "waiting for changes to take effect"
sleep 3

missing=$(keyfile_to_key_id "$(cat noksk-ksk.key)")
echo_i "checking that expired RRSIGs from missing KSK $missing are not deleted ($n)"
ret=0
$JOURNALPRINT ns3/noksk.example.db.jnl \
  | awk '{if ($1 == "del" && $5 == "RRSIG" && $12 == id) {error=1}} END {exit error}' id=$missing || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

missing=$(keyfile_to_key_id "$(cat nozsk-zsk.key)")
ksk=$(keyfile_to_key_id "$(cat nozsk-ksk.key)")
echo_i "checking that expired RRSIGs from missing ZSK $missing are replaced ($n)"
ret=0
$JOURNALPRINT ns3/nozsk.example.db.jnl \
  | awk '{if ($1 == "del" && $5 == "RRSIG" && $12 == id) {ok=1}} END {exit ok?0:1}' id=$missing || ret=1
$JOURNALPRINT ns3/nozsk.example.db.jnl \
  | awk '{if ($1 == "add" && $5 == "RRSIG" && $12 == id) {ok=1}} END {exit ok?0:1}' id=$ksk || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

inactive=$(keyfile_to_key_id "$(cat inaczsk-zsk.key)")
ksk=$(keyfile_to_key_id "$(cat inaczsk-ksk.key)")
echo_i "checking that expired RRSIGs from inactive ZSK $inactive are replaced ($n)"
ret=0
$JOURNALPRINT ns3/inaczsk.example.db.jnl \
  | awk '{if ($1 == "del" && $5 == "RRSIG" && $12 == id) {ok=1}} END {exit ok?0:1}' id=$inactive || ret=1
$JOURNALPRINT ns3/inaczsk.example.db.jnl \
  | awk '{if ($1 == "add" && $5 == "RRSIG" && $12 == id) {ok=1}} END {exit ok?0:1}' id=$ksk || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that replaced RRSIGs are not logged (missing ZSK private key) ($n)"
ret=0
loglines=$(grep "Key nozsk.example/$DEFAULT_ALGORITHM/$missing .* retaining signatures" ns3/named.run | wc -l)
[ "$loglines" -eq 0 ] || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that replaced RRSIGs are not logged (inactive ZSK private key) ($n)"
ret=0
loglines=$(grep "Key inaczsk.example/$DEFAULT_ALGORITHM/$inactive .* retaining signatures" ns3/named.run | wc -l)
[ "$loglines" -eq 0 ] || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Send rndc sync command to ns1, ns2 and ns3, to force the dynamically
# signed zones to be dumped to their zone files
echo_i "dumping zone files"
($RNDCCMD 10.53.0.1 sync 2>&1 | sed 's/^/ns1 /' | cat_i) || ret=1
($RNDCCMD 10.53.0.2 sync 2>&1 | sed 's/^/ns2 /' | cat_i) || ret=1
($RNDCCMD 10.53.0.3 sync 2>&1 | sed 's/^/ns3 /' | cat_i) || ret=1

now="$(TZ=UTC date +%Y%m%d%H%M%S)"
check_expiry() (
  $DIG $DIGOPTS AXFR oldsigs.example @10.53.0.3 >dig.out.test$n || return 1
  nearest_expiration="$(awk '$4 == "RRSIG" { print $9 }' <dig.out.test$n | sort -n | head -1)"
  if [ "$nearest_expiration" -le "$now" ]; then
    echo_i "failed: $nearest_expiration <= $now"
    return 1
  fi
)

echo_i "checking expired signatures were updated ($n)"
retry 10 check_expiry || ret=1
$DIG $DIGOPTS +noauth a.oldsigs.example. @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.oldsigs.example. @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Check jitter distribution.
echo_i "checking expired signatures were jittered correctly ($n)"
ret=0
$DIG $DIGOPTS axfr oldsigs.example @10.53.0.3 >dig.out.ns3.test$n || ret=1
checkjitter dig.out.ns3.test$n || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking NSEC->NSEC3 conversion succeeded ($n)"
ret=0
$DIG $DIGOPTS nsec3.example. nsec3param @10.53.0.3 >dig.out.ns3.ok.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.ok.test$n >/dev/null || ret=1
$DIG $DIGOPTS +noauth q.nsec3.example. @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth q.nsec3.example. @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking direct NSEC3 autosigning succeeded ($n)"
ret=0
$DIG $DIGOPTS +noall +answer autonsec3.example. nsec3param @10.53.0.3 >dig.out.ns3.ok.test$n || ret=1
[ -s dig.out.ns3.ok.test$n ] || ret=1
grep "NSEC3PARAM" dig.out.ns3.ok.test$n >/dev/null || ret=1
$DIG $DIGOPTS +noauth q.autonsec3.example. @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth q.autonsec3.example. @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TTLs of imported DNSKEYs (no default) ($n)"
ret=0
$DIG $DIGOPTS +tcp +noall +answer dnskey ttl1.example. @10.53.0.3 >dig.out.ns3.test$n || ret=1
[ -s dig.out.ns3.test$n ] || ret=1
(awk 'BEGIN {r=0} $2 != 300 {r=1; print "found TTL " $2} END {exit r}' dig.out.ns3.test$n | cat_i) || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TTLs of imported DNSKEYs (with default) ($n)"
ret=0
$DIG $DIGOPTS +tcp +noall +answer dnskey ttl2.example. @10.53.0.3 >dig.out.ns3.test$n || ret=1
[ -s dig.out.ns3.test$n ] || ret=1
(awk 'BEGIN {r=0} $2 != 60 {r=1; print "found TTL " $2} END {exit r}' dig.out.ns3.test$n | cat_i) || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TTLs of imported DNSKEYs (mismatched) ($n)"
ret=0
$DIG $DIGOPTS +tcp +noall +answer dnskey ttl3.example. @10.53.0.3 >dig.out.ns3.test$n || ret=1
[ -s dig.out.ns3.test$n ] || ret=1
(awk 'BEGIN {r=0} $2 != 30 {r=1; print "found TTL " $2} END {exit r}' dig.out.ns3.test$n | cat_i) || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TTLs of imported DNSKEYs (existing RRset) ($n)"
ret=0
$DIG $DIGOPTS +tcp +noall +answer dnskey ttl4.example. @10.53.0.3 >dig.out.ns3.test$n || ret=1
[ -s dig.out.ns3.test$n ] || ret=1
(awk 'BEGIN {r=0} $2 != 30 {r=1; print "found TTL " $2} END {exit r}' dig.out.ns3.test$n | cat_i) || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking positive validation NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.example. @10.53.0.2 a >dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth a.example. @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking positive validation NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth a.nsec3.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.nsec3.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking positive validation OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth a.optout.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.optout.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking negative validation NXDOMAIN NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth q.example. @10.53.0.2 a >dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth q.example. @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking negative validation NXDOMAIN NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth q.nsec3.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth q.nsec3.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking negative validation NXDOMAIN OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth q.optout.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth q.optout.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n >/dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking negative validation NODATA NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.example. @10.53.0.2 txt >dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth a.example. @10.53.0.4 txt >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking negative validation NODATA NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth a.nsec3.example. \
  @10.53.0.3 txt >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.nsec3.example. \
  @10.53.0.4 txt >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking negative validation NODATA OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth a.optout.example. \
  @10.53.0.3 txt >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.optout.example. \
  @10.53.0.4 txt >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Check the insecure.example domain

echo_i "checking 1-server insecurity proof NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.insecure.example. @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.insecure.example. @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking 1-server negative insecurity proof NSEC ($n)"
ret=0
$DIG $DIGOPTS q.insecure.example. a @10.53.0.3 \
  >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS q.insecure.example. a @10.53.0.4 \
  >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n >/dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Check the secure.example domain

echo_i "checking multi-stage positive validation NSEC/NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.secure.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.secure.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking multi-stage positive validation NSEC/NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth a.nsec3.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.nsec3.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking multi-stage positive validation NSEC/OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth a.optout.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.optout.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking multi-stage positive validation NSEC3/NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.secure.nsec3.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.secure.nsec3.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking multi-stage positive validation NSEC3/NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth a.nsec3.nsec3.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.nsec3.nsec3.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking multi-stage positive validation NSEC3/OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth a.optout.nsec3.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.optout.nsec3.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking multi-stage positive validation OPTOUT/NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.secure.optout.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.secure.optout.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking multi-stage positive validation OPTOUT/NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth a.nsec3.optout.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.nsec3.optout.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking multi-stage positive validation OPTOUT/OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth a.optout.optout.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.optout.optout.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking empty NODATA OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth empty.optout.example. \
  @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth empty.optout.example. \
  @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
#grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Check the insecure.secure.example domain (insecurity proof)

echo_i "checking 2-server insecurity proof ($n)"
ret=0
$DIG $DIGOPTS +noauth a.insecure.secure.example. @10.53.0.2 a \
  >dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth a.insecure.secure.example. @10.53.0.4 a \
  >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Check a negative response in insecure.secure.example

echo_i "checking 2-server insecurity proof with a negative answer ($n)"
ret=0
$DIG $DIGOPTS q.insecure.secure.example. @10.53.0.2 a >dig.out.ns2.test$n \
  || ret=1
$DIG $DIGOPTS q.insecure.secure.example. @10.53.0.4 a >dig.out.ns4.test$n \
  || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n >/dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking security root query ($n)"
ret=0
$DIG $DIGOPTS . @10.53.0.4 key >dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking positive validation RSASHA256 NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.rsasha256.example. @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.rsasha256.example. @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking positive validation RSASHA512 NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.rsasha512.example. @10.53.0.3 a >dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.rsasha512.example. @10.53.0.4 a >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that positive validation in a privately secure zone works ($n)"
ret=0
$DIG $DIGOPTS +noauth a.private.secure.example. a @10.53.0.2 \
  >dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth a.private.secure.example. a @10.53.0.4 \
  >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that negative validation in a privately secure zone works ($n)"
ret=0
$DIG $DIGOPTS +noauth q.private.secure.example. a @10.53.0.2 \
  >dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth q.private.secure.example. a @10.53.0.4 \
  >dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "NXDOMAIN" dig.out.ns4.test$n >/dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking privately secure to nxdomain works ($n)"
ret=0
$DIG $DIGOPTS +noauth private2secure-nxdomain.private.secure.example. SOA @10.53.0.4 >dig.out.ns4.test$n || ret=1
grep "NXDOMAIN" dig.out.ns4.test$n >/dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Try validating with a revoked trusted key.
# This should fail.

echo_i "checking that validation returns insecure due to revoked trusted key ($n)"
ret=0
$DIG $DIGOPTS example. soa @10.53.0.5 >dig.out.ns5.test$n || ret=1
grep "flags:.*; QUERY" dig.out.ns5.test$n >/dev/null || ret=1
grep "flags:.* ad.*; QUERY" dig.out.ns5.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that revoked key is present ($n)"
ret=0
id=$(cat rev.key)
$DIG $DIGOPTS +multi dnskey . @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep '; key id = '"$id"'$' dig.out.ns1.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that revoked key self-signs ($n)"
ret=0
id=$(cat rev.key)
$DIG $DIGOPTS dnskey . @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep 'RRSIG.*'" $id "'\. ' dig.out.ns1.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking for unpublished key ($n)"
ret=0
id=$(keyfile_to_key_id "$(cat unpub.key)")
$DIG $DIGOPTS +multi dnskey . @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep '; key id = '"$id"'$' dig.out.ns1.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking for activated but unpublished key ($n)"
ret=0
id=$(keyfile_to_key_id "$(cat activate-now-publish-1day.key)")
$DIG $DIGOPTS +multi dnskey . @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep '; key id = '"$id"'$' dig.out.ns1.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that standby key does not sign records ($n)"
ret=0
id=$(keyfile_to_key_id "$(cat standby.key)")
$DIG $DIGOPTS dnskey . @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep 'RRSIG.*'" $id "'\. ' dig.out.ns1.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that deactivated key does not sign records ($n)"
ret=0
id=$(keyfile_to_key_id "$(cat inact.key)")
$DIG $DIGOPTS dnskey . @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep 'RRSIG.*'" $id "'\. ' dig.out.ns1.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking insertion of public-only key ($n)"
ret=0
id=$(keyfile_to_key_id "$(cat nopriv.key)")
file="ns1/$(cat nopriv.key).key"
keydata=$(grep DNSKEY $file)
$NSUPDATE >/dev/null 2>&1 <<END || status=1
server 10.53.0.1 ${PORT}
zone .
ttl 3600
update add $keydata
send
END
sleep 1
$DIG $DIGOPTS dnskey . @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep 'RRSIG.*'" $id "'\. ' dig.out.ns1.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking key deletion ($n)"
ret=0
id=$(keyfile_to_key_id "$(cat del.key)")
$DIG $DIGOPTS +multi dnskey . @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep '; key id = '"$id"'$' dig.out.ns1.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that serial number and RRSIGs are both updated (rt21045) ($n)"
ret=0
$DIG $DIGOPTS +short soa prepub.example @10.53.0.3 >dig.out.ns3.test$n || ret=1
oldserial=$(cat dig.out.ns3.test$n | awk '$0 !~ /SOA/ {print $3}')
oldinception=$(cat dig.out.ns3.test$n | awk '/SOA/ {print $6}' | sort -u)
id=$(keyfile_to_key_id "$(cat prepub.key)")
echo_i "prepublish key for ZSK $id"
($RNDCCMD 10.53.0.3 dnssec -rollover -key $id prepub.example 2>&1 | sed 's/^/ns3 /' | cat_i) || ret=1
newserial=$oldserial
try=0
while [ $oldserial -eq $newserial -a $try -lt 42 ]; do
  $DIG $DIGOPTS +short soa prepub.example @10.53.0.3 >dig.out.ns3.test$n.2 || true
  newserial=$(cat dig.out.ns3.test$n.2 | awk '$0 !~ /SOA/ {print $3}')
  sleep 1
  try=$((try + 1))
done
newinception=$(cat dig.out.ns3.test$n.2 | awk '/SOA/ {print $6}' | sort -u)
#echo "$oldserial : $newserial"
#echo "$oldinception : $newinception"

[ "$oldserial" = "$newserial" ] && ret=1
[ "$oldinception" = "$newinception" ] && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "preparing ZSK roll"
starttime=$($PERL -e 'print time(), "\n";')
oldfile=$(cat active.key)
oldid=$(keyfile_to_key_id "$(cat active.key)")
newfile=$(cat standby.key)
newid=$(keyfile_to_key_id "$(cat standby.key)")
$SETTIME -K ns1 -I now -D now+25 $oldfile >settime.out.test$n.1 || ret=1
$SETTIME -K ns1 -i 0 -S $oldfile $newfile >settime.out.test$n.2 || ret=1

# note previous zone serial number
oldserial=$($DIG $DIGOPTS +short soa . @10.53.0.1 | awk '{print $3}')

($RNDCCMD 10.53.0.1 freeze . 2>&1 | sed 's/^/ns1 /' | cat_i) || ret=1
cp ns1/root.db.signed ns1/root.db.1
$SIGNER -S -o . -O full -K ns1 -f ns1/root.db.signed ns1/root.db.1 >signing.root.out$n 2>&1 || ret=1
($RNDCCMD 10.53.0.1 thaw . 2>&1 | sed 's/^/ns1 /' | cat_i) || ret=1
sleep 4

echo_i "revoking key to duplicated key ID"
$SETTIME -R now -K ns2 Kbar.+013+59973.key >settime.out.test$n.3 || ret=1

($RNDCCMD 10.53.0.2 freeze bar. 2>&1 | sed 's/^/ns2 /' | cat_i) || ret=1
cp ns2/bar.db.signed ns2/bar.db
$SIGNER -S -o bar. -O full -K ns2 ns2/bar.db >signing.bar.out$n 2>&1 || ret=1
($RNDCCMD 10.53.0.2 thaw bar. 2>&1 | sed 's/^/ns2 /' | cat_i) || ret=1

echo_i "waiting for changes to take effect"
sleep 5

echo_i "checking former standby key $newid is now active ($n)"
ret=0
$DIG $DIGOPTS dnskey . @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep 'RRSIG.*'" $newid "'\. ' dig.out.ns1.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking former standby key has only signed incrementally ($n)"
ret=0
$DIG $DIGOPTS txt . @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep 'RRSIG.*'" $newid "'\. ' dig.out.ns1.test$n >/dev/null && ret=1
grep 'RRSIG.*'" $oldid "'\. ' dig.out.ns1.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that signing records have been marked as complete ($n)"
ret=0
checkprivate example 10.53.0.2 0 type65280 || ret=1      # sig-signing-type 65280
checkprivate private.secure.example 10.53.0.3 2 || ret=1 # pre-signed
checkprivate nsec3.example 10.53.0.3 || ret=1
checkprivate nsec3.nsec3.example 10.53.0.3 || ret=1
checkprivate nsec3.optout.example 10.53.0.3 || ret=1
checkprivate nsec3-to-nsec.example 10.53.0.3 2 || ret=1  # automatically removed
checkprivate nsec3-to-nsec3.example 10.53.0.3 2 || ret=1 # automatically removed
if [ $RSASHA1_SUPPORTED = 1 ]; then
  checkprivate nsec-only.example 10.53.0.3 || ret=1
fi
checkprivate oldsigs.example 10.53.0.3 2 || ret=1 # pre-signed
checkprivate optout.example 10.53.0.3 || ret=1
checkprivate optout.nsec3.example 10.53.0.3 || ret=1
checkprivate optout.optout.example 10.53.0.3 || ret=1
checkprivate prepub.example 10.53.0.3 2 || ret=1 # pre-signed
checkprivate rsasha256.example 10.53.0.3 || ret=1
checkprivate rsasha512.example 10.53.0.3 || ret=1
checkprivate secure.example 10.53.0.3 || ret=1
checkprivate secure.nsec3.example 10.53.0.3 || ret=1
checkprivate secure.optout.example 10.53.0.3 || ret=1
checkprivate ttl1.example 10.53.0.3 || ret=1
checkprivate ttl2.example 10.53.0.3 || ret=1
checkprivate ttl3.example 10.53.0.3 || ret=1
checkprivate ttl4.example 10.53.0.3 || ret=1
n=$((n + 1))
status=$((status + ret))

echo_i "forcing full sign ($n)"
ret=0
($RNDCCMD 10.53.0.1 freeze . 2>&1 | sed 's/^/ns1 /' | cat_i) || ret=1
cp ns1/root.db.signed ns1/root.db.2
$SIGNER -S -o . -O full -K ns1 -f ns1/root.db.signed ns1/root.db >signing.root.out$n 2>&1 || ret=1
($RNDCCMD 10.53.0.1 thaw . 2>&1 | sed 's/^/ns1 /' | cat_i) || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi

echo_i "waiting for change to take effect"
sleep 5

echo_i "checking former standby key $newid has now signed fully ($n)"
ret=0
$DIG $DIGOPTS txt . @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep 'RRSIG.*'" $newid "'\. ' dig.out.ns1.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking SOA serial number has been incremented ($n)"
ret=0
newserial=$($DIG $DIGOPTS +short soa . @10.53.0.1 | awk '{print $3}')
[ "$newserial" != "$oldserial" ] || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "preparing to test key change corner cases"
echo_i "removing a private key file"
file="ns1/$(cat vanishing.key).private"
rm -f $file

echo_i "checking delayed key publication/activation ($n)"
ret=0
zsk=$(cat delayzsk.key)
ksk=$(cat delayksk.key)
# publication and activation times should be unset
$SETTIME -K ns3 -pA -pP $zsk >settime.out.test$n.zsk || ret=1
grep -v UNSET settime.out.test$n.zsk >/dev/null && ret=1
$SETTIME -K ns3 -pA -pP $ksk >settime.out.test$n.ksk || ret=1
grep -v UNSET settime.out.test$n.ksk >/dev/null && ret=1
$DIG $DIGOPTS +noall +answer dnskey delay.example. @10.53.0.3 >dig.out.ns3.test$n || ret=1
# DNSKEY not expected:
awk 'BEGIN {r=1} $4=="DNSKEY" {r=0} END {exit r}' dig.out.ns3.test$n && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking scheduled key publication, not activation ($n)"
ret=0
# Ensure initial zone is loaded.
wait_for_notifies "delay.example" "ns3" || ret=1
$SETTIME -K ns3 -P now -A none $zsk >settime.out.test$n.zsk || ret=1
$SETTIME -K ns3 -P now -A none $ksk >settime.out.test$n.ksk || ret=1
cp ns3/delay.example.db ns3/delay.example.1
# This will create a non valid DNSSEC zone, disable post-sign verification
$SIGNER -P -S -o delay.example. -O full -N increment -K ns3 -f ns3/delay.example.db ns3/delay.example.1 >signing.delay.out.$n 2>&1 || ret=1
($RNDCCMD 10.53.0.3 reload delay.example | sed 's/^/ns3 /' | cat_i) || ret=1
check_has_dnskey() {
  $DIG $DIGOPTS +noall +answer dnskey delay.example. @10.53.0.3 >dig.out.ns3.test$n || return 1
  # DNSKEY expected:
  awk 'BEGIN {r=1} $4=="DNSKEY" {r=0} END {exit r}' dig.out.ns3.test$n || return 1
  # RRSIG not expected:
  awk 'BEGIN {r=1} $4=="RRSIG" {r=0} END {exit r}' dig.out.ns3.test$n && return 1
  return 0
}
retry_quiet 5 check_has_dnskey || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking scheduled key activation ($n)"
ret=0
sleep 1 # ensure file system time stamp of ns3/delay.example.db changes
$SETTIME -K ns3 -A now $zsk >settime.out.test$n.zsk || ret=1
$SETTIME -K ns3 -A now $ksk >settime.out.test$n.ksk || ret=1
cp ns3/delay.example.db ns3/delay.example.2
$SIGNER -S -o delay.example. -O full -N increment -K ns3 -f ns3/delay.example.db ns3/delay.example.2 >signing.delay.out.$n 2>&1 || ret=1
($RNDCCMD 10.53.0.3 reload delay.example | sed 's/^/ns3 /' | cat_i) || ret=1
check_is_signed() {
  $DIG $DIGOPTS +noall +answer dnskey delay.example. @10.53.0.3 >dig.out.ns3.1.test$n || return 1
  # DNSKEY expected:
  awk 'BEGIN {r=1} $4=="DNSKEY" {r=0} END {exit r}' dig.out.ns3.1.test$n || return 1
  # RRSIG expected:
  awk 'BEGIN {r=1} $4=="RRSIG" {r=0} END {exit r}' dig.out.ns3.1.test$n || return 1
  $DIG $DIGOPTS +noall +answer a a.delay.example. @10.53.0.3 >dig.out.ns3.2.test$n || return 1
  # A expected:
  awk 'BEGIN {r=1} $4=="A" {r=0} END {exit r}' dig.out.ns3.2.test$n || return 1
  # RRSIG expected:
  awk 'BEGIN {r=1} $4=="RRSIG" {r=0} END {exit r}' dig.out.ns3.2.test$n || return 1
  return 0
}
retry_quiet 5 check_is_signed || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking former active key was removed ($n)"
#
# Work out how long we need to sleep. Allow 4 seconds for the records
# to be removed.
#
now=$($PERL -e 'print time(), "\n";')
sleep=$((starttime + 29 - now))
case $sleep in
  -* | 0) ;;
  *)
    echo_i "waiting $sleep seconds for timer to have activated"
    sleep $sleep
    ;;
esac
ret=0
($RNDCCMD 10.53.0.1 freeze . 2>&1 | sed 's/^/ns1 /' | cat_i) || ret=1
cp ns1/root.db.signed ns1/root.db.3
$SIGNER -v 3 -S -o . -O full -K ns1 -f ns1/root.db.signed ns1/root.db.3 >signing.root.out$n 2>&1 || ret=1
($RNDCCMD 10.53.0.1 thaw . 2>&1 | sed 's/^/ns1 /' | cat_i) || ret=1
$DIG $DIGOPTS +multi dnskey . @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep '; key id = '"$oldid"'$' dig.out.ns1.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

id=$(keyfile_to_key_id "$(cat vanishing.key)")
echo_i "checking private key file $id removal caused no immediate harm ($n)"
ret=0
id=$(keyfile_to_key_id "$(cat vanishing.key)")
$DIG $DIGOPTS soa . @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep 'RRSIG.*'" $id "'\. ' dig.out.ns1.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking revoked key with duplicate key ID ($n)"
ret=0
id=59973
rid=60101
$DIG $DIGOPTS +multi dnskey bar @10.53.0.2 >dig.out.ns2.test$n || ret=1
grep '; key id = '"$id"'$' dig.out.ns2.test$n >/dev/null && ret=1
keys=$(grep '; key id = '"$rid"'$' dig.out.ns2.test$n | wc -l)
test $keys -eq 2 || ret=1
$DIG $DIGOPTS dnskey bar @10.53.0.4 >dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking key event timers are always set ($n)"
ret=0
# this is a regression test for a bug in which the next key event could
# be scheduled for the present moment, and then never fire.  check for
# visible evidence of this error in the logs:
awk '/next key event/ {if ($1 == $8 && $2 == $9) exit 1}' */named.run || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "forcing full sign with unreadable keys ($n)"
ret=0
chmod 0 ns1/K.+*+*.key ns1/K.+*+*.private || ret=1
($RNDCCMD 10.53.0.1 sign . 2>&1 | sed 's/^/ns1 /' | cat_i) || ret=1
$DIG $DIGOPTS . @10.53.0.1 dnskey >dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "test turning on dnssec-policy during reconfig ($n)"
ret=0
# first create a zone that doesn't have dnssec-policy
($RNDCCMD 10.53.0.3 addzone reconf.example '{ type primary; file "reconf.example.db"; };' 2>&1 | sed 's/^/ns3 /' | cat_i) || ret=1
rekey_calls=$(grep "zone reconf.example.*next key event" ns3/named.run | wc -l)
[ "$rekey_calls" -eq 0 ] || ret=1
# ...then we add dnssec-policy and reconfigure
($RNDCCMD 10.53.0.3 modzone reconf.example '{ type primary; file "reconf.example.db"; allow-update { any; }; dnssec-policy autosign; };' 2>&1 | sed 's/^/ns3 /' | cat_i) || ret=1
rndc_reconfig ns3 10.53.0.3
for i in 0 1 2 3 4 5 6 7 8 9; do
  lret=0
  rekey_calls=$(grep "zone reconf.example.*next key event" ns3/named.run | wc -l)
  [ "$rekey_calls" -gt 0 ] || lret=1
  if [ "$lret" -eq 0 ]; then break; fi
  echo_i "waiting ... ($i)"
  sleep 1
done
n=$((n + 1))
if [ "$lret" != 0 ]; then ret=$lret; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "test CDS and CDNSKEY auto generation ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 sync.example cds >dig.out.ns3.cdstest$n || ret=1
$DIG $DIGOPTS @10.53.0.3 sync.example cdnskey >dig.out.ns3.cdnskeytest$n || ret=1
grep -i "sync.example.*in.cds.*[1-9][0-9]* " dig.out.ns3.cdstest$n >/dev/null || ret=1
grep -i "sync.example.*in.cdnskey.*257 " dig.out.ns3.cdnskeytest$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "test 'csk' affects DNSKEY/CDS/CDNSKEY ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 sync.example dnskey >dig.out.ns3.dnskeytest$n || ret=1
$DIG $DIGOPTS @10.53.0.3 sync.example cdnskey >dig.out.ns3.cdnskeytest$n || ret=1
$DIG $DIGOPTS @10.53.0.3 sync.example cds >dig.out.ns3.cdstest$n || ret=1
lines=$(awk '$4 == "RRSIG" && $5 == "DNSKEY" {print}' dig.out.ns3.dnskeytest$n | wc -l)
test ${lines:-0} -eq 2 || ret=1
lines=$(awk '$4 == "RRSIG" && $5 == "CDNSKEY" {print}' dig.out.ns3.cdnskeytest$n | wc -l)
test ${lines:-0} -eq 2 || ret=1
lines=$(awk '$4 == "RRSIG" && $5 == "CDS" {print}' dig.out.ns3.cdstest$n | wc -l)
test ${lines:-0} -eq 2 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "test 'ksk' affects DNSKEY/CDS/CDNSKEY ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 kskonly.example dnskey >dig.out.ns3.dnskeytest$n || ret=1
$DIG $DIGOPTS @10.53.0.3 kskonly.example cdnskey >dig.out.ns3.cdnskeytest$n || ret=1
$DIG $DIGOPTS @10.53.0.3 kskonly.example cds >dig.out.ns3.cdstest$n || ret=1
lines=$(awk '$4 == "RRSIG" && $5 == "DNSKEY" {print}' dig.out.ns3.dnskeytest$n | wc -l)
test ${lines:-0} -eq 1 || ret=1
lines=$(awk '$4 == "RRSIG" && $5 == "CDNSKEY" {print}' dig.out.ns3.cdnskeytest$n | wc -l)
test ${lines:-0} -eq 1 || ret=1
lines=$(awk '$4 == "RRSIG" && $5 == "CDS" {print}' dig.out.ns3.cdstest$n | wc -l)
test ${lines:-0} -eq 1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# XXXWMM Replace this test with dnssec-policy equivalent once we have
# implemented 'cdnskey "only-during-rollovers";'.
#echo_i "setting CDS and CDNSKEY deletion times and calling 'rndc loadkeys'"
#$SETTIME -D sync now $(cat sync.key) > settime.out.test$n || ret=1
#($RNDCCMD 10.53.0.3 loadkeys sync.example | sed 's/^/ns3 /' | cat_i) || ret=1
#
#echo_i "checking that the CDS and CDNSKEY are deleted ($n)"
#ret=0
#ensure_cds_and_cdnskey_are_deleted() {
#	$DIG $DIGOPTS @10.53.0.3 sync.example. CDS > dig.out.ns3.cdstest$n || return 1
#	awk '$1 == "sync.example." && $4 == "CDS" { exit 1; }' dig.out.ns3.cdstest$n || return 1
#	$DIG $DIGOPTS @10.53.0.3 sync.example. CDNSKEY > dig.out.ns3.cdnskeytest$n || return 1
#	awk '$1 == "sync.example." && $4 == "CDNSKEY" { exit 1; }' dig.out.ns3.cdnskeytest$n || return 1
#}
#retry 10 ensure_cds_and_cdnskey_are_deleted || ret=1
#n=$((n + 1))
#if [ $ret != 0 ]; then echo_i "failed"; fi
#status=$((status + ret))

echo_i "check that dnssec-settime -p Dsync works ($n)"
ret=0
$SETTIME -p Dsync $(cat sync.key) >settime.out.test$n || ret=1
grep "SYNC Delete:" settime.out.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that dnssec-settime -p Psync works ($n)"
ret=0
$SETTIME -p Psync $(cat sync.key) >settime.out.test$n || ret=1
grep "SYNC Publish:" settime.out.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that zone with inactive ZSK and active KSK is properly autosigned ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 axfr inaczsk2.example >dig.out.ns3.test$n || ret=1
grep "SOA ${DEFAULT_ALGORITHM_NUMBER} 2" dig.out.ns3.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking for out-of-zone NSEC3 records after ZSK removal ($n)"
ret=0
# Delete the ZSK
file="ns3/inactive/$(cat delzsk.key).key"
$NSUPDATE >nsupdate.out.test$n 2>&1 <<END
server 10.53.0.3 ${PORT}
zone delzsk.example.
update del $(cat $file | grep -v ";.*")
send
END

zsk_is_gone() {
  $DIG $DIGOPTS +noall +multi +answer dnskey delzsk.example. @10.53.0.3 >dig.out.ns3.test$n || return 1
  grep '; key id = '"$oldid"'$' dig.out.ns3.test$n >/dev/null && return 1
  return 0
}
retry_quiet 5 zsk_is_gone || ret=1
if [ $ret -ne 0 ]; then
  echo_i "timed out waiting for key removal"
fi

# Check whether key removal caused NSEC3 records to be erroneously created for
# glue records due to a secure delegation already being signed by the active key
# (i.e. a key other than the one being removed but using the same algorithm).
#
# For reference:
#
#     $ nsec3hash 12345678 1 10 ns.sub.delzsk.example.
#     589R358VSPJUFVAJU949JPVF74D9PTGH (salt=12345678, hash=1, iterations=10)
#
$DIG $DIGOPTS delzsk.example AXFR @10.53.0.3 >dig.out.ns3.3.test$n || ret=1
grep "589R358VSPJUFVAJU949JPVF74D9PTGH" dig.out.ns3.3.test$n >/dev/null 2>&1 && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that DNAME at apex with NSEC3 is correctly signed (dnssec-policy) ($n)"
ret=0
$DIG $DIGOPTS txt dname-at-apex-nsec3.example @10.53.0.3 >dig.out.ns3.test$n || ret=1
grep "RRSIG NSEC3 ${DEFAULT_ALGORITHM_NUMBER} 3 600" dig.out.ns3.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that DNAME is not treated as a delegation when signing ($n)"
ret=0
$DIG $DIGOPTS dname-and-txt.secure.example. DNAME @10.53.0.3 >dig.out.ns3.1.test$n || ret=1
grep "dname-and-txt.secure.example.*RRSIG.*DNAME" dig.out.ns3.1.test$n >/dev/null 2>&1 || ret=1
$DIG $DIGOPTS dname-and-txt.secure.example. TXT @10.53.0.3 >dig.out.ns3.2.test$n || ret=1
grep "dname-and-txt.secure.example.*RRSIG.*TXT" dig.out.ns3.2.test$n >/dev/null 2>&1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking key maintenance events were logged correctly ($n)"
ret=0
pub=$(grep "DNSKEY .* is now published" ns2/named.run | grep -v "CDNSKEY" | wc -l)
[ "$pub" -eq 10 ] || ret=1
act=$(grep "DNSKEY .* is now active" ns2/named.run | wc -l)
[ "$act" -eq 10 ] || ret=1
rev=$(grep "DNSKEY .* is now revoked" ns2/named.run | wc -l)
[ "$rev" -eq 0 ] || ret=1
inac=$(grep "DNSKEY .* is now inactive" ns2/named.run | wc -l)
[ "$inac" -eq 0 ] || ret=1
del=$(grep "DNSKEY .* is now deleted" ns2/named.run | wc -l)
[ "$del" -eq 0 ] || ret=1
pub=$(grep "DNSKEY .* is now published" ns3/named.run | grep -v "CDNSKEY" | wc -l)
act=$(grep "DNSKEY .* is now active" ns3/named.run | wc -l)
if [ $RSASHA1_SUPPORTED = 1 ]; then
  # Include two log lines for nsec-only zone.
  [ "$pub" -eq 53 ] || ret=1
  [ "$act" -eq 54 ] || ret=1
else
  [ "$pub" -eq 51 ] || ret=1
  [ "$act" -eq 52 ] || ret=1
fi
rev=$(grep "DNSKEY .* is now revoked" ns3/named.run | wc -l)
[ "$rev" -eq 0 ] || ret=1
# inaczsk.example
inac=$(grep "DNSKEY .* is now inactive" ns3/named.run | wc -l)
[ "$inac" -eq 1 ] || ret=1
# delzsk.example
del=$(grep "DNSKEY .* is now deleted" ns3/named.run | wc -l)
[ "$del" -eq 1 ] || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check removal of ENT NSEC3 records when delegations are removed ($n)"
zone=nsec3-with-ent
hash=M9SFFA181BCTR8D18LQUPST4N6BL304D

# check that NSEC3 for ENT is present
echo_i "check ENT NSEC3 is initially present"
ret=0
$DIG $DIGOPTS @10.53.0.2 a "ent.${zone}" >dig.out.pre.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.pre.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0, AUTHORITY: 4, " dig.out.pre.ns2.test$n >/dev/null || ret=1
grep "^${hash}.${zone}." dig.out.pre.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check ENT NSEC3 is still present after removing one of two delegations ($n)"
ret=0
# remove first delegation of two delegations, NSEC3 for ENT should remain.
(
  echo zone $zone
  echo server 10.53.0.2 "$PORT"
  echo update del sub1.ent.$zone NS
  echo send
) | $NSUPDATE
# check that NSEC3 for ENT is still present
$DIG $DIGOPTS @10.53.0.2 a "ent.${zone}" >dig.out.pre.ns2.test$n || ret=1
$DIG $DIGOPTS @10.53.0.2 a "ent.${zone}" >dig.out.mid.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.mid.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0, AUTHORITY: 4, " dig.out.mid.ns2.test$n >/dev/null || ret=1
grep "^${hash}.${zone}." dig.out.mid.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check ENT NSEC3 is gone after removing the second delegation ($n)"
ret=0
# remove second delegation of two delegations, NSEC3 for ENT should be deleted.
(
  echo zone $zone
  echo server 10.53.0.2 "$PORT"
  echo update del sub2.ent.$zone NS
  echo send
) | $NSUPDATE
# check that NSEC3 for ENT is gone
echo_i "check ENT NSEC3 is gone for zone $zone hash $hash"
$DIG $DIGOPTS @10.53.0.2 a "ent.${zone}" >dig.out.post.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.post.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0, AUTHORITY: 4, " dig.out.post.ns2.test$n >/dev/null || ret=1
grep "^${hash}.${zone}." dig.out.post.ns2.test$n >/dev/null && ret=1
$DIG $DIGOPTS @10.53.0.2 axfr "${zone}" >dig.out.axfr.ns2.test$n || ret=1
grep "^${hash}.${zone}." dig.out.axfr.ns2.test$n >/dev/null && ret=1
n=$((n + 1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that the startup change from NSEC3 to NSEC is properly signed ($n)"
ret=0
$JOURNALPRINT ns3/nsec3-to-nsec.example.db.jnl \
  | awk 'BEGIN { private=0; rrsig=0; ok=0 }
$1 == "del" && $5 == "SOA" { if (private || rrsig) { if (private == rrsig) { exit(0); } else { exit(1); } } }
$1 == "add" && $5 == "TYPE65534" { private=1 }
$1 == "add" && $5 == "RRSIG" && $6 == "TYPE65534" { rrsig=1 }
END { if (private || rrsig) { if (private == rrsig) { exit(0); } else { exit(1); } } else { exit (1); } }
' || ret=1
n=$((n + 1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that NSEC3 to NSEC builds the NSEC chain first ($n)"
ret=0
$JOURNALPRINT ns3/nsec3-to-nsec.example.db.jnl \
  | awk 'BEGIN { nsec3param=0; nsec=0 }
$1 == "del" && $5 == "SOA" { if (nsec3param || nsec) { if (nsec3param && !nsec) { exit(1); } else { exit(0); } } }
$1 == "del" && $5 == "NSEC3PARAM" { nsec3param=1 }
$1 == "add" && $2 == "nsec3-to-nsec.example." && $5 == "NSEC" { nsec=1 }
END { if (nsec3param || nsec) { if (nsec3param && !nsec) { exit(1); } else { exit(0); } } else { exit(1); } }
' || ret=1
n=$((n + 1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that NSEC3 to NSEC3 builds the new NSEC3 chain first ($n)"
ret=0
$JOURNALPRINT ns3/nsec3-to-nsec3.example.db.jnl \
  | awk 'BEGIN { addnsec3param=0; delnsec3param=0; nsec3=0 }
$1 == "del" && $5 == "SOA" { if (delnsec3param || nsec3 || addnsec3param) { if (delnsec3param && (!nsec3 || !addnsec3param)) { exit(1); } else { exit(0); } } }
$1 == "del" && $5 == "NSEC3PARAM" { delnsec3param=1 }
$1 == "add" && $5 == "NSEC3PARAM" { addnsec3param=1 }
$1 == "add" && $5 == "NSEC3" { nsec3=1 }
END { if (delnsec3param || nsec3 || addnsec3param) { if (delnsec3param && (!nsec3 || !addnsec3param)) { exit(1); } else { exit(0); } } else { exit(1); } }
' || ret=1
n=$((n + 1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
