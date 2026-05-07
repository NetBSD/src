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

# shellcheck source=conf.sh
. ../conf.sh

set -e

OPENSSL_CONF= softhsm2-util --delete-token --token "softhsm2-enginepkcs11" >/dev/null 2>&1 || true
OPENSSL_CONF= softhsm2-util --init-token --free --pin 1234 --so-pin 1234 --label "softhsm2-enginepkcs11" | awk '/^The token has been initialized and is reassigned to slot/ { print $NF }'

printf '%s' "${HSMPIN:-1234}" >ns1/pin
PWD=$(pwd)

keygen() {
  type="$1"
  bits="$2"
  zone="$3"
  id="$4"

  label="${id}-${zone}"
  p11id=$(echo "${label}" | openssl sha1 -r | awk '{print $1}')
  OPENSSL_CONF= pkcs11-tool --module $SOFTHSM2_MODULE --token-label "softhsm2-enginepkcs11" -l -k --key-type $type:$bits --label "${label}" --id "${p11id}" --pin $(cat $PWD/ns1/pin) >pkcs11-tool.out.$zone.$id 2>pkcs11-tool.err.$zone.$id || return 1
}

keyfromlabel() {
  alg="$1"
  zone="$2"
  id="$3"
  dir="$4"
  shift 4

  $KEYFRLAB $ENGINE_ARG -K $dir -a $alg -y -l "pkcs11:token=softhsm2-enginepkcs11;object=${id}-${zone};pin-source=$PWD/ns1/pin" "$@" $zone >>keyfromlabel.out.$zone.$id 2>keyfromlabel.err.$zone.$id || return 1
  cat keyfromlabel.out.$zone.$id
}

mkdir ns1/keys

dir="ns1"
infile="${dir}/template.db.in"
for algtypebits in rsasha256:rsa:2048 rsasha512:rsa:2048 \
  ecdsap256sha256:EC:prime256v1 ecdsap384sha384:EC:prime384v1; do # Edwards curves are not yet supported by OpenSC
  # ed25519:EC:edwards25519 ed448:EC:edwards448
  alg=$(echo "$algtypebits" | cut -f 1 -d :)
  type=$(echo "$algtypebits" | cut -f 2 -d :)
  bits=$(echo "$algtypebits" | cut -f 3 -d :)
  alg_upper=$(echo "$alg" | tr '[:lower:]' '[:upper:]')
  supported=$(eval "echo \$${alg_upper}_SUPPORTED")

  tld="example"
  if [ "${supported}" = 1 ]; then
    zone="$alg.$tld"
    zonefile="zone.$alg.$tld.db"
    ret=0

    echo_i "Generate keys $alg $type:$bits for zone $zone"
    keygen $type $bits $zone enginepkcs11-zsk || ret=1
    keygen $type $bits $zone enginepkcs11-ksk || ret=1
    test "$ret" -eq 0 || exit 1

    echo_i "Get ZSK $alg $zone $type:$bits"
    zsk1=$(keyfromlabel $alg $zone enginepkcs11-zsk $dir)
    test -z "$zsk1" && exit 1

    echo_i "Get KSK $alg $zone $type:$bits"
    ksk1=$(keyfromlabel $alg $zone enginepkcs11-ksk $dir -f KSK)
    test -z "$ksk1" && exit 1

    (
      cd $dir
      zskid1=$(keyfile_to_key_id $zsk1)
      kskid1=$(keyfile_to_key_id $ksk1)
      echo "$zskid1" >$zone.zskid1
      echo "$kskid1" >$zone.kskid1
    )

    echo_i "Sign zone with $ksk1 $zsk1"
    cat "$infile" "${dir}/${ksk1}.key" "${dir}/${zsk1}.key" >"${dir}/${zonefile}"
    $SIGNER $ENGINE_ARG -K $dir -S -a -g -O full -o "$zone" "${dir}/${zonefile}" >signer.out.$zone || ret=1
    test "$ret" -eq 0 || exit 1

    echo_i "Generate successor keys $alg $type:$bits for zone $zone"
    keygen $type $bits $zone enginepkcs11-zsk2 || ret=1
    keygen $type $bits $zone enginepkcs11-ksk2 || ret=1
    test "$ret" -eq 0 || exit 1

    echo_i "Get ZSK $alg $id-$zone $type:$bits"
    zsk2=$(keyfromlabel $alg $zone enginepkcs11-zsk2 $dir)
    test -z "$zsk2" && exit 1

    echo_i "Get KSK $alg $id-$zone $type:$bits"
    ksk2=$(keyfromlabel $alg $zone enginepkcs11-ksk2 $dir -f KSK)
    test -z "$ksk2" && exit 1

    (
      cd $dir
      zskid2=$(keyfile_to_key_id $zsk2)
      kskid2=$(keyfile_to_key_id $ksk2)
      echo "$zskid2" >$zone.zskid2
      echo "$kskid2" >$zone.kskid2
      cp "${zsk2}.key" "${zsk2}.zsk2"
      cp "${ksk2}.key" "${ksk2}.ksk2"
    )

    echo_i "Add zone $alg.kasp to named.conf"
    cp $infile ${dir}/zone.${alg}.kasp.db

    echo_i "Add zone $alg.split to named.conf"
    cp $infile ${dir}/zone.${alg}.split.db

    echo_i "Add weird zone to named.conf"
    cp $infile ${dir}/zone.${alg}.weird.db

    echo_i "Add zone $zone to named.conf"
    cat >>"${dir}/named.conf" <<EOF
