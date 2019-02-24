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

set -eu

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

$SHELL clean.sh

touch empty

Z=cds.test

keyz=`$KEYGEN -q -a RSASHA256 $Z`
key1=`$KEYGEN -q -a RSASHA256 -f KSK $Z`
key2=`$KEYGEN -q -a RSASHA256 -f KSK $Z`

idz=`echo $keyz | sed 's/.*+0*//'`
id1=`echo $key1 | sed 's/.*+0*//'`
id2=`echo $key2 | sed 's/.*+0*//'`

cat <<EOF >vars.sh
Z=$Z
key1=$key1
key2=$key2
idz=$idz
id1=$id1
id2=$id2
EOF

tac() {
	$PERL -e 'print reverse <>' "$@"
}

convert() {
	key=$1
	n=$2
	$DSFROMKEY $key >DS.$n
	grep ' 8 1 ' DS.$n >DS.$n-1
	grep ' 8 2 ' DS.$n >DS.$n-2
	sed 's/ IN DS / IN CDS /' <DS.$n >>CDS.$n
	sed 's/ IN DNSKEY / IN CDNSKEY /' <$key.key >CDNSKEY.$n
	sed 's/ IN DS / 3600 IN DS /' <DS.$n >DS.ttl$n
	sed 's/ IN DS / 7200 IN DS /' <DS.$n >DS.ttlong$n
	tac <DS.$n >DS.rev$n
}
convert $key1 1
convert $key2 2

# consistent order wrt IDs
sort DS.1 DS.2 >DS.both

cp DS.1 DS.inplace
$PERL -we 'utime time, time - 7200, "DS.inplace" or die'

mangle="$PERL mangle.pl"

$mangle " IN DS $id1 8 1 " <DS.1 >DS.broke1
$mangle " IN DS $id1 8 2 " <DS.1 >DS.broke2
$mangle " IN DS $id1 8 [12] " <DS.1 >DS.broke12

sed 's/^/update add /
$a\
send
' <DS.2 >UP.add2

sed 's/^/update del /
$a\
send
' <DS.1 >UP.del1

cat UP.add2 UP.del1 | sed 3d >UP.swap

sed 's/ add \(.*\) IN DS / add \1 3600 IN DS /' <UP.swap >UP.swapttl

sign() {
	cat >db.$1
	$SIGNER >/dev/null 2>&1 \
		 -S -O full -o $Z -f sig.$1 db.$1
}

sign null <<EOF
\$TTL 1h
@	SOA	localhost.	root.localhost. (
		1	; serial
		1h	; refresh
		1h	; retry
		1w	; expiry
		1h	; minimum
		)
;
	NS	localhost.
;
EOF

cat sig.null CDS.1 >brk.unsigned-cds

cat db.null CDS.1 | sign cds.1
cat db.null CDS.2 | sign cds.2
cat db.null CDS.1 CDS.2 | sign cds.both

tac <sig.cds.1 >sig.cds.rev1

cat db.null CDNSKEY.2 | sign cdnskey.2
cat db.null CDS.2 CDNSKEY.2 | sign cds.cdnskey.2

$mangle '\s+IN\s+RRSIG\s+CDS .* '$idz' '$Z'\. ' \
	<sig.cds.1 >brk.rrsig.cds.zsk
$mangle '\s+IN\s+RRSIG\s+CDS .* '$id1' '$Z'\. ' \
	<sig.cds.1 >brk.rrsig.cds.ksk

$mangle " IN CDS $id1 8 1 " <db.cds.1 |
sign cds-mangled

bad=`$PERL -le "print ($id1 ^ 255);"`
sed 's/IN CDS '$id1' 8 1 /IN CDS '$bad' 8 1 /' <db.cds.1 |
sign bad-digests

sed '/IN CDS '$id1' 8 /p;s//IN CDS '$bad' 13 /' <db.cds.1 |
sign bad-algos

rm -f dsset-*
