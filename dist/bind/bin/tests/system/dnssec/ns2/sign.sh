#!/bin/sh
#
# Copyright (C) 2004, 2006, 2007  Internet Systems Consortium, Inc. ("ISC")
# Copyright (C) 2000-2003  Internet Software Consortium.
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

# Id: sign.sh,v 1.28.288.1 2009/12/31 21:02:44 each Exp

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

RANDFILE=../random.data

zone=example.
infile=example.db.in
zonefile=example.db

# Have the child generate a zone key and pass it to us.

( cd ../ns3 && sh sign.sh )

for subdomain in secure bogus dynamic keyless
do
	cp ../ns3/keyset-$subdomain.example. .
done

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone`
keyname2=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -g -r $RANDFILE -o $zone -k $keyname1 $zonefile $keyname2 > /dev/null

#
# lower/uppercase the signature bits with the exception of the last characters
# changing the last 4 characters will lead to a bad base64 encoding.
#
$CHECKZONE -D -q -i local $zone $zonefile.signed |
awk '
tolower($1) == "bad-cname.example." && $4 == "RRSIG" && $5 == "CNAME" {
	for (i = 1; i <= NF; i++ ) {
		if (i <= 12) {
			printf("%s ", $i);
			continue;
		}
		prefix = substr($i, 1, length($i) - 4);
		suffix = substr($i, length($i) - 4, 4);
		if (i > 12 && tolower(prefix) != prefix)
			printf("%s%s", tolower(prefix), suffix);
		else if (i > 12 && toupper(prefix) != prefix)
			printf("%s%s", toupper(prefix), suffix);
		else
			printf("%s%s ", prefix, suffix);
	}
	printf("\n");
	next;
}

tolower($1) == "bad-dname.example." && $4 == "RRSIG" && $5 == "DNAME" {
	for (i = 1; i <= NF; i++ ) {
		if (i <= 12) {
			printf("%s ", $i);
			continue;
		}
		prefix = substr($i, 1, length($i) - 4);
		suffix = substr($i, length($i) - 4, 4);
		if (i > 12 && tolower(prefix) != prefix)
			printf("%s%s", tolower(prefix), suffix);
		else if (i > 12 && toupper(prefix) != prefix)
			printf("%s%s", toupper(prefix), suffix);
		else
			printf("%s%s ", prefix, suffix);
	}
	printf("\n");
	next;
}

{ print; }' > $zonefile.signed++ && mv $zonefile.signed++ $zonefile.signed


# Sign the privately secure file

privzone=private.secure.example.
privinfile=private.secure.example.db.in
privzonefile=private.secure.example.db

privkeyname=`$KEYGEN -r $RANDFILE -a RSAMD5 -b 768 -n zone $privzone`

cat $privinfile $privkeyname.key >$privzonefile

$SIGNER -g -r $RANDFILE -o $privzone -l dlv $privzonefile > /dev/null

# Sign the DLV secure zone.


dlvzone=dlv.
dlvinfile=dlv.db.in
dlvzonefile=dlv.db

dlvkeyname=`$KEYGEN -r $RANDFILE -a RSAMD5 -b 768 -n zone $dlvzone`

cat $dlvinfile $dlvkeyname.key dlvset-$privzone > $dlvzonefile

$SIGNER -g -r $RANDFILE -o $dlvzone $dlvzonefile > /dev/null