zone "$zone" {
	type primary;
	file "${zonefile}.signed";
	allow-update { any; };
};

dnssec-policy "$alg" {
	keys {
		ksk key-store "hsm" lifetime unlimited algorithm ${alg};
		zsk key-store "pin" lifetime unlimited algorithm ${alg};
	};
};

zone "${alg}.kasp" {
	type primary;
	file "zone.${alg}.kasp.db";
	dnssec-policy "$alg";
	allow-update { any; };
};

dnssec-policy "weird-${alg}-\"\:\;\?\&\[\]\@\!\$\*\+\,\|\=\.\(\)" {
	keys {
		ksk key-store "hsm" lifetime unlimited algorithm ${alg};
		zsk key-store "pin" lifetime unlimited algorithm ${alg};
	};
};

zone "${alg}.\"\:\;\?\&\[\]\@\!\$\*\+\,\|\=\.\(\)foo.weird" {
	type primary;
	file "zone.${alg}.weird.db";
	check-names ignore;
	dnssec-policy "weird-${alg}-\"\:\;\?\&\[\]\@\!\$\*\+\,\|\=\.\(\)";
	allow-update { any; };
};

dnssec-policy "${alg}-split" {
	keys {
		ksk key-store "hsm" lifetime unlimited algorithm ${alg};
		zsk key-store "disk" lifetime unlimited algorithm ${alg};
	};
};

zone "${alg}.split" {
	type primary;
	file "zone.${alg}.split.db";
	dnssec-policy "${alg}-split";
	allow-update { any; };
};

EOF
  fi
done

mkdir ns2/keys

dir="ns2"
infile="${dir}/template.db.in"
algtypebits="ecdsap256sha256:EC:prime256v1"
alg=$(echo "$algtypebits" | cut -f 1 -d :)
type=$(echo "$algtypebits" | cut -f 2 -d :)
bits=$(echo "$algtypebits" | cut -f 3 -d :)
alg_upper=$(echo "$alg" | tr '[:lower:]' '[:upper:]')
supported=$(eval "echo \$${alg_upper}_SUPPORTED")
tld="views"

