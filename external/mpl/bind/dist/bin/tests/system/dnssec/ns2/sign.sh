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

set -e

zone=example.
infile=example.db.in
zonefile=example.db

# Have the child generate a zone key and pass it to us.

( cd ../ns3 && $SHELL sign.sh )

for subdomain in secure badds bogus dynamic keyless nsec3 optout \
	nsec3-unknown optout-unknown multiple rsasha256 rsasha512 \
	kskonly update-nsec3 auto-nsec auto-nsec3 secure.below-cname \
	ttlpatch split-dnssec split-smart expired expiring upper lower \
	dnskey-unknown dnskey-nsec3-unknown managed-future revkey \
	dname-at-apex-nsec3 occluded
do
	cp "../ns3/dsset-$subdomain.example$TP" .
done

keyname1=$("$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" -n zone -f KSK "$zone")
keyname2=$("$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" -n zone "$zone")

cat "$infile" "$keyname1.key" "$keyname2.key" > "$zonefile"

"$SIGNER" -P -g -o "$zone" -k "$keyname1" "$zonefile" "$keyname2" > /dev/null 2>&1

#
# lower/uppercase the signature bits with the exception of the last characters
# changing the last 4 characters will lead to a bad base64 encoding.
#

zonefiletmp=$(mktemp "$zonefile.XXXXXX") || exit 1
"$CHECKZONE" -D -q -i local "$zone" "$zonefile.signed" |
tr -d '\r' |
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

{ print; }' > "$zonefiletmp" && mv "$zonefiletmp" "$zonefile.signed"

#
# signed in-addr.arpa w/ a delegation for 10.in-addr.arpa which is unsigned.
#
zone=in-addr.arpa.
infile=in-addr.arpa.db.in
zonefile=in-addr.arpa.db

keyname1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
keyname2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname1.key" "$keyname2.key" > "$zonefile"
"$SIGNER" -P -g -o "$zone" -k "$keyname1" "$zonefile" "$keyname2" > /dev/null 2>&1

# Sign the privately secure file

privzone=private.secure.example.
privinfile=private.secure.example.db.in
privzonefile=private.secure.example.db

privkeyname=$("$KEYGEN" -q -a "${DEFAULT_ALGORITHM}" -b "${DEFAULT_BITS}" -n zone "$privzone")

cat "$privinfile" "$privkeyname.key" > "$privzonefile"

"$SIGNER" -P -g -o "$privzone" -l dlv "$privzonefile" > /dev/null 2>&1

# Sign the DLV secure zone.

dlvzone=dlv.
dlvinfile=dlv.db.in
dlvzonefile=dlv.db
dlvsetfile="dlvset-$(echo "$privzone" |sed -e "s/\\.$//g")$TP"

dlvkeyname=$("$KEYGEN" -q -a "${DEFAULT_ALGORITHM}" -b "${DEFAULT_BITS}" -n zone "$dlvzone")

cat "$dlvinfile" "$dlvkeyname.key" "$dlvsetfile" > "$dlvzonefile"

"$SIGNER" -P -g -o "$dlvzone" "$dlvzonefile" > /dev/null 2>&1

# Sign the badparam secure file

zone=badparam.
infile=badparam.db.in
zonefile=badparam.db

keyname1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
keyname2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname1.key" "$keyname2.key" > "$zonefile"

"$SIGNER" -P -3 - -H 1 -g -o "$zone" -k "$keyname1" "$zonefile" "$keyname2" > /dev/null 2>&1

sed -e 's/IN NSEC3 1 0 1 /IN NSEC3 1 0 10 /' "$zonefile.signed" > "$zonefile.bad"

# Sign the single-nsec3 secure zone with optout

zone=single-nsec3.
infile=single-nsec3.db.in
zonefile=single-nsec3.db

keyname1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
keyname2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname1.key" "$keyname2.key" > "$zonefile"

"$SIGNER" -P -3 - -A -H 1 -g -o "$zone" -k "$keyname1" "$zonefile" "$keyname2" > /dev/null 2>&1

#
# algroll has just has the old DNSKEY records removed and is waiting
# for them to be flushed from caches.  We still need to generate
# RRSIGs for the old DNSKEY.
#
zone=algroll.
infile=algroll.db.in
zonefile=algroll.db

