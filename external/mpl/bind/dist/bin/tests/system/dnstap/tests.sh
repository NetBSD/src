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

DIGOPTS="+short -p ${PORT}"
RNDCCMD="$RNDC -p ${CONTROLPORT} -c ../common/rndc.conf"

status=0

for bad in bad-*.conf
do
	ret=0
	echo_i "checking that named-checkconf detects error in $bad"
	$CHECKCONF $bad > /dev/null 2>&1
	if [ $? != 1 ]; then echo_i "failed"; ret=1; fi
	status=`expr $status + $ret`
done

for good in good-*.conf
do
	ret=0
	echo_i "checking that named-checkconf detects no error in $good"
	$CHECKCONF $good > /dev/null 2>&1
	if [ $? != 0 ]; then echo_i "failed"; ret=1; fi
	status=`expr $status + $ret`
done

$DIG $DIGOPTS @10.53.0.3 a.example > dig.out

# check three different dnstap reopen/roll methods:
# ns1: dnstap-reopen; ns2: dnstap -reopen; ns3: dnstap -roll
mv ns1/dnstap.out ns1/dnstap.out.save
mv ns2/dnstap.out ns2/dnstap.out.save

if [ -n "$FSTRM_CAPTURE" ] ; then
	$FSTRM_CAPTURE -t protobuf:dnstap.Dnstap -u ns4/dnstap.out \
		-w dnstap.out > fstrm_capture.out 2>&1 &
	fstrm_capture_pid=$!
fi

$RNDCCMD -s 10.53.0.1 dnstap-reopen | sed 's/^/ns1 /' | cat_i
$RNDCCMD -s 10.53.0.2 dnstap -reopen | sed 's/^/ns2 /' | cat_i
$RNDCCMD -s 10.53.0.3 dnstap -roll | sed 's/^/ns3 /' | cat_i
$RNDCCMD -s 10.53.0.4 dnstap -reopen | sed 's/^/ns4 /' | cat_i

$DIG $DIGOPTS @10.53.0.3 a.example > dig.out

# XXX: file output should be flushed once a second according
# to the libfstrm source, but it doesn't seem to happen until
# enough data has accumulated. to get all the output, we stop
# the name servers, forcing a flush on shutdown. it would be
# nice to find a better way to do this.
$RNDCCMD -s 10.53.0.1 stop | sed 's/^/ns1 /' | cat_i
$RNDCCMD -s 10.53.0.2 stop | sed 's/^/ns2 /' | cat_i
$RNDCCMD -s 10.53.0.3 stop | sed 's/^/ns3 /' | cat_i
sleep 1

echo_i "checking initial message counts"

udp1=`$DNSTAPREAD ns1/dnstap.out.save | grep "UDP " | wc -l`
tcp1=`$DNSTAPREAD ns1/dnstap.out.save | grep "TCP " | wc -l`
aq1=`$DNSTAPREAD ns1/dnstap.out.save | grep "AQ " | wc -l`
ar1=`$DNSTAPREAD ns1/dnstap.out.save | grep "AR " | wc -l`
cq1=`$DNSTAPREAD ns1/dnstap.out.save | grep "CQ " | wc -l`
cr1=`$DNSTAPREAD ns1/dnstap.out.save | grep "CR " | wc -l`
rq1=`$DNSTAPREAD ns1/dnstap.out.save | grep "RQ " | wc -l`
rr1=`$DNSTAPREAD ns1/dnstap.out.save | grep "RR " | wc -l`

udp2=`$DNSTAPREAD ns2/dnstap.out.save | grep "UDP " | wc -l`
tcp2=`$DNSTAPREAD ns2/dnstap.out.save | grep "TCP " | wc -l`
aq2=`$DNSTAPREAD ns2/dnstap.out.save | grep "AQ " | wc -l`
ar2=`$DNSTAPREAD ns2/dnstap.out.save | grep "AR " | wc -l`
cq2=`$DNSTAPREAD ns2/dnstap.out.save | grep "CQ " | wc -l`
cr2=`$DNSTAPREAD ns2/dnstap.out.save | grep "CR " | wc -l`
rq2=`$DNSTAPREAD ns2/dnstap.out.save | grep "RQ " | wc -l`
rr2=`$DNSTAPREAD ns2/dnstap.out.save | grep "RR " | wc -l`

