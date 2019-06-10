#!/bin/bash
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

TOP=$( (cd ../../.. && pwd) )
dig=${TOP}/bin/dig/dig

cmd="${dig} -p 5300 @127.127.0.0 txt"
inner() {
	zone=$1 i=$2 to=$3
	x=$i
	dout=dig$x.out
	tout=time$x.out
	while [ $i -lt $to ]
	do
		case $zone in
		.) zone=;;
		esac
		
		(time -p $cmd $i.${sub}$zone > $dout ) 2> $tout
		s=`sed -n '/real/s/[^0-9]*\([0-9]*\)\..*/\1/p' $tout`
		case $s in
		0);;
		1) t1=`expr ${t1:-0} + 1`;;
		2) t2=`expr ${t2:-0} + 1`;;
		3) t3=`expr ${t3:-0} + 1`;;
		*) echo $i `grep real $tout`;;
		esac

		grep "status: \(NXDOMAIN\|NOERROR\)" $dout > /dev/null || {
			echo $cmd $i.${sub}$zone
			cat $dout
		}
		i=`expr $i + 1`
	done
	if test ${t1:-0} -ne 0 -o ${t2:-0} -ne 0 -o ${t3:-0} -ne 0
	then
		echo "$x timeouts: t1=${t1:-0} t2=${t2:-0} t3=${t3:-0}"
	fi
}

while read zone rest
do
	for sub in "" medium. big.
	do
		case $zone in
		.) echo doing ${sub:-.};;
		*) echo doing $sub$zone;;
		esac
		( inner $zone 1 100) &
		( inner $zone 101 200) &
		( inner $zone 201 300) &
		( inner $zone 301 400) &
		( inner $zone 401 500) &
		( inner $zone 501 600) &
		( inner $zone 601 700) &
		( inner $zone 701 800) &
		( inner $zone 801 900) &
		( inner $zone 901 1000) &
		( inner $zone 1001 1100) &
		( inner $zone 1101 1200) &
		( inner $zone 1201 1300) &
		( inner $zone 1301 1400) &
		( inner $zone 1401 1500) &
		( inner $zone 1501 1600) &
		( inner $zone 1601 1700) &
		wait
	done
done
