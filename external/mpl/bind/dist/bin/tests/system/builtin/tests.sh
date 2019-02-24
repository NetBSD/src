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

DIGOPTS="-p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0
n=0

emptyzones="
10.IN-ADDR.ARPA
16.172.IN-ADDR.ARPA
17.172.IN-ADDR.ARPA
18.172.IN-ADDR.ARPA
19.172.IN-ADDR.ARPA
20.172.IN-ADDR.ARPA
21.172.IN-ADDR.ARPA
22.172.IN-ADDR.ARPA
23.172.IN-ADDR.ARPA
24.172.IN-ADDR.ARPA
25.172.IN-ADDR.ARPA
26.172.IN-ADDR.ARPA
27.172.IN-ADDR.ARPA
28.172.IN-ADDR.ARPA
29.172.IN-ADDR.ARPA
30.172.IN-ADDR.ARPA
31.172.IN-ADDR.ARPA
168.192.IN-ADDR.ARPA
64.100.IN-ADDR.ARPA
65.100.IN-ADDR.ARPA
66.100.IN-ADDR.ARPA
67.100.IN-ADDR.ARPA
68.100.IN-ADDR.ARPA
69.100.IN-ADDR.ARPA
70.100.IN-ADDR.ARPA
71.100.IN-ADDR.ARPA
72.100.IN-ADDR.ARPA
73.100.IN-ADDR.ARPA
74.100.IN-ADDR.ARPA
75.100.IN-ADDR.ARPA
76.100.IN-ADDR.ARPA
77.100.IN-ADDR.ARPA
78.100.IN-ADDR.ARPA
79.100.IN-ADDR.ARPA
80.100.IN-ADDR.ARPA
81.100.IN-ADDR.ARPA
82.100.IN-ADDR.ARPA
83.100.IN-ADDR.ARPA
84.100.IN-ADDR.ARPA
85.100.IN-ADDR.ARPA
86.100.IN-ADDR.ARPA
87.100.IN-ADDR.ARPA
88.100.IN-ADDR.ARPA
89.100.IN-ADDR.ARPA
90.100.IN-ADDR.ARPA
91.100.IN-ADDR.ARPA
92.100.IN-ADDR.ARPA
93.100.IN-ADDR.ARPA
94.100.IN-ADDR.ARPA
95.100.IN-ADDR.ARPA
96.100.IN-ADDR.ARPA
97.100.IN-ADDR.ARPA
98.100.IN-ADDR.ARPA
99.100.IN-ADDR.ARPA
100.100.IN-ADDR.ARPA
101.100.IN-ADDR.ARPA
102.100.IN-ADDR.ARPA
103.100.IN-ADDR.ARPA
104.100.IN-ADDR.ARPA
105.100.IN-ADDR.ARPA
106.100.IN-ADDR.ARPA
107.100.IN-ADDR.ARPA
108.100.IN-ADDR.ARPA
109.100.IN-ADDR.ARPA
110.100.IN-ADDR.ARPA
111.100.IN-ADDR.ARPA
112.100.IN-ADDR.ARPA
113.100.IN-ADDR.ARPA
114.100.IN-ADDR.ARPA
115.100.IN-ADDR.ARPA
116.100.IN-ADDR.ARPA
117.100.IN-ADDR.ARPA
118.100.IN-ADDR.ARPA
119.100.IN-ADDR.ARPA
120.100.IN-ADDR.ARPA
121.100.IN-ADDR.ARPA
122.100.IN-ADDR.ARPA
123.100.IN-ADDR.ARPA
124.100.IN-ADDR.ARPA
125.100.IN-ADDR.ARPA
126.100.IN-ADDR.ARPA
127.100.IN-ADDR.ARPA
0.IN-ADDR.ARPA
127.IN-ADDR.ARPA
254.169.IN-ADDR.ARPA
2.0.192.IN-ADDR.ARPA
100.51.198.IN-ADDR.ARPA
113.0.203.IN-ADDR.ARPA
255.255.255.255.IN-ADDR.ARPA
0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.IP6.ARPA
1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.IP6.ARPA
D.F.IP6.ARPA
8.E.F.IP6.ARPA
9.E.F.IP6.ARPA
A.E.F.IP6.ARPA
B.E.F.IP6.ARPA
8.B.D.0.1.0.0.2.IP6.ARPA
EMPTY.AS112.ARPA
HOME.ARPA"

n=`expr $n + 1`
ret=0
count=0
echo_i "Checking expected empty zones were configured ($n)"
for zone in ${emptyzones}
do
	grep "automatic empty zone: $zone" ns1/named.run > /dev/null || {
		echo_i "failed (empty zone $zone missing)"
		ret=1
	}
	count=`expr $count + 1`
