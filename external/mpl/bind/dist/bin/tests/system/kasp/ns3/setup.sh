#!/bin/sh -e

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

# shellcheck source=conf.sh
. ../../conf.sh

echo_i "ns3/setup.sh"

# Create key store directories.
mkdir ksk
mkdir zsk

setup() {
  zone="$1"
  echo_i "setting up zone: $zone"
  zonefile="${zone}.db"
  infile="${zone}.db.infile"
  echo "$zone" >>zones
}

# Set in the key state files the Predecessor/Successor fields.
# Key $1 is the predecessor of key $2.
key_successor() {
  id1=$(keyfile_to_key_id "$1")
  id2=$(keyfile_to_key_id "$2")
  echo "Predecessor: ${id1}" >>"${2}.state"
  echo "Successor: ${id2}" >>"${1}.state"
}

# Make lines shorter by storing key states in environment variables.
H="HIDDEN"
R="RUMOURED"
O="OMNIPRESENT"
U="UNRETENTIVE"

#
# Set up zones that will be initially signed.
#
for zn in default dnssec-keygen some-keys legacy-keys pregenerated \
  rumoured rsasha256 rsasha512 ecdsa256 ecdsa384 \
  dynamic dynamic-inline-signing inline-signing \
  checkds-ksk checkds-doubleksk checkds-csk inherit unlimited \
  keystore; do
  setup "${zn}.kasp"
  cp template.db.in "$zonefile"
done

#
# Setup special zone
#
zone="i-am.\":\;?&[]\@!\$*+,|=\.\(\)special.kasp."
echo_i "setting up zone: $zone"
cp template.db.in "i-am.special.kasp.db"

#
# Set up RSASHA1 based zones
#
for zn in rsasha1 rsasha1-nsec3; do
  if [ $RSASHA1_SUPPORTED = 1 ]; then
    setup "${zn}.kasp"
    cp template.db.in "$zonefile"
  else
    # don't add to zones.
    echo_i "setting up zone: ${zn}.kasp"
    cp template.db.in "${zn}.kasp.db"
  fi
done

if [ $ED25519_SUPPORTED = 1 ]; then
  setup "ed25519.kasp"
  cp template.db.in "$zonefile"
  cat ed25519.conf >>named.conf
fi

if [ $ED448_SUPPORTED = 1 ]; then
  setup "ed448.kasp"
  cp template.db.in "$zonefile"
  cat ed448.conf >>named.conf
fi

# Set up zones that stay unsigned.
for zn in unsigned insecure max-zone-ttl; do
  zone="${zn}.kasp"
  echo_i "setting up zone: $zone"
  zonefile="${zone}.db"
  infile="${zone}.db.infile"
  cp template.db.in $infile
  cp template.db.in $zonefile
done

# Some of these zones already have keys.
zone="dnssec-keygen.kasp"
echo_i "setting up zone: $zone"
$KEYGEN -k rsasha256 -l policies/kasp.conf $zone >keygen.out.$zone.1 2>&1

zone="some-keys.kasp"
echo_i "setting up zone: $zone"
$KEYGEN -G -a RSASHA256 -b 2048 -L 1234 $zone >keygen.out.$zone.1 2>&1
$KEYGEN -G -a RSASHA256 -f KSK -L 1234 $zone >keygen.out.$zone.2 2>&1

zone="legacy-keys.kasp"
echo_i "setting up zone: $zone"
ZSK=$($KEYGEN -a RSASHA256 -b 2048 -L 1234 $zone 2>keygen.out.$zone.1)
KSK=$($KEYGEN -a RSASHA256 -f KSK -L 1234 $zone 2>keygen.out.$zone.2)
echo $ZSK >legacy-keys.kasp.zsk
echo $KSK >legacy-keys.kasp.ksk
# Predecessor keys:
Tact="now-9mo"
Tret="now-3mo"
ZSK=$($KEYGEN -a RSASHA256 -b 2048 -L 1234 $zone 2>keygen.out.$zone.3)
KSK=$($KEYGEN -a RSASHA256 -f KSK -L 1234 $zone 2>keygen.out.$zone.4)
$SETTIME -P $Tact -A $Tact -I $Tret -D $Tret "$ZSK" >settime.out.$zone.1 2>&1
$SETTIME -P $Tact -A $Tact -I $Tret -D $Tret "$KSK" >settime.out.$zone.2 2>&1

