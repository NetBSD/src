# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0
n=0

n=`expr $n + 1`
echo_i "checking that named-checkconf handles a known good config ($n)"
ret=0
$CHECKCONF good.conf > checkconf.out$n 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf prints a known good config ($n)"
ret=0
awk 'BEGIN { ok = 0; } /cut here/ { ok = 1; getline } ok == 1 { print }' good.conf > good.conf.in
[ -s good.conf.in ] || ret=1
$CHECKCONF -p good.conf.in  > checkconf.out$n || ret=1
grep -v '^good.conf.in:' < checkconf.out$n > good.conf.out 2>&1 || ret=1
cmp good.conf.in good.conf.out || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
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
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

for bad in bad-*.conf
do
    n=`expr $n + 1`
    echo_i "checking that named-checkconf detects error in $bad ($n)"
    ret=0
    $CHECKCONF $bad > checkconf.out$n 2>&1
    if [ $? != 1 ]; then ret=1; fi
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
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
done

for good in good-*.conf
do
	n=`expr $n + 1`
	echo_i "checking that named-checkconf detects no error in $good ($n)"
	ret=0
	$CHECKCONF $good > checkconf.out$n 2>&1
	if [ $? != 0 ]; then echo_i "failed"; ret=1; fi
	status=`expr $status + $ret`
done

