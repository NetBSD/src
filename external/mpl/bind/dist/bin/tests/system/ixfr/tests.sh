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

# WARNING: The test labelled "testing request-ixfr option in view vs zone"
#          is fragile because it depends upon counting instances of records
#          in the log file - need a better approach <sdm> - until then,
#          if you add any tests above that point, you will break the test.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

wait_for_serial() (
    $DIG $DIGOPTS "@$1" "$2" SOA > "$4"
    serial=$(awk '$4 == "SOA" { print $7 }' "$4")
    [ "$3" -eq "${serial:--1}" ]
)

status=0
n=0

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"
SENDCMD="$PERL ../send.pl 10.53.0.2 ${EXTRAPORT1}"
RNDCCMD="$RNDC -p ${CONTROLPORT} -c ../common/rndc.conf -s"

n=$((n+1))
echo_i "testing initial AXFR ($n)"
ret=0

$SENDCMD <<EOF
/SOA/
nil.      	300	SOA	ns.nil. root.nil. 1 300 300 604800 300
/AXFR/
nil.      	300	SOA	ns.nil. root.nil. 1 300 300 604800 300
/AXFR/
nil.      	300	NS	ns.nil.
nil.		300	TXT	"initial AXFR"
a.nil.		60	A	10.0.0.61
b.nil.		60	A	10.0.0.62
/AXFR/
nil.      	300	SOA	ns.nil. root.nil. 1 300 300 604800 300
EOF

sleep 1

# Initially, ns1 is not authoritative for anything (see setup.sh).
# Now that ans is up and running with the right data, we make it
# a secondary for nil.

cat <<EOF >>ns1/named.conf
zone "nil" {
	type secondary;
	file "myftp.db";
	primaries { 10.53.0.2; };
};
EOF

rndc_reload ns1 10.53.0.1

retry_quiet 10 wait_for_serial 10.53.0.1 nil. 1 dig.out.test$n || ret=1

$DIG $DIGOPTS @10.53.0.1 nil. TXT | grep 'initial AXFR' >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "testing successful IXFR ($n)"
ret=0

# We change the IP address of a.nil., and the TXT record at the apex.
# Then we do a SOA-only update.

$SENDCMD <<EOF
/SOA/
nil.      	300	SOA	ns.nil. root.nil. 3 300 300 604800 300
/IXFR/
nil.      	300	SOA	ns.nil. root.nil. 3 300 300 604800 300
nil.      	300	SOA	ns.nil. root.nil. 1 300 300 604800 300
a.nil.      	60	A	10.0.0.61
nil.		300	TXT	"initial AXFR"
nil.      	300	SOA	ns.nil. root.nil. 2 300 300 604800 300
nil.		300	TXT	"successful IXFR"
a.nil.      	60	A	10.0.1.61
nil.      	300	SOA	ns.nil. root.nil. 2 300 300 604800 300
nil.      	300	SOA	ns.nil. root.nil. 3 300 300 604800 300
nil.      	300	SOA	ns.nil. root.nil. 3 300 300 604800 300
EOF

sleep 1

$RNDCCMD 10.53.0.1 refresh nil | sed 's/^/ns1 /' | cat_i

sleep 2

$DIG $DIGOPTS @10.53.0.1 nil. TXT | grep 'successful IXFR' >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "testing AXFR fallback after IXFR failure (not exact error) ($n)"
ret=0

# Provide a broken IXFR response and a working fallback AXFR response

$SENDCMD <<EOF
/SOA/
nil.      	300	SOA	ns.nil. root.nil. 4 300 300 604800 300
/IXFR/
nil.      	300	SOA	ns.nil. root.nil. 4 300 300 604800 300
nil.      	300	SOA	ns.nil. root.nil. 3 300 300 604800 300
nil.      	300	TXT	"delete-nonexistent-txt-record"
nil.      	300	SOA	ns.nil. root.nil. 4 300 300 604800 300
nil.      	300	TXT	"this-txt-record-would-be-added"
nil.      	300	SOA	ns.nil. root.nil. 4 300 300 604800 300
/AXFR/
nil.      	300	SOA	ns.nil. root.nil. 3 300 300 604800 300
/AXFR/
nil.      	300	NS	ns.nil.
nil.      	300	TXT	"fallback AXFR"
/AXFR/
nil.      	300	SOA	ns.nil. root.nil. 3 300 300 604800 300
EOF

