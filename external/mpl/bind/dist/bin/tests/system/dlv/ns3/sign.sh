#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

(cd ../ns6 && $SHELL -e ./sign.sh)

echo_i "dlv/ns3/sign.sh"

dlvzone="dlv.utld"
dlvsets=
dssets=

disableddlvzone="disabled-algorithm-dlv.utld"
disableddlvsets=
disableddssets=

unsupporteddlvzone="unsupported-algorithm-dlv.utld"
unsupporteddlvsets=
unsupporteddssets=

# Signed zone below unsigned TLD with DLV entry.
zone=child1.utld
infile=child.db.in
zonefile=child1.utld.db
outfile=child1.signed
dlvsets="$dlvsets dlvset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.${zone}${TP}
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -O full -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


# Signed zone below unsigned TLD with DLV entry in DLV zone that is signed
# with a disabled algorithm.
zone=child3.utld
infile=child.db.in
zonefile=child3.utld.db
outfile=child3.signed
disableddlvsets="$disableddlvsets dlvset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.${zone}${TP}
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -O full -l $disableddlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


# Signed zone below unsigned TLD with DLV entry.  This one is slightly
# different because its children (the grandchildren) don't have a DS record in
# this zone.  The grandchild zones are served by ns6.
zone=child4.utld
infile=child.db.in
zonefile=child4.utld.db
outfile=child4.signed
dlvsets="$dlvsets dlvset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -O full -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


# Signed zone below unsigned TLD with DLV entry in DLV zone that is signed
# with an unsupported algorithm.
zone=child5.utld
infile=child.db.in
zonefile=child5.utld.db
outfile=child5.signed
unsupporteddlvsets="$unsupporteddlvsets dlvset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.${zone}${TP}
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -O full -l $unsupporteddlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

# Signed zone below unsigned TLD without DLV entry.
zone=child7.utld
infile=child.db.in
zonefile=child7.utld.db
outfile=child7.signed

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.${zone}${TP}
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -O full -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


# Signed zone below unsigned TLD without DLV entry and no DS records for the
# grandchildren.
zone=child8.utld
infile=child.db.in
zonefile=child8.utld.db
outfile=child8.signed

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -O full -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

# Signed zone below unsigned TLD with DLV entry.
zone=child9.utld
infile=child.db.in
zonefile=child9.utld.db
outfile=child9.signed
dlvsets="$dlvsets dlvset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -O full -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

# Unsigned zone below an unsigned TLD with DLV entry.  We still need to sign
# the zone to generate the DLV set.
zone=child10.utld
infile=child.db.in
zonefile=child10.utld.db
outfile=child10.signed
dlvsets="$dlvsets dlvset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -O full -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


# Zone signed with a disabled algorithm (an algorithm that is disabled in
# one of the test resolvers) with DLV entry.
zone=disabled-algorithm.utld
infile=child.db.in
zonefile=disabled-algorithm.utld.db
outfile=disabled-algorithm.utld.signed
dlvsets="$dlvsets dlvset-${zone}${TP}"

