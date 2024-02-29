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
  "$DIG" @10.53.0.1 -p "$PORT" +tcp "$@"
}

rndc_with_opts() {
  "$RNDC" -c ../_common/rndc.conf -p "$CONTROLPORT" -s "$@"
}

status=0
n=0

n=$((n + 1))
echo_i "check outdated journal rolled forward (dynamic) ($n)"
ret=0
dig_with_opts changed soa >dig.out.test$n
grep 'status: NOERROR' dig.out.test$n >/dev/null || ret=1
grep '2012010902' dig.out.test$n >/dev/null || ret=1
grep 'zone changed/IN: journal rollforward completed successfully using old journal format' ns1/named.run >/dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check outdated empty journal did not cause an error (dynamic) ($n)"
ret=0
dig_with_opts unchanged soa >dig.out.test$n
grep 'status: NOERROR' dig.out.test$n >/dev/null || ret=1
grep '2012010901' dig.out.test$n >/dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check outdated journals were updated or removed (dynamic) ($n)"
ret=0
cat -v ns1/changed.db.jnl | grep "BIND LOG V9.2" >/dev/null || ret=1
[ -f ns1/unchanged.db.jnl ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check updated journal has correct RR count (dynamic) ($n)"
ret=0
$JOURNALPRINT -x ns1/changed.db.jnl | grep "rrcount 3 " >/dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check new-format journal rolled forward (dynamic) ($n)"
ret=0
dig_with_opts changed2 soa >dig.out.test$n
grep 'status: NOERROR' dig.out.test$n >/dev/null || ret=1
grep '2012010902' dig.out.test$n >/dev/null || ret=1
grep 'zone changed2/IN: journal rollforward completed successfully: success' ns1/named.run >/dev/null || ret=1
grep 'zone changed2/IN: journal rollforward completed successfully using old journal format' ns1/named.run >/dev/null && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check new-format empty journal did not cause error (dynamic) ($n)"
ret=0
dig_with_opts unchanged2 soa >dig.out.test$n
grep 'status: NOERROR' dig.out.test$n >/dev/null || ret=1
grep '2012010901' dig.out.test$n >/dev/null || ret=1
grep 'zone unchanged2/IN: journal rollforward completed successfully' ns1/named.run >/dev/null && ret=1
grep 'zone unchanged2/IN: journal rollforward completed successfully using old journal format' ns1/named.run >/dev/null && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check new-format journals were updated or removed (dynamic) ($n)"
ret=0
cat -v ns1/changed2.db.jnl | grep "BIND LOG V9.2" >/dev/null || ret=1
[ -f ns1/unchanged2.db.jnl ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check outdated up-to-date journal succeeded (ixfr-from-differences) ($n)"
ret=0
dig_with_opts -t soa ixfr >dig.out.test$n
grep 'status: NOERROR' dig.out.test$n >/dev/null || ret=1
grep '2012010902' dig.out.test$n >/dev/null || ret=1
grep 'zone ixfr/IN: journal rollforward completed successfully using old journal format: up to date' ns1/named.run >/dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check outdated journal was updated (ixfr-from-differences) ($n)"
ret=0
cat -v ns1/ixfr.db.jnl | grep "BIND LOG V9.2" >/dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check journal with mixed headers succeeded (version 1,2,1,2) ($n)"
ret=0
dig_with_opts -t soa hdr1d1d2d1d2 >dig.out.test$n
grep 'status: NOERROR' dig.out.test$n >/dev/null || ret=1
grep '2012010905' dig.out.test$n >/dev/null || ret=1
grep 'zone hdr1d1d2d1d2/IN: journal rollforward completed successfully using old journal format: success' ns1/named.run >/dev/null || ret=1
grep 'zone_journal_compact: zone hdr1d1d2d1d2/IN: repair full journal' ns1/named.run >/dev/null || ret=1
grep 'hdr1d1d2d1d2/IN: dns_journal_compact: success' ns1/named.run >/dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check journal with mixed headers was updated (version 1,2,1,2) ($n)"
ret=0
[ $($JOURNALPRINT -x ns1/d1212.jnl.saved | grep -c "version 1") -eq 2 ] || ret=1
[ $($JOURNALPRINT -x ns1/d1212.jnl.saved | grep -c "version 2") -eq 2 ] || ret=1
[ $($JOURNALPRINT -x ns1/d1212.db.jnl | grep -c "version 1") -eq 0 ] || ret=1
[ $($JOURNALPRINT -x ns1/d1212.db.jnl | grep -c "version 2") -eq 4 ] || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check journal with mixed headers succeeded (version 2,1,2,1) ($n)"
ret=0
dig_with_opts -t soa hdr1d2d1d2d1 >dig.out.test$n
grep 'status: NOERROR' dig.out.test$n >/dev/null || ret=1
grep '2012010905' dig.out.test$n >/dev/null || ret=1
grep 'zone hdr1d2d1d2d1/IN: journal rollforward completed successfully using old journal format: success' ns1/named.run >/dev/null || ret=1
grep 'zone_journal_compact: zone hdr1d2d1d2d1/IN: repair full journal' ns1/named.run >/dev/null || ret=1
grep 'zone hdr1d2d1d2d1/IN: dns_journal_compact: success' ns1/named.run >/dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check journal with mixed headers was updated (version 2,1,2,1) ($n)"
ret=0
[ $($JOURNALPRINT -x ns1/d2121.jnl.saved | grep -c "version 1") -eq 2 ] || ret=1
[ $($JOURNALPRINT -x ns1/d2121.jnl.saved | grep -c "version 2") -eq 2 ] || ret=1
[ $($JOURNALPRINT -x ns1/d2121.db.jnl | grep -c "version 1") -eq 0 ] || ret=1
[ $($JOURNALPRINT -x ns1/d2121.db.jnl | grep -c "version 2") -eq 4 ] || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check there are no journals left un-updated ($n)"
ret=0
c1=$(cat -v ns1/*.jnl | grep -c "BIND LOG V9")
c2=$(cat -v ns1/*.jnl | grep -c "BIND LOG V9.2")
[ ${c1} -eq ${c2} ] || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "Check that journal with mixed headers can be compacted (version 1,2,1,2) ($n)"
ret=0
journal=ns1/d1212.jnl.saved
seriallist=$($JOURNALPRINT -x $journal | awk '$1 == "Transaction:" { print $11 }')
for serial in $seriallist; do
  cp $journal tmp.jnl
  $JOURNALPRINT -c $serial tmp.jnl || ret=1
done
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "Check that journal with mixed headers can be compacted (version 2,1,2,1) ($n)"
ret=0
journal=ns1/d2121.jnl.saved
seriallist=$($JOURNALPRINT -x $journal | awk '$1 == "Transaction:" { print $11 }')
for serial in $seriallist; do
  cp ns1/d1212.jnl.saved tmp.jnl
  $JOURNALPRINT -c $serial tmp.jnl || ret=1
done
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check upgrade of managed-keys.bind.jnl succeeded($n)"
ret=0
$JOURNALPRINT ns1/managed-keys.bind.jnl >journalprint.out.test$n
lines=$(awk '$1 == "add" && $5 == "SOA" && $8 == "3297" { print }' journalprint.out.test$n | wc -l)
test $lines -eq 1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check journal downgrade/upgrade ($n)"
ret=0
cp ns1/changed.db.jnl ns1/temp.jnl
$JOURNALPRINT -d ns1/temp.jnl
[ $($JOURNALPRINT -x ns1/temp.jnl | grep -c "version 1") -eq 1 ] || ret=1
$JOURNALPRINT -x ns1/temp.jnl | grep -q "Header version = 1" || ret=1
$JOURNALPRINT -u ns1/temp.jnl
$JOURNALPRINT -x ns1/temp.jnl | grep -q "Header version = 2" || ret=1
[ $($JOURNALPRINT -x ns1/temp.jnl | grep -c "version 2") -eq 1 ] || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check max-journal-size works after journal update ($n)"
ret=0
# journal was repaired, it should still be big
[ $(wc -c <ns1/maxjournal.db.jnl) -gt 12000 ] || ret=1
# the zone hasn't been dumped yet, so 'rndc sync' should work without
# needing a zone update first.
rndc_with_opts 10.53.0.1 sync maxjournal
check_size() (
  [ $(wc -c <ns1/maxjournal.db.jnl) -lt 4000 ]
)
retry_quiet 10 check_size || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check max-journal-size works with non-updated journals ($n)"
ret=0
# journal was not repaired, so it should still be big
[ $(wc -c <ns1/maxjournal2.db.jnl) -gt 12000 ] || ret=1
# the zone hasn't been dumped yet, so 'rndc sync' should work without
# needing a zone update first.
rndc_with_opts 10.53.0.1 sync maxjournal2
check_size() (
  [ $(wc -c <ns1/maxjournal2.db.jnl) -lt 4000 ]
)
retry_quiet 10 check_size || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check journal index consistency ($n)"
ret=0
for jnl in ns1/*.jnl; do
  $JOURNALPRINT -x $jnl 2>&1 | grep -q "Offset mismatch" && ret=1
done
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check that journal is applied to zone with keydata placeholder record"
ret=0
grep 'managed-keys-zone: journal rollforward completed successfully: up to date' ns2/named.run >/dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
