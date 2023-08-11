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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0
n=0

mkdir keys

n=`expr $n + 1`
echo_i "checking that named-checkconf handles a known good config ($n)"
ret=0
$CHECKCONF good.conf > checkconf.out$n 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf prints a known good config ($n)"
ret=0
awk 'BEGIN { ok = 0; } /cut here/ { ok = 1; getline } ok == 1 { print }' good.conf > good.conf.in
[ -s good.conf.in ] || ret=1
$CHECKCONF -p good.conf.in  > checkconf.out$n || ret=1
grep -v '^good.conf.in:' < checkconf.out$n > good.conf.out 2>&1 || ret=1
cmp good.conf.in good.conf.out || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -x removes secrets ($n)"
ret=0
# ensure there is a secret and that it is not the check string.
grep 'secret "' good.conf.in > /dev/null || ret=1
grep 'secret "????????????????"' good.conf.in > /dev/null 2>&1 && ret=1
$CHECKCONF -p -x good.conf.in > checkconf.out$n || ret=1
grep -v '^good.conf.in:' < checkconf.out$n > good.conf.out 2>&1 || ret=1
grep 'secret "????????????????"' good.conf.out > /dev/null 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

for bad in bad-*.conf
do
    n=`expr $n + 1`
    echo_i "checking that named-checkconf detects error in $bad ($n)"
    ret=0
    $CHECKCONF $bad > checkconf.out$n 2>&1
    if [ $? -ne 1 ]; then ret=1; fi
    grep "^$bad:[0-9]*: " < checkconf.out$n > /dev/null || ret=1
    case $bad in
    bad-update-policy[123].conf)
	pat="identity and name fields are not the same"
	grep "$pat" < checkconf.out$n > /dev/null || ret=1
	;;
    bad-update-policy[4589].conf|bad-update-policy1[01].conf)
	pat="name field not set to placeholder value"
	grep "$pat" < checkconf.out$n > /dev/null || ret=1
	;;
    bad-update-policy[67].conf|bad-update-policy1[2345].conf)
	pat="missing name field type '.*' found"
	grep "$pat" < checkconf.out$n > /dev/null || ret=1
	;;
    esac
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
done

for good in good-*.conf
do
	n=`expr $n + 1`
	echo_i "checking that named-checkconf detects no error in $good ($n)"
	ret=0
	$CHECKCONF $good > checkconf.out$n 2>&1
	if [ $? -ne 0 ]; then echo_i "failed"; ret=1; fi
	status=`expr $status + $ret`
done

n=`expr $n + 1`
echo_i "checking that ancient options report a fatal error ($n)"
ret=0
$CHECKCONF ancient.conf > ancient.out 2>&1 && ret=1
grep "no longer exists" ancient.out > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z catches missing hint file ($n)"
ret=0
$CHECKCONF -z hint-nofile.conf > hint-nofile.out 2>&1 && ret=1
grep "could not configure root hints from 'nonexistent.db': file not found" hint-nofile.out > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf catches range errors ($n)"
ret=0
$CHECKCONF range.conf > checkconf.out$n 2>&1 && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf warns of notify inconsistencies ($n)"
ret=0
$CHECKCONF notify.conf > checkconf.out$n 2>&1
warnings=`grep "'notify' is disabled" < checkconf.out$n | wc -l`
[ $warnings -eq 3 ] || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking named-checkconf dnssec warnings ($n)"
ret=0
# dnssec.1: dnssec-enable is obsolete
$CHECKCONF dnssec.1 > checkconf.out$n.1 2>&1
grep "'dnssec-enable' is obsolete and should be removed" < checkconf.out$n.1 > /dev/null || ret=1
# dnssec.2: auto-dnssec warning
$CHECKCONF dnssec.2 > checkconf.out$n.2 2>&1
grep 'auto-dnssec may only be ' < checkconf.out$n.2 > /dev/null || ret=1
# dnssec.3: should have no warnings (other than deprecation warning)
$CHECKCONF dnssec.3 > checkconf.out$n.3 2>&1
grep "option 'auto-dnssec' is deprecated" < checkconf.out$n.3 > /dev/null || ret=1
lines=$(wc -l < "checkconf.out$n.3")
if [ $lines != 1 ]; then ret=1; fi
# dnssec.4: should have specific deprecation warning
$CHECKCONF dnssec.4 > checkconf.out$n.4 2>&1
grep "'auto-dnssec' option is deprecated and will be removed in BIND 9\.19" < checkconf.out$n.4 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking named-checkconf deprecate warnings ($n)"
ret=0
$CHECKCONF deprecated.conf > checkconf.out$n.1 2>&1
grep "option 'managed-keys' is deprecated" < checkconf.out$n.1 > /dev/null || ret=1
grep "option 'trusted-keys' is deprecated" < checkconf.out$n.1 > /dev/null || ret=1
grep "option 'dscp' is deprecated" < checkconf.out$n.1 > /dev/null || ret=1
grep "token 'dscp' is deprecated" < checkconf.out$n.1 > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
# set -i to ignore deprecate warnings
$CHECKCONF -i deprecated.conf > checkconf.out$n.2 2>&1
grep '.*' < checkconf.out$n.2 > /dev/null && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking named-checkconf servestale warnings ($n)"
ret=0
$CHECKCONF servestale.stale-refresh-time.0.conf > checkconf.out$n.1 2>&1
grep "'stale-refresh-time' should either be 0 or otherwise 30 seconds or higher" < checkconf.out$n.1 > /dev/null && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
ret=0
$CHECKCONF servestale.stale-refresh-time.29.conf > checkconf.out$n.1 2>&1
grep "'stale-refresh-time' should either be 0 or otherwise 30 seconds or higher" < checkconf.out$n.1 > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "range checking fields that do not allow zero ($n)"
ret=0
for field in max-retry-time min-retry-time max-refresh-time min-refresh-time; do
    cat > badzero.conf << EOF
