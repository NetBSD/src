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

echo_i "ns9/setup.sh"

setup() {
	zone="$1"
	echo_i "setting up zone: $zone"
	zonefile="${zone}.db"
	infile="${zone}.db.infile"
	echo "$zone" >> zones
}

# Short environment variable names for key states and times.
H="HIDDEN"
R="RUMOURED"
O="OMNIPRESENT"
U="UNRETENTIVE"
T="now-30d"
Y="now-1y"

# DS Publication.
for zn in dspublished reference missing-dspublished bad-dspublished \
	  multiple-dspublished incomplete-dspublished bad2-dspublished
do
	setup "${zn}.checkds"
	cp template.db.in "$zonefile"
	keytimes="-P $T -P sync $T -A $T"
	CSK=$($KEYGEN -k default $keytimes $zone 2> keygen.out.$zone)
	$SETTIME -s -g $O -k $O $T -r $O $T -z $O $T -d $R $T "$CSK" > settime.out.$zone 2>&1
	cat template.db.in "${CSK}.key" > "$infile"
	private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >> "$infile"
	cp $infile $zonefile
	$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1
done

# DS Withdrawal.
for zn in dswithdrawn missing-dswithdrawn bad-dswithdrawn multiple-dswithdrawn \
	  incomplete-dswithdrawn bad2-dswithdrawn
do
	setup "${zn}.checkds"
	cp template.db.in "$zonefile"
	keytimes="-P $Y -P sync $Y -A $Y"
	CSK=$($KEYGEN -k default $keytimes $zone 2> keygen.out.$zone)
	$SETTIME -s -g $H -k $O $T -r $O $T -z $O $T -d $U $T "$CSK" > settime.out.$zone 2>&1
	cat template.db.in "${CSK}.key" > "$infile"
	private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >> "$infile"
	cp $infile $zonefile
	$SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile > signer.out.$zone.1 2>&1
done