n=`expr $n + 1`
echo_i "checking that ancient options report a fatal error ($n)"
ret=0
$CHECKCONF ancient.conf > ancient.out 2>&1 && ret=1
grep "no longer exists" ancient.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z catches missing hint file ($n)"
ret=0
$CHECKCONF -z hint-nofile.conf > hint-nofile.out 2>&1 && ret=1
grep "could not configure root hints from 'nonexistent.db': file not found" hint-nofile.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf catches range errors ($n)"
ret=0
$CHECKCONF range.conf > checkconf.out$n 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf warns of notify inconsistencies ($n)"
ret=0
$CHECKCONF notify.conf > checkconf.out$n 2>&1
warnings=`grep "'notify' is disabled" < checkconf.out$n | wc -l`
[ $warnings -eq 3 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
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
# dnssec.3: should have no warnings
$CHECKCONF dnssec.3 > checkconf.out$n.3 2>&1
grep '.*' < checkconf.out$n.3 > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking named-checkconf deprecate warnings ($n)"
ret=0
$CHECKCONF deprecated.conf > checkconf.out$n.1 2>&1
grep "option 'managed-keys' is deprecated" < checkconf.out$n.1 > /dev/null || ret=1
grep "option 'trusted-keys' is deprecated" < checkconf.out$n.1 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
# set -i to ignore deprecate warnings
$CHECKCONF -i deprecated.conf > checkconf.out$n.2 2>&1
grep '.*' < checkconf.out$n.2 > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
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
    type slave;
    masters { 0.0.0.0; };
    $field 0;
};
EOF
    $CHECKCONF badzero.conf > checkconf.out$n.4 2>&1
    [ $? -eq 1 ] || { echo_i "zone $field failed" ; ret=1; }
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking options allowed in inline-signing slaves ($n)"
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
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check file + inline-signing for slave zones ($n)"
$CHECKCONF inline-no.conf > checkconf.out$n.1 2>&1
l=`grep "missing 'file' entry" < checkconf.out$n.1 | wc -l`
[ $l -eq 0 ] || ret=1
$CHECKCONF inline-good.conf > checkconf.out$n.2 2>&1
l=`grep "missing 'file' entry" < checkconf.out$n.2 | wc -l`
[ $l -eq 0 ] || ret=1
$CHECKCONF inline-bad.conf > checkconf.out$n.3 2>&1
l=`grep "missing 'file' entry" < checkconf.out$n.3 | wc -l`
[ $l -eq 1 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking named-checkconf DLZ warnings ($n)"
ret=0
$CHECKCONF dlz-bad.conf > checkconf.out$n 2>&1
grep "'dlz' and 'database'" < checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
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
if [ $ret != 0 ]; then echo_i "failed"; fi

n=`expr $n + 1`
echo_i "checking that named-checkconf -z catches conflicting ttl with max-ttl ($n)"
ret=0
$CHECKCONF -z max-ttl.conf > check.out 2>&1
grep 'TTL 900 exceeds configured max-zone-ttl 600' check.out > /dev/null 2>&1 || ret=1
grep 'TTL 900 exceeds configured max-zone-ttl 600' check.out > /dev/null 2>&1 || ret=1
grep 'TTL 900 exceeds configured max-zone-ttl 600' check.out > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z catches invalid max-ttl ($n)"
ret=0
$CHECKCONF -z max-ttl-bad.conf > checkconf.out$n 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z skips zone check with alternate databases ($n)"
ret=0
$CHECKCONF -z altdb.conf > checkconf.out$n 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z skips zone check with DLZ ($n)"
ret=0
$CHECKCONF -z altdlz.conf > checkconf.out$n 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z fails on view with ANY class ($n)"
ret=0
$CHECKCONF -z view-class-any1.conf > checkconf.out$n 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z fails on view with CLASS255 class ($n)"
ret=0
$CHECKCONF -z view-class-any2.conf > checkconf.out$n 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z passes on view with IN class ($n)"
ret=0
$CHECKCONF -z view-class-in1.conf > checkconf.out$n 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf -z passes on view with CLASS1 class ($n)"
ret=0
$CHECKCONF -z view-class-in2.conf > checkconf.out$n 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that check-names fails as configured ($n)"
ret=0
$CHECKCONF -z check-names-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "near '_underscore': bad name (check-names)" < checkconf.out$n > /dev/null || ret=1
grep "zone check-names/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that check-mx fails as configured ($n)"
ret=0
$CHECKCONF -z check-mx-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "near '10.0.0.1': MX is an address" < checkconf.out$n > /dev/null || ret=1
grep "zone check-mx/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that check-dup-records fails as configured ($n)"
ret=0
$CHECKCONF -z check-dup-records-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "has semantically identical records" < checkconf.out$n > /dev/null || ret=1
grep "zone check-dup-records/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that check-mx fails as configured ($n)"
ret=0
$CHECKCONF -z check-mx-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "failed: MX is an address" < checkconf.out$n > /dev/null || ret=1
grep "zone check-mx/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that check-mx-cname fails as configured ($n)"
ret=0
$CHECKCONF -z check-mx-cname-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "MX.* is a CNAME (illegal)" < checkconf.out$n > /dev/null || ret=1
grep "zone check-mx-cname/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that check-srv-cname fails as configured ($n)"
ret=0
$CHECKCONF -z check-srv-cname-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "SRV.* is a CNAME (illegal)" < checkconf.out$n > /dev/null || ret=1
grep "zone check-mx-cname/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that named-checkconf -p properly print a port range ($n)"
ret=0
$CHECKCONF -p portrange-good.conf > checkconf.out$n 2>&1 || ret=1
grep "range 8610 8614;" < checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that named-checkconf -z handles in-view ($n)"
ret=0
$CHECKCONF -z in-view-good.conf > checkconf.out$n 2>&1 || ret=1
grep "zone shared.example/IN: loaded serial" < checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that named-checkconf -z returns error when a later view is okay ($n)"
ret=0
$CHECKCONF -z check-missing-zone.conf > checkconf.out$n 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that named-checkconf prints max-cache-size <percentage> correctly ($n)"
ret=0
$CHECKCONF -p max-cache-size-good.conf > checkconf.out$n 2>&1 || ret=1
grep "max-cache-size 60%;" < checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that named-checkconf -l prints out the zone list ($n)"
ret=0
$CHECKCONF -l good.conf |
grep -v "is not implemented" |
grep -v "no longer exists" |
grep -v "is obsolete" > checkconf.out$n || ret=1
diff good.zonelist checkconf.out$n > diff.out$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
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
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that the 2010 ICANN ROOT KSK with the 2017 ICANN ROOT KSK does not generate a warning ($n)"
ret=0
$CHECKCONF check-root-ksk-both.conf > checkconf.out$n 2>/dev/null || ret=1
[ -s checkconf.out$n ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that the 2017 ICANN ROOT KSK alone does not generate a warning ($n)"
ret=0
$CHECKCONF check-root-ksk-2017.conf > checkconf.out$n 2>/dev/null || ret=1
[ -s checkconf.out$n ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that a static root key generates a warning ($n)"
ret=0
$CHECKCONF check-root-static-key.conf > checkconf.out$n 2>/dev/null || ret=1
grep "static entry for the root zone WILL FAIL" checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that a static root DS trust anchor generates a warning ($n)"
ret=0
$CHECKCONF check-root-static-ds.conf > checkconf.out$n 2>/dev/null || ret=1
grep "static entry for the root zone WILL FAIL" checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that a trusted-keys entry for root generates a warning ($n)"
ret=0
$CHECKCONF check-root-trusted-key.conf > checkconf.out$n 2>/dev/null || ret=1
grep "trusted-keys entry for the root zone WILL FAIL" checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that using trust-anchors and managed-keys generates an error ($n)"
ret=0
$CHECKCONF check-mixed-keys.conf > checkconf.out$n 2>/dev/null && ret=1
grep "use of managed-keys is not allowed" checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
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
grep "'inline-signing;' cannot be set to 'no' if dnssec-policy is also set on a non-dynamic DNS zone" < checkconf.out$n > /dev/null || ret=1
grep "'auto-dnssec maintain;' cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
grep "dnskey-sig-validity: cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
grep "dnssec-dnskey-kskonly: cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
grep "dnssec-secure-to-insecure: cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
grep "dnssec-update-mode: cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
grep "sig-validity-interval: cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
grep "update-check-ksk: cannot be configured if dnssec-policy is also set" < checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking named-checkconf kasp predefined key lengths ($n)"
ret=0
$CHECKCONF kasp-ignore-keylen.conf > checkconf.out$n 2>&1 || ret=1
grep "dnssec-policy: key algorithm ecdsa256 has predefined length; ignoring length value 2048" < checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that a good 'kasp' configuration is accepted ($n)"
ret=0
$CHECKCONF good-kasp.conf > checkconf.out$n 2>/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that named-checkconf prints a known good kasp config ($n)"
ret=0
awk 'BEGIN { ok = 0; } /cut here/ { ok = 1; getline } ok == 1 { print }' good-kasp.conf > good-kasp.conf.in
[ -s good-kasp.conf.in ] || ret=1
$CHECKCONF -p good-kasp.conf.in | grep -v '^good-kasp.conf.in:' > good-kasp.conf.out 2>&1 || ret=1
cmp good-kasp.conf.in good-kasp.conf.out || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