if [ "${supported}" = 1 ]; then
  zone="$alg.$tld"
  zonefile1="zone.$alg.$tld.view1.db"
  zonefile2="zone.$alg.$tld.view2.db"
  ret=0

  echo_i "Generate keys $alg $type:$bits for zone $zone"
  keygen $type $bits $zone enginepkcs11-zsk || ret=1
  keygen $type $bits $zone enginepkcs11-ksk || ret=1
  test "$ret" -eq 0 || exit 1

  echo_i "Get ZSK $alg $zone $type:$bits"
  zsk1=$(keyfromlabel $alg $zone enginepkcs11-zsk $dir)
  test -z "$zsk1" && exit 1

  echo_i "Get KSK $alg $zone $type:$bits"
  ksk1=$(keyfromlabel $alg $zone enginepkcs11-ksk $dir -f KSK)
  test -z "$ksk1" && exit 1

  (
    cd $dir
    zskid1=$(keyfile_to_key_id $zsk1)
    kskid1=$(keyfile_to_key_id $ksk1)
    echo "$zskid1" >$zone.zskid1
    echo "$kskid1" >$zone.kskid1
  )

  echo_i "Sign zone with $ksk1 $zsk1"
  cat "$infile" "${dir}/${ksk1}.key" "${dir}/${zsk1}.key" >"${dir}/${zonefile1}"
  $SIGNER $ENGINE_ARG -K $dir -S -a -g -O full -o "$zone" "${dir}/${zonefile1}" >signer.out.view1.$zone || ret=1
  test "$ret" -eq 0 || exit 1

  cat "$infile" "${dir}/${ksk1}.key" "${dir}/${zsk1}.key" >"${dir}/${zonefile2}"
  $SIGNER $ENGINE_ARG -K $dir -S -a -g -O full -o "$zone" "${dir}/${zonefile2}" >signer.out.view2.$zone || ret=1
  test "$ret" -eq 0 || exit 1

  echo_i "Generate successor keys $alg $type:$bits for zone $zone"
  keygen $type $bits $zone enginepkcs11-zsk2 || ret=1
  keygen $type $bits $zone enginepkcs11-ksk2 || ret=1
  test "$ret" -eq 0 || exit 1

  echo_i "Get ZSK $alg $id-$zone $type:$bits"
  zsk2=$(keyfromlabel $alg $zone enginepkcs11-zsk2 $dir)
  test -z "$zsk2" && exit 1

  echo_i "Get KSK $alg $id-$zone $type:$bits"
  ksk2=$(keyfromlabel $alg $zone enginepkcs11-ksk2 $dir -f KSK)
  test -z "$ksk2" && exit 1

  (
    cd $dir
    zskid2=$(keyfile_to_key_id $zsk2)
    kskid2=$(keyfile_to_key_id $ksk2)
    echo "$zskid2" >$zone.zskid2
    echo "$kskid2" >$zone.kskid2
    cp "${zsk2}.key" "${zsk2}.zsk2"
    cp "${ksk2}.key" "${ksk2}.ksk2"
  )

  echo_i "Add zone $alg.same-policy.$tld to named.conf"
  cp $infile ${dir}/zone.${alg}.same-policy.view1.db
  cp $infile ${dir}/zone.${alg}.same-policy.view2.db

  echo_i "Add zone zone-with.different-policy.$tld to named.conf"
  cp $infile ${dir}/zone.zone-with.different-policy.view1.db
  cp $infile ${dir}/zone.zone-with.different-policy.view2.db

  echo_i "Add zone $zone to named.conf"
  cat >>"${dir}/named.conf" <<EOF
dnssec-policy "$alg" {
	keys {
		csk key-store "hsm" lifetime unlimited algorithm ${alg};
	};
};

dnssec-policy "rsasha256" {
	keys {
		csk key-store "hsm2" lifetime unlimited algorithm rsasha256 2048;
	};
};

view "view1" {
	match-clients { key "keyforview1"; };

	zone "$zone" {
		type primary;
		file "${zonefile1}.signed";
		allow-update { any; };
	};

	zone "${alg}.same-policy.${tld}" {
		type primary;
		file "zone.${alg}.same-policy.view1.db";
		dnssec-policy "$alg";
		allow-update { any; };
	};

	zone "zone-with.different-policy.${tld}" {
		type primary;
		file "zone.zone-with.different-policy.view1.db";
		dnssec-policy "$alg";
		allow-update { any; };
	};
};

view "view2" {
	match-clients { key "keyforview2"; };

	zone "$zone" {
		type primary;
		file "${zonefile2}.signed";
		allow-update { any; };
	};

	zone "${alg}.same-policy.${tld}" {
		type primary;
		file "zone.${alg}.same-policy.view2.db";
		dnssec-policy "$alg";
		allow-update { any; };
	};

	zone "zone-with.different-policy.${tld}" {
		type primary;
		file "zone.zone-with.different-policy.view2.db";
		dnssec-policy "rsasha256";
		allow-update { any; };
	};
};

EOF
fi