options {
    $field 0;
};
EOF
    $CHECKCONF badzero.conf > checkconf.out$n.1 2>&1
    [ $? -eq 1 ] || { echo_i "options $field failed" ; ret=1; }
    cat > badzero.conf << EOF
view dummy {
    $field 0;
};
EOF
    $CHECKCONF badzero.conf > checkconf.out$n.2 2>&1
    [ $? -eq 1 ] || { echo_i "view $field failed" ; ret=1; }
    cat > badzero.conf << EOF
options {
    $field 0;
};
view dummy {
};
EOF
    $CHECKCONF badzero.conf > checkconf.out$n.3 2>&1
    [ $? -eq 1 ] || { echo_i "options + view $field failed" ; ret=1; }
    cat > badzero.conf << EOF
zone dummy {
    type secondary;
    primaries { 0.0.0.0; };
    $field 0;
};
EOF
    $CHECKCONF badzero.conf > checkconf.out$n.4 2>&1
    [ $? -eq 1 ] || { echo_i "zone $field failed" ; ret=1; }
done
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking options allowed in inline-signing secondaries ($n)"
ret=0
$CHECKCONF bad-dnssec.conf > checkconf.out$n.1 2>&1
l=`grep "dnssec-dnskey-kskonly.*requires inline" < checkconf.out$n.1 | wc -l`
[ $l -eq 1 ] || ret=1
$CHECKCONF bad-dnssec.conf > checkconf.out$n.2 2>&1
l=`grep "dnssec-loadkeys-interval.*requires inline" < checkconf.out$n.2 | wc -l`
[ $l -eq 1 ] || ret=1
$CHECKCONF bad-dnssec.conf > checkconf.out$n.3 2>&1
l=`grep "update-check-ksk.*requires inline" < checkconf.out$n.3 | wc -l`
[ $l -eq 1 ] || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check file + inline-signing for secondary zones ($n)"
$CHECKCONF inline-no.conf > checkconf.out$n.1 2>&1
l=`grep "missing 'file' entry" < checkconf.out$n.1 | wc -l`
[ $l -eq 0 ] || ret=1
$CHECKCONF inline-good.conf > checkconf.out$n.2 2>&1
l=`grep "missing 'file' entry" < checkconf.out$n.2 | wc -l`
[ $l -eq 0 ] || ret=1
$CHECKCONF inline-bad.conf > checkconf.out$n.3 2>&1
l=`grep "missing 'file' entry" < checkconf.out$n.3 | wc -l`
[ $l -eq 1 ] || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking named-checkconf DLZ warnings ($n)"
ret=0
$CHECKCONF dlz-bad.conf > checkconf.out$n 2>&1
grep "'dlz' and 'database'" < checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking for missing key directory warning ($n)"
ret=0
rm -rf test.keydir
$CHECKCONF warn-keydir.conf > checkconf.out$n.1 2>&1
l=`grep "'test.keydir' does not exist" < checkconf.out$n.1 | wc -l`
[ $l -eq 1 ] || ret=1
touch test.keydir
$CHECKCONF warn-keydir.conf > checkconf.out$n.2 2>&1
l=`grep "'test.keydir' is not a directory" < checkconf.out$n.2 | wc -l`
[ $l -eq 1 ] || ret=1
rm -f test.keydir
mkdir test.keydir
$CHECKCONF warn-keydir.conf > checkconf.out$n.3 2>&1
l=`grep "key-directory" < checkconf.out$n.3 | wc -l`
[ $l -eq 0 ] || ret=1
rm -rf test.keydir
if [ $ret -ne 0 ]; then echo_i "failed"; fi