zone="pregenerated.kasp"
echo_i "setting up zone: $zone"
$KEYGEN -G -k rsasha256 -l policies/kasp.conf $zone >keygen.out.$zone.1 2>&1
$KEYGEN -G -k rsasha256 -l policies/kasp.conf $zone >keygen.out.$zone.2 2>&1

zone="rumoured.kasp"
echo_i "setting up zone: $zone"
Tpub="now"
Tact="now+1d"
keytimes="-P ${Tpub} -A ${Tact}"
KSK=$($KEYGEN -a RSASHA256 -f KSK -L 1234 $keytimes $zone 2>keygen.out.$zone.1)
ZSK1=$($KEYGEN -a RSASHA256 -b 3072 -L 1234 $keytimes $zone 2>keygen.out.$zone.2)
ZSK2=$($KEYGEN -a RSASHA256 -L 1234 $keytimes $zone 2>keygen.out.$zone.3)
$SETTIME -s -g $O -k $R $Tpub -r $R $Tpub -d $H $Tpub "$KSK" >settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $R $Tpub -z $R $Tpub "$ZSK1" >settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $R $Tpub -z $R $Tpub "$ZSK2" >settime.out.$zone.2 2>&1

#
# Set up zones that are already signed.
#

# We are signing the raw version of the zone here. This is unusual and not
# common operation, but want to make sure that in such a case BIND 9 does not
# schedule a resigning operation on the raw version. Add expired signatures so
# a resign is imminent.
setup dynamic-signed-inline-signing.kasp
T="now-1d"
csktimes="-P $T -A $T -P sync $T"
CSK=$($KEYGEN -K keys -a $DEFAULT_ALGORITHM -L 3600 -f KSK $csktimes $zone 2>keygen.out.$zone.1)
$SETTIME -s -g $O -d $O $T -k $O $T -z $O $T -r $O $T "keys/$CSK" >settime.out.$zone.1 2>&1
cat template.db.in "keys/${CSK}.key" >"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "keys/$CSK" >>"$infile"
cp $infile $zonefile
$SIGNER -PS -K keys -z -x -s now-2w -e now-1mi -o $zone -f "${zonefile}.signed" $infile >signer.out.$zone.1 2>&1

# Treat the next zones as if they were signed six months ago.
T="now-6mo"
keytimes="-P $T -A $T"

# These signatures are set to expire long in the past, update immediately.
setup expired-sigs.autosign
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $keytimes $zone 2>keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 $keytimes $zone 2>keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" >settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T "$ZSK" >settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" >"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >>"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >>"$infile"
cp $infile $zonefile
$SIGNER -PS -x -s now-2mo -e now-1mo -o $zone -O raw -f "${zonefile}.signed" $infile >signer.out.$zone.1 2>&1

# The DNSKEY's TTLs do not match the policy.
setup dnskey-ttl-mismatch.autosign
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 30 -f KSK $keytimes $zone 2>keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 30 $keytimes $zone 2>keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" >settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T "$ZSK      " >settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" >"$infile"
cp $infile $zonefile
$SIGNER -PS -x -o $zone -O raw -f "${zonefile}.signed" $infile >signer.out.$zone.1 2>&1

# These signatures are still good, and can be reused.
setup fresh-sigs.autosign
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $keytimes $zone 2>keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 $keytimes $zone 2>keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" >settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T "$ZSK" >settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" >"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >>"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >>"$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile >signer.out.$zone.1 2>&1

