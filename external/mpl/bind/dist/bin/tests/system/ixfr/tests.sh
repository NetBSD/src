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


# WARNING: The test labelled "testing request-ixfr option in view vs zone"
#          is fragile because it depends upon counting instances of records
#          in the log file - need a better approach <sdm> - until then,
#          if you add any tests above that point, you will break the test.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"
SENDCMD="$PERL ../send.pl 10.53.0.2 ${EXTRAPORT1}"
RNDCCMD="$RNDC -p ${CONTROLPORT} -c ../common/rndc.conf -s"

echo_i "testing initial AXFR"

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
# a slave for nil.

cat <<EOF >>ns1/named.conf
zone "nil" {
	type slave;
	file "myftp.db";
	masters { 10.53.0.2; };
};
EOF

$RNDCCMD 10.53.0.1 reload | sed 's/^/ns1 /' | cat_i

for i in 0 1 2 3 4 5 6 7 8 9
do
	$DIG $DIGOPTS @10.53.0.1 nil. SOA > dig.out
	grep "SOA" dig.out > /dev/null && break
	sleep 1
done

$DIG $DIGOPTS @10.53.0.1 nil. TXT | grep 'initial AXFR' >/dev/null || {
    echo_i "failed"
    status=1
}

echo_i "testing successful IXFR"

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

$RNDCCMD 10.53.0.1 refresh nil

sleep 2

$DIG $DIGOPTS @10.53.0.1 nil. TXT | grep 'successful IXFR' >/dev/null || {
    echo_i "failed"
    status=1
}

echo_i "testing AXFR fallback after IXFR failure"

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

$RNDCCMD 10.53.0.1 refresh nil

sleep 2

$DIG $DIGOPTS @10.53.0.1 nil. TXT | grep 'fallback AXFR' >/dev/null || {
    echo_i "failed"
    status=1
}

echo_i "testing ixfr-from-differences option"
# ns3 is master; ns4 is slave
$CHECKZONE test. ns3/mytest.db > /dev/null 2>&1
if [ $? -ne 0 ]
then
    echo_i "named-checkzone returned failure on ns3/mytest.db"
fi
# modify the master
#echo_i "digging against master: "
#$DIG $DIGOPTS @10.53.0.3 a host1.test.
#echo_i "digging against slave: "
#$DIG $DIGOPTS @10.53.0.4 a host1.test.

# wait for slave to be stable
for i in 0 1 2 3 4 5 6 7 8 9
do
	$DIG $DIGOPTS +tcp @10.53.0.4 SOA test > dig.out
	grep -i "hostmaster\.test\..1" dig.out > /dev/null && break
	sleep 1
done

# modify the master
cp ns3/mytest1.db ns3/mytest.db
$RNDCCMD 10.53.0.3 reload | sed 's/^/ns3 /' | cat_i

#wait for master to reload load
for i in 0 1 2 3 4 5 6 7 8 9
do
	$DIG $DIGOPTS +tcp @10.53.0.3 SOA test > dig.out
	grep -i "hostmaster\.test\..2" dig.out > /dev/null && break
	sleep 1
done

#wait for slave to transfer zone
for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14
do
	$DIG $DIGOPTS +tcp @10.53.0.4 SOA test > dig.out
	grep -i "hostmaster\.test\..2" dig.out > /dev/null && break

	# re-notify if we've been waiting a long time
	if [ $i -ge 5 ]; then
	    $RNDCCMD 10.53.0.3 notify test | set 's/^/ns3 /' | cat_i
	fi
	sleep 1
done

# slave should have gotten notify and updated

for i in 0 1 2 3 4 5 6 7 8 9
do
	INCR=`grep "test/IN/primary" ns4/named.run|grep "got incremental"|wc -l`
	[ $INCR -eq 1 ] && break
	sleep 1
done
if [ $INCR -ne 1 ]
then
    echo_i "failed to get incremental response"
    status=1
fi

echo_i "testing request-ixfr option in view vs zone"
# There's a view with 2 zones. In the view, "request-ixfr yes"
# but in the zone "sub.test", request-ixfr no"
# we want to make sure that a change to sub.test results in AXFR, while
# changes to test. result in IXFR

echo_i " this result should be AXFR"
cp ns3/subtest1.db ns3/subtest.db # change to sub.test zone, should be AXFR
$RNDCCMD 10.53.0.3 reload | sed 's/^/ns3 /' | cat_i

