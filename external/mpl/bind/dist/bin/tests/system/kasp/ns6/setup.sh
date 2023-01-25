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

echo_i "ns6/setup.sh"

setup() {
	zone="$1"
	echo_i "setting up zone: $zone"
	zonefile="${zone}.db"
	infile="${zone}.db.infile"
}

# Make lines shorter by storing key states in environment variables.
H="HIDDEN"
R="RUMOURED"
O="OMNIPRESENT"
U="UNRETENTIVE"

# The child zones (step1, step2) beneath these zones represent the various
# steps of unsigning a zone.
for zn in going-insecure.kasp going-insecure-dynamic.kasp
do
	# Step 1:
	# Set up a zone with dnssec-policy that is going insecure.
	setup step1.$zn
	echo "$zone" >> zones
	T="now-10d"
	ksktimes="-P $T -A $T -P sync $T"
	zsktimes="-P $T -A $T"
	KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
	ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200        $zsktimes $zone 2> keygen.out.$zone.2)
	cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
	private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >> "$infile"
	private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
	cp $infile $zonefile
	$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

	# Step 2:
	# Set up a zone with dnssec-policy that is going insecure. Don't add
	# this zone to the zones file, because this zone is no longer expected
	# to be fully signed.
	setup step2.$zn
	# The DS was withdrawn from the parent zone 26 hours ago.
	Trem="now-26h"
	ksktimes="-P $T -A $T -P sync $T"
	zsktimes="-P $T -A $T"
	KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
	ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 7200        $zsktimes $zone 2> keygen.out.$zone.2)
	$SETTIME -s -g $H -k $O $T -r $O $T -d $U $Trem -D ds $Trem "$KSK" > settime.out.$zone.1 2>&1
	$SETTIME -s -g $H -k $O $T -z $O $T                         "$ZSK" > settime.out.$zone.2 2>&1
	# Fake lifetime of old algorithm keys.
	echo "Lifetime: 0" >> "${KSK}.state"
	echo "Lifetime: 5184000" >> "${ZSK}.state"
	cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
	private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK" >> "$infile"
	private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK" >> "$infile"
	cp $infile $zonefile
	$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1
done