# These signatures are still good, but not fresh enough, update immediately.
setup unfresh-sigs.autosign
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $keytimes $zone 2>keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 $keytimes $zone 2>keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" >settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T "$ZSK" >settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" >"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >>"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >>"$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1w -e now+1w -o $zone -O raw -f "${zonefile}.signed" $infile >signer.out.$zone.1 2>&1

# These signatures are still good, but the private KSK is missing.
setup ksk-missing.autosign
# KSK file will be gone missing, so we set expected times during setup.
TI="now+550d"     # Lifetime of 2 years minus 6 months equals 550 days
TD="now+13226h"   # 550 days plus retire time of 1 day 2 hours equals 13226 hours
TS="now-257755mi" # 6 months minus 1 day, 5 minutes equals 257695 minutes
ksktimes="$keytimes -P sync $TS -I $TI -D $TD"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $ksktimes $zone 2>keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 $keytimes $zone 2>keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" >settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T "$ZSK" >settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" >"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >>"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >>"$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1w -e now+1w -o $zone -O raw -f "${zonefile}.signed" $infile >signer.out.$zone.1 2>&1
echo "KSK: yes" >>"${KSK}".state
echo "ZSK: no" >>"${KSK}".state
echo "Lifetime: 63072000" >>"${KSK}".state # PT2Y
rm -f "${KSK}".private

# These signatures are still good, but the private ZSK is missing.
setup zsk-missing.autosign
# ZSK file will be gone missing, so we set expected times during setup.
TI="now+185d"     # Lifetime of 1 year minus 6 months equals 185 days
TD="now+277985mi" # 185 days plus retire time (sign delay, retire safety, propagation, zone TTL)
zsktimes="$keytimes -I $TI -D $TD"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $keytimes $zone 2>keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 $zsktimes $zone 2>keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" >settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T "$ZSK" >settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" >"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >>"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >>"$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1w -e now+1w -o $zone -O raw -f "${zonefile}.signed" $infile >signer.out.$zone.1 2>&1
echo "KSK: no" >>"${ZSK}".state
echo "ZSK: yes" >>"${ZSK}".state
echo "Lifetime: 31536000" >>"${ZSK}".state # PT1Y
rm -f "${ZSK}".private

# These signatures are still good, but the key files will be removed
# before a second run of reconfiguring keys.
setup keyfiles-missing.autosign
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $keytimes $zone 2>keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 $keytimes $zone 2>keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" >settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T "$ZSK" >settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" >"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >>"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >>"$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1w -e now+1w -o $zone -O raw -f "${zonefile}.signed" $infile >signer.out.$zone.1 2>&1

# These signatures are still good, but the key files will be removed
# before a second run of reconfiguring keys, now in manual-mode.
setup keyfiles-missing.manual
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $keytimes $zone 2>keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 $keytimes $zone 2>keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" >settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T "$ZSK" >settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" >"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >>"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >>"$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1w -e now+1w -o $zone -O raw -f "${zonefile}.signed" $infile >signer.out.$zone.1 2>&1

# These signatures are already expired, and the private ZSK is retired.
setup zsk-retired.autosign
zsktimes="$keytimes -I now"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $keytimes $zone 2>keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 $zsktimes $zone 2>keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" >settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T "$ZSK" >settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" >"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >>"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >>"$infile"
cp $infile $zonefile
$SIGNER -PS -x -s now-2w -e now-1mi -o $zone -O raw -f "${zonefile}.signed" $infile >signer.out.$zone.1 2>&1
$SETTIME -s -g HIDDEN "$ZSK" >settime.out.$zone.3 2>&1
# An old key that is being purged should not prevent keymgr to be run.
T1="now-1y"
T2="now-2y"
oldtimes="-P $T2 -A $T2 -I $T1 -D $T1"
OLD=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 $oldtimes $zone 2>keygen.out.$zone.3)
$SETTIME -s -g $H -k $H $T1 -z $H $T1 "$OLD" >settime.out.$zone.3 2>&1
