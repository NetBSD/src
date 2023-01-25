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

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

# Fake an unsupported key
unsupportedkey=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone unsupported)
awk '$3 == "DNSKEY" { $6 = 255 } { print }' ${unsupportedkey}.key > ${unsupportedkey}.tmp
mv ${unsupportedkey}.tmp ${unsupportedkey}.key

zone=bits
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >> ../ns1/root.db

zone=noixfr
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >> ../ns1/root.db

zone=master
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >> ../ns1/root.db

zone=dynamic
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >> ../ns1/root.db

zone=updated
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >> ../ns1/root.db
$SIGNER -S -O raw -L 2000042407 -o ${zone} ${zone}.db > /dev/null
cp master2.db.in updated.db

# signatures are expired and should be regenerated on startup
zone=expired
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >> ../ns1/root.db
$SIGNER -PS -s 20100101000000 -e 20110101000000 -O raw -L 2000042407 -o ${zone} ${zone}.db > /dev/null

zone=retransfer
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >> ../ns1/root.db

zone=nsec3
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >> ../ns1/root.db

zone=retransfer3
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >> ../ns1/root.db

zone=inactiveksk
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -P now -A now+3600 -f KSK $zone)
keyname=$($KEYGEN -q -a ${ALTERNATIVE_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${ALTERNATIVE_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >> ../ns1/root.db

zone=inactivezsk
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -P now -A now+3600 $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
keyname=$($KEYGEN -q -a ${ALTERNATIVE_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${ALTERNATIVE_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >> ../ns1/root.db

zone=delayedkeys
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
# Keys for the "delayedkeys" zone should not be initially accessible.
mv K${zone}.+*+*.* ../

zone=removedkeys-primary
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)

zone=removedkeys-secondary
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)

for s in a c d h k l m q z
do
	zone=test-$s
	keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
done

for s in b f i o p t v
do
	zone=test-$s
	keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
	keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
done

zone=externalkey
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private

for alg in ${DEFAULT_ALGORITHM} ${ALTERNATIVE_ALGORITHM}
do
    k1=$($KEYGEN -q -a $alg -n zone -f KSK $zone)
    k2=$($KEYGEN -q -a $alg -n zone $zone)
    k3=$($KEYGEN -q -a $alg -n zone $zone)
    k4=$($KEYGEN -q -a $alg -n zone -f KSK $zone)
    $DSFROMKEY -T 1200 $k4 >> ../ns1/root.db

    # Convert k1 and k2 in to External Keys.
    rm -f $k1.private 
    mv $k1.key a-file
    $IMPORTKEY -P now -D now+3600 -f a-file $zone > /dev/null 2>&1 ||
        ( echo_i "importkey failed: $alg" )
    rm -f $k2.private 
    mv $k2.key a-file
    $IMPORTKEY -f a-file $zone > /dev/null 2>&1 ||
        ( echo_i "importkey failed: $alg" )
done