# This zone is going straight to "none" policy. This is undefined behavior.
setup step1.going-straight-to-none.kasp
echo "$zone" >> zones
TactN="now"
csktimes="-P ${TactN} -A ${TactN} -P sync ${TactN}"
CSK=$($KEYGEN -k default $csktimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -k $O $TactN -z $O $TactN -r $O $TactN -d $O $TactN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -z -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

#
# The zones at algorithm-roll.kasp represent the various steps of a ZSK/KSK
# algorithm rollover.
#

# Step 1:
# Introduce the first key. This will immediately be active.
setup step1.algorithm-roll.kasp
echo "$zone" >> zones
TactN="now"
ksktimes="-P ${TactN} -A ${TactN} -P sync ${TactN}"
zsktimes="-P ${TactN} -A ${TactN}"
KSK=$($KEYGEN -a RSASHA256 -L 3600 -f KSK $ksktimes $zone 2> keygen.out.$zone.1)
ZSK=$($KEYGEN -a RSASHA256 -L 3600        $zsktimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -d $O $TactN "$KSK" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN -z $O $TactN              "$ZSK" > settime.out.$zone.2 2>&1
cat template.db.in "${KSK}.key" "${ZSK}.key" > "$infile"
private_type_record $zone 8 "$KSK" >> "$infile"
private_type_record $zone 8 "$ZSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 2:
# After the publication interval has passed the DNSKEY is OMNIPRESENT.
setup step2.algorithm-roll.kasp
# The time passed since the new algorithm keys have been introduced is 3 hours.
TactN="now-3h"
TpubN1="now-3h"
# Tsbm(N+1) = TpubN1 + Ipub = now + TTLsig + Dprp + publish-safety =
# now - 3h + 6h + 1h + 1h = now + 5h
TsbmN1="now+5h"
ksk1times="-P ${TactN}  -A ${TactN}  -P sync ${TactN}  -I now"
zsk1times="-P ${TactN}  -A ${TactN}                    -I now"
ksk2times="-P ${TpubN1} -A ${TpubN1} -P sync ${TsbmN1}"
zsk2times="-P ${TpubN1} -A ${TpubN1}"
KSK1=$($KEYGEN -a RSASHA256          -L 3600 -f KSK $ksk1times $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a RSASHA256          -L 3600        $zsk1times $zone 2> keygen.out.$zone.2)
KSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600 -f KSK $ksk2times $zone 2> keygen.out.$zone.3)
ZSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $zsk2times $zone 2> keygen.out.$zone.4)
$SETTIME -s -g $H -k $O $TactN  -r $O $TactN  -d $O $TactN  "$KSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $H -k $O $TactN  -z $O $TactN                "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $R $TpubN1 -r $R $TpubN1 -d $H $TpubN1 "$KSK2" > settime.out.$zone.3 2>&1
$SETTIME -s -g $O -k $R $TpubN1 -z $R $TpubN1               "$ZSK2" > settime.out.$zone.4 2>&1
# Fake lifetime of old algorithm keys.
echo "Lifetime: 0" >> "${KSK1}.state"
echo "Lifetime: 0" >> "${ZSK1}.state"
cat template.db.in "${KSK1}.key" "${ZSK1}.key" "${KSK2}.key" "${ZSK2}.key" > "$infile"
private_type_record $zone 8  "$KSK1" >> "$infile"
private_type_record $zone 8  "$ZSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK2" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 3:
# The zone signatures are also OMNIPRESENT.
setup step3.algorithm-roll.kasp
# The time passed since the new algorithm keys have been introduced is 9 hours.
TactN="now-9h"
TretN="now-6h"
TpubN1="now-9h"
TsbmN1="now-1h"
ksk1times="-P ${TactN}  -A ${TactN}  -P sync ${TactN}  -I ${TretN}"
zsk1times="-P ${TactN}  -A ${TactN}                    -I ${TretN}"
ksk2times="-P ${TpubN1} -A ${TpubN1} -P sync ${TsbmN1}"
zsk2times="-P ${TpubN1} -A ${TpubN1}"
KSK1=$($KEYGEN -a RSASHA256          -L 3600 -f KSK $ksk1times $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a RSASHA256          -L 3600        $zsk1times $zone 2> keygen.out.$zone.2)
KSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600 -f KSK $ksk2times $zone 2> keygen.out.$zone.3)
ZSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $zsk2times $zone 2> keygen.out.$zone.4)
$SETTIME -s -g $H -k $O $TactN  -r $O $TactN  -d $O $TactN  "$KSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $H -k $O $TactN  -z $O $TactN                "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TpubN1 -r $O $TpubN1 -d $H $TpubN1 "$KSK2" > settime.out.$zone.3 2>&1
$SETTIME -s -g $O -k $O $TpubN1 -z $R $TpubN1               "$ZSK2" > settime.out.$zone.4 2>&1
# Fake lifetime of old algorithm keys.
echo "Lifetime: 0" >> "${KSK1}.state"
echo "Lifetime: 0" >> "${ZSK1}.state"
cat template.db.in "${KSK1}.key" "${ZSK1}.key" "${KSK2}.key" "${ZSK2}.key" > "$infile"
private_type_record $zone 8  "$KSK1" >> "$infile"
private_type_record $zone 8  "$ZSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK2" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 4:
# The DS is swapped and can become OMNIPRESENT.
setup step4.algorithm-roll.kasp
# The time passed since the DS has been swapped is 29 hours.
TactN="now-38h"
TretN="now-35h"
TpubN1="now-38h"
TsbmN1="now-30h"
TactN1="now-29h"
ksk1times="-P ${TactN}  -A ${TactN}  -P sync ${TactN}  -I ${TretN}"
zsk1times="-P ${TactN}  -A ${TactN}                    -I ${TretN}"
ksk2times="-P ${TpubN1} -A ${TpubN1} -P sync ${TsbmN1}"
zsk2times="-P ${TpubN1} -A ${TpubN1}"
KSK1=$($KEYGEN -a RSASHA256          -L 3600 -f KSK $ksk1times $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a RSASHA256          -L 3600        $zsk1times $zone 2> keygen.out.$zone.2)
KSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600 -f KSK $ksk2times $zone 2> keygen.out.$zone.3)
ZSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $zsk2times $zone 2> keygen.out.$zone.4)
$SETTIME -s -g $H -k $O $TactN  -r $O $TactN  -d $U $TactN1 -D ds $TactN1 "$KSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $H -k $O $TactN  -z $O $TactN                              "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TpubN1 -r $O $TpubN1 -d $R $TactN1 -P ds $TactN1 "$KSK2" > settime.out.$zone.3 2>&1
$SETTIME -s -g $O -k $O $TpubN1 -z $R $TpubN1                             "$ZSK2" > settime.out.$zone.4 2>&1
# Fake lifetime of old algorithm keys.
echo "Lifetime: 0" >> "${KSK1}.state"
echo "Lifetime: 0" >> "${ZSK1}.state"
cat template.db.in "${KSK1}.key" "${ZSK1}.key" "${KSK2}.key" "${ZSK2}.key" > "$infile"
private_type_record $zone 8  "$KSK1" >> "$infile"
private_type_record $zone 8  "$ZSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK2" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 5:
# The DNSKEY is removed long enough to be HIDDEN.
setup step5.algorithm-roll.kasp
# The time passed since the DNSKEY has been removed is 2 hours.
TactN="now-40h"
TretN="now-37h"
TremN="now-2h"
TpubN1="now-40h"
TsbmN1="now-32h"
TactN1="now-31h"
ksk1times="-P ${TactN}  -A ${TactN}  -P sync ${TactN}  -I ${TretN}"
zsk1times="-P ${TactN}  -A ${TactN}                    -I ${TretN}"
ksk2times="-P ${TpubN1} -A ${TpubN1} -P sync ${TsbmN1}"
zsk2times="-P ${TpubN1} -A ${TpubN1}"
KSK1=$($KEYGEN -a RSASHA256          -L 3600 -f KSK $ksk1times $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a RSASHA256          -L 3600        $zsk1times $zone 2> keygen.out.$zone.2)
KSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600 -f KSK $ksk2times $zone 2> keygen.out.$zone.3)
ZSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $zsk2times $zone 2> keygen.out.$zone.4)
$SETTIME -s -g $H -k $U $TremN  -r $U $TremN  -d $H $TactN1 "$KSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $H -k $U $TremN  -z $U $TremN                "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TpubN1 -r $O $TpubN1 -d $O $TactN1 "$KSK2" > settime.out.$zone.3 2>&1
$SETTIME -s -g $O -k $O $TpubN1 -z $R $TpubN1               "$ZSK2" > settime.out.$zone.4 2>&1
# Fake lifetime of old algorithm keys.
echo "Lifetime: 0" >> "${KSK1}.state"
echo "Lifetime: 0" >> "${ZSK1}.state"
cat template.db.in "${KSK1}.key" "${ZSK1}.key" "${KSK2}.key" "${ZSK2}.key" > "$infile"
private_type_record $zone 8  "$KSK1" >> "$infile"
private_type_record $zone 8  "$ZSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK2" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 6:
# The RRSIGs have been removed long enough to be HIDDEN.
setup step6.algorithm-roll.kasp
# Additional time passed: 7h.
TactN="now-47h"
TretN="now-44h"
TremN="now-7h"
TpubN1="now-47h"
TsbmN1="now-39h"
TactN1="now-38h"
TdeaN="now-9h"
ksk1times="-P ${TactN}  -A ${TactN}  -P sync ${TactN}  -I ${TretN}"
zsk1times="-P ${TactN}  -A ${TactN}                    -I ${TretN}"
ksk2times="-P ${TpubN1} -A ${TpubN1} -P sync ${TsbmN1}"
zsk2times="-P ${TpubN1} -A ${TpubN1}"
KSK1=$($KEYGEN -a RSASHA256          -L 3600 -f KSK $ksk1times $zone 2> keygen.out.$zone.1)
ZSK1=$($KEYGEN -a RSASHA256          -L 3600        $zsk1times $zone 2> keygen.out.$zone.2)
KSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600 -f KSK $ksk2times $zone 2> keygen.out.$zone.3)
ZSK2=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600        $zsk2times $zone 2> keygen.out.$zone.4)
$SETTIME -s -g $H -k $H $TremN  -r $U $TdeaN  -d $H $TactN1 "$KSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $H -k $H $TremN  -z $U $TdeaN                "$ZSK1" > settime.out.$zone.2 2>&1
$SETTIME -s -g $O -k $O $TpubN1 -r $O $TpubN1 -d $O $TactN1 "$KSK2" > settime.out.$zone.3 2>&1
$SETTIME -s -g $O -k $O $TpubN1 -z $R $TpubN1               "$ZSK2" > settime.out.$zone.4 2>&1
# Fake lifetime of old algorithm keys.
echo "Lifetime: 0" >> "${KSK1}.state"
echo "Lifetime: 0" >> "${ZSK1}.state"
cat template.db.in "${KSK1}.key" "${ZSK1}.key" "${KSK2}.key" "${ZSK2}.key" > "$infile"
private_type_record $zone 8  "$KSK1" >> "$infile"
private_type_record $zone 8  "$ZSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$KSK2" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$ZSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

