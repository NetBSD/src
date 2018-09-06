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

dlvzone=dlv.utld.
dlvsets=
dssets=

zone=child1.utld.
infile=child.db.in
zonefile=child1.utld.db
outfile=child1.signed
dlvsets="$dlvsets dlvset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null` 
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.`echo $zone |sed -e "s/\.$//g"`$TP
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


zone=child3.utld.
infile=child.db.in
zonefile=child3.utld.db
outfile=child3.signed
dlvsets="$dlvsets dlvset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.`echo $zone |sed -e "s/\.$//g"`$TP
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


zone=child4.utld.
infile=child.db.in
zonefile=child4.utld.db
outfile=child4.signed
dlvsets="$dlvsets dlvset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


zone=child5.utld.
infile=child.db.in
zonefile=child5.utld.db
outfile=child5.signed
dlvsets="$dlvsets dlvset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.`echo $zone |sed -e "s/\.$//g"`$TP
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


zone=child7.utld.
infile=child.db.in
zonefile=child7.utld.db
outfile=child7.signed

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.`echo $zone |sed -e "s/\.$//g"`$TP
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -r $RANDFILE -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


zone=child8.utld.
infile=child.db.in
zonefile=child8.utld.db
outfile=child8.signed

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


zone=child9.utld.
infile=child.db.in
zonefile=child9.utld.db
outfile=child9.signed
dlvsets="$dlvsets dlvset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

zone=child10.utld.
infile=child.db.in
zonefile=child10.utld.db
outfile=child10.signed
dlvsets="$dlvsets dlvset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

zone=child1.druz.
infile=child.db.in
zonefile=child1.druz.db
outfile=child1.druz.signed
dlvsets="$dlvsets dlvset-`echo $zone |sed -e "s/.$//g"`$TP"
dssets="$dssets dsset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null` 
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.`echo $zone |sed -e "s/\.$//g"`$TP
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


zone=child3.druz.
infile=child.db.in
zonefile=child3.druz.db
outfile=child3.druz.signed
dlvsets="$dlvsets dlvset-`echo $zone |sed -e "s/.$//g"`$TP"
dssets="$dssets dsset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.`echo $zone |sed -e "s/\.$//g"`$TP
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


zone=child4.druz.
infile=child.db.in
zonefile=child4.druz.db
outfile=child4.druz.signed
dlvsets="$dlvsets dlvset-`echo $zone |sed -e "s/.$//g"`$TP"
dssets="$dssets dsset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


zone=child5.druz.
infile=child.db.in
zonefile=child5.druz.db
outfile=child5.druz.signed
dlvsets="$dlvsets dlvset-`echo $zone |sed -e "s/.$//g"`$TP"
dssets="$dssets dsset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.`echo $zone |sed -e "s/\.$//g"`$TP
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


zone=child7.druz.
infile=child.db.in
zonefile=child7.druz.db
outfile=child7.druz.signed
dssets="$dssets dsset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

dsfilename=../ns6/dsset-grand.`echo $zone |sed -e "s/\.$//g"`$TP
cat $infile $keyname1.key $keyname2.key $dsfilename >$zonefile

$SIGNER -r $RANDFILE -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


zone=child8.druz.
infile=child.db.in
zonefile=child8.druz.db
outfile=child8.druz.signed

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


zone=child9.druz.
infile=child.db.in
zonefile=child9.druz.db
outfile=child9.druz.signed
dlvsets="$dlvsets dlvset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

zone=child10.druz.
infile=child.db.in
zonefile=child10.druz.db
outfile=child10.druz.signed
dlvsets="$dlvsets dlvset-`echo $zone |sed -e "s/.$//g"`$TP"
dssets="$dssets dsset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -r $RANDFILE -l $dlvzone -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"


zone=dlv.utld.
infile=dlv.db.in
zonefile=dlv.utld.db
outfile=dlv.signed

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

cat $infile $dlvsets $keyname1.key $keyname2.key >$zonefile

$SIGNER -r $RANDFILE -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

keyfile_to_trusted_keys $keyname2 > trusted-dlv.conf
cp trusted-dlv.conf ../ns5

cp $dssets ../ns2