mv ns3/dnstap.out.0 ns3/dnstap.out.save
udp3=`$DNSTAPREAD ns3/dnstap.out.save | grep "UDP " | wc -l`
tcp3=`$DNSTAPREAD ns3/dnstap.out.save | grep "TCP " | wc -l`
aq3=`$DNSTAPREAD ns3/dnstap.out.save | grep "AQ " | wc -l`
ar3=`$DNSTAPREAD ns3/dnstap.out.save | grep "AR " | wc -l`
cq3=`$DNSTAPREAD ns3/dnstap.out.save | grep "CQ " | wc -l`
cr3=`$DNSTAPREAD ns3/dnstap.out.save | grep "CR " | wc -l`
rq3=`$DNSTAPREAD ns3/dnstap.out.save | grep "RQ " | wc -l`
rr3=`$DNSTAPREAD ns3/dnstap.out.save | grep "RR " | wc -l`

echo_i "checking UDP message counts"
ret=0
[ $udp1 -eq 0 ] || {
	echo_i "ns1 $udp1 expected 0"
	ret=1
}
[ $udp2 -eq 2 ] || {
	echo_i "ns2 $udp2 expected 2"
	ret=1
}
[ $udp3 -eq 4 ] || {
	echo_i "ns3 $udp3 expected 4"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking TCP message counts"
ret=0
[ $tcp1 -eq 6 ] || {
	echo_i "ns1 $tcp1 expected 6"
	ret=1
}
[ $tcp2 -eq 2 ] || {
	echo_i "ns2 $tcp2 expected 2"
	ret=1
}
[ $tcp3 -eq 6 ] || {
	echo_i "ns3 $tcp3 expected 6"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AUTH_QUERY message counts"
ret=0
[ $aq1 -eq 2 ] || {
	echo_i "ns1 $aq1 exepcted 2"
	ret=1
}
[ $aq2 -eq 1 ] || {
	echo_i "ns2 $aq2 expected 1"
	ret=1
}
[ $aq3 -eq 0 ] || {
	echo_i "ns3 $aq3 expected 0"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AUTH_RESPONSE message counts"
ret=0
[ $ar1 -eq 2 ] || {
	echo_i "ns1 $ar1 expected 2"
	ret=1
}
[ $ar2 -eq 1 ] || {
	echo_i "ns2 $ar2 expected 1"
	ret=1
}
[ $ar3 -eq 0 ] || {
	echo_i "ns3 $ar3 expected 0"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking CLIENT_QUERY message counts"
ret=0
[ $cq1 -eq 1 ] || {
	echo_i "ns1 $cq1 expected 1"
	ret=1
}
[ $cq2 -eq 1 ] || {
	echo_i "ns2 $cq2 expected 1"
	ret=1
}
[ $cq3 -eq 2 ] || {
	echo_i "ns3 $cq3 expected 2"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking CLIENT_RESPONSE message counts"
ret=0
[ $cr1 -eq 1 ] || {
	echo_i "ns1 $cr1 expected 1"
	ret=1
}
[ $cr2 -eq 1 ] || {
	echo_i "ns2 $cr2 expected 1"
	ret=1
}
[ $cr3 -eq 2 ] || {
	echo_i "ns3 $cr3 expected 2"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking RESOLVER_QUERY message counts"
ret=0
[ $rq1 -eq 0 ] || {
	echo_i "ns1 $rq1 expected 0"
	ret=1
}
[ $rq2 -eq 0 ] || {
	echo_i "ns2 $rq2 expected 0"
	ret=1
}
[ $rq3 -eq 3 ] || {
	echo_i "ns3 $rq3 expected 3"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking RESOLVER_RESPONSE message counts"
ret=0
[ $rr1 -eq 0 ] || {
	echo_i "ns1 $rr1 expected 0"
	ret=1
}
[ $rr2 -eq 0 ] || {
	echo_i "ns2 $rr2 expected 0"
	ret=1
}
[ $rr3 -eq 3 ] || {
	echo_i "ns3 $rr3 expected 3"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking reopened message counts"

udp1=`$DNSTAPREAD ns1/dnstap.out | grep "UDP " | wc -l`
tcp1=`$DNSTAPREAD ns1/dnstap.out | grep "TCP " | wc -l`
aq1=`$DNSTAPREAD ns1/dnstap.out | grep "AQ " | wc -l`
ar1=`$DNSTAPREAD ns1/dnstap.out | grep "AR " | wc -l`
cq1=`$DNSTAPREAD ns1/dnstap.out | grep "CQ " | wc -l`
cr1=`$DNSTAPREAD ns1/dnstap.out | grep "CR " | wc -l`
rq1=`$DNSTAPREAD ns1/dnstap.out | grep "RQ " | wc -l`
rr1=`$DNSTAPREAD ns1/dnstap.out | grep "RR " | wc -l`

udp2=`$DNSTAPREAD ns2/dnstap.out | grep "UDP " | wc -l`
tcp2=`$DNSTAPREAD ns2/dnstap.out | grep "TCP " | wc -l`
aq2=`$DNSTAPREAD ns2/dnstap.out | grep "AQ " | wc -l`
ar2=`$DNSTAPREAD ns2/dnstap.out | grep "AR " | wc -l`
cq2=`$DNSTAPREAD ns2/dnstap.out | grep "CQ " | wc -l`
cr2=`$DNSTAPREAD ns2/dnstap.out | grep "CR " | wc -l`
rq2=`$DNSTAPREAD ns2/dnstap.out | grep "RQ " | wc -l`
rr2=`$DNSTAPREAD ns2/dnstap.out | grep "RR " | wc -l`

udp3=`$DNSTAPREAD ns3/dnstap.out | grep "UDP " | wc -l`
tcp3=`$DNSTAPREAD ns3/dnstap.out | grep "TCP " | wc -l`
aq3=`$DNSTAPREAD ns3/dnstap.out | grep "AQ " | wc -l`
ar3=`$DNSTAPREAD ns3/dnstap.out | grep "AR " | wc -l`
cq3=`$DNSTAPREAD ns3/dnstap.out | grep "CQ " | wc -l`
cr3=`$DNSTAPREAD ns3/dnstap.out | grep "CR " | wc -l`
rq3=`$DNSTAPREAD ns3/dnstap.out | grep "RQ " | wc -l`
rr3=`$DNSTAPREAD ns3/dnstap.out | grep "RR " | wc -l`

echo_i "checking UDP message counts"
ret=0
[ $udp1 -eq 0 ] || {
	echo_i "ns1 $udp1 expected 0"
	ret=1
}
[ $udp2 -eq 0 ] || {
	echo_i "ns2 $udp2 expected 0"
	ret=1
}
[ $udp3 -eq 2 ] || {
	echo_i "ns3 $udp3 expected 2"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking TCP message counts"
ret=0
[ $tcp1 -eq 0 ] || {
	echo_i "ns1 $tcp1 expected 0"
	ret=1
}
[ $tcp2 -eq 0 ] || {
	echo_i "ns2 $tcp2 expected 0"
	ret=1
}
[ $tcp3 -eq 0 ] || {
	echo_i "ns3 $tcp3 expected 0"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AUTH_QUERY message counts"
ret=0
[ $aq1 -eq 0 ] || {
	echo_i "ns1 $aq1 exepcted 0"
	ret=1
}
[ $aq2 -eq 0 ] || {
	echo_i "ns2 $aq2 expected 0"
	ret=1
}
[ $aq3 -eq 0 ] || {
	echo_i "ns3 $aq3 expected 0"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AUTH_RESPONSE message counts"
ret=0
[ $ar1 -eq 0 ] || {
	echo_i "ns1 $ar1 expected 0"
	ret=1
}
[ $ar2 -eq 0 ] || {
	echo_i "ns2 $ar2 expected 0"
	ret=1
}
[ $ar3 -eq 0 ] || {
	echo_i "ns3 $ar3 expected 0"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking CLIENT_QUERY message counts"
ret=0
[ $cq1 -eq 0 ] || {
	echo_i "ns1 $cq1 expected 0"
	ret=1
}
[ $cq2 -eq 0 ] || {
	echo_i "ns2 $cq2 expected 0"
	ret=1
}
[ $cq3 -eq 1 ] || {
	echo_i "ns3 $cq3 expected 1"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking CLIENT_RESPONSE message counts"
ret=0
[ $cr1 -eq 0 ] || {
	echo_i "ns1 $cr1 expected 0"
	ret=1
}
[ $cr2 -eq 0 ] || {
	echo_i "ns2 $cr2 expected 0"
	ret=1
}
[ $cr3 -eq 1 ] || {
	echo_i "ns3 $cr3 expected 1"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking RESOLVER_QUERY message counts"
ret=0
[ $rq1 -eq 0 ] || {
	echo_i "ns1 $rq1 expected 0"
	ret=1
}
[ $rq2 -eq 0 ] || {
	echo_i "ns2 $rq2 expected 0"
	ret=1
}
[ $rq3 -eq 0 ] || {
	echo_i "ns3 $rq3 expected 0"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking RESOLVER_RESPONSE message counts"
ret=0
[ $rr1 -eq 0 ] || {
	echo_i "ns1 $rr1 expected 0"
	ret=1
}
[ $rr2 -eq 0 ] || {
	echo_i "ns2 $rr2 expected 0"
	ret=1
}
[ $rr3 -eq 0 ] || {
	echo_i "ns3 $rr3 expected 0"
	ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

HAS_PYYAML=0
if [ -n "$PYTHON" ] ; then
	$PYTHON -c "import yaml" 2> /dev/null && HAS_PYYAML=1
fi

if [ $HAS_PYYAML -ne 0 ] ; then
	echo_i "checking dnstap-read YAML output"
	ret=0
        {
	  $PYTHON ydump.py "$DNSTAPREAD" "ns3/dnstap.out.save" > ydump.out || ret=1
        } | cat_i
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`
fi

echo_i "checking dnstap-read hex output"
ret=0
hex=`$DNSTAPREAD -x ns3/dnstap.out | tail -1`
echo $hex | $WIRETEST > dnstap.hex
grep 'status: NOERROR' dnstap.hex > /dev/null 2>&1 || ret=1
grep 'ANSWER: 3, AUTHORITY: 1' dnstap.hex > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -n "$FSTRM_CAPTURE" ] ; then
	$DIG $DIGOPTS @10.53.0.4 a.example > dig.out

	echo_i "checking unix socket message counts"
	sleep 2
	kill $fstrm_capture_pid
	wait
	udp4=`$DNSTAPREAD dnstap.out | grep "UDP " | wc -l`
	tcp4=`$DNSTAPREAD dnstap.out | grep "TCP " | wc -l`
	aq4=`$DNSTAPREAD dnstap.out | grep "AQ " | wc -l`
	ar4=`$DNSTAPREAD dnstap.out | grep "AR " | wc -l`
	cq4=`$DNSTAPREAD dnstap.out | grep "CQ " | wc -l`
	cr4=`$DNSTAPREAD dnstap.out | grep "CR " | wc -l`
	rq4=`$DNSTAPREAD dnstap.out | grep "RQ " | wc -l`
	rr4=`$DNSTAPREAD dnstap.out | grep "RR " | wc -l`

	echo_i "checking UDP message counts"
	ret=0
	[ $udp4 -eq 2 ] || {
		echo_i "ns4 $udp4 expected 2"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking TCP message counts"
	ret=0
	[ $tcp4 -eq 0 ] || {
		echo_i "ns4 $tcp4 expected 0"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking AUTH_QUERY message counts"
	ret=0
	[ $aq4 -eq 0 ] || {
		echo_i "ns4 $aq4 expected 0"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking AUTH_RESPONSE message counts"
	ret=0
	[ $ar4 -eq 0 ] || {
		echo_i "ns4 $ar4 expected 0"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking CLIENT_QUERY message counts"
	ret=0
	[ $cq4 -eq 1 ] || {
		echo_i "ns4 $cq4 expected 1"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking CLIENT_RESPONSE message counts"
	ret=0
	[ $cr4 -eq 1 ] || {
		echo_i "ns4 $cr4 expected 1"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking RESOLVER_QUERY message counts"
	ret=0
	[ $rq4 -eq 0 ] || {
		echo_i "ns4 $rq4 expected 0"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking RESOLVER_RESPONSE message counts"
	ret=0
	[ $rr4 -eq 0 ] || {
		echo_i "ns4 $rr4 expected 0"
		ret=1
	}
	mv dnstap.out dnstap.out.save
	$FSTRM_CAPTURE -t protobuf:dnstap.Dnstap -u ns4/dnstap.out \
		-w dnstap.out > fstrm_capture.out 2>&1 &
	fstrm_capture_pid=$!
	$RNDCCMD -s 10.53.0.4 dnstap -reopen | sed 's/^/ns4 /' | cat_i
	$DIG $DIGOPTS @10.53.0.4 a.example > dig.out

	echo_i "checking reopened unix socket message counts"
	sleep 2
	kill $fstrm_capture_pid
	wait
	udp4=`$DNSTAPREAD dnstap.out | grep "UDP " | wc -l`
	tcp4=`$DNSTAPREAD dnstap.out | grep "TCP " | wc -l`
	aq4=`$DNSTAPREAD dnstap.out | grep "AQ " | wc -l`
	ar4=`$DNSTAPREAD dnstap.out | grep "AR " | wc -l`
	cq4=`$DNSTAPREAD dnstap.out | grep "CQ " | wc -l`
	cr4=`$DNSTAPREAD dnstap.out | grep "CR " | wc -l`
	rq4=`$DNSTAPREAD dnstap.out | grep "RQ " | wc -l`
	rr4=`$DNSTAPREAD dnstap.out | grep "RR " | wc -l`

	echo_i "checking UDP message counts"
	ret=0
	[ $udp4 -eq 2 ] || {
		echo_i "ns4 $udp4 expected 2"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking TCP message counts"
	ret=0
	[ $tcp4 -eq 0 ] || {
		echo_i "ns4 $tcp4 expected 0"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking AUTH_QUERY message counts"
	ret=0
	[ $aq4 -eq 0 ] || {
		echo_i "ns4 $aq4 expected 0"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking AUTH_RESPONSE message counts"
	ret=0
	[ $ar4 -eq 0 ] || {
		echo_i "ns4 $ar4 expected 0"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking CLIENT_QUERY message counts"
	ret=0
	[ $cq4 -eq 1 ] || {
		echo_i "ns4 $cq4 expected 1"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking CLIENT_RESPONSE message counts"
	ret=0
	[ $cr4 -eq 1 ] || {
		echo_i "ns4 $cr4 expected 1"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking RESOLVER_QUERY message counts"
	ret=0
	[ $rq4 -eq 0 ] || {
		echo_i "ns4 $rq4 expected 0"
		ret=1
	}
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`

	echo_i "checking RESOLVER_RESPONSE message counts"
	ret=0
	[ $rr4 -eq 0 ] || {
		echo_i "ns4 $rr4 expected 0"
		ret=1
	}
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
