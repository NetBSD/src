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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

$SHELL ./clean.sh

test -r $RANDFILE || $GENRANDOM 800 $RANDFILE

pzone=parent.nil
czone=child.parent.nil

echo_i "generating keys"

# active zsk
zsk=`$KEYGEN -q -a rsasha1 -r $RANDFILE $czone`
echo $zsk > zsk.key

# not yet published or active
pending=`$KEYGEN -q -a rsasha1 -r $RANDFILE -P none -A none $czone`
echo $pending > pending.key

# published but not active
standby=`$KEYGEN -q -a rsasha1 -r $RANDFILE -A none $czone`
echo $standby > standby.key

# inactive
inact=`$KEYGEN -q -a rsasha1 -r $RANDFILE -P now-24h -A now-24h -I now $czone`
echo $inact > inact.key

# active ksk
ksk=`$KEYGEN -q -a rsasha1 -r $RANDFILE -fk $czone`
echo $ksk > ksk.key

# published but not YET active; will be active in 15 seconds
rolling=`$KEYGEN -q -a rsasha1 -r $RANDFILE -fk $czone`
$SETTIME -A now+15s $rolling > /dev/null
echo $rolling > rolling.key

# revoked
revoke1=`$KEYGEN -q -a rsasha1 -r $RANDFILE -fk $czone`
echo $revoke1 > prerev.key
revoke2=`$REVOKE $revoke1`
echo $revoke2 | sed -e 's#\./##' -e "s/\.key.*$//" > postrev.key

pzsk=`$KEYGEN -q -a rsasha1 -r $RANDFILE $pzone`
echo $pzsk > parent.zsk.key

pksk=`$KEYGEN -q -a rsasha1 -r $RANDFILE -fk $pzone`
echo $pksk > parent.ksk.key

oldstyle=`$KEYGEN -Cq -a rsasha1 -r $RANDFILE $pzone`
echo $oldstyle > oldstyle.key

