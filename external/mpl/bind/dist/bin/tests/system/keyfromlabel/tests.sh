#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

set -e

# shellcheck source=conf.sh
. ../conf.sh

PWD=$(pwd)

keygen() {
  type="$1"
  bits="$2"
  zone="$3"
  id="$4"

  label="${id}-${zone}"
  p11id=$(echo "${label}" | openssl sha1 -r | awk '{print $1}')
  pkcs11-tool --module $SOFTHSM2_MODULE --token-label "softhsm2-keyfromlabel" -l -k --key-type $type:$bits --label "${label}" --id "${p11id}" --pin $(cat $PWD/pin) >pkcs11-tool.out.$zone.$id || return 1
}

keyfromlabel() {
  alg="$1"
  zone="$2"
  id="$3"
  shift 3

  $KEYFRLAB -E pkcs11 -a $alg -l "token=softhsm2-keyfromlabel;object=${id}-${zone};pin-source=$PWD/pin" "$@" $zone >>keyfromlabel.out.$zone.$id 2>>/dev/null || return 1
  cat keyfromlabel.out.$zone.$id
}

infile="template.db.in"
for algtypebits in rsasha256:rsa:2048 rsasha512:rsa:2048 \
  ecdsap256sha256:EC:prime256v1 ecdsap384sha384:EC:prime384v1; do # Edwards curves are not yet supported by OpenSC
  # ed25519:EC:edwards25519 ed448:EC:edwards448
  alg=$(echo "$algtypebits" | cut -f 1 -d :)
  type=$(echo "$algtypebits" | cut -f 2 -d :)
  bits=$(echo "$algtypebits" | cut -f 3 -d :)

  if $SHELL ../testcrypto.sh $alg; then
    zone="$alg.example"
    zonefile="zone.$alg.example.db"
    ret=0

    echo_i "Generate keys $alg $type:$bits for zone $zone"
    keygen $type $bits $zone keyfromlabel-zsk || ret=1
    keygen $type $bits $zone keyfromlabel-ksk || ret=1
    test "$ret" -eq 0 || echo_i "failed"
    status=$((status + ret))

    # Skip dnssec-keyfromlabel if key generation failed.
    test $ret -eq 0 || continue

    echo_i "Get ZSK $alg $zone $type:$bits"
    ret=0
    zsk=$(keyfromlabel $alg $zone keyfromlabel-zsk)
    test -z "$zsk" && ret=1
    test "$ret" -eq 0 || echo_i "failed (zsk=$zsk)"
    status=$((status + ret))

    echo_i "Get KSK $alg $zone $type:$bits"
    ret=0
    ksk=$(keyfromlabel $alg $zone keyfromlabel-ksk -f KSK)
    test -z "$ksk" && ret=1
    test "$ret" -eq 0 || echo_i "failed (ksk=$ksk)"
    status=$((status + ret))

    # Skip signing if dnssec-keyfromlabel failed.
    test $ret -eq 0 || continue

    echo_i "Sign zone with $ksk $zsk"
    ret=0
    cat "$infile" "$ksk.key" "$zsk.key" >"$zonefile"
    $SIGNER -E pkcs11 -S -a -g -o "$zone" "$zonefile" >signer.out.$zone || ret=1
    test "$ret" -eq 0 || echo_i "failed"
    status=$((status + ret))
  fi
done

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