sleep 1

$RNDCCMD 10.53.0.1 refresh nil | sed 's/^/ns1 /' | cat_i

sleep 2

$DIG $DIGOPTS @10.53.0.1 nil. TXT | grep 'fallback AXFR' >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "testing AXFR fallback after IXFR failure (bad SOA owner) ($n)"
ret=0

# Prepare for checking the logs later on.
nextpart ns1/named.run >/dev/null

# Provide a broken IXFR response and a working fallback AXFR response.
$SENDCMD <<EOF
/SOA/
nil.      	300	SOA	ns.nil. root.nil. 4 300 300 604800 300
/IXFR/
nil.      	300	SOA	ns.nil. root.nil. 4 300 300 604800 300
nil.      	300	SOA	ns.nil. root.nil. 3 300 300 604800 300
bad-owner.    	300	SOA	ns.nil. root.nil. 4 300 300 604800 300
test.nil.	300	TXT	"serial 4, malformed IXFR"
nil.      	300	SOA	ns.nil. root.nil. 4 300 300 604800 300
/AXFR/
nil.      	300	SOA	ns.nil. root.nil. 4 300 300 604800 300
/AXFR/
nil.      	300	NS	ns.nil.
test.nil.      	300	TXT	"serial 4, fallback AXFR"
/AXFR/
nil.      	300	SOA	ns.nil. root.nil. 4 300 300 604800 300
EOF
$RNDCCMD 10.53.0.1 refresh nil | sed 's/^/ns1 /' | cat_i

# A broken server would accept the malformed IXFR and apply its contents to the
# zone.  A fixed one would reject the IXFR and fall back to AXFR.  Both IXFR and
# AXFR above bring the nil. zone up to serial 4, but we cannot reliably query
# for the SOA record to check whether the transfer was finished because a broken
# server would send back SERVFAIL responses to SOA queries after accepting the
# malformed IXFR.  Instead, check transfer progress by querying for a TXT record
# at test.nil. which is present in both IXFR and AXFR (with different contents).
_wait_until_transfer_is_finished() {
	$DIG $DIGOPTS +tries=1 +time=1 @10.53.0.1 test.nil. TXT > dig.out.test$n.1 &&
	grep -q -F "serial 4" dig.out.test$n.1
}
if ! retry_quiet 10 _wait_until_transfer_is_finished; then
	echo_i "timed out waiting for version 4 of zone nil. to be transferred"
	ret=1
fi

# At this point a broken server would be serving a zone with no SOA records.
# Try crashing it by triggering a SOA refresh query.
$RNDCCMD 10.53.0.1 refresh nil | sed 's/^/ns1 /' | cat_i

# Do not wait until the zone refresh completes - even if a crash has not
# happened by now, a broken server would never serve the record which is only
# present in the fallback AXFR, so checking for that is enough to verify if a
# server is broken or not; if it is, it is bound to crash shortly anyway.
$DIG $DIGOPTS test.nil. TXT @10.53.0.1 > dig.out.test$n.2 || ret=1
grep -q -F "serial 4, fallback AXFR" dig.out.test$n.2 || ret=1

# Ensure the expected error is logged.
nextpart ns1/named.run | grep -q -F "SOA name mismatch" || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "testing ixfr-from-differences option ($n)"
# ns3 is primary; ns4 is secondary
$CHECKZONE test. ns3/mytest.db > /dev/null 2>&1
if [ $? -ne 0 ]
then
    echo_i "named-checkzone returned failure on ns3/mytest.db"
fi

retry_quiet 10 wait_for_serial 10.53.0.4 test. 1 dig.out.test$n || ret=1

nextpart ns4/named.run > /dev/null

# modify the primary
sleep 1
cp ns3/mytest1.db ns3/mytest.db
$RNDCCMD 10.53.0.3 reload | sed 's/^/ns3 /' | cat_i

# wait for primary to reload
retry_quiet 10 wait_for_serial 10.53.0.3 test. 2 dig.out.test$n || ret=1

