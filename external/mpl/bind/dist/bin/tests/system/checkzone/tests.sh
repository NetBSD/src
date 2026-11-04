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
n=1

for db in zones/good*.db; do
  echo_i "checking $db ($n)"
  ret=0
  case $db in
    zones/good-gc-msdcs.db | zones/good-spf-exception.db)
      $CHECKZONE -k fail -i local example $db >test.out.$n 2>&1 || ret=1
      ;;
    zones/good-dns-sd-reverse.db)
      $CHECKZONE -k fail -i local 0.0.0.0.in-addr.arpa $db >test.out.$n 2>&1 || ret=1
      ;;
    *)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || ret=1
      ;;
  esac
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
done

for db in zones/bad*.db; do
  echo_i "checking $db ($n)"
  ret=0 v=0
  case $db in
    zones/bad-_dns-svcb*.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "SVCB record not valid:" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-caa-rr.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "dns_rdata_fromtext: zones/bad-caa-rr\.db:15: near .*: syntax error" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-cdnskey.db | zones/bad-cds.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "CDS/CDNSKEY consistency checks failed" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-cname-and*.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "CNAME and other data" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-dhcid.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "zones/bad-dhcid\.db:12: near eol: unexpected end of input" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-dns-sd-reverse.db)
      $CHECKZONE -k fail -i local 0.0.0.0.in-addr.arpa $db >test.out.$n 2>&1 || v=$?
      grep "${db}:19: near '!@#3.': bad name (check-names)" test.out.$n >/dev/null || ret=1
      grep "${db}:20: near '!@#3.': bad name (check-names)" test.out.$n >/dev/null || ret=1
      grep "${db}:21: near '!@#3.': bad name (check-names)" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-ds.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "DS record at top of zone (example)" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-ds-2.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "DS not at delegation point" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-eid.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "zones/bad-eid.db:12: near eol: unexpected end of input" test.out.$n >/dev/null || ret=1
      grep "zones/bad-eid.db:13: near eol: unexpected end of input" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-eui48-*.db | zones/bad-eui64-*.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "bad EUI" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-generate-garbage.db | zones/bad-generate-missing-brace.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "\$GENERATE: ${db}:17: syntax error" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-generate-range.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "\$GENERATE: ${db}:18: out of range" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-generate-tkey.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "\$GENERATE: ${db}:17: meta RR type 'TKEY'" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-include-directory.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "dns_master_load: .:1: isc_lex_gettoken() failed: not a file" test.out.$n >/dev/null || ret=1
      grep "dns_master_load: ..:1: isc_lex_gettoken() failed: not a file" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-nsap-empty.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "near '0x': unexpected end of input" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-nsap-odd-nibble.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "near '0x47000580005a0000000001e133ffffff000161000': unexpected end of input" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-nsec3-length.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "zones/bad-nsec3-length.db:17: near 'IMQ912BREQP1POLAH3RMONG': out of range" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-nsec3-padded.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "zones/bad-nsec3-padded.db:19: near 'CPNMU===': bad base32 encoding" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-nsec3owner-padded.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "zones/bad-nsec3owner-padded.db:18: CPNMU===.example: bad owner name (check-names)" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-svcb-alpn[123456].db)
      $CHECKZONE -i local db $db >test.out.$n 2>&1 || v=$?
      grep "${db}:17: near 'alpn=.*': syntax error" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-svcb-mandatory.db)
      $CHECKZONE -i local db $db >test.out.$n 2>&1 || v=$?
      grep "zones/bad-svcb-mandatory.db:17: near eol: disallowed (by application policy)" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-svcb-servername.db)
      $CHECKZONE -k fail -i local 0.0.0.0.in-addr.arpa $db >test.out.$n 2>&1 || v=$?
      grep "${db}:17: near '_underscore.example.': bad name (check-names)" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-svcb.db)
      $CHECKZONE -i local db $db >test.out.$n 2>&1 || v=$?
      grep "zones/bad-svcb.db:17: near 'unknown=wha': syntax error" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-tkey.db | zones/bad-tsig.db)
      $CHECKZONE -i local db $db >test.out.$n 2>&1 || v=$?
      grep "type '[A-Z]*': invalid use of a meta type" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad-unspec.db)
      $CHECKZONE -i local db $db >test.out.$n 2>&1 || v=$?
      grep "zones/bad-unspec.db:16: near '^#': unknown class/type" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad1.db)
      $CHECKZONE -i local db $db >test.out.$n 2>&1 || v=$?
      grep "zones/bad1.db:15: extra input text" test.out.$n >/dev/null || ret=1
      ;;
    zones/bad[234].db)
      $CHECKZONE -i local db $db >test.out.$n 2>&1 || v=$?
      grep "\$GENERATE: ${db}:19: invalid range" test.out.$n >/dev/null || ret=1
      ;;
    zones/badttl.db)
      $CHECKZONE -i local example $db >test.out.$n 2>&1 || v=$?
      grep "zones/badttl.db:12: unexpected end of line" test.out.$n >/dev/null || ret=1
      ;;
    *)
      ret=1
      echo "add case entry for $db"
      $CHECKZONE -i local example $db || v=$?
      ;;
  esac
  test $v = 1 || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