done
lines=`grep "automatic empty zone: " ns1/named.run | wc -l`
test $count -eq $lines -a $count -eq 99 || {
	ret=1; echo_i "failed (count mismatch)";
}
if [ $ret != 0 ] ; then status=`expr $status + $ret`; fi

n=`expr $n + 1`
echo_i "Checking that reconfiguring empty zones is silent ($n)"
$RNDCCMD 10.53.0.1 reconfig
ret=0
grep "automatic empty zone" ns1/named.run > /dev/null || ret=1
grep "received control channel command 'reconfig'" ns1/named.run > /dev/null || ret=1
grep "reloading configuration succeeded" ns1/named.run > /dev/null || ret=1
sleep 1
grep "zone serial (0) unchanged." ns1/named.run > /dev/null && ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

n=`expr $n + 1`
echo_i "Checking that reloading empty zones is silent ($n)"
rndc_reload ns1 10.53.0.1
ret=0
grep "automatic empty zone" ns1/named.run > /dev/null || ret=1
grep "received control channel command 'reload'" ns1/named.run > /dev/null || ret=1
grep "reloading configuration succeeded" ns1/named.run > /dev/null || ret=1
sleep 1
grep "zone serial (0) unchanged." ns1/named.run > /dev/null && ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

VERSION=`../../../../isc-config.sh  --version | cut -d = -f 2`
HOSTNAME=`$FEATURETEST --gethostname`

n=`expr $n + 1`
ret=0
echo_i "Checking that default version works for rndc ($n)"
$RNDCCMD 10.53.0.1 status > rndc.status.ns1.$n 2>&1
grep "^version: BIND $VERSION " rndc.status.ns1.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

n=`expr $n + 1`
ret=0
echo_i "Checking that custom version works for rndc ($n)"
$RNDCCMD 10.53.0.3 status > rndc.status.ns3.$n 2>&1
grep "^version: BIND $VERSION ${DESCRIPTION}${DESCRIPTION:+ }<id:........*> (this is a test of version)" rndc.status.ns3.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

n=`expr $n + 1`
ret=0
echo_i "Checking that default version works for query ($n)"
$DIG $DIGOPTS +short version.bind txt ch @10.53.0.1 > dig.out.ns1.$n
grep "^\"$VERSION\"$" dig.out.ns1.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

n=`expr $n + 1`
ret=0
echo_i "Checking that custom version works for query ($n)"
$DIG $DIGOPTS +short version.bind txt ch @10.53.0.3 > dig.out.ns3.$n
grep "^\"this is a test of version\"$" dig.out.ns3.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

n=`expr $n + 1`
ret=0
echo_i "Checking that default hostname works for query ($n)"
$DIG $DIGOPTS +short hostname.bind txt ch @10.53.0.1 > dig.out.ns1.$n
grep "^\"$HOSTNAME\"$" dig.out.ns1.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

n=`expr $n + 1`
ret=0
echo_i "Checking that custom hostname works for query ($n)"
$DIG $DIGOPTS +short hostname.bind txt ch @10.53.0.3 > dig.out.ns3.$n
grep "^\"this.is.a.test.of.hostname\"$" dig.out.ns3.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

n=`expr $n + 1`
ret=0
echo_i "Checking that default server-id is none for query ($n)"
$DIG $DIGOPTS id.server txt ch @10.53.0.1 > dig.out.ns1.$n
grep "status: NOERROR" dig.out.ns1.$n > /dev/null || ret=1
grep "ANSWER: 0" dig.out.ns1.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

n=`expr $n + 1`
ret=0
echo_i "Checking that server-id hostname works for query ($n)"
$DIG $DIGOPTS +short id.server txt ch @10.53.0.2 > dig.out.ns2.$n
grep "^\"$HOSTNAME\"$" dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

n=`expr $n + 1`
ret=0
echo_i "Checking that server-id hostname works for EDNS name server ID request ($n)"
$DIG $DIGOPTS +norec +nsid foo @10.53.0.2 > dig.out.ns2.$n
grep "^; NSID: .* (\"$HOSTNAME\")$" dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

n=`expr $n + 1`
ret=0
echo_i "Checking that custom server-id works for query ($n)"
$DIG $DIGOPTS +short id.server txt ch @10.53.0.3 > dig.out.ns3.$n
grep "^\"this.is.a.test.of.server-id\"$" dig.out.ns3.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

n=`expr $n + 1`
ret=0
echo_i "Checking that custom server-id works for EDNS name server ID request ($n)"
$DIG $DIGOPTS +norec +nsid foo @10.53.0.3 > dig.out.ns3.$n
grep "^; NSID: .* (\"this.is.a.test.of.server-id\")$" dig.out.ns3.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
