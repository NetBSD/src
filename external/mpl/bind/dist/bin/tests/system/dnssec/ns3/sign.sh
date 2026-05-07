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

echo_i "ns3/sign.sh"

infile=key.db.in
for tld in managed trusted; do
  # A secure zone to test.
  zone=secure.${tld}
  zonefile=${zone}.db

  keyname1=$("$KEYGEN" -f KSK -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
  cat "$infile" "$keyname1.key" >"$zonefile"
  "$SIGNER" -z -P -3 - -o "$zone" -O full -f ${zonefile}.signed "$zonefile" >/dev/null

  # Zone to test trust anchor that matches disabled algorithm.
  zone=disabled.${tld}
  zonefile=${zone}.db

  keyname2=$("$KEYGEN" -f KSK -q -a "$DISABLED_ALGORITHM" -b "$DISABLED_BITS" -n zone "$zone")
  cat "$infile" "$keyname2.key" >"$zonefile"
  "$SIGNER" -z -P -3 - -o "$zone" -O full -f ${zonefile}.signed "$zonefile" >/dev/null

  # Zone to test trust anchor that has disabled algorithm for other domain.
  zone=enabled.${tld}
  zonefile=${zone}.db

  keyname3=$("$KEYGEN" -f KSK -q -a "$DISABLED_ALGORITHM" -b "$DISABLED_BITS" -n zone "$zone")
  cat "$infile" "$keyname3.key" >"$zonefile"
  "$SIGNER" -z -P -3 - -o "$zone" -O full -f ${zonefile}.signed "$zonefile" >/dev/null

  # Zone to test trust anchor with unsupported algorithm.
  zone=unsupported.${tld}
  zonefile=${zone}.db

  keyname4=$("$KEYGEN" -f KSK -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
  cat "$infile" "$keyname4.key" >"$zonefile"
  "$SIGNER" -z -3 - -o "$zone" -O full -f ${zonefile}.tmp "$zonefile" >/dev/null
  awk '$4 == "DNSKEY" { $7 = 255 } $4 == "RRSIG" { $6 = 255 } { print }' ${zonefile}.tmp >${zonefile}.signed

  # Make trusted-keys and managed keys conf sections for ns8.
  mv ${keyname4}.key ${keyname4}.tmp
  awk '$1 == "unsupported.'"${tld}"'." { $6 = 255 } { print }' ${keyname4}.tmp >${keyname4}.key

  # Zone to test trust anchor that is revoked.
  zone=revoked.${tld}
  zonefile=${zone}.db

  keyname5=$("$KEYGEN" -f KSK -f REVOKE -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
  cat "$infile" "$keyname5.key" >"$zonefile"
  "$SIGNER" -z -P -3 - -o "$zone" -O full -f ${zonefile}.signed "$zonefile" >/dev/null

  case $tld in
    "managed")
      keyfile_to_initial_keys $keyname1 $keyname2 $keyname3 $keyname4 $keyname5 >../ns8/managed.conf
      ;;
    "trusted")
      keyfile_to_static_keys $keyname1 $keyname2 $keyname3 $keyname4 $keyname5 >../ns8/trusted.conf
      ;;
  esac
done

echo_i "ns3/sign.sh: example zones"

# A zone that will be treated as insecure as the DEFAULT_ALGORITHM is
# disabled for it.
zone=badalg.secure.example.
infile=badalg.secure.example.db.in
zonefile=badalg.secure.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -o "$zone" "$zonefile" >/dev/null

# A zone that will be treated as insecure as the DEFAULT_ALGORITHM is
# disabled for ent.secure.example.
zone=zonecut.ent.secure.example.
infile=zonecut.ent.secure.example.db.in
zonefile=zonecut.ent.secure.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -o "$zone" "$zonefile" >/dev/null

#
zone=secure.example.
infile=secure.example.db.in
zonefile=secure.example.db

cnameandkey=$("$KEYGEN" -T KEY -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n host "cnameandkey.$zone")
dnameandkey=$("$KEYGEN" -T KEY -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n host "dnameandkey.$zone")
keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" dsset-badalg.secure.example. "$cnameandkey.key" "$dnameandkey.key" "$keyname.key" >"$zonefile"

"$SIGNER" -z -D -o "$zone" "$zonefile" >/dev/null
cat "$zonefile" "$zonefile".signed >"$zonefile".tmp
mv "$zonefile".tmp "$zonefile".signed

zone=bogus.example.
infile=bogus.example.db.in
zonefile=bogus.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -o "$zone" "$zonefile" >/dev/null

zone=dynamic.example.
infile=dynamic.example.db.in
zonefile=dynamic.example.db

keyname1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
keyname2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone -f KSK "$zone")