# wait for secondary to reload
tret=0
retry_quiet 5 wait_for_serial 10.53.0.4 test. 2 dig.out.test$n || tret=1
if [ $tret -eq 1 ]; then
    # re-noitfy after 5 seconds, then wait another 10
    $RNDCCMD 10.53.0.3 notify test | set 's/^/ns3 /' | cat_i
    retry_quiet 10 wait_for_serial 10.53.0.4 test. 2 dig.out.test$n || ret=1
fi

wait_for_log 10 'got incremental' ns4/named.run || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "testing 'request-ixfr no' option inheritance from view ($n)"
ret=0
# There's a view with 2 zones. In the view, "request-ixfr yes"
# but in the zone "sub.test", request-ixfr no"
# we want to make sure that a change to sub.test results in AXFR, while
# changes to test. result in IXFR

sleep 1
cp ns3/subtest1.db ns3/subtest.db # change to sub.test zone, should be AXFR
nextpart ns4/named.run > /dev/null
$RNDCCMD 10.53.0.3 reload | sed 's/^/ns3 /' | cat_i

# wait for primary to reload
retry_quiet 10 wait_for_serial 10.53.0.3 sub.test. 3 dig.out.test$n || ret=1

# wait for secondary to reload
tret=0
retry_quiet 5 wait_for_serial 10.53.0.4 sub.test. 3 dig.out.test$n || tret=1
if [ $tret -eq 1 ]; then
    # re-noitfy after 5 seconds, then wait another 10
    $RNDCCMD 10.53.0.3 notify sub.test | set 's/^/ns3 /' | cat_i
    retry_quiet 10 wait_for_serial 10.53.0.4 sub.test. 3 dig.out.test$n || ret=1
fi

wait_for_log 10 'got nonincremental response' ns4/named.run || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "testing 'request-ixfr yes' option inheritance from view ($n)"
ret=0
sleep 1
cp ns3/mytest2.db ns3/mytest.db # change to test zone, should be IXFR
nextpart ns4/named.run > /dev/null
$RNDCCMD 10.53.0.3 reload | sed 's/^/ns3 /' | cat_i

# wait for primary to reload
retry_quiet 10 wait_for_serial 10.53.0.3 test. 3 dig.out.test$n || ret=1

# wait for secondary to reload
tret=0
retry_quiet 5 wait_for_serial 10.53.0.4 test. 3 dig.out.test$n || tret=1
if [ $tret -eq 1 ]; then
    # re-noitfy after 5 seconds, then wait another 10
    $RNDCCMD 10.53.0.3 notify test | set 's/^/ns3 /' | cat_i
    retry_quiet 10 wait_for_serial 10.53.0.4 test. 3 dig.out.test$n || ret=1
fi

wait_for_log 10 'got incremental response' ns4/named.run || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
ret=0
echo_i "testing DiG's handling of a multi message AXFR style IXFR response ($n)"
(
(sleep 10 && kill $$) 2>/dev/null &
sub=$!
$DIG -p ${PORT} ixfr=0 large @10.53.0.3 > dig.out.test$n
kill $sub
)
lines=`grep hostmaster.large dig.out.test$n | wc -l`
test ${lines:-0} -eq 2 || ret=1
messages=`sed -n 's/^;;.*messages \([0-9]*\),.*/\1/p' dig.out.test$n`
test ${messages:-0} -gt 1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "test 'dig +notcp ixfr=<value>' vs 'dig ixfr=<value> +notcp' vs 'dig ixfr=<value>' ($n)"
ret=0
# Should be "switch to TCP" response
$DIG $DIGOPTS +notcp ixfr=1 test @10.53.0.4 > dig.out1.test$n || ret=1
$DIG $DIGOPTS ixfr=1 +notcp test @10.53.0.4 > dig.out2.test$n || ret=1
digcomp dig.out1.test$n dig.out2.test$n || ret=1
awk '$4 == "SOA" { soacnt++} END {if (soacnt == 1) exit(0); else exit(1);}' dig.out1.test$n || ret=1
awk '$4 == "SOA" { if ($7 == 3) exit(0); else exit(1);}' dig.out1.test$n || ret=1
#
nextpart ns4/named.run > /dev/null
# Should be incremental transfer.
$DIG $DIGOPTS ixfr=1 test @10.53.0.4 > dig.out3.test$n || ret=1
awk '$4 == "SOA" { soacnt++} END { if (soacnt == 6) exit(0); else exit(1);}' dig.out3.test$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check estimated IXFR size ($n)"
ret=0
# note IXFR delta size will be slightly bigger with version 1 transaction
# headers as there is no correction for the overall record length storage.
# Ver1 = 4 * (6 + 10 + 10 + 17 + 5 * 4) + 2 * (13 + 10 + 4) + (6 * 4) = 330
# Ver2 = 4 * (6 + 10 + 10 + 17 + 5 * 4) + 2 * (13 + 10 + 4) = 306
nextpart ns4/named.run | grep "IXFR delta size (306 bytes)" > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# make sure ns5 has transfered the zone
# wait for secondary to reload
tret=0
retry_quiet 5 wait_for_serial 10.53.0.5 test. 4 dig.out.test$n || tret=1
if [ $tret -eq 1 ]; then
    # re-noitfy after 5 seconds, then wait another 10
    $RNDCCMD 10.53.0.3 notify test | set 's/^/ns3 /' | cat_i
    retry_quiet 10 wait_for_serial 10.53.0.5 test. 3 dig.out.test$n || ret=1