n=`expr $n + 1`
echo_i "checking that named-checkconf -z catches conflicting ttl with max-ttl ($n)"
ret=0
$CHECKCONF -z max-ttl.conf > check.out 2>&1
grep 'TTL 900 exceeds configured max-zone-ttl 600' check.out > /dev/null 2>&1 || ret=1
grep 'TTL 900 exceeds configured max-zone-ttl 600' check.out > /dev/null 2>&1 || ret=1
grep 'TTL 900 exceeds configured max-zone-ttl 600' check.out > /dev/null 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z catches invalid max-ttl ($n)"
ret=0
$CHECKCONF -z max-ttl-bad.conf > checkconf.out$n 2>&1 && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z skips zone check with alternate databases ($n)"
ret=0
$CHECKCONF -z altdb.conf > checkconf.out$n 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z skips zone check with DLZ ($n)"
ret=0
$CHECKCONF -z altdlz.conf > checkconf.out$n 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z fails on view with ANY class ($n)"
ret=0
$CHECKCONF -z view-class-any1.conf > checkconf.out$n 2>&1 && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z fails on view with CLASS255 class ($n)"
ret=0
$CHECKCONF -z view-class-any2.conf > checkconf.out$n 2>&1 && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z passes on view with IN class ($n)"
ret=0
$CHECKCONF -z view-class-in1.conf > checkconf.out$n 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z passes on view with CLASS1 class ($n)"
ret=0
$CHECKCONF -z view-class-in2.conf > checkconf.out$n 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that check-names fails as configured ($n)"
ret=0
$CHECKCONF -z check-names-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "near '_underscore': bad name (check-names)" < checkconf.out$n > /dev/null || ret=1
grep "zone check-names/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that check-mx fails as configured ($n)"
ret=0
$CHECKCONF -z check-mx-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "near '10.0.0.1': MX is an address" < checkconf.out$n > /dev/null || ret=1
grep "zone check-mx/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that check-dup-records fails as configured ($n)"
ret=0
$CHECKCONF -z check-dup-records-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "has semantically identical records" < checkconf.out$n > /dev/null || ret=1
grep "zone check-dup-records/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that check-mx fails as configured ($n)"
ret=0
$CHECKCONF -z check-mx-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "failed: MX is an address" < checkconf.out$n > /dev/null || ret=1
grep "zone check-mx/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that check-mx-cname fails as configured ($n)"
ret=0
$CHECKCONF -z check-mx-cname-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "MX.* is a CNAME (illegal)" < checkconf.out$n > /dev/null || ret=1
grep "zone check-mx-cname/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that check-srv-cname fails as configured ($n)"
ret=0
$CHECKCONF -z check-srv-cname-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "SRV.* is a CNAME (illegal)" < checkconf.out$n > /dev/null || ret=1
grep "zone check-mx-cname/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that named-checkconf -p properly print a port range ($n)"
ret=0
$CHECKCONF -p portrange-good.conf > checkconf.out$n 2>&1 || ret=1
grep "range 8610 8614;" < checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that named-checkconf -z handles in-view ($n)"
ret=0
$CHECKCONF -z in-view-good.conf > checkconf.out$n 2>&1 || ret=1
grep "zone shared.example/IN: loaded serial" < checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that named-checkconf -z returns error when a later view is okay ($n)"
ret=0
$CHECKCONF -z check-missing-zone.conf > checkconf.out$n 2>&1 && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that named-checkconf prints max-cache-size <percentage> correctly ($n)"
ret=0
$CHECKCONF -p max-cache-size-good.conf > checkconf.out$n 2>&1 || ret=1
grep "max-cache-size 60%;" < checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that named-checkconf -l prints out the zone list ($n)"
ret=0
$CHECKCONF -l good.conf |
grep -v "is deprecated" |
grep -v "is not implemented" |
grep -v "is not recommended" |
grep -v "no longer exists" |
grep -v "is obsolete" > checkconf.out$n || ret=1
diff good.zonelist checkconf.out$n > diff.out$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that 'dnssec-lookaside auto;' generates a warning ($n)"
ret=0
$CHECKCONF warn-dlv-auto.conf > checkconf.out$n 2>/dev/null || ret=1
grep "option 'dnssec-lookaside' is obsolete and should be removed" < checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that 'dnssec-lookaside . trust-anchor dlv.isc.org;' generates a warning ($n)"
ret=0
$CHECKCONF warn-dlv-dlv.isc.org.conf > checkconf.out$n 2>/dev/null || ret=1
grep "option 'dnssec-lookaside' is obsolete and should be removed" < checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that 'dnssec-lookaside . trust-anchor dlv.example.com;' generates a warning ($n)"
ret=0
$CHECKCONF warn-dlv-dlv.example.com.conf > checkconf.out$n 2>/dev/null || ret=1
grep "option 'dnssec-lookaside' is obsolete and should be removed" < checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that the 2010 ICANN ROOT KSK without the 2017 ICANN ROOT KSK generates a warning ($n)"
ret=0
$CHECKCONF check-root-ksk-2010.conf > checkconf.out$n 2>/dev/null || ret=1
[ -s checkconf.out$n ] || ret=1
grep "key without the updated" < checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that the 2010 ICANN ROOT KSK with the 2017 ICANN ROOT KSK does not generate a warning ($n)"
ret=0
$CHECKCONF check-root-ksk-both.conf > checkconf.out$n 2>/dev/null || ret=1
[ -s checkconf.out$n ] && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that the 2017 ICANN ROOT KSK alone does not generate a warning ($n)"
ret=0
$CHECKCONF check-root-ksk-2017.conf > checkconf.out$n 2>/dev/null || ret=1
[ -s checkconf.out$n ] && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that a static root key generates a warning ($n)"
ret=0
$CHECKCONF check-root-static-key.conf > checkconf.out$n 2>/dev/null || ret=1
grep "static entry for the root zone WILL FAIL" checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that a static root DS trust anchor generates a warning ($n)"
ret=0
$CHECKCONF check-root-static-ds.conf > checkconf.out$n 2>/dev/null || ret=1
grep "static entry for the root zone WILL FAIL" checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that a trusted-keys entry for root generates a warning ($n)"
ret=0
$CHECKCONF check-root-trusted-key.conf > checkconf.out$n 2>/dev/null || ret=1
grep "trusted-keys entry for the root zone WILL FAIL" checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that using trust-anchors and managed-keys generates an error ($n)"
ret=0
$CHECKCONF check-mixed-keys.conf > checkconf.out$n 2>/dev/null && ret=1
grep "use of managed-keys is not allowed" checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that 'geoip-use-ecs no' generates a warning ($n)"
ret=0
$CHECKCONF warn-geoip-use-ecs.conf > checkconf.out$n 2>/dev/null || ret=1
[ -s checkconf.out$n ] || ret=1
grep "'geoip-use-ecs' is obsolete" < checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking named-checkconf kasp errors ($n)"
ret=0
$CHECKCONF kasp-and-other-dnssec-options.conf > checkconf.out$n 2>&1 && ret=1
grep "'inline-signing yes;' must also be configured explicitly for zones using dnssec-policy without a configured 'allow-update' or 'update-policy'" < checkconf.out$n > /dev/null || ret=1
grep "'auto-dnssec maintain;' cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
grep "dnskey-sig-validity: cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
grep "dnssec-dnskey-kskonly: cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
grep "dnssec-secure-to-insecure: cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
grep "dnssec-update-mode: cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
grep "sig-validity-interval: cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
grep "update-check-ksk: cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking named-checkconf kasp nsec3 iterations errors ($n)"
ret=0
$CHECKCONF kasp-bad-nsec3-iter.conf > checkconf.out$n 2>&1 && ret=1
grep "dnssec-policy: nsec3 iterations value 151 out of range" < checkconf.out$n > /dev/null || ret=1
lines=$(wc -l < "checkconf.out$n")
if [ $lines -ne 3 ]; then ret=1; fi
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking named-checkconf kasp nsec3 algorithm errors ($n)"
ret=0
$CHECKCONF kasp-bad-nsec3-alg.conf > checkconf.out$n 2>&1 && ret=1
grep "dnssec-policy: cannot use nsec3 with algorithm 'RSASHA1'" < checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking named-checkconf kasp key errors ($n)"
ret=0
$CHECKCONF kasp-bad-keylen.conf > checkconf.out$n 2>&1 && ret=1
grep "dnssec-policy: key with algorithm rsasha1 has invalid key length 511" < checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking named-checkconf kasp predefined key length ($n)"
ret=0
$CHECKCONF kasp-ignore-keylen.conf > checkconf.out$n 2>&1 || ret=1
grep "dnssec-policy: key algorithm ecdsa256 has predefined length; ignoring length value 2048" < checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that a good 'kasp' configuration is accepted ($n)"
ret=0
$CHECKCONF good-kasp.conf > checkconf.out$n 2>/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf prints a known good kasp config ($n)"
ret=0
awk 'BEGIN { ok = 0; } /cut here/ { ok = 1; getline } ok == 1 { print }' good-kasp.conf > good-kasp.conf.in
[ -s good-kasp.conf.in ] || ret=1
$CHECKCONF -p good-kasp.conf.in | grep -v '^good-kasp.conf.in:' > good-kasp.conf.out 2>&1 || ret=1
cmp good-kasp.conf.in good-kasp.conf.out || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that max-ixfr-ratio 100% generates a warning ($n)"
ret=0
$CHECKCONF warn-maxratio1.conf > checkconf.out$n 2>/dev/null || ret=1
grep "exceeds 100%" < checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that *-source options with specified port generate warnings ($n)"
ret=0
$CHECKCONF warn-transfer-source.conf > checkconf.out$n 2>/dev/null || ret=1
grep "not recommended" < checkconf.out$n > /dev/null || ret=1
$CHECKCONF warn-notify-source.conf > checkconf.out$n 2>/dev/null || ret=1
grep "not recommended" < checkconf.out$n > /dev/null || ret=1
$CHECKCONF warn-parental-source.conf > checkconf.out$n 2>/dev/null || ret=1
grep "not recommended" < checkconf.out$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that using both max-zone-ttl and dnssec-policy generates a warning ($n)"
ret=0
$CHECKCONF warn-kasp-max-zone-ttl.conf > checkconf.out$n 2>/dev/null || ret=1
grep "option 'max-zone-ttl' is ignored when used together with 'dnssec-policy'" < checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=$((n+1))
echo_i "check that masterfile-format map generates deprecation warning ($n)"
ret=0
$CHECKCONF deprecated-masterfile-format-map.conf > checkconf.out$n 2>/dev/null || ret=1
grep "is deprecated" < checkconf.out$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=$((status+ret))

n=$((n+1))
echo_i "check that masterfile-format text and raw don't generate deprecation warning ($n)"
ret=0
$CHECKCONF good-masterfile-format-text.conf > checkconf.out$n 2>/dev/null || ret=1
grep "is deprecated" < checkconf.out$n >/dev/null && ret=1
$CHECKCONF good-masterfile-format-raw.conf > checkconf.out$n 2>/dev/null || ret=1
grep "is deprecated" < checkconf.out$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=$((status+ret))

n=$((n+1))
echo_i "check that 'check-wildcard no;' succeeds as configured ($n)"
ret=0
$CHECKCONF -z check-wildcard-no.conf > checkconf.out$n 2>&1 || ret=1
grep -F "warning: ownername 'foo.*.check-wildcard' contains an non-terminal wildcard" checkconf.out$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that 'check-wildcard yes;' warns as configured ($n)"
ret=0
$CHECKCONF -z check-wildcard.conf > checkconf.out$n 2>&1 || ret=1
grep -F "warning: ownername 'foo.*.check-wildcard' contains an non-terminal wildcard" checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

rmdir keys

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