cat "$infile" "$keyname1.key" "$keyname2.key" >"$zonefile"

"$SIGNER" -o "$zone" "$zonefile" >/dev/null

zone=keyless.example.
infile=generic.example.db.in
zonefile=keyless.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -o "$zone" "$zonefile" >/dev/null

# Change the signer field of the a.b.keyless.example RRSIG A
# to point to a provably nonexistent DNSKEY record.
zonefiletmp=$(mktemp "$zonefile.XXXXXX") || exit 1
mv "$zonefile.signed" "$zonefiletmp"
"$PERL" <"$zonefiletmp" -p -e 's/ keyless.example/ b.keyless.example/
    if /^a.b.keyless.example/../A RRSIG NSEC/;' >"$zonefile.signed"
rm -f "$zonefiletmp"

#
#  NSEC3/NSEC test zone
#
zone=secure.nsec3.example.
infile=secure.nsec3.example.db.in
zonefile=secure.nsec3.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -o "$zone" "$zonefile" >/dev/null

#
#  NSEC3/NSEC3 test zone
#
zone=nsec3.nsec3.example.
infile=nsec3.nsec3.example.db.in
zonefile=nsec3.nsec3.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -3 - -o "$zone" "$zonefile" >/dev/null

#
#  OPTOUT/NSEC3 test zone
#
zone=optout.nsec3.example.
infile=optout.nsec3.example.db.in
zonefile=optout.nsec3.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -3 - -A -o "$zone" "$zonefile" >/dev/null

#
# A nsec3 zone (non-optout).
#
zone=nsec3.example.
infile=nsec3.example.db.in
zonefile=nsec3.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -g -3 - -o "$zone" "$zonefile" >/dev/null

#
#  OPTOUT/NSEC test zone
#
zone=secure.optout.example.
infile=secure.optout.example.db.in
zonefile=secure.optout.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -o "$zone" "$zonefile" >/dev/null

#
#  OPTOUT/NSEC3 test zone
#
zone=nsec3.optout.example.
infile=nsec3.optout.example.db.in
zonefile=nsec3.optout.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -3 - -o "$zone" "$zonefile" >/dev/null

#
#  OPTOUT/OPTOUT test zone
#
zone=optout.optout.example.
infile=optout.optout.example.db.in
zonefile=optout.optout.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -3 - -A -o "$zone" "$zonefile" >/dev/null

#
# A optout nsec3 zone.
#
zone=optout.example.
infile=optout.example.db.in
zonefile=optout.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -g -3 - -A -o "$zone" "$zonefile" >/dev/null

#
# A nsec3 zone (non-optout) with unknown nsec3 hash algorithm (-U).
#
zone=nsec3-unknown.example.
infile=nsec3-unknown.example.db.in
zonefile=nsec3-unknown.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -3 - -PU -o "$zone" "$zonefile" >/dev/null

#
# A optout nsec3 zone with a unknown nsec3 hash algorithm (-U).
#
zone=optout-unknown.example.
infile=optout-unknown.example.db.in
zonefile=optout-unknown.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -3 - -PU -A -o "$zone" "$zonefile" >/dev/null

#
# A zone that is signed with an unknown DNSKEY algorithm.
# Algorithm 7 is replaced by 100 in the zone and dsset.
#
zone=dnskey-unknown.example
infile=dnskey-unknown.example.db.in
zonefile=dnskey-unknown.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -3 - -o "$zone" -O full -f ${zonefile}.tmp "$zonefile" >/dev/null

awk '$4 == "DNSKEY" { $7 = 100 } $4 == "RRSIG" { $6 = 100 } { print }' ${zonefile}.tmp >${zonefile}.signed

DSFILE="dsset-${zone}."
$DSFROMKEY -A -f ${zonefile}.signed "$zone" >"$DSFILE"

