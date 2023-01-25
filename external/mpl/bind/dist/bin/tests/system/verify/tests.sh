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
failed () {
	cat verify.out.$n | sed 's/^/D:/';
	echo_i "failed";
	status=1;
}

n=0
status=0

for file in zones/*.good
do
	n=`expr $n + 1`
	zone=`expr "$file" : 'zones/\(.*\).good'`
	echo_i "checking supposedly good zone: $zone ($n)"
	ret=0
	case $zone in
	zsk-only.*) only=-z;;
	ksk-only.*) only=-z;;
	*) only=;;
	esac
	$VERIFY ${only} -o $zone $file > verify.out.$n 2>&1 || ret=1
	[ $ret = 0 ] || failed
done

for file in zones/*.bad
do
	n=`expr $n + 1`
	zone=`expr "$file" : 'zones/\(.*\).bad'`
	echo_i "checking supposedly bad zone: $zone ($n)"
	ret=0
	dumpit=0
	case $zone in
	zsk-only.*) only=-z;;
	ksk-only.*) only=-z;;
	*) only=;;
	esac
	expect1= expect2=
	case $zone in
	*.dnskeyonly)
		expect1="DNSKEY is not signed"
		;;
	*.expired)
		expect1="signature has expired"
		expect2="No self-signed .*DNSKEY found"
		;;
	*.ksk-expired)
		expect1="signature has expired"
		expect2="No self-signed .*DNSKEY found"
		;;
	*.out-of-zone-nsec|*.below-bottom-of-zone-nsec|*.below-dname-nsec)
		expect1="unexpected NSEC RRset at"
		;;
	*.nsec.broken-chain)
		expect1="Bad NSEC record for.*, next name mismatch"
		;;
	*.bad-bitmap)
		expect1="bit map mismatch"
		;;
	*.missing-empty)
		expect1="Missing NSEC3 record for";
		;;
	unsigned)
		expect1="Zone contains no DNSSEC keys"
		;;
	*.extra-nsec3)
		expect1="Expected and found NSEC3 chains not equal";
		;;
	*)
		dumpit=1
		;;
	esac
	$VERIFY ${only} -o $zone $file > verify.out.$n 2>&1 && ret=1
	grep "${expect1:-.}" verify.out.$n > /dev/null || ret=1
	grep "${expect2:-.}" verify.out.$n > /dev/null || ret=1
	[ $ret = 0 ] || failed
	[ $dumpit = 1 ] && cat verify.out.$n
done

n=`expr $n + 1`
echo_i "checking error message when -o is not used and a SOA record not at top of zone is found ($n)"
ret=0
# When -o is not used, origin is set to zone file name, which should cause an error in this case
$VERIFY zones/ksk+zsk.nsec.good > verify.out.$n 2>&1 && ret=1
grep "not at top of zone" verify.out.$n > /dev/null || ret=1
grep "use -o to specify a different zone origin" verify.out.$n > /dev/null || ret=1
[ $ret = 0 ] || failed

n=`expr $n + 1`
echo_i "checking error message when an invalid -o is specified and a SOA record not at top of zone is found ($n)"
ret=0
$VERIFY -o invalid.origin zones/ksk+zsk.nsec.good > verify.out.$n 2>&1 && ret=1
grep "not at top of zone" verify.out.$n > /dev/null || ret=1
grep "use -o to specify a different zone origin" verify.out.$n > /dev/null && ret=1
[ $ret = 0 ] || failed

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
