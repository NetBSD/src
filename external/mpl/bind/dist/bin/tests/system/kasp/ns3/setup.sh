#!/bin/sh -e
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

private_type_record() {
	_zone=$1
	_algorithm=$2
	_keyfile=$3

	_id=$(keyfile_to_key_id "$_keyfile")

	printf "%s. 0 IN TYPE65534 %s 5 %02x%04x0000\n" "$_zone" "\\#" "$_algorithm" "$_id"
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
for zn in default rsasha1 dnssec-keygen some-keys legacy-keys pregenerated \
	  rumoured rsasha1-nsec3 rsasha256 rsasha512 ecdsa256 ecdsa384 \
	  dynamic dynamic-inline-signing inline-signing \
	  inherit unlimited
do
	setup "${zn}.kasp"
	cp template.db.in "$zonefile"
done

# Set up zone that stays unsigned.
zone="unsigned.kasp"
echo_i "setting up zone: $zone"
zonefile="${zone}.db"
infile="${zone}.db.infile"
cp template.db.in $zonefile

# Some of these zones already have keys.
zone="dnssec-keygen.kasp"
$KEYGEN -k rsasha1 -l policies/kasp.conf $zone > keygen.out.$zone.1 2>&1

zone="some-keys.kasp"
$KEYGEN -G -a RSASHA1 -b 2000 -L 1234 $zone > keygen.out.$zone.1 2>&1
$KEYGEN -G -a RSASHA1 -f KSK  -L 1234 $zone > keygen.out.$zone.2 2>&1

zone="legacy.kasp"
$KEYGEN -a RSASHA1 -b 2000 -L 1234 $zone > keygen.out.$zone.1 2>&1
$KEYGEN -a RSASHA1 -f KSK  -L 1234 $zone > keygen.out.$zone.2 2>&1

zone="pregenerated.kasp"
$KEYGEN -G -k rsasha1 -l policies/kasp.conf $zone > keygen.out.$zone.1 2>&1
$KEYGEN -G -k rsasha1 -l policies/kasp.conf $zone > keygen.out.$zone.2 2>&1

zone="rumoured.kasp"
Tpub="now"
Tact="now+1d"
keytimes="-P ${Tpub} -A ${Tact}"
KSK=$($KEYGEN  -a RSASHA1 -f KSK  -L 1234 $keytimes $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a RSASHA1 -b 2000 -L 1234 $keytimes $zone 2> keygen.out.$zone.2)
ZSK2=$($KEYGEN -a RSASHA1         -L 1234 $keytimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $O -k $R $Tpub -r $R $Tpub -d $H $Tpub  "$KSK"  > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $R $Tpub -z $R $Tpub              "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $R $Tpub -z $R $Tpub              "$ZSK2" > settime.out.$zone.2 2>&1

#
# Set up zones that are already signed.
#

# These signatures are set to expire long in the past, update immediately.
setup expired-sigs.autosign
T="now-6mo"
ksktimes="-P $T -A $T -P sync $T"
zsktimes="-P $T -A $T"
KSK=$($KEYGEN -a ECDSAP256SHA256 -L 300 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a ECDSAP256SHA256 -L 300        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T          "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone 13 "$KSK" >> "$infile"
private_type_record $zone 13 "$ZSK" >> "$infile"
$SIGNER -PS -x -s now-2mo -e now-1mo -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# These signatures are still good, and can be reused.
setup fresh-sigs.autosign
T="now-6mo"
ksktimes="-P $T -A $T -P sync $T"
zsktimes="-P $T -A $T"
KSK=$($KEYGEN -a ECDSAP256SHA256 -L 300 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a ECDSAP256SHA256 -L 300        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T          "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone 13 "$KSK" >> "$infile"
private_type_record $zone 13 "$ZSK" >> "$infile"
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# These signatures are still good, but not fresh enough, update immediately.
setup unfresh-sigs.autosign
T="now-6mo"
ksktimes="-P $T -A $T -P sync $T"
zsktimes="-P $T -A $T"
KSK=$($KEYGEN -a ECDSAP256SHA256 -L 300 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a ECDSAP256SHA256 -L 300        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T          "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone 13 "$KSK" >> "$infile"
private_type_record $zone 13 "$ZSK" >> "$infile"
$SIGNER -S -x -s now-1w -e now+1w -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# These signatures are already expired, and the private ZSK is missing.
setup zsk-missing.autosign
T="now-6mo"
ksktimes="-P $T -A $T -P sync $T"
zsktimes="-P $T -A $T"
KSK=$($KEYGEN -a ECDSAP256SHA256 -L 300 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a ECDSAP256SHA256 -L 300        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T          "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone 13 "$KSK" >> "$infile"
private_type_record $zone 13 "$ZSK" >> "$infile"
$SIGNER -PS -x -s now-2w -e now-1mi -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1
rm -f "${ZSK}".private

# These signatures are already expired, and the private ZSK is retired.
setup zsk-retired.autosign
T="now-6mo"
ksktimes="-P $T -A $T -P sync $T"
zsktimes="-P $T -A $T -I now"
KSK=$($KEYGEN -a ECDSAP256SHA256 -L 300 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a ECDSAP256SHA256 -L 300        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $T -z $O $T          "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone 13 "$KSK" >> "$infile"
private_type_record $zone 13 "$ZSK" >> "$infile"
$SIGNER -PS -x -s now-2w -e now-1mi -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1
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
private_type_record $zone 13 "$CSK" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

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
private_type_record $zone 13 "$CSK" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1
setup step3.enable-dnssec.autosign

# Step 4:
# The DS has been submitted long enough ago to become OMNIPRESENT.
setup step4.enable-dnssec.autosign
# DS TTL:                    1 day (86400 seconds)
# parent-registration-delay: 1 day (86400 seconds)
# parent-propagation-delay:  1 hour (3600 seconds)
# retire-safety:             20 minutes (1200 seconds)
# Total aditional time:      98400 seconds
# 44700 + 98400 = 143100
TpubN="now-143100s"
# 43800 + 98400 = 142200
TcotN="now-142200s"
TsbmN="now-98400s"
keytimes="-P ${TpubN} -P sync ${TsbmN} -A ${TpubN}"
CSK=$($KEYGEN -k enable-dnssec -l policies/autosign.conf $keytimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -k $O $TcotN -r $O $TcotN -d $R $TsbmN -z $O $TsbmN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone 13 "$CSK" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1
setup step3.enable-dnssec.autosign

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
KSK=$($KEYGEN -a ECDSAP256SHA256 -L 3600 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a ECDSAP256SHA256 -L 3600        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN -z $O $TactN              "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone 13 "$KSK" >> "$infile"
private_type_record $zone 13 "$ZSK" >> "$infile"
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

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
KSK=$($KEYGEN -a ECDSAP256SHA256 -L 3600 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a ECDSAP256SHA256 -L 3600        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN -z $O $TactN              "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone 13 "$KSK" >> "$infile"
private_type_record $zone 13 "$ZSK" >> "$infile"
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

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
KSK=$($KEYGEN  -a ECDSAP256SHA256 -L 3600 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a ECDSAP256SHA256 -L 3600        $zsktimes $zone 2> keygen.out.$zone.2)
ZSK2=$($KEYGEN -a ECDSAP256SHA256 -L 3600        $newtimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $O -k $O $TactN  -r $O $TactN  -d $O $TactN "$KSK"  > settime.out.$zone.1 2>&1
$SETTIME -s -g $H -k $O $TactN  -z $O $TactN               "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $R $TpubN1 -z $H $TpubN1              "$ZSK2" > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $ZSK1 $ZSK2
# Sign zone.
cat template.db.in "${KSK}.key" "${ZSK1}.key" "${ZSK2}.key" > "$infile"
private_type_record $zone 13 "$KSK"  >> "$infile"
private_type_record $zone 13 "$ZSK1" >> "$infile"
private_type_record $zone 13 "$ZSK2" >> "$infile"
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

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
KSK=$($KEYGEN  -a ECDSAP256SHA256 -L 3600 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a ECDSAP256SHA256 -L 3600        $zsktimes $zone 2> keygen.out.$zone.2)
ZSK2=$($KEYGEN -a ECDSAP256SHA256 -L 3600        $newtimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $O -k $O $TactN  -r $O $TactN -d $O $TactN "$KSK"  > settime.out.$zone.1 2>&1
$SETTIME -s -g $H -k $O $TactN  -z $U $TretN              "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TactN1 -z $R $TactN1             "$ZSK2" > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $ZSK1 $ZSK2
# Sign zone.
cat template.db.in "${KSK}.key" "${ZSK1}.key" "${ZSK2}.key" > "$infile"
$SIGNER -PS -x -s now-2w -e now-1mi -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

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
KSK=$($KEYGEN  -a ECDSAP256SHA256  -L 3600 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a ECDSAP256SHA256  -L 3600        $zsktimes $zone 2> keygen.out.$zone.2)
ZSK2=$($KEYGEN -a ECDSAP256SHA256  -L 3600        $newtimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $O -k $O $TactN  -r $O $TactN -d $O $TactN "$KSK"  > settime.out.$zone.1 2>&1
$SETTIME -s -g $H -k $U $TdeaN  -z $H $TdeaN              "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TactN1 -z $O $TdeaN              "$ZSK2" > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $ZSK1 $ZSK2
# Sign zone.
cat template.db.in "${KSK}.key" "${ZSK1}.key" "${ZSK2}.key" > "$infile"
private_type_record $zone 13 "$KSK"  >> "$infile"
private_type_record $zone 13 "$ZSK1" >> "$infile"
private_type_record $zone 13 "$ZSK2" >> "$infile"
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

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
KSK=$($KEYGEN -a ECDSAP256SHA256 -L 7200 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a ECDSAP256SHA256 -L 7200        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O              -k $O $TactN -z $O $TactN "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

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
KSK=$($KEYGEN -a ECDSAP256SHA256 -L 7200 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a ECDSAP256SHA256 -L 7200        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN -z $O $TactN              "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone 13 "$KSK" >> "$infile"
private_type_record $zone 13 "$ZSK" >> "$infile"
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

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
# Dreg:           1d
# DprpP:          1h
# TTLds:          1h
# retire-safety:  2d
# Iret:           50h
# DprpC:          1h
# TTLkey:         2h
# publish-safety: 1d
# IpubC:          27h
#
# Tact(N)    = Tnow + Dreg - Lksk = now + 1d - 60d = now - 59d
# Tret(N)    = Tnow + Dreg = now + 1d
# Trem(N)    = Tnow + Dreg + Iret = now + 1d + 50h = now + 74h
# Tpub(N+1)  = Tnow - IpubC = now - 27h
# Tsbm(N+1)  = now
# Tact(N+1)  = Tret(N)
# Tret(N+1)  = Tnow + Dreg + Lksk = now + 1d + 60d = now + 61d
# Trem(N+1)  = Tnow + Dreg + Lksk + Iret = now + 61d + 50h
#            = now + 1464h + 50h = 1514h
TactN="now-59d"
TretN="now+1d"
TremN="now+74h"
TpubN1="now-27h"
TsbmN1="now"
TactN1="${TretN}"
TretN1="now+61d"
TremN1="now+1514h"
ksktimes="-P ${TactN}  -A ${TactN}  -P sync ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -A ${TactN1} -P sync ${TsbmN1} -I ${TretN1} -D ${TremN1}"
zsktimes="-P ${TactN}  -A ${TactN}"
KSK1=$($KEYGEN -a ECDSAP256SHA256 -L 7200 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
KSK2=$($KEYGEN -a ECDSAP256SHA256 -L 7200 -f KSK $newtimes $zone 2> keygen.out.$zone.2)
ZSK=$($KEYGEN  -a ECDSAP256SHA256 -L 7200        $zsktimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $H -k $O $TactN   -r $O $TactN  -d $O $TactN  "$KSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $R $TpubN1  -r $R $TpubN1 -d $H $TpubN1 "$KSK2" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TactN   -z $O $TactN                "$ZSK"  > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $KSK1 $KSK2
# Sign zone.
cat template.db.in "${KSK1}.key" "${KSK2}.key" "${ZSK}.key" > "$infile"
private_type_record $zone 13 "$KSK1" >> "$infile"
private_type_record $zone 13 "$KSK2" >> "$infile"
private_type_record $zone 13 "$ZSK" >> "$infile"
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# Step 4:
# The DS should be swapped now.
setup step4.ksk-doubleksk.autosign
# According to RFC 7583:
#
# Tret(N)   = Tsbm(N+1) + Dreg
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
# Dreg: 1d
# Iret: 50h
#
# Tact(N)   = Tnow - Lksk - Iret = now - 60d - 50h
#           = now - 1440h - 50h = now - 1490h
# Tret(N)   = Tnow - Iret = now - 50h
# Trem(N)   = Tnow
# Tpub(N+1) = Tnow - Iret - Dreg - IpubC = now - 50h - 1d - 27h
#           = now - 101h
# Tsbm(N+1) = Tnow - Iret - Dreg = now - 50h - 1d = now - 74h
# Tact(N+1) = Tret(N)
# Tret(N+1) = Tnow + Lksk - Iret = now + 60d - 50h = now + 1390h
# Trem(N+1) = Tnow + Lksk = now + 60d
TactN="now-1490h"
TretN="now-50h"
TremN="now"
TpubN1="now-101h"
TsbmN1="now-74h"
TactN1="${TretN}"
TretN1="now+1390h"
TremN1="now+60d"
ksktimes="-P ${TactN}  -A ${TactN} -P sync ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -A ${TretN} -P sync ${TsbmN1} -I ${TretN1} -D ${TremN1}"
zsktimes="-P ${TactN}  -A ${TactN}"
KSK1=$($KEYGEN -a ECDSAP256SHA256 -L 7200 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
KSK2=$($KEYGEN -a ECDSAP256SHA256 -L 7200 -f KSK $newtimes $zone 2> keygen.out.$zone.2)
ZSK=$($KEYGEN  -a ECDSAP256SHA256 -L 7200        $zsktimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $H -k $O $TactN  -r $O $TactN  -d $U $TsbmN1 "$KSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $R $TsbmN1 "$KSK2" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TactN  -z $O $TactN                "$ZSK"  > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $KSK1 $KSK2
# Sign zone.
cat template.db.in "${KSK1}.key" "${KSK2}.key" "${ZSK}.key" > "$infile"
private_type_record $zone 13 "$KSK1" >> "$infile"
private_type_record $zone 13 "$KSK2" >> "$infile"
private_type_record $zone 13 "$ZSK" >> "$infile"
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# Step 5:
# The predecessor DNSKEY is removed long enough that is has become HIDDEN.
setup step5.ksk-doubleksk.autosign
# Subtract DNSKEY TTL from all the times (2h).
# Tact(N)   = now - 1490h - 2h = now - 1492h
# Tret(N)   = now - 52h - 2h = now - 52h
# Trem(N)   = now - 2h
# Tpub(N+1) = now - 101h - 2h = now - 103h
# Tsbm(N+1) = now - 74h - 2h = now - 76h
# Tact(N+1) = Tret(N)
# Tret(N+1) = now + 1390h - 2h = now + 1388h
# Trem(N+1) = now + 60d + 2h = now + 1442h
TactN="now-1492h"
TretN="now-52h"
TremN="now-2h"
TpubN1="now-103h"
TsbmN1="now-76h"
TactN1="${TretN}"
TretN1="now+1388h"
TremN1="now+1438h"
ksktimes="-P ${TactN}  -A ${TactN} -P sync ${TactN}  -I ${TretN}  -D ${TremN}"
newtimes="-P ${TpubN1} -A ${TretN} -P sync ${TsbmN1} -I ${TretN1} -D ${TremN1}"
zsktimes="-P ${TactN}  -A ${TactN}"
KSK1=$($KEYGEN -a ECDSAP256SHA256 -L 7200 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
KSK2=$($KEYGEN -a ECDSAP256SHA256 -L 7200 -f KSK $newtimes $zone 2> keygen.out.$zone.2)
ZSK=$($KEYGEN  -a ECDSAP256SHA256 -L 7200        $zsktimes $zone 2> keygen.out.$zone.3)
$SETTIME -s -g $H -k $U $TretN  -r $U $TretN  -d $H $TretN  "$KSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN1 -r $O $TactN1 -d $O $TactN1 "$KSK2" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TactN  -z $O $TactN                "$ZSK"  > settime.out.$zone.3 2>&1
# Set key rollover relationship.
key_successor $KSK1 $KSK2
# Sign zone.
cat template.db.in "${KSK1}.key" "${KSK2}.key" "${ZSK}.key" > "$infile"
private_type_record $zone 13 "$KSK1" >> "$infile"
private_type_record $zone 13 "$KSK2" >> "$infile"
private_type_record $zone 13 "$ZSK" >> "$infile"
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

#
# The zones at csk-roll.autosign represent the various steps of a CSK rollover
# (which is essentially a ZSK Pre-Publication / KSK Double-KSK rollover).
#
#
# The activation time for zone signing (ZSK) is different than for chain of
# trust validation (KSK). Therefor, for zone signing we use TactZ and TretZ
# instead of Tact and Tret.
#

# Step 1:
# Introduce the first key. This will immediately be active.
setup step1.csk-roll.autosign
TactN="now"
csktimes="-P ${TactN} -P sync ${TactN} -A ${TactN}"
CSK=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN -z $O $TactN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone 13 "$CSK" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# Step 2:
# It is time to introduce the new CSK.
setup step2.csk-roll.autosign
# According to RFC 7583:
# KSK: Tpub(N+1) <= Tact(N) + Lksk - Dreg - IpubC
# ZSK: Tpub(N+1) <= TactZ(N) + Lzsk - Ipub
# IpubC = DprpC + TTLkey (+publish-safety)
# Ipub  = IpubC
# Lcsk = Lksk = Lzsk
#
# Lcsk:           6mo (186d, 4464h)
# Dreg:           1d
# DprpC:          1h
# TTLkey:         1h
# publish-safety: 1h
# Ipub:           3h
#
# Tact(N)  = Tnow - Lcsk + Ipub + Dreg = now - 186d + 3h + 1d
#          = now - 4464h + 3h + 24h = now - 4437h
# TactZ(N) = Tnow - Lcsk + IpubC = now - 186d + 3h
#          = now - 4464h + 3h = now - 4461h
TactN="now-4437h"
TactZN="now-4461h"
csktimes="-P ${TactN} -P sync ${TactZN} -A ${TactZN}"
CSK=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -k $O $TactZN -r $O $TactZN -d $O $TactN -z $O $TactZN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone 13 "$CSK" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# Step 3:
# It is time to submit the DS and to roll signatures.
setup step3.csk-roll.autosign
# According to RFC 7583:
#
# Tsbm(N+1) >= Trdy(N+1)
# KSK: Tact(N+1)  = Tsbm(N+1) + Dreg
# ZSK: TactZ(N+1) = Tpub(N+1) + Ipub = Tsbm(N+1)
# KSK: Iret  = DprpP + TTLds (+retire-safety)
# ZSK: IretZ = Dsgn + Dprp + TTLsig (+retire-safety)
#
# Lcsk:           186d
# Dprp:           1h
# DprpP:          1h
# Dreg:           1d
# Dsgn:           25d
# TTLds:          1h
# TTLsig:         1d
# retire-safety:  2h
# Iret:           4h
# IretZ:          26d3h
# Ipub:           3h
#
# TactZ(N)   = Tnow - Lcsk = now - 186d
# TretZ(N)   = now
# Tact(N)    = Tnow + Dreg - Lcsk = now + 1d - 186d = now - 185d
# Tret(N)    = Tnow + Dreg = now + 1d
# Trem(N)    = Tnow + IretZ = now + 26d3h = now + 627h
# Tpub(N+1)  = Tnow - Ipub = now - 3h
# Tsbm(N+1)  = TretZ(N)
# TactZ(N+1) = TretZ(N)
# TretZ(N+1) = Tnow + Lcsk = now + 186d
# Tact(N+1)  = Tret(N)
# Tret(N+1)  = Tnow + Dreg + Lcsk = now + 1d + 186d = now + 187d
# Trem(N+1)  = Tnow + Lcsk + IretZ = now + 186d + 26d3h =
#            = now + 5091h
TactZN="now-186d"
TretZN="now"
TactN="now-185d"
TretN="now+1d"
TremN="now+627h"
TpubN1="now-3h"
TsbmN1="now"
TactZN1="${TsbmN1}"
TretZN1="now+186d"
TactN1="${TretN}"
TretN1="now+187d"
TremN1="now+5091h"
csktimes="-P ${TactN}  -P sync ${TactZN} -A ${TactZN}  -I ${TretZN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactZN1} -I ${TretZN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactZN  -r $O $TactZN -d $O $TactN  -z $O $TactZN "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $R $TpubN1  -r $R $TpubN1 -d $H $TpubN1 -z $H $TpubN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 13 "$CSK1" >> "$infile"
private_type_record $zone 13 "$CSK2" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# Step 4:
# Some time later all the ZRRSIG records should be from the new CSK, and the
# DS should be swapped.  The ZRRSIG records are all replaced after IretZ
# (which is 26d3h).  The DS is swapped after Dreg + Iret (which is 1d4h).
# In other words, the DS is swapped before all zone signatures are replaced.
setup step4.csk-roll.autosign
# According to RFC 7583:
# Trem(N)    = TretZ(N) + IretZ
# Tnow       = Tsbm(N+1) + Dreg + Iret
#
# Lcsk:   186d
# Iret:   4h
# IretZ:  26d3h
#
# TactZ(N)   = Tnow - Iret - Dreg - Lcsk = now - 4h - 24h - 4464h
#            = now - 4492h
# TretZ(N)   = Tnow - Iret - Dreg = now - 4h - 1d = now - 28h
# Tact(N)    = Tnow - Iret - Lcsk = now - 4h - 186d = now - 4468h
# Tret(N)    = Tnow - Iret = now - 4h = now - 4h
# Trem(N)    = Tnow - Iret - Dreg + IretZ = now - 4h - 1d + 26d3h
#            = now + 24d23h = now + 599h
# Tpub(N+1)  = Tnow - Iret - Dreg - IpubC = now - 4h - 1d - 3h = now - 31h
# Tsbm(N+1)  = TretZ(N)
# TactZ(N+1) = TretZ(N)
# TretZ(N+1) = Tnow - Iret - Dreg + Lcsk = now - 4h - 1d + 186d
#            = now + 4436h
# Tact(N+1)  = Tret(N)
# Tret(N+1)  = Tnow - Iret + Lcsk = now + 6mo - 4h = now + 4460h
# Trem(N+1)  = Tnow - Iret - Dreg + Lcsk + IretZ = now - 4h - 1d + 186d + 26d3h
#	     = now + 5063h
TactZN="now-4492h"
TretZN="now-28h"
TactN="now-4468h"
TretN="now-4h"
TremN="now+599h"
TpubN1="now-31h"
TsbmN1="${TretZN}"
TactZN1="${TretZN}"
TretZN1="now+4436h"
TactN1="${TretN}"
TretN1="now+4460h"
TremN1="now+5063h"
csktimes="-P ${TactN}  -P sync ${TactZN} -A ${TactZN}  -I ${TretZN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactZN1} -I ${TretZN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactZN -r $O $TactZN -d $U $TsbmN1 -z $U $TsbmN1 "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $R $TsbmN1 -z $R $TsbmN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 13 "$CSK1" >> "$infile"
private_type_record $zone 13 "$CSK2" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# Step 5:
# After the DS is swapped in step 4, also the KRRSIG records can be removed.
# At this time these have all become hidden.
setup step5.csk-roll.autosign
# Subtract DNSKEY TTL plus zone propagation delay from all the times (2h).
# TactZ(N)   = now - 4492h - 2h = now - 4494h
# TretZ(N)   = now - 28h - 2h = now - 30h
# Tact(N)    = now - 4468h - 2h = now - 4470h
# Tret(N)    = now - 4h - 2h = now - 6h
# Trem(N)    = now + 599h - 2h = now + 597h
# Tpub(N+1)  = now - 31h - 2h = now - 33h
# Tsbm(N+1)  = TretZ(N)
# TactZ(N+1) = TretZ(N)
# TretZ(N+1) = now + 4436h - 2h = now + 4434h
# Tact(N+1)  = Tret(N)
# Tret(N+1)  = now + 4460h - 2h = now + 4458h
# Trem(N+1)  = now + 5063h - 2h = now + 5061h
TactZN="now-4494h"
TretZN="now-30h"
TactN="now-4470h"
TretN="now-6h"
TremN="now+597h"
TpubN1="now-33h"
TsbmN1="now-30h"
TactZN1="${TsbmN1}"
TretZN1="now+4434h"
TactN1="${TretN}"
TretN1="now+4458h"
TremN1="now+5061h"
csktimes="-P ${TactN}  -P sync ${TactZN} -A ${TactZN}  -I ${TretZN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactZN1} -I ${TretZN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactZN -r $U now-2h  -d $H now-2h -z $U $TactZN1 "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $O now-2h -z $R $TactZN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 13 "$CSK1" >> "$infile"
private_type_record $zone 13 "$CSK2" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# Step 6:
# After the retire interval has passed the predecessor DNSKEY can be
# removed from the zone.
setup step6.csk-roll.autosign
# According to RFC 7583:
# Trem(N) = TretZ(N) + IretZ
# TretZ(N) = TactZ(N) + Lcsk
#
# Lcsk:   186d
# Iret:   4h
# IretZ:  26d3h
#
# TactZ(N)   = Tnow - IretZ - Lcsk = now - 627h - 186d
#            = now - 627h - 4464h = now - 5091h
# TretZ(N)   = Tnow - IretZ = now - 627h
# Tact(N)    = Tnow - IretZ - Lcsk + Dreg = now - 627h - 186d + 1d =
#              now - 627h - 4464h + 24h = now - 5067h
# Tret(N)    = Tnow - IretZ + Dreg = now - 627h + 24h
#            = Tnow - 603h
# Trem(N)    = Tnow
# Tpub(N+1)  = Tnow - IretZ - Ipub = now - 627h - 3h = now - 630h
# Tsbm(N+1)  = TretZ(N)
# TactZ(N+1) = TretZ(N)
# TretZ(N+1) = Tnow - IretZ + Lcsk = now - 627h + 186d = now + 3837h
# Tact(N+1)  = Tret(N)
# Tret(N+1)  = Tnow - Iret + Lcsk = now - 4h + 186d = now + 4460h
# Trem(N+1)  = Tnow + Lcsk = now + 186d
TactZN="now-5091h"
TretZN="now-627h"
TactN="now-5067h"
TretN="now-603h"
TremN="now"
TpubN1="now-630h"
TsbmN1="${TretZN}"
TactZN1="${TretZN}"
TretZN1="now+3837h"
TactN1="${TretN}"
TretN1="now+4460h"
TremN1="now+186d"
csktimes="-P ${TactN}  -P sync ${TactZN} -A ${TactZN}  -I ${TretZN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactZN1} -I ${TretZN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactZN -r $H $TremN  -d $H $TremN  -z $U $TsbmN1 "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $O $TremN  -z $R $TsbmN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 13 "$CSK1" >> "$infile"
private_type_record $zone 13 "$CSK2" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# Step 7:
# Some time later the predecessor DNSKEY enters the HIDDEN state.
setup step7.csk-roll.autosign
# Subtract DNSKEY TTL plus zone propagation delay from all the times (2h).
# TactZ(N) = now - 5091h - 2h = now - 5093h
# TretZ(N) = now - 627h - 2h  = now - 629h
# Tact(N)  = now - 5067h - 2h = now - 5069h
# Tret(N)  = now - 603h - 2h  = now - 605h
# Trem(N) = now - 2h
# Tpub(N+1) = now - 630h - 2h = now - 632h
# Tsbm(N+1) = now - 627h - 2h = now - 629h
# TactZ(N+1) = Tsbm(N+1)
# TretZ(N+1) = now + 3837h - 2h = now + 3835h
# Tact(N+1) = Tret(N)
# Tret(N+1) = now + 4460h - 2h = now + 4458h
# Trem(N+1) = now + 186d - 2h = now + 4462h
TactZN="now-5093h"
TretZN="now-629h"
TactN="now-5069h"
TretN="now-605h"
TremN="now-2h"
TpubN1="now-632h"
TsbmN1="${TretZN}"
TactZN1="${TretZN}"
TretZN1="now+3835h"
TactN1="${TretN}"
TretN1="now+4458h"
TremN1="now+4462h"
csktimes="-P ${TactN}  -P sync ${TactZN} -A ${TactZN}  -I ${TretZN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactZN1} -I ${TretZN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $U $TremN  -r $H $TremN  -d $H $TremN  -z $H $TactZN1 "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $O $TactN1 -z $O $TactZN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 13 "$CSK1" >> "$infile"
private_type_record $zone 13 "$CSK2" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

#
# The zones at csk-roll2.autosign represent the various steps of a CSK rollover
# (which is essentially a ZSK Pre-Publication / KSK Double-KSK rollover).
# This scenario differs from the above one because the zone signatures (ZRRSIG)
# are replaced with the new key sooner than the DS is swapped.
#
#
# The activation time for zone signing (ZSK) is different than for chain of
# trust validation (KSK). Therefor, for zone signing we use TactZ and TretZ
# instead of Tact and Tret.
#

# Step 1:
# Introduce the first key. This will immediately be active.
setup step1.csk-roll2.autosign
TactN="now"
csktimes="-P ${TactN} -P sync ${TactN} -A ${TactN}"
CSK=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN -z $O $TactN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone 13 "$CSK" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# Step 2:
# It is time to introduce the new CSK.
setup step2.csk-roll2.autosign
# According to RFC 7583:
# KSK: Tpub(N+1) <= Tact(N) + Lksk - Dreg - IpubC
# ZSK: Tpub(N+1) <= TactZ(N) + Lzsk - Ipub
# IpubC = DprpC + TTLkey (+publish-safety)
# Ipub  = IpubC
# Lcsk = Lksk = Lzsk
#
# Lcsk:           6mo (186d, 4464h)
# Dreg:           1w
# DprpC:          1h
# TTLkey:         1h
# publish-safety: 1h
# Ipub:           3h
#
# Tact(N)  = Tnow - Lcsk + Ipub + Dreg = now - 186d + 3h + 1w
#          = now - 4464h + 3h + 168h = now - 4293h
# TactZ(N) = Tnow - Lcsk + IpubC = now - 186d + 3h
#          = now - 4464h + 3h = now - 4461h
TactN="now-4293h"
TactZN="now-4461h"
csktimes="-P ${TactN} -P sync ${TactZN} -A ${TactZN}"
CSK=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -k $O $TactZN -r $O $TactZN -d $O $TactN -z $O $TactZN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone 13 "$CSK" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# Step 3:
# It is time to submit the DS and to roll signatures.
setup step3.csk-roll2.autosign
# According to RFC 7583:
#
# Tsbm(N+1) >= Trdy(N+1)
# KSK: Tact(N+1)  = Tsbm(N+1) + Dreg
# ZSK: TactZ(N+1) = Tpub(N+1) + Ipub = Tsbm(N+1)
# KSK: Iret  = DprpP + TTLds (+retire-safety)
# ZSK: IretZ = Dsgn + Dprp + TTLsig (+retire-safety)
#
# Lcsk:           186d
# Dprp:           1h
# DprpP:          1h
# Dreg:           1w
# Dsgn:           12h
# TTLds:          1h
# TTLsig:         1d
# retire-safety:  1h
# Iret:           3h
# IretZ:          38h
# Ipub:           3h
#
# TactZ(N)   = Tnow - Lcsk = now - 186d
# TretZ(N)   = now
# Tact(N)    = Tnow + Dreg - Lcsk = now + 1w - 186d = now - 179d
# Tret(N)    = Tnow + Dreg = now + 7d
# Trem(N)    = Tnow + Dreg + Iret = now + 1w + 3h = now + 171h
# Tpub(N+1)  = Tnow - Ipub = now - 3h
# Tsbm(N+1)  = TretZ(N)
# TactZ(N+1) = TretZ(N)
# TretZ(N+1) = Tnow + Lcsk = now + 186d
# Tact(N+1)  = Tret(N)
# Tret(N+1)  = Tnow + Lcsk + Dreg = now + 186d + 7d = now + 193d
# Trem(N+1)  = Tnow + Lcsk + Dreg + Iret = now + 186d + 7d + 3h =
#            = now + 193d + 3h = now + 4632h + 3h = now + 4635h
TactZN="now-186d"
TretZN="now"
TactN="now-179d"
TretN="now+7d"
TremN="now+171h"
TpubN1="now-3h"
TsbmN1="${TretZN}"
TactZN1="${TretZN}"
TretZN1="now+186d"
TactN1="${TretN}"
TretN1="now+193d"
TremN1="now+4635h"
csktimes="-P ${TactN}  -P sync ${TactZN} -A ${TactZN}  -I ${TretZN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactZN1} -I ${TretZN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactZN  -r $O $TactZN -d $O $TactN  -z $O $TactZN "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $R $TpubN1  -r $R $TpubN1 -d $H $TpubN1 -z $H $TpubN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 13 "$CSK1" >> "$infile"
private_type_record $zone 13 "$CSK2" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# Step 4:
# Some time later all the ZRRSIG records should be from the new CSK, and the
# DS should be swapped.  The ZRRSIG records are all replaced after IretZ (38h).
# The DS is swapped after Dreg + Iret (1w3h). In other words, the zone
# signatures are replaced before the DS is swapped.
setup step4.csk-roll2.autosign
# According to RFC 7583:
# Trem(N)    = Tret(N) + Iret
# Tnow       = TretZ(N) + IretZ
#
# Lcsk:   186d
# Dreg:   1w
# Iret:   3h
# IretZ:  38h
#
# TactZ(N)   = Tnow - IretZ = Lcsk = now - 38h - 186d
#            = now - 38h - 4464h = now - 4502h
# TretZ(N)   = Tnow - IretZ = now - 38h
# Tact(N)    = Tnow - IretZ - Lcsk + Dreg = now - 38h - 4464h + 168h
#            = now - 4334h
# Tret(N)    = Tnow - IretZ + Dreg = now - 38h + 168h = now + 130h
# Trem(N)    = Tnow - IretZ + Dreg + Iret = now + 130h + 3h = now + 133h
# Tpub(N+1)  = Tnow - IretZ - IpubC = now - 38h - 3h = now - 41h
# Tsbm(N+1)  = TretZ(N)
# TactZ(N+1) = TretZ(N)
# TretZ(N+1) = Tnow - IretZ + Lcsk = now - 38h + 186d
#            = now + 4426h
# Tact(N+1)  = Tret(N)
# Tret(N+1)  = Tnow - IretZ + Dreg + Lcsk = now - 38h + 168h + 4464h
#            = now + 4594h
# Trem(N+1)  = Tnow - IretZ + Dreg + Lcsk + Iret
#            = now + 4594h + 3h = now + 4597h
TactZN="now-4502h"
TretZN="now-38h"
TactN="now-4334h"
TretN="now+130h"
TremN="now+133h"
TpubN1="now-41h"
TsbmN1="${TretZN}"
TactZN1="${TretZN}"
TretZN1="now+4426h"
TactN1="${TretN}"
TretN1="now+4594h"
TremN1="now+4597h"
csktimes="-P ${TactN}  -P sync ${TactZN} -A ${TactZN}  -I ${TretZN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactZN1} -I ${TretZN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactZN -r $O $TactZN -d $U $TsbmN1 -z $U $TretZN  "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $R $TsbmN1 -z $R $TactZN1 "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 13 "$CSK1" >> "$infile"
private_type_record $zone 13 "$CSK2" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# Step 5:
# Some time later the DS can be swapped and the old DNSKEY can be removed from
# the zone.
setup step5.csk-roll2.autosign
# Subtract Dreg + Iret (171h) - IretZ (38h) = 133h.
#
# TactZ(N)   = now - 4502h - 133h = now - 4635h
# TretZ(N)   = now - 38h - 133h = now - 171h
# Tact(N)    = now - 4334h = 133h = now - 4467h
# Tret(N)    = now + 130h - 133h = now - 3h
# Trem(N)    = now + 133h - 133h = now
# Tpub(N+1)  = now - 41h - 133h = now - 174h
# Tsbm(N+1)  = TretZ(N)
# TactZ(N+1) = TretZ(N)
# TretZ(N+1) = now + 4426h - 133h = now + 4293h
# Tact(N+1)  = Tret(N)
# Tret(N+1)  = now + 4594h - 133h = now + 4461h
# Trem(N+1)  = now + 4597h - 133h = now + 4464h = now + 186d
TactZN="now-4635h"
TretZN="now-171h"
TactN="now-4467h"
TretN="now-3h"
TremN="now"
TpubN1="now-174h"
TsbmN1="${TretZN}"
TactZN1="${TretZN}"
TretZN1="now+4293h"
TactN1="${TretN}"
TretN1="now+4461h"
TremN1="now+186d"
csktimes="-P ${TactN}  -P sync ${TactZN} -A ${TactZN}  -I ${TretZN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactZN1} -I ${TretZN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactZN -r $O $TactZN -d $U $TsbmN1 -z $H now-133h "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $R $TsbmN1 -z $O now-133h "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 13 "$CSK1" >> "$infile"
private_type_record $zone 13 "$CSK2" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1

# Step 6:
# Some time later the predecessor DNSKEY enters the HIDDEN state.
setup step6.csk-roll2.autosign
# Subtract DNSKEY TTL plus zone propagation delay (2h).
#
# TactZ(N)   = now - 4635h - 2h = now - 4637h
# TretZ(N)   = now - 171h - 2h = now - 173h
# Tact(N)    = now - 4467h - 2h = now - 4469h
# Tret(N)    = now - 3h - 2h = now - 5h
# Trem(N)    = now - 2h
# Tpub(N+1)  = now - 174h - 2h = now - 176h
# Tsbm(N+1)  = TretZ(N)
# TactZ(N+1) = TretZ(N)
# TretZ(N+1) = now + 4293h - 2h = now + 4291h
# Tact(N+1)  = Tret(N)
# Tret(N+1)  = now + 4461h - 2h = now + 4459h
# Trem(N+1)  = now + 4464h - 2h = now + 4462h
TactZN="now-4637h"
TretZN="now-173h"
TactN="now-4469h"
TretN="now-5h"
TremN="now-2h"
TpubN1="now-176h"
TsbmN1="${TretZN}"
TactZN1="${TretZN}"
TretZN1="now+4291h"
TactN1="${TretN}"
TretN1="now+4459h"
TremN1="now+4462h"
csktimes="-P ${TactN}  -P sync ${TactZN} -A ${TactZN}  -I ${TretZN}  -D ${TremN}"
newtimes="-P ${TpubN1} -P sync ${TsbmN1} -A ${TactZN1} -I ${TretZN1} -D ${TremN1}"
CSK1=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-roll2 -l policies/autosign.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $U $TremN  -r $U $TremN  -d $H $TremN -z $H now-135h "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TsbmN1 -r $O $TsbmN1 -d $O $TremN -z $O now-135h "$CSK2" > settime.out.$zone.2 2>&1
# Set key rollover relationship.
key_successor $CSK1 $CSK2
# Sign zone.
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 13 "$CSK1" >> "$infile"
private_type_record $zone 13 "$CSK2" >> "$infile"
$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile > signer.out.$zone.1 2>&1