#
# A zone that is signed with an unsupported DNSKEY algorithm (3).
# Algorithm 7 is replaced by 255 in the zone and dsset.
#
zone=dnskey-unsupported.example
infile=dnskey-unsupported.example.db.in
zonefile=dnskey-unsupported.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -3 - -o "$zone" -O full -f ${zonefile}.tmp "$zonefile" >/dev/null

awk '$4 == "DNSKEY" { $7 = 255 } $4 == "RRSIG" { $6 = 255 } { print }' ${zonefile}.tmp >${zonefile}.signed

DSFILE="dsset-${zone}."
$DSFROMKEY -A -f ${zonefile}.signed "$zone" >"$DSFILE"

#
# A zone which uses an unsupported algorithm for a DNSKEY and an unsupported
# digest for another DNSKEY
#
zone=digest-alg-unsupported.example.
infile=digest-alg-unsupported.example.db.in
zonefile=digest-alg-unsupported.example.db

cnameandkey=$("$KEYGEN" -T KEY -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n host "cnameandkey.$zone")
dnameandkey=$("$KEYGEN" -T KEY -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n host "dnameandkey.$zone")
keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
keyname2=$("$KEYGEN" -q -a ECDSAP384SHA384 -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$cnameandkey.key" "$dnameandkey.key" "$keyname.key" "$keyname2.key" >"$zonefile"

"$SIGNER" -z -D -o "$zone" "$zonefile" >/dev/null
cat "$zonefile" "$zonefile".signed >"$zonefile".tmp
mv "$zonefile".tmp "$zonefile".signed

# override generated DS record file so we can set different digest to each keys
DSFILE="dsset-${zone}"
$DSFROMKEY -a SHA-384 -A -f ${zonefile}.signed "$zone" | head -n 1 >"$DSFILE"
$DSFROMKEY -2 -A -f ${zonefile}.signed "$zone" | tail -1 >>"$DSFILE"

#
# A zone which is fine by itself (supported algorithm) but that is used
# to mimic unsupported DS digest (see ns8).
#
zone=ds-unsupported.example.
infile=ds-unsupported.example.db.in
zonefile=ds-unsupported.example.db

cnameandkey=$("$KEYGEN" -T KEY -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n host "cnameandkey.$zone")
dnameandkey=$("$KEYGEN" -T KEY -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n host "dnameandkey.$zone")
keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$cnameandkey.key" "$dnameandkey.key" "$keyname.key" >"$zonefile"

"$SIGNER" -z -D -o "$zone" "$zonefile" >/dev/null
cat "$zonefile" "$zonefile".signed >"$zonefile".tmp
mv "$zonefile".tmp "$zonefile".signed

#
# A zone with a published unsupported DNSKEY algorithm (Reserved).
# Different from above because this key is not intended for signing.
#
zone=dnskey-unsupported-2.example
infile=dnskey-unsupported-2.example.db.in
zonefile=dnskey-unsupported-2.example.db

ksk=$("$KEYGEN" -f KSK -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
zsk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$ksk.key" "$zsk.key" unsupported-algorithm.key >"$zonefile"

"$SIGNER" -3 - -o "$zone" -f ${zonefile}.signed "$zonefile" >/dev/null

#
# A zone with a unknown DNSKEY algorithm + unknown NSEC3 hash algorithm (-U).
# Algorithm 7 is replaced by 100 in the zone and dsset.
#
zone=dnskey-nsec3-unknown.example
infile=dnskey-nsec3-unknown.example.db.in
zonefile=dnskey-nsec3-unknown.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -3 - -o "$zone" -PU -O full -f ${zonefile}.tmp "$zonefile" >/dev/null

awk '$4 == "DNSKEY" { $7 = 100; print } $4 == "RRSIG" { $6 = 100; print } { print }' ${zonefile}.tmp >${zonefile}.signed

DSFILE="dsset-${zone}."
$DSFROMKEY -A -f ${zonefile}.signed "$zone" >"$DSFILE"

#
# A multiple parameter nsec3 zone.
#
zone=multiple.example.
infile=multiple.example.db.in
zonefile=multiple.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -O full -o "$zone" "$zonefile" >/dev/null
awk '$4 == "NSEC" || ( $4 == "RRSIG" && $5 == "NSEC" ) { print }' "$zonefile".signed >NSEC
"$SIGNER" -z -O full -u3 - -o "$zone" "$zonefile" >/dev/null
awk '$4 == "NSEC3" || ( $4 == "RRSIG" && $5 == "NSEC3" ) { print }' "$zonefile".signed >NSEC3
"$SIGNER" -z -O full -u3 AAAA -o "$zone" "$zonefile" >/dev/null
awk '$4 == "NSEC3" || ( $4 == "RRSIG" && $5 == "NSEC3" ) { print }' "$zonefile".signed >>NSEC3
"$SIGNER" -z -O full -u3 BBBB -o "$zone" "$zonefile" >/dev/null
awk '$4 == "NSEC3" || ( $4 == "RRSIG" && $5 == "NSEC3" ) { print }' "$zonefile".signed >>NSEC3
"$SIGNER" -z -O full -u3 CCCC -o "$zone" "$zonefile" >/dev/null
awk '$4 == "NSEC3" || ( $4 == "RRSIG" && $5 == "NSEC3" ) { print }' "$zonefile".signed >>NSEC3
"$SIGNER" -z -O full -u3 DDDD -o "$zone" "$zonefile" >/dev/null
cat NSEC NSEC3 >>"$zonefile".signed

#
# A RSASHA256 zone.
#
zone=rsasha256.example.
infile=rsasha256.example.db.in
zonefile=rsasha256.example.db

keyname=$("$KEYGEN" -q -a RSASHA256 -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -P -o "$zone" "$zonefile" >/dev/null

#
# A RSASHA512 zone.
#
zone=rsasha512.example.
infile=rsasha512.example.db.in
zonefile=rsasha512.example.db

keyname=$("$KEYGEN" -q -a RSASHA512 -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -P -o "$zone" "$zonefile" >/dev/null

#
# A zone with the DNSKEY set only signed by the KSK
#
zone=kskonly.example.
infile=kskonly.example.db.in
zonefile=kskonly.example.db

kskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK "$zone")
zskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
cat "$infile" "$kskname.key" "$zskname.key" >"$zonefile"
"$SIGNER" -x -o "$zone" "$zonefile" >/dev/null

#
# A zone with the expired signatures
#
zone=expired.example.
infile=expired.example.db.in
zonefile=expired.example.db

kskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -fk "$zone")
zskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
cat "$infile" "$kskname.key" "$zskname.key" >"$zonefile"
"$SIGNER" -P -o "$zone" -s -1d -e +1h "$zonefile" >/dev/null
rm -f "$kskname.*" "$zskname.*"

#
# A NSEC3 signed zone that will have a DNSKEY added to it via UPDATE.
#
zone=update-nsec3.example.
infile=update-nsec3.example.db.in
zonefile=update-nsec3.example.db

kskname=$("$KEYGEN" -q -3 -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -fk "$zone")
zskname=$("$KEYGEN" -q -3 -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
cat "$infile" "$kskname.key" "$zskname.key" >"$zonefile"
"$SIGNER" -P -3 - -o "$zone" "$zonefile" >/dev/null

#
# A NSEC signed zone that will have dnssec-policy enabled and
# extra keys not in the initial signed zone.
#
zone=auto-nsec.example.
infile=auto-nsec.example.db.in
zonefile=auto-nsec.example.db

kskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -fk "$zone")
zskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
"$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" -fk "$zone" >/dev/null
"$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" "$zone" >/dev/null
cat "$infile" "$kskname.key" "$zskname.key" >"$zonefile"
"$SIGNER" -P -o "$zone" "$zonefile" >/dev/null

#
# A NSEC3 signed zone that will have dnssec-policy enabled and
# extra keys not in the initial signed zone.
#
zone=auto-nsec3.example.
infile=auto-nsec3.example.db.in
zonefile=auto-nsec3.example.db

kskname=$("$KEYGEN" -q -3 -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -fk "$zone")
zskname=$("$KEYGEN" -q -3 -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
"$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" -fk "$zone" >/dev/null
"$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" "$zone" >/dev/null
cat "$infile" "$kskname.key" "$zskname.key" >"$zonefile"
"$SIGNER" -P -3 - -o "$zone" "$zonefile" >/dev/null

#
# Secure below cname test zone.
#
zone=secure.below-cname.example.
infile=secure.below-cname.example.db.in
zonefile=secure.below-cname.example.db
keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$infile" "$keyname.key" >"$zonefile"
"$SIGNER" -P -o "$zone" "$zonefile" >/dev/null

#
# Patched TTL test zone.
#
zone=ttlpatch.example.
infile=ttlpatch.example.db.in
zonefile=ttlpatch.example.db
signedfile=ttlpatch.example.db.signed
patchedfile=ttlpatch.example.db.patched

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -P -f $signedfile -o "$zone" "$zonefile" >/dev/null
$CHECKZONE -D -s full "$zone" $signedfile 2>/dev/null \
  | awk '{$2 = "3600"; print}' >$patchedfile

#
# Separate DNSSEC records.
#
zone=split-dnssec.example.
infile=split-dnssec.example.db.in
zonefile=split-dnssec.example.db
signedfile=split-dnssec.example.db.signed

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cat "$infile" "$keyname.key" >"$zonefile"
echo "\$INCLUDE \"$signedfile\"" >>"$zonefile"
: >"$signedfile"
"$SIGNER" -P -D -o "$zone" "$zonefile" >/dev/null

#
# Separate DNSSEC records smart signing.
#
zone=split-smart.example.
infile=split-smart.example.db.in
zonefile=split-smart.example.db
signedfile=split-smart.example.db.signed

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cp "$infile" "$zonefile"
# shellcheck disable=SC2016
echo "\$INCLUDE \"$signedfile\"" >>"$zonefile"
: >"$signedfile"
"$SIGNER" -P -S -D -o "$zone" "$zonefile" >/dev/null

#
# Zone with signatures about to expire, but no private key to replace them
#
zone="expiring.example."
infile="expiring.example.db.in"
zonefile="expiring.example.db"
signedfile="expiring.example.db.signed"
kskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
zskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK "$zone")
cp "$infile" "$zonefile"
"$SIGNER" -S -e now+1mi -o "$zone" "$zonefile" >/dev/null
mv -f "${zskname}.private" "${zskname}.private.moved"
mv -f "${kskname}.private" "${kskname}.private.moved"

#
# A zone where the signer's name has been forced to uppercase.
#
zone="upper.example."
infile="upper.example.db.in"
zonefile="upper.example.db"
lower="upper.example.db.lower"
signedfile="upper.example.db.signed"
kskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
zskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK "$zone")
cp "$infile" "$zonefile"
"$SIGNER" -P -S -o "$zone" -f $lower "$zonefile" >/dev/null
$CHECKZONE -D upper.example $lower 2>/dev/null \
  | sed '/RRSIG/s/ upper.example. / UPPER.EXAMPLE. /' >$signedfile

#
# Check that the signer's name is in lower case when zone name is in
# upper case.
#
zone="LOWER.EXAMPLE."
infile="lower.example.db.in"
zonefile="lower.example.db"
signedfile="lower.example.db.signed"
kskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
zskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK "$zone")
cp "$infile" "$zonefile"
"$SIGNER" -P -S -o "$zone" "$zonefile" >/dev/null