fi

n=$((n+1))
echo_i "test 'provide-ixfr no;' (serial < current) ($n)"
ret=0
nextpart ns5/named.run > /dev/null
# Should be "AXFR style" response
$DIG $DIGOPTS ixfr=1 test @10.53.0.5 > dig.out1.test$n || ret=1
# Should be "switch to TCP" response
$DIG $DIGOPTS ixfr=1 +notcp test @10.53.0.5 > dig.out2.test$n || ret=1
awk '$4 == "SOA" { soacnt++} END {if (soacnt == 2) exit(0); else exit(1);}' dig.out1.test$n || ret=1
awk '$4 == "SOA" { soacnt++} END {if (soacnt == 1) exit(0); else exit(1);}' dig.out2.test$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking whether dig calculates IXFR statistics correctly ($n)"
ret=0
$DIG $DIGOPTS +noedns +stat -b 10.53.0.4 @10.53.0.4 test. ixfr=2 > dig.out1.test$n
get_dig_xfer_stats dig.out1.test$n > stats.dig
diff ixfr-stats.good stats.dig > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Note: in the next two tests, we use ns4 logs for checking both incoming and
# outgoing transfer statistics as ns4 is both a secondary server (for ns3) and a
# primary server (for dig queries from the previous test) for "test".

_wait_for_stats () {
    get_named_xfer_stats ns4/named.run "$1" test "$2" > "$3"
    diff ixfr-stats.good "$3" > /dev/null || return 1
    return 0
}

n=$((n+1))
echo_i "checking whether named calculates incoming IXFR statistics correctly ($n)"
ret=0
retry_quiet 10 _wait_for_stats 10.53.0.3 "Transfer completed" stats.incoming
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking whether named calculates outgoing IXFR statistics correctly ($n)"
retry_quiet 10 _wait_for_stats 10.53.0.4 "IXFR ended" stats.outgoing
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
ret=0
echo_i "testing fallback to AXFR when max-ixfr-ratio is exceeded ($n)"
nextpart ns4/named.run > /dev/null

sleep 1
cp ns3/mytest3.db ns3/mytest.db # change to test zone, too big for IXFR
$RNDCCMD 10.53.0.3 reload | sed 's/^/ns3 /' | cat_i

# wait for secondary to reload
tret=0
retry_quiet 5 wait_for_serial 10.53.0.4 test. 4 dig.out.test$n || tret=1
if [ $tret -eq 1 ]; then
    # re-noitfy after 5 seconds, then wait another 10
    $RNDCCMD 10.53.0.3 notify test | set 's/^/ns3 /' | cat_i
    retry_quiet 10 wait_for_serial 10.53.0.4 test. 4 dig.out.test$n || ret=1
fi

wait_for_log 10 'got nonincremental response' ns4/named.run || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