done

echo_i "checking with journal file ($n)"
ret=0
$CHECKZONE -D -o test.orig.db test zones/test1.db >/dev/null 2>&1 || ret=1
$CHECKZONE -D -o test.changed.db test zones/test2.db >/dev/null 2>&1 || ret=1
$MAKEJOURNAL test test.orig.db test.changed.db test.orig.db.jnl 2>&1 || ret=1
jlines=$($JOURNALPRINT test.orig.db.jnl | wc -l)
[ $jlines = 3 ] || ret=1
$CHECKZONE -D -j -o test.out1.db test test.orig.db >/dev/null 2>&1 || ret=1
cmp -s test.changed.db test.out1.db || ret=1
mv -f test.orig.db.jnl test.journal
$CHECKZONE -D -J test.journal -o test.out2.db test test.orig.db >/dev/null 2>&1 || ret=1
cmp -s test.changed.db test.out2.db || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking with spf warnings ($n)"
ret=0
$CHECKZONE example zones/spf.db >test.out1.$n 2>&1 || ret=1
$CHECKZONE -T ignore example zones/spf.db >test.out2.$n 2>&1 || ret=1
grep "'x.example' found type SPF" test.out1.$n >/dev/null && ret=1
grep "'y.example' found type SPF" test.out1.$n >/dev/null || ret=1
grep "'example' found type SPF" test.out1.$n >/dev/null && ret=1
grep "'x.example' found type SPF" test.out2.$n >/dev/null && ret=1
grep "'y.example' found type SPF" test.out2.$n >/dev/null && ret=1
grep "'example' found type SPF" test.out2.$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking with max ttl (text) ($n)"
ret=0
$CHECKZONE -i local -l 300 example zones/good1.db >test.out1.$n 2>&1 && ret=1
$CHECKZONE -i local -l 600 example zones/good1.db >test.out2.$n 2>&1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking with max ttl (raw) ($n)"
ret=0
$CHECKZONE -f raw -l 300 example good1.db.raw >test.out1.$n 2>&1 && ret=1
$CHECKZONE -f raw -l 600 example good1.db.raw >test.out2.$n 2>&1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking for no 'inherited owner' warning on '\$INCLUDE file' with no new \$ORIGIN ($n)"
ret=0
$CHECKZONE example zones/nowarn.inherited.owner.db >test.out1.$n 2>&1 || ret=1
grep "inherited.owner" test.out1.$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking for 'inherited owner' warning on '\$ORIGIN + \$INCLUDE file' ($n)"
ret=0
$CHECKZONE example zones/warn.inherit.origin.db >test.out1.$n 2>&1 || ret=1
grep "inherited.owner" test.out1.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking for 'inherited owner' warning on '\$INCLUDE file origin' ($n)"
ret=0
$CHECKZONE example zones/warn.inherited.owner.db >test.out1.$n 2>&1 || ret=1
grep "inherited.owner" test.out1.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that raw zone with bad class is handled ($n)"
ret=0
$CHECKZONE -f raw example zones/bad-badclass.raw >test.out.$n 2>&1 && ret=1
grep "failed: bad class" test.out.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that expirations that loop using serial arithmetic are handled ($n)"
ret=0
q=-q
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
test $ret -eq 1 || $CHECKZONE $q dyn.example.net zones/crashzone.db >test.out.$n 2>&1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that nameserver below DNAME is reported even with occulted address record present ($n)"
ret=0
$CHECKZONE example.com zones/ns-address-below-dname.db >test.out.$n 2>&1 && ret=1
grep "is below a DNAME" test.out.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that delegating nameserver below DNAME is reported even with occulted address record present ($n)"
ret=0
$CHECKZONE example.com zones/delegating-ns-address-below-dname.db >test.out.$n 2>&1 || ret=1
grep "is below a DNAME" test.out.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that named-compilezone works when reading input from stdin ($n)"
ret=0
# Step 1: take raw input from stdin and convert it to text/relative format.
# Last argument "-" is optional, but it says more explicitly that we're reading from stdin.
cat zones/zone1.db | ./named-compilezone -f text -F text -s relative \
  -o zones/zone1_stdin.txt zone1.com - >/dev/null || ret=1