#
# An inline signing zone
#
zone=inline.example.
kskname=$("$KEYGEN" -q -3 -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -fk "$zone")
zskname=$("$KEYGEN" -q -3 -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")

#
# A zone which will change its signatures-validity
#
zone=siginterval.example
infile=siginterval.example.db.in
zonefile=siginterval.example.db
kskname=$("$KEYGEN" -q -3 -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -fk "$zone")
zskname=$("$KEYGEN" -q -3 -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
cp "$infile" "$zonefile"

#
# A zone with a bad DS in the parent
# (sourced from bogus.example.db.in)
#
zone=badds.example.
infile=bogus.example.db.in
zonefile=badds.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -P -o "$zone" "$zonefile" >/dev/null
sed -e 's/bogus/badds/g' <dsset-bogus.example. >dsset-badds.example.

#
# A zone with future signatures.
#
zone=future.example
infile=future.example.db.in
zonefile=future.example.db
kskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK "$zone")
zskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
cat "$infile" "$kskname.key" "$zskname.key" >"$zonefile"
"$SIGNER" -P -s +3600 -o "$zone" "$zonefile" >/dev/null
cp -f "$kskname.key" trusted-future.key

#
# A zone with future signatures.
#
zone=managed-future.example
infile=managed-future.example.db.in
zonefile=managed-future.example.db
kskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK "$zone")
zskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
cat "$infile" "$kskname.key" "$zskname.key" >"$zonefile"
"$SIGNER" -P -s +3600 -o "$zone" "$zonefile" >/dev/null

