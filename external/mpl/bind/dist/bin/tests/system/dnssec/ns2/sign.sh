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

set -e

# Sign child zones (served by ns3).
(cd ../ns3 && $SHELL sign.sh)

echo_i "ns2/sign.sh"

# Get the DS records for the "trusted." and "managed." zones.
for subdomain in secure unsupported disabled enabled; do
  cp "../ns3/dsset-$subdomain.managed." .
  cp "../ns3/dsset-$subdomain.trusted." .
done

# Sign the "trusted." and "managed." zones.
zone=managed.
infile=key.db.in
zonefile=managed.db

keyname1=$("$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" -n zone -f KSK "$zone")
keyname2=$("$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" -n zone "$zone")

cat "$infile" "$keyname1.key" "$keyname2.key" >"$zonefile"

"$SIGNER" -g -o "$zone" -k "$keyname1" "$zonefile" "$keyname2" >/dev/null 2>&1

zone=trusted.
infile=key.db.in
zonefile=trusted.db

keyname1=$("$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" -n zone -f KSK "$zone")
keyname2=$("$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" -n zone "$zone")

cat "$infile" "$keyname1.key" "$keyname2.key" >"$zonefile"

"$SIGNER" -g -o "$zone" -k "$keyname1" "$zonefile" "$keyname2" >/dev/null 2>&1

# The "example." zone.
zone=example.
infile=example.db.in
zonefile=example.db

# Get the DS records for the "example." zone.
for subdomain in secure badds bogus dynamic keyless nsec3 optout \
  nsec3-unknown optout-unknown multiple rsasha256 rsasha512 \
  kskonly update-nsec3 auto-nsec auto-nsec3 secure.below-cname \
  ttlpatch split-dnssec split-smart expired expiring upper lower \
  dnskey-unknown dnskey-unsupported dnskey-unsupported-2 \
  dnskey-nsec3-unknown managed-future revkey \
  dname-at-apex-nsec3 occluded; do
  cp "../ns3/dsset-$subdomain.example." .
done

# Sign the "example." zone.
keyname1=$("$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" -n zone -f KSK "$zone")
keyname2=$("$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" -n zone "$zone")

cat "$infile" "$keyname1.key" "$keyname2.key" >"$zonefile"

"$SIGNER" -g -o "$zone" -k "$keyname1" "$zonefile" "$keyname2" >/dev/null 2>&1

#
# lower/uppercase the signature bits with the exception of the last characters
# changing the last 4 characters will lead to a bad base64 encoding.
#

zonefiletmp=$(mktemp "$zonefile.XXXXXX") || exit 1
"$CHECKZONE" -D -q -i local "$zone" "$zonefile.signed" \
  | awk '
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

{ print; }' >"$zonefiletmp" && mv "$zonefiletmp" "$zonefile.signed"

#
# signed in-addr.arpa w/ a delegation for 10.in-addr.arpa which is unsigned.
#
zone=in-addr.arpa.
infile=in-addr.arpa.db.in
zonefile=in-addr.arpa.db

keyname1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
keyname2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname1.key" "$keyname2.key" >"$zonefile"
"$SIGNER" -g -o "$zone" -k "$keyname1" "$zonefile" "$keyname2" >/dev/null 2>&1

# Sign the badparam secure file

zone=badparam.
infile=badparam.db.in
zonefile=badparam.db

keyname1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
keyname2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname1.key" "$keyname2.key" >"$zonefile"

"$SIGNER" -3 - -H 1 -g -o "$zone" -k "$keyname1" "$zonefile" "$keyname2" >/dev/null 2>&1

sed -e 's/IN NSEC3 1 0 1 /IN NSEC3 1 0 10 /' "$zonefile.signed" >"$zonefile.bad"

# Sign the single-nsec3 secure zone with optout

zone=single-nsec3.
infile=single-nsec3.db.in
zonefile=single-nsec3.db

keyname1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
keyname2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname1.key" "$keyname2.key" >"$zonefile"

"$SIGNER" -3 - -A -H 1 -g -o "$zone" -k "$keyname1" "$zonefile" "$keyname2" >/dev/null 2>&1

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

cat "$infile" "$keynew1.key" "$keynew2.key" >"$zonefile"

"$SIGNER" -o "$zone" -k "$keyold1" -k "$keynew1" "$zonefile" "$keyold1" "$keyold2" "$keynew1" "$keynew2" >/dev/null 2>&1

#
# Make a zone big enough that it takes several seconds to generate a new
# nsec3 chain.
#
zone=nsec3chain-test
zonefile=nsec3chain-test.db
cat >"$zonefile" <<EOF
\$TTL 10
@	10	SOA	ns2 hostmaster 0 3600 1200 864000 1200
@	10	NS	ns2
@	10	NS	ns3
ns2	10	A	10.53.0.2
ns3	10	A	10.53.0.3
EOF
i=1
while [ $i -le 300 ]; do
  echo "host$i 10 IN NS ns.elsewhere"
  i=$((i + 1))
