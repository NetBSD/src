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

set -e

SYSTEMTESTTOP=..
# shellcheck source=conf.sh
. "$SYSTEMTESTTOP/conf.sh"

count_rrsigs() (
    grep -c "IN[[:space:]]*RRSIG" "$@" || true
)

dig_with_opts() (
    $DIG +tcp +noadd +nosea +nostat +nocmd +dnssec -p "$PORT" "$@"
)

dig_for_rr() (
    alg=$1
    rrtype=$2
    count0=$3
    dig_with_opts "$alg.example." @10.53.0.1 "$rrtype" > "dig.out.$rrtype.$alg" &&
    count=$(count_rrsigs "dig.out.$rrtype.$alg") &&
    test "$count" -gt "$count0"
)

test_done() {
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))
    ret=0
}

status=0
ret=0

n=0
while read -r alg; do
    zonefile=ns1/$alg.example.db
    echo_i "testing PKCS#11 key generation ($alg)"
    count=$($PK11LIST | grep -c "pkcs11-$alg-ksk" || true)
    [ "$count" -eq 4 ] || ret=1
    test_done

    echo_i "testing offline signing with PKCS#11 keys ($alg)"

    count=$(grep -c "[0-9][[:space:]]*RRSIG" "$zonefile.signed")
    [ "$count" -eq 9 ] || ret=1
    test_done

    echo_i "testing inline signing with new PKCS#11 ZSK ($alg)"

    dig_with_opts "$alg.example." @10.53.0.1 "SOA" > "dig.out.SOA.$alg.0" || ret=1
    countSOA0=$(count_rrsigs "dig.out.SOA.$alg.0")
    new_zsk=$(grep -v ';' "ns1/$alg.zsk")

    cat > "upd.cmd.ZSK.$alg" <<EOF
server 10.53.0.1 $PORT
ttl 300
zone $alg.example.
update add $new_zsk
send
EOF

    $NSUPDATE -v > "upd.log.ZSK.$alg" < "upd.cmd.ZSK.$alg" || ret=1

    retry_quiet 20 dig_for_rr "$alg" "SOA" "$countSOA0" || ret=1
    test_done

    echo_i "testing inline signing with new PKCS#11 KSK ($alg)"

    dig_with_opts "$alg.example." @10.53.0.1 "DNSKEY" > "dig.out.DNSKEY.$alg.0" || ret=1
    countDNSKEY0=$(count_rrsigs "dig.out.DNSKEY.$alg.0")
    new_ksk=$(grep -v ';' "ns1/$alg.ksk")

    cat > "upd.cmd.KSK.$alg" <<EOF
server 10.53.0.1 $PORT
ttl 300
zone $alg.example.
update add $new_ksk
send
EOF

    $NSUPDATE -v > "upd.log.KSK.$alg" < "upd.cmd.KSK.$alg" || ret=1

    retry_quiet 20 dig_for_rr "$alg" "DNSKEY" "$countDNSKEY0" || ret=1
    test_done

    echo_i "testing PKCS#11 key destroy ($alg)"

    # Lookup all existing keys
    echo_i "looking up all existing keys ($alg)"
    $PK11LIST > "pkcs11-list.out.id.$alg" || ret=1
    test_done

    echo_i "destroying key with 'pkcs11-$alg-ksk1' label ($alg)"
    $PK11DEL -l "pkcs11-$alg-ksk1" > /dev/null 2>&1 || ret=1
    test_done

    echo_i "destroying key with 'pkcs11-$alg-zsk1' label ($alg)"
    $PK11DEL -l "pkcs11-$alg-zsk1" > /dev/null 2>&1 || ret=1
    test_done

    id=$(awk -v label="'pkcs11-$alg-ksk2'" '{ if ($7 == label) { print $9; exit; } }' < "pkcs11-list.out.id.$alg")
    echo_i "destroying key with $id id ($alg)"
    if [ -n "$id" ]; then
	$PK11DEL -i "$id" > /dev/null 2>&1 || ret=1
    else
	ret=1
    fi
    test_done

    id=$(awk -v label="'pkcs11-$alg-zsk2'" '{ if ($7 == label) { print $9; exit; } }' < "pkcs11-list.out.id.$alg")
    echo_i "destroying key with $id id ($alg)"
    if [ -n "$id" ]; then
	$PK11DEL -i "$id" > /dev/null 2>&1 || ret=1
    else
	ret=1
    fi
    test_done

    echo_i "checking if all keys have been destroyed ($alg)"
    $PK11LIST > "pkcs11-list.out.$alg" || ret=1
    count=$(grep -c "pkcs11-$alg-[kz]sk[0-9]*" "pkcs11-list.out.$alg" || true)
    [ "$count" -eq 0 ] || ret=1
    test_done
    n=$((n+1))
done < supported

echo_i "Checking if all supported algorithms were tested"
[ "$n" -eq "$(wc -l < supported)" ] || ret=1
test_done

echo_i "Checking for assertion failure in pk11_numbits()"
$PERL ../packet.pl -a "10.53.0.1" -p "$PORT" -t udp 2037-pk11_numbits-crash-test.pkt
dig_with_opts @10.53.0.1 version.bind. CH TXT > dig.out.pk11_numbits || ret=1
test_done

echo_i "exit status: $status"
[ "$status" -eq 0 ] || exit 1
