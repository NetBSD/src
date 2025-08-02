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

. ../../conf.sh

# Fake an unsupported key
unsupportedkey=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone unsupported)
awk '$3 == "DNSKEY" { $6 = 255 } { print }' ${unsupportedkey}.key >${unsupportedkey}.tmp
mv ${unsupportedkey}.tmp ${unsupportedkey}.key

zone=bits
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >>../ns1/root.db

zone=noixfr
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >>../ns1/root.db

zone=primary
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >>../ns1/root.db

zone=dynamic
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >>../ns1/root.db

zone=updated
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
zsk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -L 3600 -n zone $zone)
ksk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -L 3600 -n zone -f KSK $zone)
$SETTIME -s -g OMNIPRESENT -k RUMOURED now -z RUMOURED now "$zsk" >settime.out.updated.1 2>&1
$SETTIME -s -g OMNIPRESENT -k RUMOURED now -r RUMOURED now -d HIDDEN now "$ksk" >settime.out.updated.2 2>&1
$DSFROMKEY -T 1200 $ksk >>../ns1/root.db
$SIGNER -S -x -O raw -L 2000042407 -o ${zone} ${zone}.db >/dev/null
cp primary2.db.in updated.db

# signatures are expired and should be regenerated on startup
zone=expired
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >>../ns1/root.db
$SIGNER -PS -s 20100101000000 -e 20110101000000 -O raw -L 2000042407 -o ${zone} ${zone}.db >/dev/null

zone=retransfer
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >>../ns1/root.db

zone=nsec3
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
$DSFROMKEY -T 1200 $keyname >>../ns1/root.db

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

for s in a c d h k l m q z; do
  zone=test-$s
  keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
done

for s in b f i o p t v; do
  zone=test-$s
  keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
  keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
done

zone=externalkey
zonefile=${zone}.db
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private

for alg in ${DEFAULT_ALGORITHM} ${ALTERNATIVE_ALGORITHM}; do
  k1=$($KEYGEN -q -a $alg -n zone -f KSK $zone)
  k2=$($KEYGEN -q -a $alg -n zone $zone)
  k3=$($KEYGEN -q -a $alg -n zone $zone)
  k4=$($KEYGEN -q -a $alg -n zone -f KSK $zone)
  $DSFROMKEY -T 1200 $k4 >>../ns1/root.db

  cat $k1.key $k2.key >>$zonefile

  rm -f $k1.key
  rm -f $k1.private
  rm -f $k2.key
  rm -f $k2.private
done
