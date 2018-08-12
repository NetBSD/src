#!/bin/sh
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

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd +dnssec -p ${PORT}"
DELVOPTS="-a ns1/trusted.conf -p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

wait_for_log() {
	msg=$1
	file=$2
	for i in 1 2 3 4 5 6 7 8 9 10; do
		nextpart "$file" | grep "$msg" > /dev/null && return
		sleep 1
	done
	echo_i "exceeded time limit waiting for '$msg' in $file"
	ret=1
}

mkeys_reconfig_on() {
	nsidx=$1
	$RNDCCMD 10.53.0.${nsidx} reconfig . | sed "s/^/ns${nsidx} /" | cat_i
}

mkeys_reload_on() {
	nsidx=$1
	nextpart ns${nsidx}/named.run > /dev/null
	$RNDCCMD 10.53.0.${nsidx} reload . | sed "s/^/ns${nsidx} /" | cat_i
	wait_for_log "loaded serial" ns${nsidx}/named.run
}

mkeys_loadkeys_on() {
	nsidx=$1
	nextpart ns${nsidx}/named.run > /dev/null
	$RNDCCMD 10.53.0.${nsidx} loadkeys . | sed "s/^/ns${nsidx} /" | cat_i
	wait_for_log "next key event" ns${nsidx}/named.run
}

mkeys_refresh_on() {
	nsidx=$1
	nextpart ns${nsidx}/named.run > /dev/null
	$RNDCCMD 10.53.0.${nsidx} managed-keys refresh | sed "s/^/ns${nsidx} /" | cat_i
	wait_for_log "Returned from key fetch in keyfetch_done()" ns${nsidx}/named.run
}

mkeys_sync_on() {
	# No race with mkeys_refresh_on() is possible as even if the latter
	# returns immediately after the expected log message is written, the
	# managed-keys zone is already locked and the command below calls
	# dns_zone_flush(), which also attempts to take that zone's lock
	nsidx=$1
	nextpart ns${nsidx}/named.run > /dev/null
	$RNDCCMD 10.53.0.${nsidx} managed-keys sync | sed "s/^/ns${nsidx} /" | cat_i
	wait_for_log "dump_done" ns${nsidx}/named.run
}

mkeys_status_on() {
	# No race with mkeys_refresh_on() is possible as even if the latter
	# returns immediately after the expected log message is written, the
	# managed-keys zone is already locked and the command below calls
	# mkey_status(), which in turn calls dns_zone_getrefreshkeytime(),
	# which also attempts to take that zone's lock
	nsidx=$1
	$RNDCCMD 10.53.0.${nsidx} managed-keys status
}

mkeys_flush_on() {
	nsidx=$1
	$RNDCCMD 10.53.0.${nsidx} flush | sed "s/^/ns${nsidx} /" | cat_i
}

mkeys_secroots_on() {
	nsidx=$1
	$RNDCCMD 10.53.0.${nsidx} secroots | sed "s/^/ns${nsidx} /" | cat_i
}

original=`cat ns1/managed.key`
originalid=`cat ns1/managed.key.id`

status=0
n=1

rm -f dig.out.*