status=$((status + ret))

ret=0
# Step 2: take raw input from file and convert it to text format.
./named-compilezone -f text -F text -s relative -o zones/zone1_file.txt \
  zone1.com zones/zone1.db >/dev/null || ret=1
status=$((status + ret))

ret=0
# Step 3: Ensure that output conversion from stdin is the same as the output conversion from a file.
diff zones/zone1_file.txt zones/zone1_stdin.txt >/dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
ret=0
echo_i "checking integer overflow is prevented in \$GENERATE ($n)"
$CHECKZONE -D example.com zones/generate-overflow.db >test.out.$n 2>&1 || ret=1
lines=$(grep -c CNAME test.out.$n)
[ "$lines" -eq 1 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking for RSASHA1 deprecated warning ($n)"
ret=0
$CHECKZONE example zones/warn.deprecated.rsasha1.db >test.out.$n || ret=1
grep "deprecated DNSKEY algorithm found: 5 (RSASHA1)" test.out.$n >/dev/null || ret=1
grep "all DNSKEY algorithms found are deprecated" test.out.$n >/dev/null || ret=1
grep "loaded serial 0 (DNSSEC signed)" test.out.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking for NSECRSASHA1 deprected warning ($n)"
ret=0
$CHECKZONE example zones/warn.deprecated.nsec3rsasha1.db >test.out.$n || ret=1
grep "deprecated DNSKEY algorithm found: 7 (NSEC3RSASHA1)" test.out.$n >/dev/null || ret=1
grep "all DNSKEY algorithms found are deprecated" test.out.$n >/dev/null || ret=1
grep "loaded serial 0 (DNSSEC signed)" test.out.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking for SHA1 CDS digest warning ($n)"
ret=0
$CHECKZONE example zones/warn.deprecated.cds-sha1.db >test.out.$n || ret=1
grep "zone example/IN: deprecated CDS digest type 1 (SHA-1)" test.out.$n >/dev/null || ret=1
grep "loaded serial 0 (DNSSEC signed)" test.out.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking for SHA1 DS digest warning ($n)"
ret=0
$CHECKZONE example zones/warn.deprecated.digest-sha1.db >test.out.$n || ret=1
grep "zone example/IN: child.example/DS deprecated digest type 1 (SHA-1)" test.out.$n >/dev/null || ret=1
grep "loaded serial 0 (DNSSEC signed)" test.out.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking for RSASHA1 DS algorithm warning ($n)"
ret=0
$CHECKZONE example zones/warn.deprecated.ds-alg.db >test.out.$n || ret=1
grep "zone example/IN: child.example/DS deprecated algorithm 5 (RSASHA1)" test.out.$n >/dev/null || ret=1
grep "loaded serial 0 (DNSSEC signed)" test.out.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking for RSASHA1 KEY algorithm warning ($n)"
ret=0
$CHECKZONE example zones/warn.deprecated.key-alg.db >test.out.$n || ret=1
grep "zone example/IN: example/KEY deprecated algorithm 5 (RSASHA1)" test.out.$n >/dev/null || ret=1
grep "loaded serial 0 (DNSSEC signed)" test.out.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