done >>"$zonefile"
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$key1.key" "$key2.key" >>"$zonefile"
"$SIGNER" -3 - -A -H 1 -g -o "$zone" -k "$key1" "$zonefile" "$key2" >/dev/null 2>&1

zone=cds.secure
infile=cds.secure.db.in
zonefile=cds.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
"$DSFROMKEY" -C "$key1.key" >"$key1.cds"
cat "$infile" "$key1.key" "$key2.key" "$key1.cds" >$zonefile
"$SIGNER" -g -o "$zone" "$zonefile" >/dev/null 2>&1

zone=cds-x.secure
infile=cds.secure.db.in
zonefile=cds-x.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key3=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
"$DSFROMKEY" -C "$key2.key" >"$key2.cds"
cat "$infile" "$key1.key" "$key2.key" "$key3.key" "$key2.cds" >"$zonefile"
"$SIGNER" -g -x -o "$zone" "$zonefile" >/dev/null 2>&1

zone=cds-update.secure
infile=cds-update.secure.db.in
zonefile=cds-update.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$infile" "$key1.key" "$key2.key" >"$zonefile"
"$SIGNER" -g -o "$zone" "$zonefile" >/dev/null 2>&1

zone=cds-kskonly.secure
infile=cds-kskonly.secure.db.in
zonefile=cds-kskonly.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$infile" "$key1.key" "$key2.key" >"$zonefile"
"$SIGNER" -g -o "$zone" "$zonefile" >/dev/null 2>&1
keyfile_to_key_id "$key1" >cds-kskonly.secure.id

zone=cds-auto.secure
infile=cds-auto.secure.db.in
zonefile=cds-auto.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
$SETTIME -P sync now "$key1" >/dev/null
cat "$infile" >"$zonefile.signed"

zone=cdnskey.secure
infile=cdnskey.secure.db.in
zonefile=cdnskey.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
sed 's/DNSKEY/CDNSKEY/' "$key1.key" >"$key1.cds"
cat "$infile" "$key1.key" "$key2.key" "$key1.cds" >"$zonefile"
"$SIGNER" -g -o "$zone" "$zonefile" >/dev/null 2>&1

zone=cdnskey-x.secure
infile=cdnskey.secure.db.in
zonefile=cdnskey-x.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key3=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
sed 's/DNSKEY/CDNSKEY/' "$key1.key" >"$key1.cds"
cat "$infile" "$key1.key" "$key2.key" "$key3.key" "$key1.cds" >"$zonefile"
"$SIGNER" -g -x -o "$zone" "$zonefile" >/dev/null 2>&1

zone=cdnskey-update.secure
infile=cdnskey-update.secure.db.in
zonefile=cdnskey-update.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$infile" "$key1.key" "$key2.key" >"$zonefile"
"$SIGNER" -g -o "$zone" "$zonefile" >/dev/null 2>&1

zone=cdnskey-kskonly.secure
infile=cdnskey-kskonly.secure.db.in
zonefile=cdnskey-kskonly.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$infile" "$key1.key" "$key2.key" >"$zonefile"
"$SIGNER" -g -o "$zone" "$zonefile" >/dev/null 2>&1
keyfile_to_key_id "$key1" >cdnskey-kskonly.secure.id

zone=cdnskey-auto.secure
infile=cdnskey-auto.secure.db.in
zonefile=cdnskey-auto.secure.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
$SETTIME -P sync now "$key1" >/dev/null
cat "$infile" >"$zonefile.signed"

zone=updatecheck-kskonly.secure
infile=template.secure.db.in
zonefile=${zone}.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
# Save key id's for checking active key usage
keyfile_to_key_id "$key1" >$zone.ksk.id
keyfile_to_key_id "$key2" >$zone.zsk.id
echo "${key1}" >$zone.ksk.key
echo "${key2}" >$zone.zsk.key
# Add CDS and CDNSKEY records
sed 's/DNSKEY/CDNSKEY/' "$key1.key" >"$key1.cdnskey"
"$DSFROMKEY" -C "$key1.key" >"$key1.cds"
cat "$infile" "$key1.key" "$key2.key" "$key1.cdnskey" "$key1.cds" >"$zonefile"
# Don't sign, let auto-dnssec maintain do it.
mv $zonefile "$zonefile.signed"

zone=hours-vs-days
infile=hours-vs-days.db.in
zonefile=hours-vs-days.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
$SETTIME -P sync now "$key1" >/dev/null
cat "$infile" >"$zonefile.signed"

#
# Negative result from this zone should come back as insecure.
#
zone=too-many-iterations
infile=too-many-iterations.db.in
zonefile=too-many-iterations.db
key1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")
key2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$infile" "$key1.key" "$key2.key" >"$zonefile"
"$SIGNER" -P -3 - -H too-many -g -o "$zone" "$zonefile" >/dev/null 2>&1
