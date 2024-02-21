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

# Wait for the zone to be fully signed before beginning test
#
# We expect the zone to have the following:
#
# - 5 signatures for signing.test.
# - 3 signatures for ns.signing.test.
# - 2 x 500 signatures for a{0000-0499}.signing.test.
#
# for a total of 1008.
fully_signed() {
  $DIG axfr signing.test -p ${PORT} @10.53.0.1 >"dig.out.ns1.axfr"
  awk 'BEGIN { lines = 0 }
             $4 == "RRSIG" {lines++}
             END { if (lines != 1008) exit(1) }' <"dig.out.ns1.axfr"
}

# Wait for the last NSEC record in the zone to be signed. This is a lightweight
# alternative to avoid many AXFR requests while waiting for the zone to be
# fully signed.
_wait_for_last_nsec_signed() {
  $DIG +dnssec a0499.signing.test -p ${PORT} @10.53.0.1 nsec >"dig.out.ns1.wait" || return 1
  grep "signing.test\..*IN.*RRSIG.*signing.test" "dig.out.ns1.wait" >/dev/null || return 1
  return 0
}

echo_i "wait for the zone to be fully signed"
retry_quiet 60 _wait_for_last_nsec_signed
retry_quiet 10 fully_signed || status=1
if [ $status != 0 ]; then echo_i "failed"; fi

start=$(date +%s)
now=$start
end=$((start + 140))

while [ $now -lt $end ] && [ $status -eq 0 ]; do
  et=$((now - start))
  echo_i "............... $et ............"
  $JOURNALPRINT ns1/signing.test.db.signed.jnl | $PERL check_journal.pl | cat_i
  $DIG axfr signing.test -p ${PORT} @10.53.0.1 >dig.out.at$et
  awk '$4 == "RRSIG" { print $11 }' dig.out.at$et | sort | uniq -c | cat_i
  lines=$(awk '$4 == "RRSIG" { print}' dig.out.at$et | wc -l)
  if [ ${et} -ne 0 -a ${lines} -ne 1008 ]; then
    echo_i "failed"
    status=$((status + 1))
  fi
  sleep 5
  now=$(date +%s)
done

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
