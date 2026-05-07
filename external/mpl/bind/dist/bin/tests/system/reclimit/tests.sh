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

dig_with_opts() {
  $DIG -p "${PORT}" +retries=0 "$@"
}

status=0
n=0

ns3_reset() {
  $RNDC -c ../_common/rndc.conf -s 10.53.0.3 -p ${CONTROLPORT} reconfig 2>&1 | sed 's/^/I:ns3 /'
  $RNDC -c ../_common/rndc.conf -s 10.53.0.3 -p ${CONTROLPORT} flush | sed 's/^/I:ns3 /'
}

ns3_flush() {
  $RNDC -c ../_common/rndc.conf -s 10.53.0.3 -p ${CONTROLPORT} flush | sed 's/^/I:ns3 /'
}

ns3_sends_aaaa_queries() {
  if grep "started AAAA fetch" ns3/named.run >/dev/null; then
    return 0
  else
    return 1
  fi
}

# Check whether the number of queries ans2 received from ns3 (this value is
# read from dig output stored in file $1) is as expected.  The expected query
# count is variable:
#   - if ns3 sends AAAA queries, the query count should equal $2,
#   - if ns3 does not send AAAA queries, the query count should equal $3.
check_query_count() {
  count1=$(sed 's/[^0-9]//g;' $1)
  count2=$(sed 's/[^0-9]//g;' $2)
  count=$((count1 + count2))
  #echo_i "count1=$count1 count2=$count2 count=$count"
  expected_count_with_aaaa=$3
  expected_count_without_aaaa=$4

  if ns3_sends_aaaa_queries; then
    expected_count=$expected_count_with_aaaa
  else
    expected_count=$expected_count_without_aaaa
  fi

  if [ $count -ne $expected_count ]; then
    echo_i "count $count (actual) != $expected_count (expected)"
    ret=1
  fi
}

echo_i "set max-recursion-depth=12"

n=$((n + 1))
echo_i "attempt excessive-depth lookup ($n)"
ret=0
echo "1000" >ans2/ans.limit
echo "1000" >ans4/ans.limit
dig_with_opts @10.53.0.2 reset >/dev/null || ret=1
dig_with_opts @10.53.0.4 reset >/dev/null || ret=1
dig_with_opts @10.53.0.3 indirect1.example.org >dig.out.1.test$n || ret=1
grep "status: SERVFAIL" dig.out.1.test$n >/dev/null || ret=1
dig_with_opts +short @10.53.0.2 count txt >dig.out.2.test$n || ret=1
dig_with_opts +short @10.53.0.4 count txt >dig.out.4.test$n || ret=1
check_query_count dig.out.2.test$n dig.out.4.test$n 27 14
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "attempt permissible lookup ($n)"
ret=0
echo "12" >ans2/ans.limit
echo "12" >ans4/ans.limit
ns3_reset
dig_with_opts @10.53.0.2 reset >/dev/null || ret=1
dig_with_opts @10.53.0.4 reset >/dev/null || ret=1
dig_with_opts @10.53.0.3 indirect2.example.org >dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n >/dev/null || ret=1
dig_with_opts +short @10.53.0.2 count txt >dig.out.2.test$n || ret=1
dig_with_opts +short @10.53.0.4 count txt >dig.out.4.test$n || ret=1
check_query_count dig.out.2.test$n dig.out.4.test$n 50 26
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "set max-recursion-depth=5"

n=$((n + 1))
echo_i "attempt excessive-depth lookup ($n)"
ret=0
echo "12" >ans2/ans.limit
cp ns3/named2.conf ns3/named.conf
ns3_reset
dig_with_opts @10.53.0.2 reset >/dev/null || ret=1
dig_with_opts @10.53.0.4 reset >/dev/null || ret=1
dig_with_opts @10.53.0.3 indirect3.example.org >dig.out.1.test$n || ret=1
grep "status: SERVFAIL" dig.out.1.test$n >/dev/null || ret=1
dig_with_opts +short @10.53.0.2 count txt >dig.out.2.test$n || ret=1
dig_with_opts +short @10.53.0.4 count txt >dig.out.4.test$n || ret=1
check_query_count dig.out.2.test$n dig.out.4.test$n 13 7
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "attempt permissible lookup ($n)"
ret=0
echo "5" >ans2/ans.limit
echo "5" >ans4/ans.limit
ns3_reset
dig_with_opts @10.53.0.2 reset >/dev/null || ret=1
dig_with_opts @10.53.0.4 reset >/dev/null || ret=1
dig_with_opts @10.53.0.3 indirect4.example.org >dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n >/dev/null || ret=1
dig_with_opts +short @10.53.0.2 count txt >dig.out.2.test$n || ret=1
dig_with_opts +short @10.53.0.4 count txt >dig.out.4.test$n || ret=1
check_query_count dig.out.2.test$n dig.out.4.test$n 22 12
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "set max-recursion-depth=100, max-recursion-queries=50"