#
# The zones at csk-algorithm-roll.kasp represent the various steps of a CSK
# algorithm rollover.
#

# Step 1:
# Introduce the first key. This will immediately be active.
setup step1.csk-algorithm-roll.kasp
echo "$zone" >> zones
TactN="now"
csktimes="-P ${TactN} -P sync ${TactN} -A ${TactN}"
CSK=$($KEYGEN -k csk-algoroll -l policies/csk1.conf $csktimes $zone 2> keygen.out.$zone.1)
$SETTIME -s -g $O -k $O $TactN -r $O $TactN -z $O $TactN -d $O $TactN "$CSK" > settime.out.$zone.1 2>&1
cat template.db.in "${CSK}.key" > "$infile"
private_type_record $zone 5 "$CSK" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -z -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 2:
# After the publication interval has passed the DNSKEY is OMNIPRESENT.
setup step2.csk-algorithm-roll.kasp
# The time passed since the new algorithm keys have been introduced is 3 hours.
TactN="now-3h"
TpubN1="now-3h"
csktimes="-P ${TactN}  -A ${TactN} -P sync ${TactN} -I now"
newtimes="-P ${TpubN1} -A ${TpubN1}"
CSK1=$($KEYGEN -k csk-algoroll -l policies/csk1.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-algoroll -l policies/csk2.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactN  -r $O $TactN  -z $O $TactN  -d $O $TactN  "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $R $TpubN1 -r $R $TpubN1 -z $R $TpubN1 -d $H $TpubN1 "$CSK2" > settime.out.$zone.2 2>&1
# Fake lifetime of old algorithm keys.
echo "Lifetime: 0" >> "${CSK1}.state"
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 5  "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -z -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 3:
# The zone signatures are also OMNIPRESENT.
setup step3.csk-algorithm-roll.kasp
# The time passed since the new algorithm keys have been introduced is 9 hours.
TactN="now-9h"
TretN="now-6h"
TpubN1="now-9h"
TactN1="now-6h"
csktimes="-P ${TactN}  -A ${TactN} -P sync ${TactN} -I ${TretN}"
newtimes="-P ${TpubN1} -A ${TpubN1}"
CSK1=$($KEYGEN -k csk-algoroll -l policies/csk1.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-algoroll -l policies/csk2.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactN  -r $O $TactN  -z $O $TactN  -d $O $TactN  "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN1 -r $O $TactN1 -z $R $TpubN1 -d $H $TpubN1 "$CSK2" > settime.out.$zone.2 2>&1
# Fake lifetime of old algorithm keys.
echo "Lifetime: 0" >> "${CSK1}.state"
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 5  "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -z -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 4:
# The DS is swapped and can become OMNIPRESENT.
setup step4.csk-algorithm-roll.kasp
# The time passed since the DS has been swapped is 29 hours.
TactN="now-38h"
TretN="now-35h"
TpubN1="now-38h"
TactN1="now-35h"
TsubN1="now-29h"
csktimes="-P ${TactN}  -A ${TactN} -P sync ${TactN} -I ${TretN}"
newtimes="-P ${TpubN1} -A ${TpubN1}"
CSK1=$($KEYGEN -k csk-algoroll -l policies/csk1.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-algoroll -l policies/csk2.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $O $TactN  -r $O $TactN  -z $O $TactN  -d $U $TactN1 -D ds $TactN1 "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN1 -r $O $TactN1 -z $O $TsubN1 -d $R $TsubN1 -P ds $TsubN1 "$CSK2" > settime.out.$zone.2 2>&1
# Fake lifetime of old algorithm keys.
echo "Lifetime: 0" >> "${CSK1}.state"
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 5  "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -z -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 5:
# The DNSKEY is removed long enough to be HIDDEN.
setup step5.csk-algorithm-roll.kasp
# The time passed since the DNSKEY has been removed is 2 hours.
TactN="now-40h"
TretN="now-37h"
TremN="now-2h"
TpubN1="now-40h"
TactN1="now-37h"
TsubN1="now-31h"
csktimes="-P ${TactN}  -A ${TactN} -P sync ${TactN} -I ${TretN}"
newtimes="-P ${TpubN1} -A ${TpubN1}"
CSK1=$($KEYGEN -k csk-algoroll -l policies/csk1.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-algoroll -l policies/csk2.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $U $TremN  -r $U $TremN  -z $U $TremN  -d $H $TremN  "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN1 -r $O $TactN1 -z $O $TsubN1 -d $O $TremN  "$CSK2" > settime.out.$zone.2 2>&1
# Fake lifetime of old algorithm keys.
echo "Lifetime: 0" >> "${CSK1}.state"
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 5  "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -z -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

# Step 6:
# The RRSIGs have been removed long enough to be HIDDEN.
setup step6.csk-algorithm-roll.kasp
# Additional time passed: 7h.
TactN="now-47h"
TretN="now-44h"
TdeaN="now-9h"
TremN="now-7h"
TpubN1="now-47h"
TactN1="now-44h"
TsubN1="now-38h"
csktimes="-P ${TactN}  -A ${TactN} -P sync ${TactN} -I ${TretN}"
newtimes="-P ${TpubN1} -A ${TpubN1}"
CSK1=$($KEYGEN -k csk-algoroll -l policies/csk1.conf $csktimes $zone 2> keygen.out.$zone.1)
CSK2=$($KEYGEN -k csk-algoroll -l policies/csk2.conf $newtimes $zone 2> keygen.out.$zone.2)
$SETTIME -s -g $H -k $H $TremN  -r $U $TdeaN  -z $U $TdeaN  -d $H $TactN1 "$CSK1" > settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O $TactN1 -r $O $TactN1 -z $O $TsubN1 -d $O $TactN1 "$CSK2" > settime.out.$zone.2 2>&1
# Fake lifetime of old algorithm keys.
echo "Lifetime: 0" >> "${CSK1}.state"
cat template.db.in "${CSK1}.key" "${CSK2}.key" > "$infile"
private_type_record $zone 5  "$CSK1" >> "$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK2" >> "$infile"
cp $infile $zonefile
$SIGNER -S -x -z -s now-1h -e now+2w -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1

#
# Reload testing
#
echo "example" >> zones
cp example.db.in example.db

setup "dynamic2inline.kasp"
cp template.db.in $zonefile