#
# A zone with a revoked key
#
zone=revkey.example.
infile=generic.example.db.in
zonefile=revkey.example.db

ksk1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -3fk "$zone")
ksk1=$("$REVOKE" "$ksk1")
ksk2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -3fk "$zone")
zsk1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -3 "$zone")

cat "$infile" "${ksk1}.key" "${ksk2}.key" "${zsk1}.key" >"$zonefile"
"$SIGNER" -P -o "$zone" "$zonefile" >/dev/null

#
# Check that NSEC3 are correctly signed and returned from below a DNAME
#
zone=dname-at-apex-nsec3.example
infile=dname-at-apex-nsec3.example.db.in
zonefile=dname-at-apex-nsec3.example.db

kskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -3fk "$zone")
zskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -3 "$zone")
cat "$infile" "${kskname}.key" "${zskname}.key" >"$zonefile"
"$SIGNER" -P -3 - -o "$zone" "$zonefile" >/dev/null

#
# A NSEC zone with occluded data at the delegation
#
zone=occluded.example
infile=occluded.example.db.in
zonefile=occluded.example.db
kskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -fk "$zone")
zskname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" "$zone")
dnskeyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -fk "delegation.$zone")
keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -n HOST -T KEY "delegation.$zone")
$DSFROMKEY "$dnskeyname.key" >"dsset-delegation.${zone}."
cat "$infile" "${kskname}.key" "${zskname}.key" "${keyname}.key" \
  "${dnskeyname}.key" "dsset-delegation.${zone}." >"$zonefile"
