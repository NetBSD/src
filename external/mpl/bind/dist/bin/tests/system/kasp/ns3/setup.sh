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
. "$SYSTEMTESTTOP/conf.sh"

echo_i "ns3/setup.sh"

setup() {
	zone="$1"
	echo_i "setting up zone: $zone"
	zonefile="${zone}.db"
	infile="${zone}.db.infile"
	echo "$zone" >> zones
}

# Set in the key state files the Predecessor/Successor fields.
# Key $1 is the predecessor of key $2.
key_successor() {
	id1=$(keyfile_to_key_id "$1")
	id2=$(keyfile_to_key_id "$2")
	echo "Predecessor: ${id1}" >> "${2}.state"
	echo "Successor: ${id2}" >> "${1}.state"
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
	  manual-rollover multisigner-model2
do
	setup "${zn}.kasp"
	cp template.db.in "$zonefile"
done

#
# Set up RSASHA1 based zones
#
for zn in rsasha1 rsasha1-nsec3
do
	if (cd ..; $SHELL ../testcrypto.sh -q RSASHA1)
	then
		setup "${zn}.kasp"
		cp template.db.in "$zonefile"
	else
		# don't add to zones.
		echo_i "setting up zone: ${zn}.kasp"
		cp template.db.in "${zn}.kasp.db"
	fi
done

if [ -f ../ed25519-supported.file ]; then
	setup "ed25519.kasp"
	cp template.db.in "$zonefile"
	cat ed25519.conf >> named.conf
fi

if [ -f ../ed448-supported.file ]; then
	setup "ed448.kasp"
	cp template.db.in "$zonefile"
	cat ed448.conf >> named.conf
fi

# Set up zones that stay unsigned.
for zn in unsigned insecure max-zone-ttl
do
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
$KEYGEN -k rsasha256 -l policies/kasp.conf $zone > keygen.out.$zone.1 2>&1

zone="some-keys.kasp"
echo_i "setting up zone: $zone"
$KEYGEN -G -a RSASHA256 -b 2048 -L 1234 $zone > keygen.out.$zone.1 2>&1
$KEYGEN -G -a RSASHA256 -f KSK  -L 1234 $zone > keygen.out.$zone.2 2>&1

zone="legacy-keys.kasp"
echo_i "setting up zone: $zone"
ZSK=$($KEYGEN -a RSASHA256 -b 2048 -L 1234 $zone 2> keygen.out.$zone.1)
KSK=$($KEYGEN -a RSASHA256 -f KSK  -L 1234 $zone 2> keygen.out.$zone.2)
echo $ZSK > legacy-keys.kasp.zsk
echo $KSK > legacy-keys.kasp.ksk
# Predecessor keys:
Tact="now-9mo"
Tret="now-3mo"
ZSK=$($KEYGEN -a RSASHA256 -b 2048 -L 1234 $zone 2> keygen.out.$zone.3)
KSK=$($KEYGEN -a RSASHA256 -f KSK  -L 1234 $zone 2> keygen.out.$zone.4)
$SETTIME -P $Tact -A $Tact -I $Tret -D $Tret "$ZSK"  > settime.out.$zone.1 2>&1
$SETTIME -P $Tact -A $Tact -I $Tret -D $Tret "$KSK"  > settime.out.$zone.2 2>&1

zone="pregenerated.kasp"
echo_i "setting up zone: $zone"
$KEYGEN -G -k rsasha256 -l policies/kasp.conf $zone > keygen.out.$zone.1 2>&1
$KEYGEN -G -k rsasha256 -l policies/kasp.conf $zone > keygen.out.$zone.2 2>&1

zone="multisigner-model2.kasp"
echo_i "setting up zone: $zone"
# Import the ZSK sets of the other providers into their DNSKEY RRset.
ZSK1=$($KEYGEN -K ../ -a $DEFAULT_ALGORITHM -L 3600 $zone 2> keygen.out.$zone.1)
ZSK2=$($KEYGEN -K ../ -a $DEFAULT_ALGORITHM -L 3600 $zone 2> keygen.out.$zone.2)
# ZSK1 will be added to the unsigned zonefile.
cat "../${ZSK1}.key" | grep -v ";.*" >> "${zone}.db"
cat "../${ZSK1}.key" | grep -v ";.*" > "${zone}.zsk1"
rm -f "../${ZSK1}.*"
# ZSK2 will be used with a Dynamic Update.
cat "../${ZSK2}.key" | grep -v ";.*" > "${zone}.zsk2"
rm -f "../${ZSK2}.*"

zone="rumoured.kasp"
echo_i "setting up zone: $zone"
Tpub="now"
Tact="now+1d"
keytimes="-P ${Tpub} -A ${Tact}"
KSK=$($KEYGEN  -a RSASHA256 -f KSK  -L 1234 $keytimes $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a RSASHA256 -b 3072 -L 1234 $keytimes $zone 2> keygen.out.$zone.2)
ZSK2=$($KEYGEN -a RSASHA256         -L 1234 $keytimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $O -k $R $Tpub -r $R $Tpub -d $H $Tpub  "$KSK"  > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $R $Tpub -z $R $Tpub              "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $R $Tpub -z $R $Tpub              "$ZSK2" > settime.out.$zone.2 2>&1

#
# Set up zones that are already signed.
#

# Zone to test manual rollover.
setup manual-rollover.kasp
T="now-1d"
ksktimes="-P $T -A $T -P sync $T"
zsktimes="-P $T -A $T"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T          "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -PS -x -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# These signatures are set to expire long in the past, update immediately.
setup expired-sigs.autosign
T="now-6mo"
ksktimes="-P $T -A $T -P sync $T"
zsktimes="-P $T -A $T"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T          "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -PS -x -s now-2mo -e now-1mo -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# These signatures are still good, and can be reused.
setup fresh-sigs.autosign
T="now-6mo"
ksktimes="-P $T -A $T -P sync $T"
zsktimes="-P $T -A $T"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T          "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# These signatures are still good, but not fresh enough, update immediately.
setup unfresh-sigs.autosign
T="now-6mo"
ksktimes="-P $T -A $T -P sync $T"
zsktimes="-P $T -A $T"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T          "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1w -e now+1w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# These signatures are still good, but the private KSK is missing.
setup ksk-missing.autosign
T="now-6mo"
ksktimes="-P $T -A $T -P sync $T"
zsktimes="-P $T -A $T"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T          "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1w -e now+1w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1
echo "KSK: yes" >> "${KSK}".state
echo "ZSK: no" >> "${KSK}".state
echo "Lifetime: 63072000" >> "${KSK}".state # PT2Y
rm -f "${KSK}".private

# These signatures are still good, but the private ZSK is missing.
setup zsk-missing.autosign
T="now-6mo"
ksktimes="-P $T -A $T -P sync $T"
zsktimes="-P $T -A $T"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T          "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1w -e now+1w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1
echo "KSK: no" >> "${ZSK}".state
echo "ZSK: yes" >> "${ZSK}".state
echo "Lifetime: 31536000" >> "${ZSK}".state # PT1Y
rm -f "${ZSK}".private

# These signatures are already expired, and the private ZSK is retired.
setup zsk-retired.autosign
T="now-6mo"
ksktimes="-P $T -A $T -P sync $T"
zsktimes="-P $T -A $T -I now"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 300        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T          "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -PS -x -s now-2w -e now-1mi -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1
$SETTIME -s -g HIDDEN "$ZSK" > settime.out.$zone.3 2>&1

#
# The zones at enable-dnssec.autosign represent the various steps of the
# initial signing of a zone.
#

# Step 1:
# This is an unsigned zone and named should perform the initial steps of
# introducing the DNSSEC records in the right order.
setup step1.enable-dnssec.autosign
cp template.db.in $zonefile

# Step 2:
# The DNSKEY has been published long enough to become OMNIPRESENT.
setup step2.enable-dnssec.autosign
# DNSKEY TTL:             300 seconds
# zone-propagation-delay: 5 minutes (300 seconds)
# publish-safety:         5 minutes (300 seconds)
# Total:                  900 seconds
TpubN="now-900s"
# RRSIG TTL:              12 hour (43200 seconds)
# zone-propagation-delay: 5 minutes (300 seconds)
# retire-safety:          20 minutes (1200 seconds)
# Already passed time:    -900 seconds
# Total:                  43800 seconds
TsbmN="now+43800s"
keytimes="-P ${TpubN} -P sync ${TsbmN} -A ${TpubN}"
CSK=$($KEYGEN -k enable-dnssec -l policies/autosign.conf $keytimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -k $R $TpubN -r $R $TpubN -d $H $TpubN -z $R $TpubN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 3:
# The zone signatures have been published long enough to become OMNIPRESENT.
setup step3.enable-dnssec.autosign
# Passed time since publications: 43800 + 900 = 44700 seconds.
TpubN="now-44700s"
# The key is secure for using in chain of trust when the DNSKEY is OMNIPRESENT.
TcotN="now-43800s"
# We can submit the DS now.
TsbmN="now"
keytimes="-P ${TpubN} -P sync ${TsbmN} -A ${TpubN}"
CSK=$($KEYGEN -k enable-dnssec -l policies/autosign.conf $keytimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -k $O $TcotN -r $O $TcotN -d $H $TpubN -z $R $TpubN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 4:
# The DS has been submitted long enough ago to become OMNIPRESENT.
setup step4.enable-dnssec.autosign
# DS TTL:                    2 hour (7200 seconds)
# parent-propagation-delay:  1 hour (3600 seconds)
# retire-safety:             20 minutes (1200 seconds)
# Total aditional time:      12000 seconds
# 44700 + 12000 = 56700
TpubN="now-56700s"
# 43800 + 12000 = 55800
TcotN="now-55800s"
TsbmN="now-12000s"
keytimes="-P ${TpubN} -P sync ${TsbmN} -A ${TpubN}"
CSK=$($KEYGEN -k enable-dnssec -l policies/autosign.conf $keytimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -P ds $TsbmN -k $O $TcotN -r $O $TcotN -d $R $TsbmN -z $O $TsbmN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1
setup step4.enable-dnssec.autosign

#
# The zones at zsk-prepub.autosign represent the various steps of a ZSK
# Pre-Publication rollover.
#

# Step 1:
# Introduce the first key. This will immediately be active.
setup step1.zsk-prepub.autosign
TactN="now"
ksktimes="-P ${TactN} -A ${TactN} -P sync ${TactN}"
zsktimes="-P ${TactN} -A ${TactN}"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN -z $O $TactN              "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 2:
# It is time to pre-publish the successor ZSK.
setup step2.zsk-prepub.autosign
# According to RFC 7583:
#
# Tpub(N+1) <= Tact(N) + Lzsk - Ipub
# Ipub = Dprp + TTLkey (+publish-safety)
#
#                 |3|   |4|      |5|  |6|
#                  |     |        |    |
#   Key N          |<-------Lzsk------>|
#                  |     |        |    |
#   Key N+1        |     |<-Ipub->|<-->|
#                  |     |        |    |
#   Key N         Tact
#   Key N+1             Tpub     Trdy Tact
#
#                       Tnow
#
# Lzsk:           30d
# Dprp:           1h
# TTLkey:         1h
# publish-safety: 1d
# Ipub:           26h
#
# Tact(N) = Tnow + Ipub - Lzsk = now + 26h - 30d
#         = now + 26h - 30d = now − 694h
TactN="now-694h"
ksktimes="-P ${TactN} -A ${TactN} -P sync ${TactN}"
zsktimes="-P ${TactN} -A ${TactN}"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN -z $O $TactN              "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 3:
# After the publication interval has passed the DNSKEY of the successor ZSK
# is OMNIPRESENT and the zone can thus be signed with the successor ZSK.
setup step3.zsk-prepub.autosign
# According to RFC 7583:
#
# Tpub(N+1) <= Tact(N) + Lzsk - Ipub
# Tret(N) = Tact(N+1) = Tact(N) + Lzsk
# Trem(N) = Tret(N) + Iret
# Iret = Dsgn + Dprp + TTLsig (+retire-safety)
#
#                 |3|   |4|      |5|  |6|      |7|   |8|
#                  |     |        |    |        |     |
#   Key N          |<-------Lzsk------>|<-Iret->|<--->|
#                  |     |        |    |        |     |
#   Key N+1        |     |<-Ipub->|<-->|<---Lzsk---- - -
#                  |     |        |    |        |     |
#   Key N         Tact                Tret     Tdea  Trem
#   Key N+1             Tpub     Trdy Tact
#
#                                     Tnow
#
# Lzsk:          30d
# Ipub:          26h
# Dsgn:          1w
# Dprp:          1h
# TTLsig:        1d
# retire-safety: 2d
# Iret:          10d1h = 241h
#
# Tact(N)   = Tnow - Lzsk = now - 30d
# Tret(N)   = now
# Trem(N)   = Tnow + Iret = now + 241h
# Tpub(N+1) = Tnow - Ipub = now - 26h
# Tret(N+1) = Tnow + Lzsk = now + 30d
# Trem(N+1) = Tnow + Lzsk + Iret = now + 30d + 241h
#           = now + 961h
TactN="now-30d"
TretN="now"
TremN="now+241h"
TpubN1="now-26h"
TactN1="now"
TretN1="now+30d"
TremN1="now+961h"
ksktimes="-P ${TactN}  -A ${TactN}  -P sync ${TactN}"
zsktimes="-P ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
KSK=$($KEYGEN  -a $DEFAULT_ALGORITHM -L 3600 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $zsktimes $zone 2> keygen.out.$zone.2)
ZSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $newtimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $O -k $O $TactN  -r $O $TactN  -d $O $TactN "$KSK"  > settime.out.$zone.1 2>&1
$SETTIME -s -g $H -k $O $TactN  -z $O $TactN               "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $R $TpubN1 -z $H $TpubN1              "$ZSK2" > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $ZSK1 $ZSK2
# Sign zone.
cat template.db.in "${KSK}.key" "${ZSK1}.key" "${ZSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK"  >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 4:
# After the retire interval has passed the predecessor DNSKEY can be
# removed from the zone.
setup step4.zsk-prepub.autosign
# According to RFC 7583:
#
# Tret(N) = Tact(N) + Lzsk
# Tdea(N) = Tret(N) + Iret
#
#                 |3|   |4|      |5|  |6|      |7|   |8|
#                  |     |        |    |        |     |
#   Key N          |<-------Lzsk------>|<-Iret->|<--->|
#                  |     |        |    |        |     |
#   Key N+1        |     |<-Ipub->|<-->|<---Lzsk---- - -
#                  |     |        |    |        |     |
#   Key N         Tact                Tret     Tdea  Trem
#   Key N+1             Tpub     Trdy Tact
#
#                                                    Tnow
#
# Lzsk: 30d
# Ipub: 26h
# Iret: 241h
#
# Tact(N)   = Tnow - Iret - Lzsk
#           = now - 241h - 30d = now - 241h - 720h
#           = now - 961h
# Tret(N)   = Tnow - Iret = now - 241h
# Trem(N)   = Tnow
# Tpub(N+1) = Tnow - Iret - Ipub
#           = now - 241h - 26h
#           = now - 267h
# Tact(N+1) = Tnow - Iret = Tret(N)
# Tret(N+1) = Tnow - Iret + Lzsk
#           = now - 241h + 30d = now - 241h + 720h
#           = now + 479h
# Trem(N+1) = Tnow + Lzsk = now + 30d
TactN="now-961h"
TretN="now-241h"
TremN="now"
TpubN1="now-267h"
TactN1="${TretN}"
TretN1="now+479h"
TremN1="now+30d"
ksktimes="-P ${TactN}  -A ${TactN}  -P sync ${TactN}"
zsktimes="-P ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
KSK=$($KEYGEN  -a $DEFAULT_ALGORITHM -L 3600 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $zsktimes $zone 2> keygen.out.$zone.2)
ZSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $newtimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $O -k $O $TactN  -r $O $TactN -d $O $TactN "$KSK"  > settime.out.$zone.1 2>&1
$SETTIME -s -g $H -k $O $TactN  -z $U $TretN              "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TactN1 -z $R $TactN1             "$ZSK2" > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $ZSK1 $ZSK2
# Sign zone.
cat template.db.in "${KSK}.key" "${ZSK1}.key" "${ZSK2}.key" > "$infile"
cp $infile $zonefile
$SIGNER -PS -x -s now-2w -e now-1mi -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 5:
# The predecessor DNSKEY is removed long enough that is has become HIDDEN.
setup step5.zsk-prepub.autosign
# Subtract DNSKEY TTL from all the times (1h).
# Tact(N)   = now - 961h - 1h = now - 962h
# Tret(N)   = now - 241h - 1h = now - 242h
# Tdea(N)   = now - 2d - 1h = now - 49h
# Trem(N)   = now - 1h
# Tpub(N+1) = now - 267h - 1h = now - 268h
# Tact(N+1) = Tret(N)
# Tret(N+1) = now + 479h - 1h = now + 478h
# Trem(N+1) = now + 30d - 1h = now + 719h
TactN="now-962h"
TretN="now-242h"
TremN="now-1h"
TdeaN="now-49h"
TpubN1="now-268h"
TactN1="${TretN}"
TretN1="now+478h"
TremN1="now+719h"
ksktimes="-P ${TactN}  -A ${TactN}  -P sync ${TactN}"
zsktimes="-P ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
KSK=$($KEYGEN  -a $DEFAULT_ALGORITHM -L 3600 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $zsktimes $zone 2> keygen.out.$zone.2)
ZSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $newtimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $O -k $O $TactN  -r $O $TactN -d $O $TactN "$KSK"  > settime.out.$zone.1 2>&1
$SETTIME -s -g $H -k $U $TdeaN  -z $H $TdeaN              "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TactN1 -z $O $TdeaN              "$ZSK2" > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $ZSK1 $ZSK2
# Sign zone.
cat template.db.in "${KSK}.key" "${ZSK1}.key" "${ZSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK"  >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 6:
# The predecessor DNSKEY can be purged.
setup step6.zsk-prepub.autosign
# Subtract purge-keys interval from all the times (1h).
# Tact(N)   = now - 962h - 1h = now - 963h
# Tret(N)   = now - 242h - 1h = now - 243h
# Tdea(N)   = now - 49h - 1h = now - 50h
# Trem(N)   = now - 1h - 1h = now - 2h
# Tpub(N+1) = now - 268h - 1h = now - 269h
# Tact(N+1) = Tret(N)
# Tret(N+1) = now + 478h - 1h = now + 477h
# Trem(N+1) = now + 719h - 1h = now + 718h
TactN="now-963h"
TretN="now-243h"
TremN="now-2h"
TdeaN="now-50h"
TpubN1="now-269h"
TactN1="${TretN}"
TretN1="now+477h"
TremN1="now+718h"
ksktimes="-P ${TactN}  -A ${TactN}  -P sync ${TactN}"
zsktimes="-P ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
KSK=$($KEYGEN  -a $DEFAULT_ALGORITHM -L 3600 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $zsktimes $zone 2> keygen.out.$zone.2)
ZSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $newtimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $O -k $O $TactN  -r $O $TactN -d $O $TactN "$KSK"  > settime.out.$zone.1 2>&1
$SETTIME -s -g $H -k $H $TdeaN  -z $H $TdeaN              "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TactN1 -z $O $TdeaN              "$ZSK2" > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $ZSK1 $ZSK2
# Sign zone.
cat template.db.in "${KSK}.key" "${ZSK1}.key" "${ZSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK"  >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

#
# The zones at ksk-doubleksk.autosign represent the various steps of a KSK
# Double-KSK rollover.
#

# Step 1:
# Introduce the first key. This will immediately be active.
setup step1.ksk-doubleksk.autosign
TactN="now"
ksktimes="-P ${TactN} -A ${TactN} -P sync ${TactN}"
zsktimes="-P ${TactN} -A ${TactN}"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O              -k $O $TactN -z $O $TactN "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 2:
# It is time to submit the introduce the new KSK.
setup step2.ksk-doubleksk.autosign
# According to RFC 7583:
#
# Tpub(N+1) <= Tact(N) + Lksk - Dreg - IpubC
# IpubC = DprpC + TTLkey (+publish-safety)
#
#                       |1|       |2|   |3|      |4|
#                        |         |     |        |
#       Key N            |<-IpubC->|<--->|<-Dreg->|<-----Lksk--- - -
#                        |         |     |        |
#       Key N+1          |         |     |        |
#                        |         |     |        |
#       Key N           Tpub      Trdy  Tsbm     Tact
#       Key N+1
#
#               (continued ...)
#
#                   |5|       |6|   |7|      |8|      |9|    |10|
#                    |         |     |        |        |       |
#       Key N   - - --------------Lksk------->|<-Iret->|<----->|
#                    |         |     |        |        |       |
#       Key N+1      |<-IpubC->|<--->|<-Dreg->|<--------Lksk----- - -
#                    |         |     |        |        |       |
#       Key N                                Tret     Tdea    Trem
#       Key N+1     Tpub      Trdy  Tsbm     Tact
#
#                   Tnow
#
# Lksk:           60d
# Dreg:           1d
# DprpC:          1h
# TTLkey:         2h
# publish-safety: 1d
# IpubC:          27h
#
# Tact(N) = Tnow - Lksk + Dreg + IpubC = now - 60d + 27h
#         = now - 1440h + 27h = now - 1413h
TactN="now-1413h"
ksktimes="-P ${TactN} -A ${TactN} -P sync ${TactN}"
zsktimes="-P ${TactN} -A ${TactN}"
KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN -z $O $TactN              "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 3:
# It is time to submit the DS.
setup step3.ksk-doubleksk.autosign
# According to RFC 7583:
#
# Tsbm(N+1) >= Trdy(N+1)
# Tact(N+1) = Tsbm(N+1) + Dreg
# Iret = DprpP + TTLds (+retire-safety)
#
#                   |5|       |6|   |7|      |8|      |9|    |10|
#                    |         |     |        |        |       |
#       Key N   - - --------------Lksk------->|<-Iret->|<----->|
#                    |         |     |        |        |       |
#       Key N+1      |<-IpubC->|<--->|<-Dreg->|<--------Lksk----- - -
#                    |         |     |        |        |       |
#       Key N                                Tret     Tdea    Trem
#       Key N+1     Tpub      Trdy  Tsbm     Tact
#
#                                   Tnow
#
# Lksk:           60d
# Dreg:           N/A
# DprpP:          1h
# TTLds:          1h
# retire-safety:  2d
# Iret:           50h
# DprpC:          1h
# TTLkey:         2h
# publish-safety: 1d
# IpubC:          27h
#
# Tact(N)    = Tnow + Lksk = now - 60d = now - 60d
# Tret(N)    = now
# Trem(N)    = Tnow + Iret = now + 50h
# Tpub(N+1)  = Tnow - IpubC = now - 27h
# Tsbm(N+1)  = now
# Tact(N+1)  = Tret(N)
# Tret(N+1)  = Tnow + Lksk = now + 60d
# Trem(N+1)  = Tnow + Lksk + Iret = now + 60d + 50h
#            = now + 1440h + 50h = 1490h
TactN="now-60d"
TretN="now"
TremN="now+50h"
TpubN1="now-27h"
TsbmN1="now"
TactN1="${TretN}"
TretN1="now+60d"
TremN1="now+1490h"
ksktimes="-P ${TactN}  -A ${TactN}  -P sync ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -A ${TactN1} -P sync ${TsbmN1} -I ${TretN1} -D ${TremN1}"
zsktimes="-P ${TactN}  -A ${TactN}"
KSK1=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
KSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200 -f KSK $newtimes $zone 2> keygen.out.$zone.2)
ZSK=$($KEYGEN  -a $DEFAULT_ALGORITHM -L 7200        $zsktimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $H -k $O $TactN   -r $O $TactN  -d $O $TactN  "$KSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $R $TpubN1  -r $R $TpubN1 -d $H $TpubN1 "$KSK2" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TactN   -z $O $TactN                "$ZSK"  > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $KSK1 $KSK2
# Sign zone.
cat template.db.in "${KSK1}.key" "${KSK2}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK2" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 4:
# The DS should be swapped now.
setup step4.ksk-doubleksk.autosign
# According to RFC 7583:
#
# Tret(N)   = Tsbm(N+1)
# Tdea(N)   = Tret(N) + Iret
# Tact(N+1) = Tret(N)
#
#                   |5|       |6|   |7|      |8|      |9|    |10|
#                    |         |     |        |        |       |
#       Key N   - - --------------Lksk------->|<-Iret->|<----->|
#                    |         |     |        |        |       |
#       Key N+1      |<-IpubC->|<--->|<-Dreg->|<--------Lksk----- - -
#                    |         |     |        |        |       |
#       Key N                                Tret     Tdea    Trem
#       Key N+1     Tpub      Trdy  Tsbm     Tact
#
#                                                             Tnow
#
# Lksk: 60d
# Dreg: N/A
# Iret: 50h
#
# Tact(N)   = Tnow - Lksk - Iret = now - 60d - 50h
#           = now - 1440h - 50h = now - 1490h
# Tret(N)   = Tnow - Iret = now - 50h
# Trem(N)   = Tnow
# Tpub(N+1) = Tnow - Iret - IpubC = now - 50h - 27h
#           = now - 77h
# Tsbm(N+1) = Tnow - Iret = now - 50h
# Tact(N+1) = Tret(N)
# Tret(N+1) = Tnow + Lksk - Iret = now + 60d - 50h = now + 1390h
# Trem(N+1) = Tnow + Lksk = now + 60d
TactN="now-1490h"
TretN="now-50h"
TremN="now"
TpubN1="now-77h"
TsbmN1="now-50h"
TactN1="${TretN}"
TretN1="now+1390h"
TremN1="now+60d"
ksktimes="-P ${TactN}  -A ${TactN} -P sync ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -A ${TretN} -P sync ${TsbmN1} -I ${TretN1} -D ${TremN1}"
zsktimes="-P ${TactN}  -A ${TactN}"
KSK1=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
KSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200 -f KSK $newtimes $zone 2> keygen.out.$zone.2)
ZSK=$($KEYGEN  -a $DEFAULT_ALGORITHM -L 7200        $zsktimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $H -k $O $TactN  -r $O $TactN  -d $U $TsbmN1 -D ds $TsbmN1 "$KSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $R $TsbmN1 -P ds $TsbmN1 "$KSK2" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TactN  -z $O $TactN                              "$ZSK"  > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $KSK1 $KSK2
# Sign zone.
cat template.db.in "${KSK1}.key" "${KSK2}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK2" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 5:
# The predecessor DNSKEY is removed long enough that is has become HIDDEN.
setup step5.ksk-doubleksk.autosign
# Subtract DNSKEY TTL from all the times (2h).
# Tact(N)   = now - 1490h - 2h = now - 1492h
# Tret(N)   = now - 50h - 2h = now - 52h
# Trem(N)   = now - 2h
# Tpub(N+1) = now - 77h - 2h = now - 79h
# Tsbm(N+1) = now - 50h - 2h = now - 52h
# Tact(N+1) = Tret(N)
# Tret(N+1) = now + 1390h - 2h = now + 1388h
# Trem(N+1) = now + 60d - 2h = now + 1442h
TactN="now-1492h"
TretN="now-52h"
TremN="now-2h"
TpubN1="now-79h"
TsbmN1="now-52h"
TactN1="${TretN}"
TretN1="now+1388h"
TremN1="now+1442h"
ksktimes="-P ${TactN}  -A ${TactN} -P sync ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -A ${TretN} -P sync ${TsbmN1} -I ${TretN1} -D ${TremN1}"
zsktimes="-P ${TactN}  -A ${TactN}"
KSK1=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
KSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200 -f KSK $newtimes $zone 2> keygen.out.$zone.2)
ZSK=$($KEYGEN  -a $DEFAULT_ALGORITHM -L 7200        $zsktimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $H -k $U $TretN  -r $U $TretN  -d $H $TretN  "$KSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN1 -r $O $TactN1 -d $O $TactN1 "$KSK2" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TactN  -z $O $TactN                "$ZSK"  > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $KSK1 $KSK2
# Sign zone.
cat template.db.in "${KSK1}.key" "${KSK2}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK2" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 6:
# The predecessor DNSKEY can be purged.
setup step6.ksk-doubleksk.autosign
# Subtract purge-keys interval from all the times (1h).
# Tact(N)   = now - 1492h - 1h = now - 1493h
# Tret(N)   = now - 52h - 1h = now - 53h
# Trem(N)   = now - 2h - 1h = now - 3h
# Tpub(N+1) = now - 79h - 1h = now - 80h
# Tsbm(N+1) = now - 52h - 1h = now - 53h
# Tact(N+1) = Tret(N)
# Tret(N+1) = now + 1388h - 1h = now + 1387h
# Trem(N+1) = now + 1442h - 1h = now + 1441h
TactN="now-1493h"
TretN="now-53h"
TremN="now-3h"
TpubN1="now-80h"
TsbmN1="now-53h"
TactN1="${TretN}"
TretN1="now+1387h"
TremN1="now+1441h"
ksktimes="-P ${TactN}  -A ${TactN} -P sync ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -A ${TretN} -P sync ${TsbmN1} -I ${TretN1} -D ${TremN1}"
zsktimes="-P ${TactN}  -A ${TactN}"
KSK1=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
KSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200 -f KSK $newtimes $zone 2> keygen.out.$zone.2)
ZSK=$($KEYGEN  -a $DEFAULT_ALGORITHM -L 7200        $zsktimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $H -k $H $TretN  -r $H $TretN  -d $H $TretN  "$KSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN1 -r $O $TactN1 -d $O $TactN1 "$KSK2" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TactN  -z $O $TactN                "$ZSK"  > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $KSK1 $KSK2
# Sign zone.
cat template.db.in "${KSK1}.key" "${KSK2}.key" "${ZSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK2" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

#
# The zones at csk-roll.autosign represent the various steps of a CSK rollover
# (which is essentially a ZSK Pre-Publication / KSK Double-KSK rollover).
#

# Step 1:
# Introduce the first key. This will immediately be active.
setup step1.csk-roll.autosign
TactN="now"
csktimes="-P ${TactN} -P sync ${TactN} -A ${TactN}"
CSK=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN -z $O $TactN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 2:
# It is time to introduce the new CSK.
setup step2.csk-roll.autosign
# According to RFC 7583:
# KSK: Tpub(N+1) <= Tact(N) + Lksk - IpubC
# ZSK: Tpub(N+1) <= Tact(N) + Lzsk - Ipub
# IpubC = DprpC + TTLkey (+publish-safety)
# Ipub  = IpubC
# Lcsk = Lksk = Lzsk
#
# Lcsk:           6mo (186d, 4464h)
# Dreg:           N/A
# DprpC:          1h
# TTLkey:         1h
# publish-safety: 1h
# Ipub:           3h
#
# Tact(N) = Tnow - Lcsk + Ipub = now - 186d + 3h
#         = now - 4464h + 3h = now - 4461h
TactN="now-4461h"
csktimes="-P ${TactN} -P sync ${TactN} -A ${TactN}"
CSK=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN -z $O $TactN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 3:
# It is time to submit the DS and to roll signatures.
setup step3.csk-roll.autosign
# According to RFC 7583:
#
# Tsbm(N+1) >= Trdy(N+1)
# KSK: Tact(N+1) = Tsbm(N+1)
# ZSK: Tact(N+1) = Tpub(N+1) + Ipub = Tsbm(N+1)
# KSK: Iret  = DprpP + TTLds (+retire-safety)
# ZSK: IretZ = Dsgn + Dprp + TTLsig (+retire-safety)
#
# Lcsk:           186d
# Dprp:           1h
# DprpP:          1h
# Dreg:           N/A
# Dsgn:           25d
# TTLds:          1h
# TTLsig:         1d
# retire-safety:  2h
# Iret:           4h
# IretZ:          26d3h
# Ipub:           3h
#
# Tact(N)   = Tnow - Lcsk = now - 186d
# Tret(N)   = now
# Trem(N)   = Tnow + IretZ = now + 26d3h = now + 627h
# Tpub(N+1) = Tnow - Ipub = now - 3h
# Tsbm(N+1) = Tret(N)
# Tact(N+1) = Tret(N)
# Tret(N+1) = Tnow + Lcsk = now + 186d = now + 186d
# Trem(N+1) = Tnow + Lcsk + IretZ = now + 186d + 26d3h =
#           = now + 5091h
TactN="now-186d"
TretN="now"
TremN="now+627h"
TpubN1="now-3h"
TsbmN1="now"
TactN1="${TretN}"
TretN1="now+186d"
TremN1="now+5091h"
csktimes="-P ${TactN}  -P sync ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactN  -r $O $TactN  -d $O $TactN  -z $O $TactN  "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $R $TpubN1 -r $R $TpubN1 -d $H $TpubN1 -z $H $TpubN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 4:
# Some time later all the ZRRSIG records should be from the new CSK, and the
# DS should be swapped.  The ZRRSIG records are all replaced after IretZ
# (which is 26d3h).  The DS is swapped after Iret (which is 4h).
# In other words, the DS is swapped before all zone signatures are replaced.
setup step4.csk-roll.autosign
# According to RFC 7583:
# Trem(N)    = Tret(N) - Iret + IretZ
# Tnow       = Tsbm(N+1) + Iret
#
# Lcsk:   186d
# Iret:   4h
# IretZ:  26d3h
#
# Tact(N)   = Tnow - Iret - Lcsk = now - 4h - 186d = now - 4468h
# Tret(N)   = Tnow - Iret = now - 4h = now - 4h
# Trem(N)   = Tnow - Iret + IretZ = now - 4h + 26d3h
#           = now + 623h
# Tpub(N+1) = Tnow - Iret - IpubC = now - 4h - 3h = now - 7h
# Tsbm(N+1) = Tret(N)
# Tact(N+1) = Tret(N)
# Tret(N+1) = Tnow - Iret + Lcsk = now - 4h + 186d = now + 4460h
# Trem(N+1) = Tnow - Iret + Lcsk + IretZ = now - 4h + 186d + 26d3h
#	    = now + 5087h
TactN="now-4468h"
TretN="now-4h"
TremN="now+623h"
TpubN1="now-7h"
TsbmN1="${TretN}"
TactN1="${TretN}"
TretN1="now+4460h"
TremN1="now+5087h"
csktimes="-P ${TactN}  -P sync ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactN  -r $O $TactN  -d $U $TsbmN1 -z $U $TsbmN1 -D ds $TsbmN1 "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $R $TsbmN1 -z $R $TsbmN1 -P ds $TsbmN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 5:
# After the DS is swapped in step 4, also the KRRSIG records can be removed.
# At this time these have all become hidden.
setup step5.csk-roll.autosign
# Subtract DNSKEY TTL plus zone propagation delay from all the times (2h).
# Tact(N)   = now - 4468h - 2h = now - 4470h
# Tret(N)   = now - 4h - 2h = now - 6h
# Trem(N)   = now + 623h - 2h = now + 621h
# Tpub(N+1) = now - 7h - 2h = now - 9h
# Tsbm(N+1) = Tret(N)
# Tact(N+1) = Tret(N)
# Tret(N+1) = now + 4460h - 2h = now + 4458h
# Trem(N+1) = now + 5087h - 2h = now + 5085h
TactN="now-4470h"
TretN="now-6h"
TremN="now+621h"
TpubN1="now-9h"
TsbmN1="${TretN}"
TactN1="${TretN}"
TretN1="now+4458h"
TremN1="now+5085h"
csktimes="-P ${TactN}  -P sync ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactN  -r $U now-2h  -d $H now-2h -z $U $TactN1 "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $O now-2h -z $R $TactN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 6:
# After the retire interval has passed the predecessor DNSKEY can be
# removed from the zone.
setup step6.csk-roll.autosign
# According to RFC 7583:
# Trem(N) = Tret(N) + IretZ
# Tret(N) = Tact(N) + Lcsk
#
# Lcsk:   186d
# Iret:   4h
# IretZ:  26d3h
#
# Tact(N)   = Tnow - IretZ - Lcsk = now - 627h - 186d
#           = now - 627h - 4464h = now - 5091h
# Tret(N)   = Tnow - IretZ = now - 627h
# Trem(N)   = Tnow
# Tpub(N+1) = Tnow - IretZ - Ipub = now - 627h - 3h = now - 630h
# Tsbm(N+1) = Tret(N)
# Tact(N+1) = Tret(N)
# Tret(N+1) = Tnow - IretZ + Lcsk = now - 627h + 186d = now + 3837h
# Trem(N+1) = Tnow + Lcsk = now + 186d
TactN="now-5091h"
TretN="now-627h"
TremN="now"
TpubN1="now-630h"
TsbmN1="${TretN}"
TactN1="${TretN}"
TretN1="now+3837h"
TremN1="now+186d"
csktimes="-P ${TactN}  -P sync ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactN  -r $H $TremN  -d $H $TremN  -z $U $TsbmN1 "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $O $TremN  -z $R $TsbmN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 7:
# Some time later the predecessor DNSKEY enters the HIDDEN state.
setup step7.csk-roll.autosign
# Subtract DNSKEY TTL plus zone propagation delay from all the times (2h).
# Tact(N) = now - 5091h - 2h = now - 5093h
# Tret(N) = now - 627h - 2h  = now - 629h
# Trem(N) = now - 2h
# Tpub(N+1) = now - 630h - 2h = now - 632h
# Tsbm(N+1) = Tret(N)
# Tact(N+1) = Tret(N)
# Tret(N+1) = now + 3837h - 2h = now + 3835h
# Trem(N+1) = now + 186d - 2h = now + 4462h
TactN="now-5093h"
TretN="now-629h"
TremN="now-2h"
TpubN1="now-632h"
TsbmN1="${TretN}"
TactN1="${TretN}"
TretN1="now+3835h"
TremN1="now+4462h"
csktimes="-P ${TactN}  -P sync ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $U $TremN  -r $H $TremN  -d $H $TremN  -z $H $TactN1 "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $O $TactN1 -z $O $TactN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 8:
# The predecessor DNSKEY can be purged.
setup step8.csk-roll.autosign
# Subtract purge-keys interval from all the times (1h).
# Tact(N) = now - 5093h - 1h = now - 5094h
# Tret(N) = now - 629h - 1h  = now - 630h
# Trem(N) = now - 2h - 1h = now - 3h
# Tpub(N+1) = now - 632h - 1h = now - 633h
# Tsbm(N+1) = Tret(N)
# Tact(N+1) = Tret(N)
# Tret(N+1) = now + 3835h - 1h = now + 3834h
# Trem(N+1) = now + 4462h - 1h = now + 4461h
TactN="now-5094h"
TretN="now-630h"
TremN="now-3h"
TpubN1="now-633h"
TsbmN1="${TretN}"
TactN1="${TretN}"
TretN1="now+3834h"
TremN1="now+4461h"
csktimes="-P ${TactN}  -P sync ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $H $TremN  -r $H $TremN  -d $H $TremN  -z $H $TactN1 "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $O $TactN1 -z $O $TactN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

#
# The zones at csk-roll2.autosign represent the various steps of a CSK rollover
# (which is essentially a ZSK Pre-Publication / KSK Double-KSK rollover).
# This scenario differs from the above one because the zone signatures (ZRRSIG)
# are replaced with the new key sooner than the DS is swapped.
#

# Step 1:
# Introduce the first key. This will immediately be active.
setup step1.csk-roll2.autosign
TactN="now"
csktimes="-P ${TactN} -P sync ${TactN} -A ${TactN}"
CSK=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN -z $O $TactN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 2:
# It is time to introduce the new CSK.
setup step2.csk-roll2.autosign
# According to RFC 7583:
# KSK: Tpub(N+1) <= Tact(N) + Lksk - IpubC
# ZSK: Tpub(N+1) <= Tact(N) + Lzsk - Ipub
# IpubC = DprpC + TTLkey (+publish-safety)
# Ipub  = IpubC
# Lcsk = Lksk = Lzsk
#
# Lcsk:           6mo (186d, 4464h)
# Dreg:           N/A
# DprpC:          1h
# TTLkey:         1h
# publish-safety: 1h
# Ipub:           3h
#
# Tact(N)  = Tnow - Lcsk + Ipub = now - 186d + 3h
#          = now - 4464h + 3h = now - 4461h
TactN="now-4461h"
csktimes="-P ${TactN} -P sync ${TactN} -A ${TactN}"
CSK=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN -z $O $TactN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 3:
# It is time to submit the DS and to roll signatures.
setup step3.csk-roll2.autosign
# According to RFC 7583:
#
# Tsbm(N+1) >= Trdy(N+1)
# KSK: Tact(N+1) = Tsbm(N+1)
# ZSK: Tact(N+1) = Tpub(N+1) + Ipub = Tsbm(N+1)
# KSK: Iret  = DprpP + TTLds (+retire-safety)
# ZSK: IretZ = Dsgn + Dprp + TTLsig (+retire-safety)
#
# Lcsk:           186d
# Dprp:           1h
# DprpP:          1w
# Dreg:           N/A
# Dsgn:           12h
# TTLds:          1h
# TTLsig:         1d
# retire-safety:  1h
# Iret:           170h
# IretZ:          38h
# Ipub:           3h
#
# Tact(N)   = Tnow - Lcsk = now - 186d
# Tret(N)   = now
# Trem(N)   = Tnow + Iret = now + 170h
# Tpub(N+1) = Tnow - Ipub = now - 3h
# Tsbm(N+1) = Tret(N)
# Tact(N+1) = Tret(N)
# Tret(N+1) = Tnow + Lcsk = now + 186d
# Trem(N+1) = Tnow + Lcsk + Iret = now + 186d + 170h =
#           = now + 4464h + 170h = now + 4634h
TactN="now-186d"
TretN="now"
TremN="now+170h"
TpubN1="now-3h"
TsbmN1="${TretN}"
TactN1="${TretN}"
TretN1="now+186d"
TremN1="now+4634h"
csktimes="-P ${TactN}  -P sync ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactN  -r $O $TactN  -d $O $TactN  -z $O $TactN  "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $R $TpubN1 -r $R $TpubN1 -d $H $TpubN1 -z $H $TpubN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 4:
# Some time later all the ZRRSIG records should be from the new CSK, and the
# DS should be swapped.  The ZRRSIG records are all replaced after IretZ (38h).
# The DS is swapped after Dreg + Iret (1w3h). In other words, the zone
# signatures are replaced before the DS is swapped.
setup step4.csk-roll2.autosign
# According to RFC 7583:
# Trem(N)    = Tret(N) + IretZ
#
# Lcsk:   186d
# Dreg:   N/A
# Iret:   170h
# IretZ:  38h
#
# Tact(N)    = Tnow - IretZ = Lcsk = now - 38h - 186d
#            = now - 38h - 4464h = now - 4502h
# Tret(N)    = Tnow - IretZ = now - 38h
# Trem(N)    = Tnow - IretZ + Iret = now - 38h + 170h = now + 132h
# Tpub(N+1)  = Tnow - IretZ - IpubC = now - 38h - 3h = now - 41h
# Tsbm(N+1)  = Tret(N)
# Tact(N+1)  = Tret(N)
# Tret(N+1)  = Tnow - IretZ + Lcsk = now - 38h + 186d
#            = now + 4426h
# Trem(N+1)  = Tnow - IretZ + Lcsk + Iret
#            = now + 4426h + 3h = now + 4429h
TactN="now-4502h"
TretN="now-38h"
TremN="now+132h"
TpubN1="now-41h"
TsbmN1="${TretN}"
TactN1="${TretN}"
TretN1="now+4426h"
TremN1="now+4429h"
csktimes="-P ${TactN}  -P sync ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactN  -r $O $TactN  -z $U $TretN  -d $U $TsbmN1 -D ds $TsbmN1 "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -z $R $TactN1 -d $R $TsbmN1 -P ds $TsbmN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 5:
# Some time later the DS can be swapped and the old DNSKEY can be removed from
# the zone.
setup step5.csk-roll2.autosign
# Subtract Iret (170h) - IretZ (38h) = 132h.
#
# Tact(N)   = now - 4502h - 132h = now - 4634h
# Tret(N)   = now - 38h - 132h = now - 170h
# Trem(N)   = now + 132h - 132h = now
# Tpub(N+1) = now - 41h - 132h = now - 173h
# Tsbm(N+1) = Tret(N)
# Tact(N+1) = Tret(N)
# Tret(N+1) = now + 4426h - 132h = now + 4294h
# Trem(N+1) = now + 4492h - 132h = now + 4360h
TactN="now-4634h"
TretN="now-170h"
TremN="now"
TpubN1="now-173h"
TsbmN1="${TretN}"
TactN1="${TretN}"
TretN1="now+4294h"
TremN1="now+4360h"
csktimes="-P ${TactN}  -P sync ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactN  -r $O $TactN  -z $H now-133h -d $U $TsbmN1 -D ds $TsbmN1 "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -z $O now-133h -d $R $TsbmN1 -P ds $TsbmN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 6:
# Some time later the predecessor DNSKEY enters the HIDDEN state.
setup step6.csk-roll2.autosign
# Subtract DNSKEY TTL plus zone propagation delay (2h).
#
# Tact(N)   = now - 4634h - 2h = now - 4636h
# Tret(N)   = now - 170h - 2h = now - 172h
# Trem(N)   = now - 2h
# Tpub(N+1) = now - 173h - 2h = now - 175h
# Tsbm(N+1) = Tret(N)
# Tact(N+1) = Tret(N)
# Tret(N+1) = now + 4294h - 2h = now + 4292h
# Trem(N+1) = now + 4360h - 2h = now + 4358h
TactN="now-4636h"
TretN="now-172h"
TremN="now-2h"
TpubN1="now-175h"
TsbmN1="${TretN}"
TactN1="${TretN}"
TretN1="now+4292h"
TremN1="now+4358h"
csktimes="-P ${TactN}  -P sync ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $U $TremN  -r $U $TremN  -d $H $TremN -z $H now-135h "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $O $TremN -z $O now-135h "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 7:
# The predecessor DNSKEY can be purged, but purge-keys is disabled.
setup step7.csk-roll2.autosign
# Subtract 90 days (default, 2160h) from all the times.
# Tact(N)   = now - 4636h - 2160h = now - 6796h
# Tret(N)   = now - 172h - 2160h = now - 2332h
# Trem(N)   = now - 2h - 2160h = now - 2162h
# Tpub(N+1) = now - 175h - 2160h = now - 2335h
# Tsbm(N+1) = Tret(N)
# Tact(N+1) = Tret(N)
# Tret(N+1) = now + 4294h - 2160h = now + 2134h
# Trem(N+1) = now + 4360h - 2160h = now + 2200h
TactN="now-6796h"
TretN="now-2332h"
TremN="now-2162h"
TpubN1="now-2335h"
TsbmN1="${TretN}"
TactN1="${TretN}"
TretN1="now+2134h"
TremN1="now+2200h"
csktimes="-P ${TactN}  -P sync ${TactN}  -A ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactN1} -I ${TretN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $U $TremN  -r $U $TremN  -d $H $TremN -z $H now-135h "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $O $TremN -z $O now-135h "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1
