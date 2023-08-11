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

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

SYSTESTDIR=verify

dumpit () {
	echo_d "${debug}: dumping ${1}"
	cat "${1}" | cat_d
}

setup () {
	echo_i "setting up $2 zone: $1"
	debug="$1"
	zone="$1"
	file="$1.$2"
	n=$((${n:-0} + 1))
}

# A unsigned zone should fail validation.
setup unsigned bad
cp unsigned.db unsigned.bad

# A set of nsec zones.
setup zsk-only.nsec good
$KEYGEN -a ${DEFAULT_ALGORITHM} ${zone}> kg.out$n 2>&1 || dumpit kg.out$n
$SIGNER -SP -o ${zone} -f ${file} unsigned.db > s.out$n || dumpit s.out$n

setup ksk-only.nsec good
$KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} > kg.out$n 2>&1 || dumpit kg.out$n
$SIGNER -SPz -o ${zone} -f ${file} unsigned.db > s.out$n || dumpit s.out$n

setup ksk+zsk.nsec good
$KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} > kg1.out$n 2>&1 || dumpit kg1.out$n
$KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} > kg2.out$n 2>&1 || dumpit kg2.out$n
$SIGNER -SPx -o ${zone} -f ${file} unsigned.db > s.out$n || dumpit s.out$n

setup ksk+zsk.nsec.apex-dname good
zsk=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2> kg1.out$n) || dumpit kg1.out$n
ksk=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2> kg2.out$n) || dumpit kg2.out$n
cp unsigned.db ${file}.tmp
echo "@ DNAME data" >> ${file}.tmp
$SIGNER -SP -o ${zone} -f ${file} ${file}.tmp > s.out$n || dumpit s.out$n

# A set of nsec3 zones.
setup zsk-only.nsec3 good
$KEYGEN -a ${DEFAULT_ALGORITHM} ${zone}> kg.out$n 2>&1 || dumpit kg.out$n
$SIGNER -3 - -SP -o ${zone} -f ${file} unsigned.db > s.out$n || dumpit s.out$n

setup ksk-only.nsec3 good
$KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} > kg.out$n 2>&1 || dumpit kg.out$n
$SIGNER -3 - -SPz -o ${zone} -f ${file} unsigned.db > s.out$n || dumpit s.out$n

setup ksk+zsk.nsec3 good
$KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} > kg1.out$n 2>&1 || dumpit kg1.out$n
$KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} > kg2.out$n 2>&1 || dumpit kg2.out$n
$SIGNER -3 - -SPx -o ${zone} -f ${file} unsigned.db > s.out$n || dumpit s.out$n

setup ksk+zsk.optout good
$KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} > kg1.out$n 2>&1 || dumpit kg1.out$n
$KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} > kg2.out$n 2>&1 || dumpit kg2.out$n
$SIGNER -3 - -A -SPx -o ${zone} -f ${file} unsigned.db > s.out$n || dumpit s.out$n

setup ksk+zsk.nsec3.apex-dname good
zsk=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2> kg1.out$n) || dumpit kg1.out$n
ksk=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2> kg2.out$n) || dumpit kg2.out$n
cp unsigned.db ${file}.tmp
echo "@ DNAME data" >> ${file}.tmp
$SIGNER -3 - -SP -o ${zone} -f ${file} ${file}.tmp > s.out$n || dumpit s.out$n

#
# generate an NSEC record like
#	aba NSEC FOO ...
# then downcase all the FOO records so the next name in the database
# becomes foo when the zone is loaded.
#
setup nsec-next-name-case-mismatch good
ksk=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2> kg2.out$n) || dumpit kg2.out$n
zsk=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2> kg2.out$n) || dumpit kg2.out$n
cat << EOF > ${zone}.tmp
\$TTL 0
@ IN SOA  foo . ( 1 28800 7200 604800 1800 )
@ NS foo
\$include $ksk.key
\$include $zsk.key
FOO AAAA ::1
FOO A 127.0.0.2
aba CNAME FOO
EOF
$SIGNER -zP -o ${zone} -f ${file}.tmp ${zone}.tmp > s.out$n || dumpit s.out$n
sed 's/^FOO\./foo\./' < ${file}.tmp > ${file}

# A set of zones with only DNSKEY records.
setup zsk-only.dnskeyonly bad
key1=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2>kg.out) || dumpit kg.out$n
cat unsigned.db $key1.key > ${file}

setup ksk-only.dnskeyonly bad
key1=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2>kg.out) || dumpit kg.out$n
cat unsigned.db $key1.key > ${file}

setup ksk+zsk.dnskeyonly bad
key1=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2>kg.out) || dumpit kg.out$n
key2=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2>kg.out) || dumpit kg.out$n
cat unsigned.db $key1.key $key2.key > ${file}