keyname1=`$KEYGEN -a $DISABLED_ALGORITHM -b $DISABLED_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DISABLED_ALGORITHM -b $DISABLED_BITS -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -O full -l $dlvzone -o $zone -f ${outfile} $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


# Zone signed with an unsupported algorithm with DLV entry.
zone=unsupported-algorithm.utld
infile=child.db.in
zonefile=unsupported-algorithm.utld.db
outfile=unsupported-algorithm.utld.signed
dlvsets="$dlvsets dlvset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -O full -l $dlvzone -o $zone -f ${outfile}.tmp $zonefile > /dev/null 2> signer.err || cat signer.err
awk '$4 == "DNSKEY" { $7 = 255 } $4 == "RRSIG" { $6 = 255 } { print }' ${outfile}.tmp > $outfile

cp ${keyname2}.key ${keyname2}.tmp
awk '$3 == "DNSKEY" { $6 = 255 } { print }' ${keyname2}.tmp > ${keyname2}.key
cp dlvset-${zone}${TP} dlvset-${zone}tmp
awk '$3 == "DLV" { $5 = 255 } { print }' dlvset-${zone}tmp > dlvset-${zone}${TP}

echo_i "signed $zone"

# Signed zone below signed TLD with DLV entry and DS set.
zone=child1.druz
infile=child.db.in
zonefile=child1.druz.db
outfile=child1.druz.signed
dlvsets="$dlvsets dlvset-${zone}${TP}"
dssets="$dssets dsset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.${zone}${TP}
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -O full -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


# Signed zone below signed TLD with DLV entry and DS set.  The DLV zone is
# signed with a disabled algorithm.
zone=child3.druz
infile=child.db.in
zonefile=child3.druz.db
outfile=child3.druz.signed
disableddlvsets="$disableddlvsets dlvset-${zone}${TP}"
disableddssets="$disableddssets dsset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.${zone}${TP}
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -O full -l $disableddlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


# Signed zone below signed TLD with DLV entry and DS set, but missing
# DS records for the grandchildren.
zone=child4.druz
infile=child.db.in
zonefile=child4.druz.db
outfile=child4.druz.signed
dlvsets="$dlvsets dlvset-${zone}${TP}"
dssets="$dssets dsset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -O full -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


# Signed zone below signed TLD with DLV entry and DS set.  The DLV zone is
# signed with an unsupported algorithm algorithm.
zone=child5.druz
infile=child.db.in
zonefile=child5.druz.db
outfile=child5.druz.signed
unsupporteddlvsets="$unsupporteddlvsets dlvset-${zone}${TP}"
unsupporteddssets="$unsupportedssets dsset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.${zone}${TP}
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -O full -l $unsupporteddlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


# Signed zone below signed TLD without DLV entry, but with normal DS set.
zone=child7.druz
infile=child.db.in
zonefile=child7.druz.db
outfile=child7.druz.signed
dssets="$dssets dsset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.${zone}${TP}
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -O full -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


# Signed zone below signed TLD without DLV entry and no DS set.  Also DS
# records for the grandchildren are not included in the zone.
zone=child8.druz
infile=child.db.in
zonefile=child8.druz.db
outfile=child8.druz.signed

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -O full -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


# Signed zone below signed TLD with DLV entry but no DS set.  Also DS
# records for the grandchildren are not included in the zone.
zone=child9.druz
infile=child.db.in
zonefile=child9.druz.db
outfile=child9.druz.signed
dlvsets="$dlvsets dlvset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -O full -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


# Unsigned zone below signed TLD with DLV entry and DS set.  We still need to
# sign the zone to generate the DS sets.
zone=child10.druz
infile=child.db.in
zonefile=child10.druz.db
outfile=child10.druz.signed
dlvsets="$dlvsets dlvset-${zone}${TP}"
dssets="$dssets dsset-${zone}${TP}"

keyname1=`$KEYGEN -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -O full -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

cp $dssets ../ns2
cp $disableddssets ../ns2
cp $unsupporteddssets ../ns2

# DLV zones
infile=dlv.db.in
for zone in dlv.utld disabled-algorithm-dlv.utld unsupported-algorithm-dlv.utld
do
	zonefile="${zone}.db"
	outfile="${zone}.signed"

	case $zone in
	"dlv.utld")
		algorithm=$DEFAULT_ALGORITHM
		bits=$DEFAULT_BITS
		dlvfiles=$dlvsets
		;;
	"disabled-algorithm-dlv.utld")
		algorithm=$DISABLED_ALGORITHM
		bits=$DISABLED_BITS
		dlvfiles=$disableddlvsets
		;;
	"unsupported-algorithm-dlv.utld")
		algorithm=$DEFAULT_ALGORITHM
		bits=$DEFAULT_BITS
		dlvfiles=$unsupporteddlvsets
		;;
	esac

	keyname1=`$KEYGEN -a $algorithm -b $bits -n zone $zone 2> /dev/null`
	keyname2=`$KEYGEN -f KSK -a $algorithm -b $bits -n zone $zone 2> /dev/null`

	cat $infile $dlvfiles $keyname1.key $keyname2.key >$zonefile

	case $zone in
	"dlv.utld")
		$SIGNER -O full -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
	        keyfile_to_trusted_keys $keyname2 > ../ns5/trusted-dlv.conf
		;;
	"disabled-algorithm-dlv.utld")
		$SIGNER -O full -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
		keyfile_to_trusted_keys $keyname2 > ../ns8/trusted-dlv-disabled.conf
		;;
	"unsupported-algorithm-dlv.utld")
		cp ${keyname2}.key ${keyname2}.tmp
		$SIGNER -O full -o $zone -f ${outfile}.tmp $zonefile > /dev/null 2> signer.err || cat signer.err
		awk '$4 == "DNSKEY" { $7 = 255 } $4 == "RRSIG" { $6 = 255 } { print }' ${outfile}.tmp > $outfile
		awk '$3 == "DNSKEY" { $6 = 255 } { print }' ${keyname2}.tmp > ${keyname2}.key
		keyfile_to_trusted_keys $keyname2 > ../ns7/trusted-dlv-unsupported.conf
		;;
	esac

	echo_i "signed $zone"
done