keyold1=$("$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" -n zone -f KSK "$zone")
keyold2=$("$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" -n zone "$zone")
keynew1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
keynew2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keynew1.key" "$keynew2.key" > "$zonefile"

"$SIGNER" -P -o "$zone" -k "$keyold1" -k "$keynew1" "$zonefile" "$keyold1" "$keyold2" "$keynew1" "$keynew2" > /dev/null 2>&1

#
# Make a zone big enough that it takes several seconds to generate a new
# nsec3 chain.
#
zone=nsec3chain-test
zonefile=nsec3chain-test.db
cat > "$zonefile" << EOF
\$TTL 10
@	10	SOA	ns2 hostmaster 0 3600 1200 864000 1200
@	10	NS	ns2
@	10	NS	ns3
ns2	10	A	10.53.0.2
ns3	10	A	10.53.0.3
EOF
for i in $(seq 300); do
    echo "host$i 10 IN NS ns.elsewhere"
done >> "$zonefile"
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$key1.key" "$key2.key" >> "$zonefile"
"$SIGNER" -P -3 - -A -H 1 -g -o "$zone" -k "$key1" "$zonefile" "$key2" > /dev/null 2>&1

zone=cds.secure
infile=cds.secure.db.in
zonefile=cds.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
"$DSFROMKEY" -C "$key1.key" > "$key1.cds"
cat "$infile" "$key1.key" "$key2.key" "$key1.cds" >$zonefile
"$SIGNER" -P -g -o "$zone" "$zonefile" > /dev/null 2>&1

zone=cds-x.secure
infile=cds.secure.db.in
zonefile=cds-x.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key3=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
"$DSFROMKEY" -C "$key2.key" > "$key2.cds"
cat "$infile" "$key1.key" "$key3.key" "$key2.cds" > "$zonefile"
"$SIGNER" -P -g -x -o "$zone" "$zonefile" > /dev/null 2>&1

zone=cds-update.secure
infile=cds-update.secure.db.in
zonefile=cds-update.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$infile" "$key1.key" "$key2.key" > "$zonefile"
"$SIGNER" -P -g -o "$zone" "$zonefile" > /dev/null 2>&1

zone=cds-kskonly.secure
infile=cds-kskonly.secure.db.in
zonefile=cds-kskonly.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$infile" "$key1.key" "$key2.key" > "$zonefile"
"$SIGNER" -P -g -o "$zone" "$zonefile" > /dev/null 2>&1

zone=cds-auto.secure
infile=cds-auto.secure.db.in
zonefile=cds-auto.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
"$DSFROMKEY" -C "$key1.key" > "$key1.cds"
cat "$infile" "$key1.cds" > "$zonefile.signed"

zone=cdnskey.secure
infile=cdnskey.secure.db.in
zonefile=cdnskey.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
sed 's/DNSKEY/CDNSKEY/' "$key1.key" > "$key1.cds"
cat "$infile" "$key1.key" "$key2.key" "$key1.cds" > "$zonefile"
"$SIGNER" -P -g -o "$zone" "$zonefile" > /dev/null 2>&1

zone=cdnskey-x.secure
infile=cdnskey.secure.db.in
zonefile=cdnskey-x.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key3=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
sed 's/DNSKEY/CDNSKEY/' "$key1.key" > "$key1.cds"
cat "$infile" "$key2.key" "$key3.key" "$key1.cds" > "$zonefile"
"$SIGNER" -P -g -x -o "$zone" "$zonefile" > /dev/null 2>&1

zone=cdnskey-update.secure
infile=cdnskey-update.secure.db.in
zonefile=cdnskey-update.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$infile" "$key1.key" "$key2.key" > "$zonefile"
"$SIGNER" -P -g -o "$zone" "$zonefile" > /dev/null 2>&1

zone=cdnskey-kskonly.secure
infile=cdnskey-kskonly.secure.db.in
zonefile=cdnskey-kskonly.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$infile" "$key1.key" "$key2.key" > "$zonefile"
"$SIGNER" -P -g -o "$zone" "$zonefile" > /dev/null 2>&1

zone=cdnskey-auto.secure
infile=cdnskey-auto.secure.db.in
zonefile=cdnskey-auto.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
sed 's/DNSKEY/CDNSKEY/' "$key1.key" > "$key1.cds"
cat "$infile" "$key1.cds" > "$zonefile.signed"