# A set of zones with expired records
s="-s -2678400"
setup zsk-only.nsec.expired bad
$KEYGEN -a ${DEFAULT_ALGORITHM} ${zone}> kg.out$n 2>&1 || dumpit kg.out$n
$SIGNER -SP ${s} -o ${zone} -f ${file} unsigned.db > s.out$n || dumpit s.out$n

setup ksk-only.nsec.expired bad
$KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} > kg.out$n 2>&1 || dumpit kg.out$n
$SIGNER -SPz ${s} -o ${zone} -f ${file} unsigned.db > s.out$n || dumpit s.out$n

setup ksk+zsk.nsec.expired bad
$KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} > kg1.out$n 2>&1 || dumpit kg1.out$n
$KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} > kg2.out$n 2>&1 || dumpit kg2.out$n
$SIGNER -SP ${s} -o ${zone} -f ${file} unsigned.db > s.out$n || dumpit s.out$n

setup zsk-only.nsec3.expired bad
$KEYGEN -a ${DEFAULT_ALGORITHM} ${zone}> kg.out$n 2>&1 || dumpit kg.out$n
$SIGNER -3 - ${s} -SP -o ${zone} -f ${file} unsigned.db > s.out$n || dumpit s.out$n

setup ksk-only.nsec3.expired bad
$KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} > kg.out$n 2>&1 || dumpit kg.out$n
$SIGNER -3 - ${s} -SPz -o ${zone} -f ${file} unsigned.db > s.out$n || dumpit s.out$n

setup ksk+zsk.nsec3.expired bad
$KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} > kg1.out$n 2>&1 || dumpit kg1.out$n
$KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} > kg2.out$n 2>&1 || dumpit kg2.out$n
$SIGNER -3 - ${s} -SPx -o ${zone} -f ${file} unsigned.db > s.out$n || dumpit s.out$n

# ksk expired
setup ksk+zsk.nsec.ksk-expired bad
zsk=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2> kg1.out$n) || dumpit kg1.out$n
ksk=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2> kg2.out$n) || dumpit kg2.out$n
cat unsigned.db $ksk.key $zsk.key > $file
$SIGNER -Px -o ${zone} -f ${file} ${file} $zsk > s.out$n || dumpit s.out$n
$SIGNER ${s} -P -O full -o ${zone} -f ${file} ${file} $ksk > s.out$n || dumpit s.out$n
now=$(date -u +%Y%m%d%H%M%S)
exp=$(awk '$4 == "RRSIG" && $5 == "DNSKEY" { print $9;}' ${file})
[ "${exp:-40001231246060}" -lt ${now:-0} ] || dumpit $file

setup ksk+zsk.nsec3.ksk-expired bad
zsk=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2> kg1.out$n) || dumpit kg1.out$n
ksk=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2> kg2.out$n) || dumpit kg2.out$n
cat unsigned.db $ksk.key $zsk.key > $file
$SIGNER -3 - -Px -o ${zone} -f ${file} ${file} $zsk > s.out$n || dumpit s.out$n
$SIGNER -3 - ${s} -P -O full -o ${zone} -f ${file} ${file} $ksk > s.out$n || dumpit s.out$n
now=$(date -u +%Y%m%d%H%M%S)
exp=$(awk '$4 == "RRSIG" && $5 == "DNSKEY" { print $9;}' ${file})
[ "${exp:-40001231246060}" -lt ${now:-0} ] || dumpit $file

# broken nsec chain
setup ksk+zsk.nsec.broken-chain bad
zsk=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2> kg1.out$n) || dumpit kg1.out$n
ksk=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2> kg2.out$n) || dumpit kg2.out$n
cat unsigned.db $ksk.key $zsk.key > $file
$SIGNER -P -O full -o ${zone} -f ${file} ${file} $ksk > s.out$n || dumpit s.out$n
awk '$4 == "NSEC" { $5 = "'$zone'."; print } { print }' ${file} > ${file}.tmp
$SIGNER -Px -Z nonsecify -o ${zone} -f ${file} ${file}.tmp $zsk > s.out$n || dumpit s.out$n

# bad nsec bitmap
setup ksk+zsk.nsec.bad-bitmap bad
zsk=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2> kg1.out$n) || dumpit kg1.out$n
ksk=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2> kg2.out$n) || dumpit kg2.out$n
cat unsigned.db $ksk.key $zsk.key > $file
$SIGNER -P -O full -o ${zone} -f ${file} ${file} $ksk > s.out$n || dumpit s.out$n
awk '$4 == "NSEC" && /SOA/ { $6=""; print } { print }' ${file} > ${file}.tmp
$SIGNER -Px -Z nonsecify -o ${zone} -f ${file} ${file}.tmp $zsk > s.out$n || dumpit s.out$n

