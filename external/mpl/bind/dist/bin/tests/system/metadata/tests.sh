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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

pzone=parent.nil pfile=parent.db
czone=child.parent.nil cfile=child.db
status=0
n=1

echo_i "setting key timers"
$SETTIME -A now+15s $(cat rolling.key) > /dev/null

inact=$(keyfile_to_key_id "$(cat inact.key)")
ksk=$(keyfile_to_key_id "$(cat ksk.key)")
pending=$(keyfile_to_key_id "$(cat pending.key)")
postrev=$(keyfile_to_key_id "$(cat postrev.key)")
prerev=$(keyfile_to_key_id "$(cat prerev.key)")
rolling=$(keyfile_to_key_id "$(cat rolling.key)")
standby=$(keyfile_to_key_id "$(cat standby.key)")
zsk=$(keyfile_to_key_id "$(cat zsk.key)")

echo_i "signing zones"
$SIGNER -Sg -o $czone $cfile > /dev/null
$SIGNER -Sg -o $pzone $pfile > /dev/null

awk '$2 ~ /RRSIG/ {
        type = $3;
        getline;
	id = $3;
	if ($4 ~ /'${czone}'/) {
		print type, id
	}
}' < ${cfile}.signed > sigs

awk '$2 ~ /DNSKEY/ {
	flags = $3;
	while ($0 !~ /key id =/)
		getline;
	id = $NF;
	print flags, id;
}' < ${cfile}.signed > keys

echo_i "checking that KSK signed DNSKEY only ($n)"
ret=0
grep "DNSKEY $ksk"'$' sigs > /dev/null || ret=1
grep "SOA $ksk"'$' sigs > /dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that ZSK signed ($n)"
ret=0
grep "SOA $zsk"'$' sigs > /dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that standby ZSK did not sign ($n)"
ret=0
grep " $standby"'$' sigs > /dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that inactive key did not sign ($n)"
ret=0
grep " $inact"'$' sigs > /dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that pending key was not published ($n)"
ret=0
grep " $pending"'$' keys > /dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that standby KSK did not sign but is delegated ($n)"
ret=0
grep " $rolling"'$' sigs > /dev/null && ret=1
grep " $rolling"'$' keys > /dev/null || ret=1
grep -E "DS[ 	]*$rolling[ 	]" ${pfile}.signed > /dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that key was revoked ($n)"
ret=0
grep " $prerev"'$' keys > /dev/null && ret=1
grep " $postrev"'$' keys > /dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that revoked key self-signed ($n)"
ret=0
grep "DNSKEY $postrev"'$' sigs > /dev/null || ret=1
grep "SOA $postrev"'$' sigs > /dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "waiting 20 seconds for key changes to occur"
sleep 20

echo_i "re-signing zone"
$SIGNER  -Sg -o $czone -f ${cfile}.new ${cfile}.signed > /dev/null

echo_i "checking that standby KSK is now active ($n)"
ret=0
grep "DNSKEY $rolling"'$' sigs > /dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking update of an old-style key ($n)"
ret=0
# printing metadata should not work with an old-style key
$SETTIME -pall $(cat oldstyle.key) > /dev/null 2>&1 && ret=1
$SETTIME -f $(cat oldstyle.key) > /dev/null 2>&1 || ret=1
# but now it should
$SETTIME -pall $(cat oldstyle.key) > /dev/null 2>&1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking warning about permissions change on key with dnssec-settime ($n)"
uname=$(uname -o 2> /dev/null)
if [ Cygwin = "$uname"  ]; then
	echo_i "Cygwin detected, skipping"
else
	ret=0
	# settime should print a warning about changing the permissions
	chmod 644 $(cat oldstyle.key).private
	$SETTIME -P none $(cat oldstyle.key) > settime1.test$n 2>&1 || ret=1
	grep "warning: Permissions on the file.*have changed" settime1.test$n > /dev/null 2>&1 || ret=1
	$SETTIME -P none $(cat oldstyle.key) > settime2.test$n 2>&1 || ret=1
	grep "warning: Permissions on the file.*have changed" settime2.test$n > /dev/null 2>&1 && ret=1
	n=$((n + 1))
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=$((status + ret))
fi

echo_i "checking warning about delete date < inactive date with dnssec-settime ($n)"
ret=0
# settime should print a warning about delete < inactive
$SETTIME -I now+15s -D now $(cat oldstyle.key) > tmp.out 2>&1 || ret=1
grep "warning" tmp.out > /dev/null 2>&1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking no warning about delete date < inactive date with dnssec-settime when delete date is unset ($n)"
ret=0
$SETTIME -D none $(cat oldstyle.key) > tmp.out 2>&1 || ret=1
$SETTIME -p all $(cat oldstyle.key) > tmp.out 2>&1 || ret=1
grep "warning" tmp.out > /dev/null 2>&1 && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking warning about delete date < inactive date with dnssec-keygen ($n)"
ret=0
# keygen should print a warning about delete < inactive
$KEYGEN -q -a ${DEFAULT_ALGORITHM} -I now+15s -D now $czone > tmp.out 2>&1 || ret=1
grep "warning" tmp.out > /dev/null 2>&1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking correct behavior setting activation without publication date ($n)"
ret=0
key=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -A +1w $czone)
pub=$($SETTIME -upP $key | awk '{print $2}')
act=$($SETTIME -upA $key | awk '{print $2}')
[ $pub -eq $act ] || ret=1
key=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -A +1w -i 1d $czone)
pub=$($SETTIME -upP $key | awk '{print $2}')
act=$($SETTIME -upA $key | awk '{print $2}')
[ $pub -lt $act ] || ret=1
key=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -A +1w -P never $czone)
pub=$($SETTIME -upP $key | awk '{print $2}')
[ $pub = "UNSET" ] || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking calculation of dates for a successor key ($n)"
ret=0
oldkey=$($KEYGEN -a ${DEFAULT_ALGORITHM} -q $czone)
newkey=$($KEYGEN -a ${DEFAULT_ALGORITHM} -q $czone)
$SETTIME -A -2d -I +2d $oldkey > settime1.test$n 2>&1 || ret=1
$SETTIME -i 1d -S $oldkey $newkey > settime2.test$n 2>&1 || ret=1
$SETTIME -pA $newkey | grep "1970" > /dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