"$SIGNER" -P -o "$zone" "$zonefile" >/dev/null

#
# Pre-signed zone for FIPS validation of RSASHA1 signed zones
# See sign-rsasha1.sh for how to regenerate rsasha1.example.db
# with non-FIPS compliant instance.
#
# We only need to generate the dsset.
#
zone=rsasha1.example
zonefile=rsasha1.example.db
awk '$4 == "DNSKEY" && $5 == 257 { print }' "$zonefile" \
  | $DSFROMKEY -f - "$zone" >"dsset-${zone}."

zone=rsasha1-1024.example
zonefile=rsasha1-1024.example.db
awk '$4 == "DNSKEY" && $5 == 257 { print }' "$zonefile" \
  | $DSFROMKEY -f - "$zone" >"dsset-${zone}."

#
#
#
zone=extrabadkey.example.
infile=template.db.in
zonefile=extrabadkey.example.db

# Add KSK and ZSK that we will mangle to RSAMD5
ksk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK "$zone")
zsk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
cat "$infile" "$ksk.key" "$zsk.key" >"$zonefile"
"$SIGNER" -g -O full -o "$zone" "$zonefile" >/dev/null 2>&1

# Mangle the signatures to RSAMD5 and save them for future use
sed -ne "s/\(IN[[:space:]]*RRSIG[[:space:]]*[A-Z]*\) $DEFAULT_ALGORITHM_NUMBER /\1 1 /p" <"$zonefile.signed" >"$zonefile.signed.rsamd5"

# Now add normal KSK and ZSK to the zone file
ksk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK "$zone")
zsk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
cat "$infile" "$ksk.key" "$zsk.key" >"$zonefile"

# Mangle the DNSKEY algorithm numbers and add them to the signed zone file
cat "$ksk.key" "$zsk.key" | sed -e "s/\(IN[[:space:]]*DNSKEY[[:space:]]*[0-9]* 3\) $DEFAULT_ALGORITHM_NUMBER /\1 1 /" >>"$zonefile"

# Sign normally
"$SIGNER" -g -o "$zone" "$zonefile" >/dev/null 2>&1

# Add the mangled signatures to signed zone file
cat "$zonefile.signed.rsamd5" >>"$zonefile.signed"
rm "$zonefile.signed.rsamd5"
