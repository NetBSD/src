#!/bin/sh -e
#
# Copyright (C) 2009  Internet Systems Consortium, Inc. ("ISC")
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

# Id: keygen.sh,v 1.3 2009/11/30 23:48:02 tbox Exp

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

RANDFILE=../random.data

zone=secure.example
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile
ksk=`$KEYGEN -q -r $RANDFILE -fk $zone`
$KEYGEN -q -r $RANDFILE $zone > /dev/null
$DSFROMKEY $ksk.key > dsset-${zone}.

#
#  NSEC3/NSEC test zone
#
zone=secure.nsec3.example
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile
ksk=`$KEYGEN -q -3 -r $RANDFILE -fk $zone`
$KEYGEN -q -3 -r $RANDFILE $zone > /dev/null
$DSFROMKEY $ksk.key > dsset-${zone}.

#
#  NSEC3/NSEC3 test zone
#
zone=nsec3.nsec3.example
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile
ksk=`$KEYGEN -q -3 -r $RANDFILE -fk $zone`
$KEYGEN -q -3 -r $RANDFILE $zone > /dev/null
$DSFROMKEY $ksk.key > dsset-${zone}.

#
#  OPTOUT/NSEC3 test zone
#
zone=optout.nsec3.example
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile
ksk=`$KEYGEN -q -3 -r $RANDFILE -fk $zone`
$KEYGEN -q -3 -r $RANDFILE $zone > /dev/null
$DSFROMKEY $ksk.key > dsset-${zone}.

#
# A nsec3 zone (non-optout).
#
zone=nsec3.example
zonefile="${zone}.db"
infile="${zonefile}.in"
cat $infile dsset-*.${zone}. > $zonefile
ksk=`$KEYGEN -q -3 -r $RANDFILE -fk $zone`
$KEYGEN -q -3 -r $RANDFILE $zone > /dev/null
$DSFROMKEY $ksk.key > dsset-${zone}.

#
#  OPTOUT/NSEC test zone
#
zone=secure.optout.example
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile
ksk=`$KEYGEN -q -3 -r $RANDFILE -fk $zone`
$KEYGEN -q -3 -r $RANDFILE $zone > /dev/null
$DSFROMKEY $ksk.key > dsset-${zone}.

#
#  OPTOUT/NSEC3 test zone
#
zone=nsec3.optout.example
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile
ksk=`$KEYGEN -q -3 -r $RANDFILE -fk $zone`
$KEYGEN -q -3 -r $RANDFILE $zone > /dev/null
$DSFROMKEY $ksk.key > dsset-${zone}.

#
#  OPTOUT/OPTOUT test zone
#
zone=optout.optout.example
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile
ksk=`$KEYGEN -q -r $RANDFILE -fk $zone`
$KEYGEN -q -r $RANDFILE $zone > /dev/null
$DSFROMKEY $ksk.key > dsset-${zone}.

#
# A optout nsec3 zone.
#
zone=optout.example
zonefile="${zone}.db"
infile="${zonefile}.in"
cat $infile dsset-*.${zone}. > $zonefile
ksk=`$KEYGEN -q -3 -r $RANDFILE -fk $zone`
$KEYGEN -q -3 -r $RANDFILE $zone > /dev/null
$DSFROMKEY $ksk.key > dsset-${zone}.

#
# A RSASHA256 zone.
#
zone=rsasha256.example
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile
ksk=`$KEYGEN -q -a RSASHA256 -b 2048 -r $RANDFILE -fk $zone`
$KEYGEN -q -a RSASHA256 -b 1024 -r $RANDFILE $zone > /dev/null
$DSFROMKEY $ksk.key > dsset-${zone}.

#
# A RSASHA512 zone.
#
zone=rsasha512.example
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile
ksk=`$KEYGEN -q -a RSASHA512 -b 2048 -r $RANDFILE -fk $zone`
$KEYGEN -q -a RSASHA512 -b 1024 -r $RANDFILE $zone > /dev/null
$DSFROMKEY $ksk.key > dsset-${zone}.
