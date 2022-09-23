#!/bin/sh

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

set -eu

SYSTEMTESTTOP=..
# shellcheck source=conf.sh
. "$SYSTEMTESTTOP/conf.sh"

echo_i "Generating keys for Native PKCS#11" >&2

infile=ns1/example.db.in

printf '%s' "${HSMPIN:-1234}" > pin
PWD=$(pwd)

copy_setports ns1/named.conf.in ns1/named.conf

get_random() {
    dd if=/dev/urandom bs=1 count=2 2>/dev/null | od -tu2 -An
}

genpkcs() (
    alg="$1"
    bits="$2"
    label="$3"
    id="$(get_random)"

    $PK11DEL -l "$label" -w0 >/dev/null || true
    $PK11GEN -a "$alg" -b "$bits" -l "$label" -i "$id" >/dev/null
)

keyfrlab() (
    alg="$1"
    bits="$2"
    label="$3"
    zone="$4"
    shift 4

    $KEYFRLAB -a "$alg" -l "pkcs11:object=$label;pin-source=$PWD/pin" "$@" "$zone"
)

genzsk() (
    genpkcs "$@"
    keyfrlab "$@"
)

genksk() (
    genpkcs "$@"
    keyfrlab "$@" -f ksk
)

algs=
for algbits in rsasha256:2048 rsasha512:2048 ecdsap256sha256:256 ecdsap384sha384:384 ed25519:256 ed448:456; do
    alg=$(echo "$algbits" | cut -f 1 -d :)
    bits=$(echo "$algbits" | cut -f 2 -d :)
    zone="$alg.example"
    zonefile="ns1/$alg.example.db"
    if $SHELL "$SYSTEMTESTTOP/testcrypto.sh" "$alg"; then
	echo "$alg" >> supported
	algs="$algs$alg "

	zsk1=$(genzsk "$alg" "$bits" "pkcs11-$alg-zsk1" "$zone")
	zsk2=$(genzsk "$alg" "$bits" "pkcs11-$alg-zsk2" "$zone")
	ksk1=$(genksk "$alg" "$bits" "pkcs11-$alg-ksk1" "$zone")
	ksk2=$(genksk "$alg" "$bits" "pkcs11-$alg-ksk2" "$zone")

	cat "$infile" "$zsk1.key" "$ksk1.key" > "$zonefile"
	$SIGNER -a -P -g -o "$zone" "$zonefile" > /dev/null
	cp "$zsk2.key" "ns1/$alg.zsk"
	cp "$ksk2.key" "ns1/$alg.ksk"
	mv "K$alg"* ns1/

	cat >> ns1/named.conf <<EOF
zone "$alg.example." {
	type primary;
	file "$alg.example.db.signed";
	allow-update { any; };
};

EOF
    fi
done
echo_i "Generated keys for Native PKCS#11: $algs"