# extra NSEC record out side of zone
setup ksk+zsk.nsec.out-of-zone-nsec bad
zsk=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2> kg1.out$n) || dumpit kg1.out$n
ksk=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2> kg2.out$n) || dumpit kg2.out$n
cat unsigned.db $ksk.key $zsk.key > $file
$SIGNER -P -O full -o ${zone} -f ${file} ${file} $ksk > s.out$n || dumpit s.out$n
echo "out-of-zone. 3600 IN NSEC ${zone}. A" >> ${file}
$SIGNER -Px -Z nonsecify -O full -o ${zone} -f ${file} ${file} $zsk > s.out$n || dumpit s.out$n

# extra NSEC record below bottom of zone
setup ksk+zsk.nsec.below-bottom-of-zone-nsec bad
zsk=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2> kg1.out$n) || dumpit kg1.out$n
ksk=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2> kg2.out$n) || dumpit kg2.out$n
cat unsigned.db $ksk.key $zsk.key > $file
$SIGNER -P -O full -o ${zone} -f ${file} ${file} $ksk > s.out$n || dumpit s.out$n
echo "ns.sub.${zone}. 3600 IN NSEC ${zone}. A AAAA" >> ${file}
$SIGNER -Px -Z nonsecify -O full -o ${zone} -f ${file}.tmp ${file} $zsk > s.out$n || dumpit s.out$n
# dnssec-signzone signs any node with a NSEC record.
awk '$1 ~ /^ns.sub/ && $4 == "RRSIG" && $5 != "NSEC" { next; } { print; }' ${file}.tmp > ${file}

# extra NSEC record below DNAME
setup ksk+zsk.nsec.below-dname-nsec bad
zsk=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2> kg1.out$n) || dumpit kg1.out$n
ksk=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2> kg2.out$n) || dumpit kg2.out$n
cat unsigned.db $ksk.key $zsk.key > $file
$SIGNER -P -O full -o ${zone} -f ${file} ${file} $ksk > s.out$n || dumpit s.out$n
echo "sub.dname.${zone}. 3600 IN NSEC ${zone}. TXT" >> ${file}
$SIGNER -Px -Z nonsecify -O full -o ${zone} -f ${file} ${file} $zsk > s.out$n || dumpit s.out$n

# missing NSEC3 record at empty node
# extract the hash fields from the empty node's NSEC 3 record then fix up
# the NSEC3 chain to remove it
setup ksk+zsk.nsec3.missing-empty bad
zsk=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2> kg1.out$n) || dumpit kg1.out$n
ksk=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2> kg2.out$n) || dumpit kg2.out$n
cat unsigned.db $ksk.key $zsk.key > $file
$SIGNER -3 - -P -O full -o ${zone} -f ${file} ${file} $ksk > s.out$n || dumpit s.out$n
a=$(awk '$4 == "NSEC3" && NF == 9 { split($1, a, "."); print a[1]; }' ${file})
b=$(awk '$4 == "NSEC3" && NF == 9 { print $9; }' ${file})
awk '
$4 == "NSEC3" && $9 == "'$a'" { $9 = "'$b'"; print; next; }
$4 == "NSEC3" && NF == 9 { next; }
{ print; }' ${file} > ${file}.tmp
$SIGNER -3 - -Px -Z nonsecify -O full -o ${zone} -f ${file} ${file}.tmp $zsk > s.out$n || dumpit s.out$n

# extra NSEC3 record
setup ksk+zsk.nsec3.extra-nsec3 bad
zsk=$($KEYGEN -a ${DEFAULT_ALGORITHM} ${zone} 2> kg1.out$n) || dumpit kg1.out$n
ksk=$($KEYGEN -a ${DEFAULT_ALGORITHM} -fK ${zone} 2> kg2.out$n) || dumpit kg2.out$n
cat unsigned.db $ksk.key $zsk.key > $file
$SIGNER -3 - -P -O full -o ${zone} -f ${file} ${file} $ksk > s.out$n || dumpit s.out$n
awk '
BEGIN {
	ZONE="'${zone}'.";
}
$4 == "NSEC3" && NF == 9 {
	$1 = "H9P7U7TR2U91D0V0LJS9L1GIDNP90U3H." ZONE;
	$9 = "H9P7U7TR2U91D0V0LJS9L1GIDNP90U3I";
	print;
}' ${file} > ${file}.tmp
cat ${file}.tmp >> ${file}
rm -f ${file}.tmp
$SIGNER -3 - -Px -Z nonsecify -O full -o ${zone} -f ${file} ${file} $zsk > s.out$n || dumpit s.out$n