echo_i "check for signed record ($n)"
ret=0
$DIG $DIGOPTS +norec example.  @10.53.0.1 TXT > dig.out.ns1.test$n || ret=1
grep "^example\.[ 	]*[0-9].*[ 	]*IN[ 	]*TXT[ 	]*\"This is a test\.\"" dig.out.ns1.test$n > /dev/null || ret=1
grep "^example\.[ 	]*[0-9].*[ 	]*IN[ 	]*RRSIG[ 	]*TXT[ 	]" dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check positive validation with valid trust anchor ($n)"
ret=0
$DIG $DIGOPTS +noauth example. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null || ret=1
grep "example..*.RRSIG..*TXT" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
ret=0
echo_i "check positive validation using delv ($n)"
$DELV $DELVOPTS @10.53.0.1 txt example > delv.out$n || ret=1
grep "; fully validated" delv.out$n > /dev/null || ret=1	# redundant
grep "example..*TXT.*This is a test" delv.out$n > /dev/null || ret=1
grep "example..*.RRSIG..*TXT" delv.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check for failed validation due to wrong key in managed-keys ($n)"
ret=0
$DIG $DIGOPTS +noauth example. @10.53.0.3 txt > dig.out.ns3.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns3.test$n > /dev/null && ret=1
grep "example..*.RRSIG..*TXT" dig.out.ns3.test$n > /dev/null && ret=1
grep "opcode: QUERY, status: SERVFAIL, id" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check new trust anchor can be added ($n)"
ret=0
standby1=`$KEYGEN -a rsasha256 -qfk -r $RANDFILE -K ns1 .`
mkeys_loadkeys_on 1
mkeys_refresh_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
# there should be two keys listed now
count=`grep -c "keyid: " rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# two lines indicating trust status
count=`grep -c "trust" rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# one indicates current trust
count=`grep -c "trusted since" rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
# one indicates pending trust
count=`grep -c "trust pending" rndc.out.$n`
[ "$count" -eq 1 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check new trust anchor can't be added with bad initial key ($n)"
ret=0
mkeys_refresh_on 3
mkeys_status_on 3 > rndc.out.$n 2>&1
# there should be one key listed now
count=`grep -c "keyid: " rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
# one line indicating trust status
count=`grep -c "trust" rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
# ... and the key is not trusted
count=`grep -c "no trust" rndc.out.$n`
[ "$count" -eq 1 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "remove untrusted standby key, check timer restarts ($n)"
ret=0
mkeys_sync_on 2
t1=`grep "trust pending" ns2/managed-keys.bind`
$SETTIME -D now -K ns1 $standby1 > /dev/null
mkeys_loadkeys_on 1
# Less than a second may have passed since the last time ns2 received a
# ./DNSKEY response from ns1.  Ensure keys are refreshed at a different
# timestamp to prevent false negatives caused by the acceptance timer getting
# reset to the same timestamp.
sleep 1
mkeys_refresh_on 2
mkeys_sync_on 2
t2=`grep "trust pending" ns2/managed-keys.bind`
# trust pending date must be different
[ -n "$t2" ] || ret=1
[ "$t1" = "$t2" ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
ret=0
echo_i "restore untrusted standby key, revoke original key ($n)"
t1=$t2
$SETTIME -D none -K ns1 $standby1 > /dev/null
$SETTIME -R now -K ns1 $original > /dev/null
mkeys_loadkeys_on 1
# Less than a second may have passed since the last time ns2 received a
# ./DNSKEY response from ns1.  Ensure keys are refreshed at a different
# timestamp to prevent false negatives caused by the acceptance timer getting
# reset to the same timestamp.
sleep 1
mkeys_refresh_on 2
mkeys_sync_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
# two keys listed
count=`grep -c "keyid: " rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# two lines indicating trust status
count=`grep -c "trust" rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# trust is revoked
count=`grep -c "trust revoked" rndc.out.$n`
[ "$count" -eq 1 ] || ret=1
# removal scheduled
count=`grep -c "remove at" rndc.out.$n`
[ "$count" -eq 1 ] || ret=1
# trust is still pending on the standby key
count=`grep -c "trust pending" rndc.out.$n`
[ "$count" -eq 1 ] || ret=1
# pending date moved forward for the standby key
t2=`grep "trust pending" ns2/managed-keys.bind`
[ -n "$t2" ] || ret=1
[ "$t1" = "$t2" ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
ret=0
echo_i "refresh managed-keys, ensure same result ($n)"
t1=$t2
# Less than a second may have passed since the last time ns2 received a
# ./DNSKEY response from ns1.  Ensure keys are refreshed at a different
# timestamp to prevent false negatives caused by the acceptance timer getting
# reset to the same timestamp.
sleep 1
mkeys_refresh_on 2
mkeys_sync_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
# two keys listed
count=`grep -c "keyid: " rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# two lines indicating trust status
count=`grep -c "trust" rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# trust is revoked
count=`grep -c "trust revoked" rndc.out.$n`
[ "$count" -eq 1 ] || ret=1
# removal scheduled
count=`grep -c "remove at" rndc.out.$n`
[ "$count" -eq 1 ] || ret=1
# trust is still pending on the standby key
count=`grep -c "trust pending" rndc.out.$n`
[ "$count" -eq 1 ] || ret=1
# pending date moved forward for the standby key
t2=`grep "trust pending" ns2/managed-keys.bind`
[ -n "$t2" ] || ret=1
[ "$t1" = "$t2" ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
ret=0
echo_i "restore revoked key, ensure same result ($n)"
t1=$t2
$SETTIME -R none -D now -K ns1 $original > /dev/null
mkeys_loadkeys_on 1
$SETTIME -D none -K ns1 $original > /dev/null
mkeys_loadkeys_on 1
# Less than a second may have passed since the last time ns2 received a
# ./DNSKEY response from ns1.  Ensure keys are refreshed at a different
# timestamp to prevent false negatives caused by the acceptance timer getting
# reset to the same timestamp.
sleep 1
mkeys_refresh_on 2
mkeys_sync_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
# two keys listed
count=`grep -c "keyid: " rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# two lines indicating trust status
count=`grep -c "trust" rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# trust is revoked
count=`grep -c "trust revoked" rndc.out.$n`
[ "$count" -eq 1 ] || ret=1
# removal scheduled
count=`grep -c "remove at" rndc.out.$n`
[ "$count" -eq 1 ] || ret=1
# trust is still pending on the standby key
count=`grep -c "trust pending" rndc.out.$n`
[ "$count" -eq 1 ] || ret=1
# pending date moved forward for the standby key
t2=`grep "trust pending" ns2/managed-keys.bind`
[ -n "$t2" ] || ret=1
[ "$t1" = "$t2" ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "reinitialize trust anchors, add second key to bind.keys"
$PERL $SYSTEMTESTTOP/stop.pl --use-rndc --port ${CONTROLPORT} . ns2
rm -f ns2/managed-keys.bind*
keyfile_to_managed_keys ns1/$original ns1/$standby1 > ns2/managed.conf
nextpart ns2/named.run > /dev/null
$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} . ns2

n=`expr $n + 1`
echo_i "check that no key from bind.keys is marked as an initializing key ($n)"
ret=0
wait_for_log "Returned from key fetch in keyfetch_done()" ns2/named.run
mkeys_secroots_on 2
grep '; initializing' ns2/named.secroots > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "reinitialize trust anchors, revert to one key in bind.keys"
$PERL $SYSTEMTESTTOP/stop.pl --use-rndc --port ${CONTROLPORT} . ns2
rm -f ns2/managed-keys.bind*
mv ns2/managed1.conf ns2/managed.conf
nextpart ns2/named.run > /dev/null
$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} . ns2

n=`expr $n + 1`
echo_i "check that standby key is now trusted ($n)"
ret=0
wait_for_log "Returned from key fetch in keyfetch_done()" ns2/named.run
mkeys_status_on 2 > rndc.out.$n 2>&1
# two keys listed
count=`grep -c "keyid: " rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# two lines indicating trust status
count=`grep -c "trust" rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# both indicate current trust
count=`grep -c "trusted since" rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "revoke original key, add new standby ($n)"
ret=0
standby2=`$KEYGEN -a rsasha256 -qfk -r $RANDFILE -K ns1 .`
$SETTIME -R now -K ns1 $original > /dev/null
mkeys_loadkeys_on 1
mkeys_refresh_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
# three keys listed
count=`grep -c "keyid: " rndc.out.$n` 
[ "$count" -eq 3 ] || ret=1
# one is revoked
count=`grep -c "REVOKE" rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
# three lines indicating trust status
count=`grep -c "trust" rndc.out.$n` 
[ "$count" -eq 3 ] || ret=1
# one indicates current trust
count=`grep -c "trusted since" rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
# one indicates revoked trust
count=`grep -c "trust revoked" rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
# one indicates trust pending
count=`grep -c "trust pending" rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
# removal scheduled
count=`grep -c "remove at" rndc.out.$n`
[ "$count" -eq 1 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "revoke standby before it is trusted ($n)"
ret=0
standby3=`$KEYGEN -a rsasha256 -qfk -r $RANDFILE -K ns1 .`
mkeys_loadkeys_on 1
mkeys_refresh_on 2
mkeys_status_on 2 > rndc.out.a.$n 2>&1
# four keys listed
count=`grep -c "keyid: " rndc.out.a.$n` 
[ "$count" -eq 4 ] || { echo "keyid: count ($count) != 4"; ret=1; }
# one revoked
count=`grep -c "trust revoked" rndc.out.a.$n` 
[ "$count" -eq 1 ] || { echo "trust revoked count ($count) != 1"; ret=1; }
# two pending
count=`grep -c "trust pending" rndc.out.a.$n` 
[ "$count" -eq 2 ] || { echo "trust pending count ($count) != 2"; ret=1; }
$SETTIME -R now -K ns1 $standby3 > /dev/null
mkeys_loadkeys_on 1
mkeys_refresh_on 2
mkeys_status_on 2 > rndc.out.b.$n 2>&1
# now three keys listed
count=`grep -c "keyid: " rndc.out.b.$n` 
[ "$count" -eq 3 ] || { echo "keyid: count ($count) != 3"; ret=1; }
# one revoked
count=`grep -c "trust revoked" rndc.out.b.$n` 
[ "$count" -eq 1 ] || { echo "trust revoked count ($count) != 1"; ret=1; }
# one pending
count=`grep -c "trust pending" rndc.out.b.$n` 
[ "$count" -eq 1 ] || { echo "trust pending count ($count) != 1"; ret=1; }
$SETTIME -D now -K ns1 $standby3 > /dev/null
mkeys_loadkeys_on 1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "wait 20 seconds for key add/remove holddowns to expire ($n)"
ret=0
sleep 20
mkeys_refresh_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
# two keys listed
count=`grep -c "keyid: " rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# none revoked
count=`grep -c "REVOKE" rndc.out.$n` 
[ "$count" -eq 0 ] || ret=1
# two lines indicating trust status
count=`grep -c "trust" rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# both indicate current trust
count=`grep -c "trusted since" rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "revoke all keys, confirm roll to insecure ($n)"
ret=0
$SETTIME -D now -K ns1 $original > /dev/null
$SETTIME -R now -K ns1 $standby1 > /dev/null
$SETTIME -R now -K ns1 $standby2 > /dev/null
mkeys_loadkeys_on 1
mkeys_refresh_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
# two keys listed
count=`grep -c "keyid: " rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# both revoked
count=`grep -c "REVOKE" rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# two lines indicating trust status
count=`grep -c "trust" rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# both indicate trust revoked
count=`grep -c "trust revoked" rndc.out.$n` 
[ "$count" -eq 2 ] || ret=1
# both have removal scheduled
count=`grep -c "remove at" rndc.out.$n`
[ "$count" -eq 2 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check for insecure response ($n)"
ret=0
mkeys_refresh_on 2
$DIG $DIGOPTS +noauth example. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null && ret=1
grep "example..*.RRSIG..*TXT" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "reset the root server"
$SETTIME -D none -R none -K ns1 $original > /dev/null
$SETTIME -D now -K ns1 $standby1 > /dev/null
$SETTIME -D now -K ns1 $standby2 > /dev/null
$SIGNER -Sg -K ns1 -N unixtime -r $RANDFILE -o . ns1/root.db > /dev/null 2>/dev/null
copy_setports ns1/named2.conf.in ns1/named.conf
rm -f ns1/root.db.signed.jnl
mkeys_reconfig_on 1

echo_i "reinitialize trust anchors"
$PERL $SYSTEMTESTTOP/stop.pl --use-rndc --port ${CONTROLPORT} . ns2
rm -f ns2/managed-keys.bind*
nextpart ns2/named.run > /dev/null
$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} . ns2

n=`expr $n + 1`
echo_i "check positive validation ($n)"
ret=0
wait_for_log "Returned from key fetch in keyfetch_done()" ns2/named.run
$DIG $DIGOPTS +noauth example. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null || ret=1
grep "example..*.RRSIG..*TXT" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "revoke key with bad signature, check revocation is ignored ($n)"
ret=0
revoked=`$REVOKE -K ns1 $original`
rkeyid=`expr $revoked : 'ns1/K\.+00.+0*\([1-9]*[0-9]*[0-9]\)'`
rm -f ns1/root.db.signed.jnl
# We need to activate at least one valid DNSKEY to prevent dnssec-signzone from
# failing.  Alternatively, we could use -P to disable post-sign verification,
# but we actually do want post-sign verification to happen to ensure the zone
# is correct before we break it on purpose.
$SETTIME -R none -D none -K ns1 $standby1 > /dev/null
$SIGNER -Sg -K ns1 -N unixtime -r $RANDFILE -O full -o . -f signer.out.$n ns1/root.db > /dev/null 2>/dev/null
cp -f ns1/root.db.signed ns1/root.db.tmp
BADSIG="SVn2tLDzpNX2rxR4xRceiCsiTqcWNKh7NQ0EQfCrVzp9WEmLw60sQ5kP xGk4FS/xSKfh89hO2O/H20Bzp0lMdtr2tKy8IMdU/mBZxQf2PXhUWRkg V2buVBKugTiOPTJSnaqYCN3rSfV1o7NtC1VNHKKK/D5g6bpDehdn5Gaq kpBhN+MSCCh9OZP2IT20luS1ARXxLlvuSVXJ3JYuuhTsQXUbX/SQpNoB Lo6ahCE55szJnmAxZEbb2KOVnSlZRA6ZBHDhdtO0S4OkvcmTutvcVV+7 w53CbKdaXhirvHIh0mZXmYk2PbPLDY7PU9wSH40UiWPOB9f00wwn6hUe uEQ1Qg=="
# Less than a second may have passed since ns1 was started.  If we call
# dnssec-signzone immediately, ns1/root.db.signed will not be reloaded by the
# subsequent "rndc reload ." call on platforms which do not set the
# "nanoseconds" field of isc_time_t, due to zone load time being seemingly
# equal to master file modification time.
sleep 1
sed -e "/ $rkeyid \./s, \. .*$, . $BADSIG," signer.out.$n > ns1/root.db.signed
mkeys_reload_on 1
mkeys_refresh_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
# one key listed
count=`grep -c "keyid: " rndc.out.$n` 
[ "$count" -eq 1 ] || { echo "'keyid:' count ($count) != 1"; ret=1; }
# it's the original key id
count=`grep -c "keyid: $originalid" rndc.out.$n` 
[ "$count" -eq 1 ] || { echo "'keyid: $originalid' count ($count) != 1"; ret=1; }
# not revoked
count=`grep -c "REVOKE" rndc.out.$n` 
[ "$count" -eq 0 ] || { echo "'REVOKE' count ($count) != 0"; ret=1; }
# trust is still current
count=`grep -c "trust" rndc.out.$n` 
[ "$count" -eq 1 ] || { echo "'trust' count != 1"; ret=1; }
count=`grep -c "trusted since" rndc.out.$n` 
[ "$count" -eq 1 ] || { echo "'trusted since' count != 1"; ret=1; }
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check validation fails with bad DNSKEY rrset ($n)"
ret=0
mkeys_flush_on 2
$DIG $DIGOPTS +noauth example. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
grep "status: SERVFAIL" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "restore DNSKEY rrset, check validation succeeds again ($n)"
ret=0
rm -f ${revoked}.key ${revoked}.private
rm -f ns1/root.db.signed.jnl
$SETTIME -D none -R none -K ns1 $original > /dev/null
$SETTIME -D now -K ns1 $standby1 > /dev/null
# Less than a second may have passed since ns1 was started.  If we call
# dnssec-signzone immediately, ns1/root.db.signed will not be reloaded by the
# subsequent "rndc reload ." call on platforms which do not set the
# "nanoseconds" field of isc_time_t, due to zone load time being seemingly
# equal to master file modification time.
sleep 1
$SIGNER -Sg -K ns1 -N unixtime -r $RANDFILE -o . ns1/root.db > /dev/null 2>/dev/null
mkeys_reload_on 1
mkeys_flush_on 2
$DIG $DIGOPTS +noauth example. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null || ret=1
grep "example..*.RRSIG..*TXT" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "reset the root server with no keys, check for minimal update ($n)"
ret=0
# Refresh keys first to prevent previous checks from influencing this one.
# Note that we might still get occasional false negatives on some really slow
# machines, when $t1 equals $t2 due to the time elapsed between "rndc
# managed-keys status" calls being equal to the normal active refresh period
# (as calculated per rules listed in RFC 5011 section 2.3) minus an "hour" (as
# set using -T mkeytimers).
mkeys_refresh_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
t1=`grep 'next refresh:' rndc.out.$n`
$PERL $SYSTEMTESTTOP/stop.pl --use-rndc --port ${CONTROLPORT} . ns1
rm -f ns1/root.db.signed.jnl
cp ns1/root.db ns1/root.db.signed
nextpart ns1/named.run > /dev/null
$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} . ns1
wait_for_log "loaded serial" ns1/named.run
mkeys_refresh_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
# one key listed
count=`grep -c "keyid: " rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
# it's the original key id
count=`grep -c "keyid: $originalid" rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
# not revoked
count=`grep -c "REVOKE" rndc.out.$n` 
[ "$count" -eq 0 ] || ret=1
# trust is still current
count=`grep -c "trust" rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
count=`grep -c "trusted since" rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
t2=`grep 'next refresh:' rndc.out.$n`
[ "$t1" = "$t2" ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "reset the root server with no signatures, check for minimal update ($n)"
ret=0
# Refresh keys first to prevent previous checks from influencing this one
mkeys_refresh_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
t1=`grep 'next refresh:' rndc.out.$n`
$PERL $SYSTEMTESTTOP/stop.pl --use-rndc --port ${CONTROLPORT} . ns1
rm -f ns1/root.db.signed.jnl
cat ns1/K*.key >> ns1/root.db.signed
nextpart ns1/named.run > /dev/null
$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} . ns1
wait_for_log "loaded serial" ns1/named.run
# Less than a second may have passed since the last time ns2 received a
# ./DNSKEY response from ns1.  Ensure keys are refreshed at a different
# timestamp to prevent minimal update from resetting it to the same timestamp.
sleep 1
mkeys_refresh_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
# one key listed
count=`grep -c "keyid: " rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
# it's the original key id
count=`grep -c "keyid: $originalid" rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
# not revoked
count=`grep -c "REVOKE" rndc.out.$n` 
[ "$count" -eq 0 ] || ret=1
# trust is still current
count=`grep -c "trust" rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
count=`grep -c "trusted since" rndc.out.$n` 
[ "$count" -eq 1 ] || ret=1
t2=`grep 'next refresh:' rndc.out.$n`
[ "$t1" = "$t2" ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "restore root server, check validation succeeds again ($n)"
ret=0
rm -f ns1/root.db.signed.jnl
$SIGNER -Sg -K ns1 -N unixtime -r $RANDFILE -o . ns1/root.db > /dev/null 2>/dev/null
mkeys_reload_on 1
mkeys_refresh_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
$DIG $DIGOPTS +noauth example. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null || ret=1
grep "example..*.RRSIG..*TXT" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that trust-anchor-telemetry queries are logged ($n)"
ret=0
grep "sending trust-anchor-telemetry query '_ta-[0-9a-f]*/NULL" ns2/named.run > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that trust-anchor-telemetry queries are received ($n)"
ret=0
grep "query '_ta-[0-9a-f][0-9a-f]*/NULL/IN' approved" ns1/named.run > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check 'rndc-managed-keys destroy' ($n)"
ret=0
$RNDCCMD 10.53.0.2 managed-keys destroy | sed 's/^/ns2 /' | cat_i
mkeys_status_on 2 > rndc.out.$n 2>&1
grep "no views with managed keys" rndc.out.$n > /dev/null || ret=1
mkeys_reconfig_on 2
mkeys_status_on 2 > rndc.out.$n 2>&1
grep "name: \." rndc.out.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that trust-anchor-telemetry queries contain the correct key ($n)"
ret=0
# convert the hexadecimal key from the TAT query into decimal and
# compare against the known key.
tathex=`grep "query '_ta-[0-9a-f][0-9a-f]*/NULL/IN' approved" ns1/named.run | awk '{print $6; exit 0}' | sed -e 's/(_ta-\([0-9a-f][0-9a-f]*\)):/\1/'`
tatkey=`$PERL -e 'printf("%d\n", hex(@ARGV[0]));' $tathex`
realkey=`$RNDCCMD 10.53.0.2 secroots - | sed -n 's#.*SHA256/\([0-9][0-9]*\) ; .*managed.*#\1#p'`
[ "$tatkey" -eq "$realkey" ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check initialization fails if managed-keys can't be created ($n)"
ret=0
mkeys_secroots_on 4
grep '; initializing managed' ns4/named.secroots > /dev/null 2>&1 || ret=1
grep '; managed' ns4/named.secroots > /dev/null 2>&1 && ret=1
grep '; trusted' ns4/named.secroots > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check failure to contact root servers does not prevent key refreshes after restart ($n)"
ret=0
# By the time we get here, ns5 should have attempted refreshing its managed
# keys.  These attempts should fail as ns1 is configured to REFUSE all queries
# from ns5.  Note that named1.args does not contain "-T mkeytimers"; this is to
# ensure key refresh retry will be scheduled to one actual hour after the first
# key refresh failure instead of just a few seconds, in order to prevent races
# between the next scheduled key refresh time and startup time of restarted ns5.
$PERL $SYSTEMTESTTOP/stop.pl --use-rndc --port ${CONTROLPORT} . ns5
nextpart ns5/named.run > /dev/null
$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} . ns5
wait_for_log "Returned from key fetch in keyfetch_done()" ns5/named.run
# ns5/named.run will contain logs from both the old instance and the new
# instance.  In order for the test to pass, both must attempt a fetch.
count=`grep -c "Creating key fetch" ns5/named.run`
[ $count -lt 2 ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check key refreshes are resumed after root servers become available ($n)"
ret=0
$PERL $SYSTEMTESTTOP/stop.pl --use-rndc --port ${CONTROLPORT} . ns5
# Prevent previous check from affecting this one
rm -f ns5/managed-keys.bind*
# named2.args adds "-T mkeytimers=2/20/40" to named1.args as we need to wait for
# an "hour" until keys are refreshed again after initial failure
cp ns5/named2.args ns5/named.args
nextpart ns5/named.run > /dev/null
$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} . ns5
wait_for_log "Returned from key fetch in keyfetch_done() for '.': failure" ns5/named.run
mkeys_secroots_on 5
grep '; initializing managed' ns5/named.secroots > /dev/null 2>&1 || ret=1
# ns1 should still REFUSE queries from ns5, so resolving should be impossible
$DIG $DIGOPTS +noauth example. @10.53.0.5 txt > dig.out.ns5.a.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.a.test$n > /dev/null && ret=1
grep "example..*.RRSIG..*TXT" dig.out.ns5.a.test$n > /dev/null && ret=1
grep "status: SERVFAIL" dig.out.ns5.a.test$n > /dev/null || ret=1
# Allow queries from ns5 to ns1
copy_setports ns1/named3.conf.in ns1/named.conf
rm -f ns1/root.db.signed.jnl
nextpart ns5/named.run > /dev/null
mkeys_reconfig_on 1
wait_for_log "Returned from key fetch in keyfetch_done() for '.': success" ns5/named.run
mkeys_secroots_on 5
grep '; managed' ns5/named.secroots > /dev/null 2>&1 || ret=1
# ns1 should not longer REFUSE queries from ns5, so managed keys should be
# correctly refreshed and resolving should succeed
$DIG $DIGOPTS +noauth example. @10.53.0.5 txt > dig.out.ns5.b.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.b.test$n > /dev/null || ret=1
grep "example..*.RRSIG..*TXT" dig.out.ns5.b.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns5.b.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