#wait for master to reload zone
for i in 0 1 2 3 4 5 6 7 8 9
do
	$DIG $DIGOPTS +tcp @10.53.0.3 SOA sub.test > dig.out
	grep -i "hostmaster\.test\..3" dig.out > /dev/null && break
	sleep 1
done

#wait for slave to transfer zone
for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14
do
	$DIG $DIGOPTS +tcp @10.53.0.4 SOA sub.test > dig.out
	grep -i "hostmaster\.test\..3" dig.out > /dev/null && break

	# re-notify if we've been waiting a long time
	if [ $i -ge 5 ]; then
	    $RNDCCMD 10.53.0.3 notify sub.test | set 's/^/ns3 /' | cat_i
	fi
	sleep 1
done

echo_i " this result should be AXFR"
for i in 0 1 2 3 4 5 6 7 8 9
do
	NONINCR=`grep 'sub\.test/IN/primary' ns4/named.run|grep "got nonincremental" | wc -l`
	[ $NONINCR -eq 2 ] && break
	sleep 1
done
if [ $NONINCR -ne 2 ]
then
    echo_i "failed to get nonincremental response in 2nd AXFR test"
    status=1
else
    echo_i "  success: AXFR it was"
fi

echo_i " this result should be IXFR"
cp ns3/mytest2.db ns3/mytest.db # change to test zone, should be IXFR
$RNDCCMD 10.53.0.3 reload | sed 's/^/ns3 /' | cat_i

# wait for master to reload zone
for i in 0 1 2 3 4 5 6 7 8 9
do
	$DIG +tcp -p 5300 @10.53.0.3 SOA test > dig.out
	grep -i "hostmaster\.test\..4" dig.out > /dev/null && break
	sleep 1
done

# wait for slave to transfer zone
for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14
do
	$DIG $DIGOPTS +tcp @10.53.0.4 SOA test > dig.out
	grep -i "hostmaster\.test\..4" dig.out > /dev/null && break

	# re-notify if we've been waiting a long time
	if [ $i -ge 5 ]; then
	    $RNDCCMD 10.53.0.3 notify test | set 's/^/ns3 /' | cat_i
	fi
	sleep 1
done

for i in 0 1 2 3 4 5 6 7 8 9
do
	INCR=`grep "test/IN/primary" ns4/named.run|grep "got incremental"|wc -l`
	[ $INCR -eq 2 ] && break
	sleep 1
done
if [ $INCR -ne 2 ]
then
    echo_i "failed to get incremental response in 2nd IXFR test"
    status=1
else
    echo_i "  success: IXFR it was"
fi

echo_i "testing DiG's handling of a multi message AXFR style IXFR response"
(
(sleep 10 && kill $$) 2>/dev/null &
sub=$!
$DIG -p ${PORT} ixfr=0 large @10.53.0.3 > dig.out
kill $sub
)
lines=`grep hostmaster.large dig.out | wc -l`
test ${lines:-0} -eq 2 || { echo_i "failed"; status=1; }
messages=`sed -n 's/^;;.*messages \([0-9]*\),.*/\1/p' dig.out`
test ${messages:-0} -gt 1 || { echo_i "failed"; status=1; }

echo_i "test 'dig +notcp ixfr=<value>' vs 'dig ixfr=<value> +notcp' vs 'dig ixfr=<value>'"
ret=0
# Should be "switch to TCP" response
$DIG $DIGOPTS +notcp ixfr=1 test @10.53.0.4 > dig.out1 || ret=1
$DIG $DIGOPTS ixfr=1 +notcp test @10.53.0.4 > dig.out2 || ret=1
digcomp dig.out1 dig.out2 || ret=1
awk '$4 == "SOA" { soacnt++} END {if (soacnt == 1) exit(0); else exit(1);}' dig.out1 || ret=1
awk '$4 == "SOA" { if ($7 == 4) exit(0); else exit(1);}' dig.out1 || ret=1
# Should be incremental transfer.
$DIG $DIGOPTS ixfr=1 test @10.53.0.4 > dig.out3 || ret=1
awk '$4 == "SOA" { soacnt++} END { if (soacnt == 6) exit(0); else exit(1);}' dig.out3 || ret=1
if [ ${ret} != 0 ]; then
	echo_i "failed";
	status=1;
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