n=$((n + 1))
echo_i "attempt excessive-queries lookup ($n)"
ret=0
echo "13" >ans2/ans.limit
echo "13" >ans4/ans.limit
cp ns3/named3.conf ns3/named.conf
ns3_reset
dig_with_opts @10.53.0.2 reset >/dev/null || ret=1
dig_with_opts @10.53.0.4 reset >/dev/null || ret=1
dig_with_opts @10.53.0.3 indirect5.example.org >dig.out.1.test$n || ret=1
if ns3_sends_aaaa_queries; then
  grep "status: SERVFAIL" dig.out.1.test$n >/dev/null || ret=1
fi
dig_with_opts +short @10.53.0.2 count txt >dig.out.2.test$n || ret=1
dig_with_opts +short @10.53.0.4 count txt >dig.out.4.test$n || ret=1
eval count=$(cat dig.out.2.test$n)
[ $count -le 50 ] || {
  ret=1
  echo_i "count ($count) !<= 50"
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "attempt permissible lookup ($n)"
ret=0
echo "12" >ans2/ans.limit
ns3_reset
dig_with_opts @10.53.0.2 reset >/dev/null || ret=1
dig_with_opts @10.53.0.3 indirect6.example.org >dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n >/dev/null || ret=1
dig_with_opts +short @10.53.0.2 count txt >dig.out.2.test$n || ret=1
eval count=$(cat dig.out.2.test$n)
[ $count -le 50 ] || {
  ret=1
  echo_i "count ($count) !<= 50"
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "set max-recursion-depth=100, max-recursion-queries=40"

n=$((n + 1))
echo_i "attempt excessive-queries lookup ($n)"
ret=0
echo "11" >ans2/ans.limit
cp ns3/named4.conf ns3/named.conf
ns3_reset
dig_with_opts @10.53.0.2 reset >/dev/null || ret=1
dig_with_opts @10.53.0.3 indirect7.example.org >dig.out.1.test$n || ret=1
if ns3_sends_aaaa_queries; then
  grep "status: SERVFAIL" dig.out.1.test$n >/dev/null || ret=1
fi
dig_with_opts +short @10.53.0.2 count txt >dig.out.2.test$n || ret=1
eval count=$(cat dig.out.2.test$n)
[ $count -le 40 ] || {
  ret=1
  echo_i "count ($count) !<= 40"
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "attempt permissible lookup ($n)"
ret=0
echo "9" >ans2/ans.limit
ns3_reset
dig_with_opts @10.53.0.2 reset >/dev/null || ret=1
dig_with_opts @10.53.0.3 indirect8.example.org >dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n >/dev/null || ret=1
dig_with_opts +short @10.53.0.2 count txt >dig.out.2.test$n || ret=1
eval count=$(cat dig.out.2.test$n)
[ $count -le 40 ] || {
  ret=1
  echo_i "count ($count) !<= 40"
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "attempting NS explosion ($n)"
ret=0
ns3_reset
dig_with_opts @10.53.0.2 reset >/dev/null || ret=1
dig_with_opts +short @10.53.0.3 ns1.1.example.net >dig.out.1.test$n || ret=1
dig_with_opts +short @10.53.0.2 count txt >dig.out.2.test$n || ret=1
eval count=$(cat dig.out.2.test$n)
[ $count -lt 50 ] || ret=1
dig_with_opts +short @10.53.0.7 count txt >dig.out.3.test$n || ret=1
eval count=$(cat dig.out.3.test$n)
[ $count -lt 50 ] || {
  ret=1
  echo_i "count ($count) !<= 50"
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking RRset that exceeds max-records-per-type ($n)"
ret=0
dig_with_opts @10.53.0.3 biganswer.big >dig.out.1.test$n || ret=1
grep 'status: SERVFAIL' dig.out.1.test$n >/dev/null || ret=1

msg="error adding 'biganswer.big/A' in './IN' (cache): too many records (must not exceed 100)"
wait_for_log 10 "$msg" ns3/named.run || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

cp ns3/named5.conf ns3/named.conf
ns3_reset
dig_with_opts @10.53.0.3 biganswer.big >dig.out.2.test$n || ret=1
grep 'status: NOERROR' dig.out.2.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

check_manytypes() (
  i=$1
  name=$2
  type=$3
  expected=$4
  exname=$5
  extype=$6
  ttl=$7
  neq_ttl=$8

  if ! dig_with_opts @10.53.0.3 IN "$type" "$name" >"dig.out.$i.$type.test$n"; then
    exit 1
  fi

  if ! grep 'status: '"${expected}"'' "dig.out.$i.$type.test$n" >/dev/null; then
    exit 1
  fi

  if [ -n "$ttl" ] && ! grep -q "^$exname.[[:space:]]*${ttl}[[:space:]]*IN[[:space:]]*$extype" "dig.out.$i.$type.test$n"; then
    exit 1
  fi

  if [ -n "${neq_ttl}" ] && grep -q "^$exname.[[:space:]]*${neq_ttl}[[:space:]]*IN[[:space:]]*$type" "dig.out.$i.$type.test$n"; then
    exit 1
  fi

  exit 0
)

n=$((n + 1))
ret=0
echo_i "checking that priority names under the max-types-per-name limit get cached ($n)"

# Query for NXDOMAIN for items on our priority list - these should get cached
for rrtype in AAAA MX NS; do
  check_manytypes 1 manytypes.big "${rrtype}" NOERROR big SOA 120 || ret=1
done
# Wait at least 1 second
for rrtype in AAAA MX NS; do
  check_manytypes 2 manytypes.big "${rrtype}" NOERROR big SOA "" 120 || ret=1
done

if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
if [ $status -ne 0 ]; then exit 1; fi

ns3_flush

n=$((n + 1))
ret=0
echo_i "checking that NXDOMAIN names under the max-types-per-name limit get cached ($n)"

# Query for 10 NXDOMAIN types
for ntype in $(seq 65270 65279); do
  check_manytypes 1 manytypes.big "TYPE${ntype}" NOERROR big SOA 120 || ret=1
done
# Wait at least 1 second
sleep 1
# Query for 10 NXDOMAIN types again - these should be cached
for ntype in $(seq 65270 65279); do
  check_manytypes 2 manytypes.big "TYPE${ntype}" NOERROR big SOA "" 120 || ret=1
done

if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
if [ $status -ne 0 ]; then exit 1; fi

n=$((n + 1))
ret=0
echo_i "checking that existing names under the max-types-per-name limit get cached ($n)"

# Limited to 10 types - these should be cached and the previous record should be evicted
for ntype in $(seq 65280 65289); do
  check_manytypes 1 manytypes.big "TYPE${ntype}" NOERROR manytypes.big "TYPE${ntype}" 120 || ret=1
done
# Wait at least one second
sleep 1
# Limited to 10 types - these should be cached
for ntype in $(seq 65280 65289); do
  check_manytypes 2 manytypes.big "TYPE${ntype}" NOERROR manytypes.big "TYPE${ntype}" "" 120 || ret=1
done

if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
if [ $status -ne 0 ]; then exit 1; fi

n=$((n + 1))
ret=0
echo_i "checking that NXDOMAIN names over the max-types-per-name limit don't get cached ($n)"

# Query for 10 NXDOMAIN types
for ntype in $(seq 65270 65279); do
  check_manytypes 1 manytypes.big "TYPE${ntype}" NOERROR big SOA 0 || ret=1
done
# Wait at least 1 second
sleep 1
# Query for 10 NXDOMAIN types again - these should not be cached
for ntype in $(seq 65270 65279); do
  check_manytypes 2 manytypes.big "TYPE${ntype}" NOERROR big SOA 0 || ret=1
done

if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
if [ $status -ne 0 ]; then exit 1; fi

n=$((n + 1))
ret=0
echo_i "checking that priority NXDOMAIN names over the max-types-per-name limit get cached ($n)"

# Query for NXDOMAIN for items on our priority list - these should get cached
for rrtype in AAAA MX NS; do
  check_manytypes 1 manytypes.big "${rrtype}" NOERROR big SOA 120 || ret=1
done
# Wait at least 1 second
for rrtype in AAAA MX NS; do
  check_manytypes 2 manytypes.big "${rrtype}" NOERROR big SOA "" 120 || ret=1
done

if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
if [ $status -ne 0 ]; then exit 1; fi

n=$((n + 1))
ret=0
echo_i "checking that priority name over the max-types-per-name get cached ($n)"

# Query for an item on our priority list - it should get cached
check_manytypes 1 manytypes.big "A" NOERROR manytypes.big A 120 || ret=1
# Wait at least 1 second
sleep 1
# Query the same name again - it should be in the cache
check_manytypes 2 manytypes.big "A" NOERROR big manytypes.A "" 120 || ret=1

if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
if [ $status -ne 0 ]; then exit 1; fi

ns3_flush

n=$((n + 1))
ret=0
echo_i "checking that priority name over the max-types-per-name don't get evicted ($n)"

# Query for an item on our priority list - it should get cached
check_manytypes 1 manytypes.big "A" NOERROR manytypes.big A 120 || ret=1
# Query for 10 more types - this should not evict A record
for ntype in $(seq 65280 65289); do
  check_manytypes 1 manytypes.big "TYPE${ntype}" NOERROR manytypes.big || ret=1
done
# Wait at least 1 second
sleep 1
# Query the same name again - it should be in the cache
check_manytypes 2 manytypes.big "A" NOERROR manytypes.big A "" 120 || ret=1
# This one was first in the list and should have been evicted
check_manytypes 2 manytypes.big "TYPE65280" NOERROR manytypes.big TYPE65280 120 || ret=1

if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
if [ $status -ne 0 ]; then exit 1; fi

ns3_flush

n=$((n + 1))
ret=0
echo_i "checking that non-priority types cause eviction ($n)"

# Everything on top of that will cause the cache eviction
for ntype in $(seq 65280 65299); do
  check_manytypes 1 manytypes.big "TYPE${ntype}" NOERROR manytypes.big "TYPE${ntype}" 120 || ret=1
done
# Wait at least one second
sleep 1
# These should have TTL != 120 now
for ntype in $(seq 65290 65299); do
  check_manytypes 2 manytypes.big "TYPE${ntype}" NOERROR manytypes.big "TYPE${ntype}" "" 120 || ret=1
done
# These should have been evicted
for ntype in $(seq 65280 65289); do
  check_manytypes 3 manytypes.big "TYPE${ntype}" NOERROR manytypes.big "TYPE${ntype}" 120 || ret=1
done
# These should have been evicted by the previous block
for ntype in $(seq 65290 65299); do
  check_manytypes 4 manytypes.big "TYPE${ntype}" NOERROR manytypes.big "TYPE${ntype}" 120 || ret=1
done

if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
if [ $status -ne 0 ]; then exit 1; fi

ns3_flush

n=$((n + 1))
ret=0
echo_i "checking that signed names under the max-types-per-name limit get cached ($n)"

# Go through the 10 items, this should result in 20 items (type + rrsig(type))
for ntype in $(seq 65280 65289); do
  check_manytypes 1 manytypes.signed "TYPE${ntype}" NOERROR manytypes.signed "TYPE${ntype}" 120 || ret=1
done

# Wait at least one second
sleep 1

# These should have TTL != 120 now
for ntype in $(seq 65285 65289); do
  check_manytypes 2 manytypes.signed "TYPE${ntype}" NOERROR manytypes.signed "TYPE${ntype}" "" 120 || ret=1
done

# These should have been evicted
for ntype in $(seq 65280 65284); do
  check_manytypes 3 manytypes.signed "TYPE${ntype}" NOERROR manytypes.signed "TYPE${ntype}" 120 || ret=1
done

# These should have been evicted by the previous block
for ntype in $(seq 65285 65289); do
  check_manytypes 4 manytypes.signed "TYPE${ntype}" NOERROR manytypes.signed "TYPE${ntype}" 120 || ret=1
done

if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
if [ $status -ne 0 ]; then exit 1; fi

n=$((n + 1))
ret=0
echo_i "checking that lifting the limit will allow everything to get cached ($n)"

# Lift the limit
cp ns3/named6.conf ns3/named.conf
ns3_reset

for ntype in $(seq 65280 65534); do
  check_manytypes 1 manytypes.big "TYPE${ntype}" NOERROR manytypes.big "TYPE${ntype}" 120 || ret=1
done
# Wait at least one second
sleep 1
for ntype in $(seq 65280 65534); do
  check_manytypes 2 manytypes.big "TYPE${ntype}" NOERROR manytypes.big "TYPE${ntype}" "" 120 || ret=1
done

if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
