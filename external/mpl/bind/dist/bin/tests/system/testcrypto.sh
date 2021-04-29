#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=${SYSTEMTESTTOP:=..}
. $SYSTEMTESTTOP/conf.sh

prog=$0

args=""
alg="-a $DEFAULT_ALGORITHM -b $DEFAULT_BITS"
quiet=0

msg="cryptography"
while test "$#" -gt 0; do
        case $1 in
        -q)
                args="$args -q"
                quiet=1
                ;;
        rsa|RSA)
                alg="-a RSASHA1"
                msg="RSA cryptography"
                ;;
	rsasha256|RSASHA256)
                alg="-a RSASHA256"
                msg="RSA cryptography"
                ;;
	rsasha512|RSASHA512)
                alg="-a RSASHA512"
                msg="RSA cryptography"
                ;;
        ecdsa|ECDSA|ecdsap256sha256|ECDSAP256SHA256)
                alg="-a ECDSAP256SHA256"
                msg="ECDSA cryptography"
                ;;
        ecdsap384sha384|ECDSAP384SHA384)
                alg="-a ECDSAP384SHA384"
                msg="ECDSA cryptography"
                ;;
        eddsa|EDDSA|ed25519|ED25519)
                alg="-a ED25519"
                msg="EDDSA cryptography"
                ;;
        ed448|ED448)
                alg="-a ED448"
                msg="EDDSA cryptography"
                ;;
        *)
                echo "${prog}: unknown argument"
                exit 1
                ;;
        esac
        shift
done

if $KEYGEN $args $alg foo > /dev/null 2>&1
then
    rm -f Kfoo*
else
    if test $quiet -eq 0; then
        echo_i "This test requires support for $msg" >&2
        echo_i "configure with --with-openssl, or --enable-native-pkcs11" \
            "--with-pkcs11" >&2
    fi
    exit 255
fi
