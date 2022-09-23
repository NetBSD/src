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

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd"
DIGCMD="$DIG $DIGOPTS -p ${PORT}"
RNDCCMD="$RNDC -p ${CONTROLPORT} -c ../common/rndc.conf"

status=0

ret=0
n=1
stats=0
rndc_stats() {
	_ns=$1
	_ip=$2

	$RNDCCMD -s $_ip stats > /dev/null 2>&1 || return 1
	[ -f "${_ns}/named.stats" ] || return 1

	last_stats=named.stats.$_ns-$stats-$n
	mv ${_ns}/named.stats $last_stats
	stats=$((stats+1))
}

echo_i "fetching a.example from ns2's initial configuration ($n)"
$DIGCMD +noauth a.example. @10.53.0.2 any > dig.out.ns2.1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "dumping initial stats for ns2 ($n)"
rndc_stats ns2 10.53.0.2 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "verifying adb records in named.stats ($n)"
grep "ADB stats" $last_stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "checking for 1 entry in adb hash table in named.stats ($n)"
grep "1 Addresses in hash table" $last_stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "verifying cache statistics in named.stats ($n)"
grep "Cache Statistics" $last_stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "checking for 2 entries in adb hash table in named.stats ($n)"
$DIGCMD a.example.info. @10.53.0.2 any > /dev/null 2>&1
rndc_stats ns2 10.53.0.2 || ret=1
grep "2 Addresses in hash table" $last_stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "dumping initial stats for ns3 ($n)"
rndc_stats ns3 10.53.0.3 || ret=1
if [ ! "$CYGWIN" ]; then
    nsock0nstat=`grep "UDP/IPv4 sockets active" $last_stats | awk '{print $1}'`
    [ 0 -ne ${nsock0nstat:-0} ] || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "sending queries to ns3"
$DIGCMD +tries=2 +time=1 +recurse @10.53.0.3 foo.info. any > /dev/null 2>&1

ret=0
echo_i "dumping updated stats for ns3 ($n)"
getstats() {
    rndc_stats ns3 10.53.0.3 || return 1
    grep "2 recursing clients" $last_stats > /dev/null || return 1
}
retry_quiet 5 getstats || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "verifying recursing clients output in named.stats ($n)"
grep "2 recursing clients" $last_stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "verifying active fetches output in named.stats ($n)"
grep "1 active fetches" $last_stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

if [ ! "$CYGWIN" ]; then
    ret=0
    echo_i "verifying active sockets output in named.stats ($n)"
    nsock1nstat=`grep "UDP/IPv4 sockets active" $last_stats | awk '{print $1}'`
    [ `expr ${nsock1nstat:-0} - ${nsock0nstat:-0}` -eq 1 ] || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
    n=`expr $n + 1`
fi

# there should be 1 UDP and no TCP queries.  As the TCP counter is zero
# no status line is emitted.
ret=0
echo_i "verifying queries in progress in named.stats ($n)"
grep "1 UDP queries in progress" $last_stats > /dev/null || ret=1
grep "TCP queries in progress" $last_stats > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "verifying bucket size output ($n)"
grep "bucket size" $last_stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "checking priming queries are counted ($n)"
grep "priming queries" $last_stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "checking that zones with slash are properly shown in XML output ($n)"
if $FEATURETEST --have-libxml2 && [ -x ${CURL} ] ; then
    ${CURL} http://10.53.0.1:${EXTRAPORT1}/xml/v3/zones > curl.out.${n} 2>/dev/null || ret=1
    grep '<zone name="32/1.0.0.127-in-addr.example" rdataclass="IN">' curl.out.${n} > /dev/null || ret=1
else
    echo_i "skipping test as libxml2 and/or curl was not found"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "checking that zones return their type ($n)"
if $FEATURETEST --have-libxml2 && [ -x ${CURL} ] ; then
    ${CURL} http://10.53.0.1:${EXTRAPORT1}/xml/v3/zones > curl.out.${n} 2>/dev/null || ret=1
    grep '<zone name="32/1.0.0.127-in-addr.example" rdataclass="IN"><type>master</type>' curl.out.${n} > /dev/null || ret=1
else
    echo_i "skipping test as libxml2 and/or curl was not found"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "checking bind9.xsl vs xml ($n)"
if $FEATURETEST --have-libxml2 && [ -x "${CURL}" ] && [ -x "${XSLTPROC}" ]  ; then
    $DIGCMD +notcp +recurse @10.53.0.3 soa . > /dev/null 2>&1
    $DIGCMD +notcp +recurse @10.53.0.3 soa example > /dev/null 2>&1
    ${CURL} http://10.53.0.3:${EXTRAPORT1}/xml/v3 > curl.out.${n}.xml 2>/dev/null || ret=1
    ${CURL} http://10.53.0.3:${EXTRAPORT1}/bind9.xsl > curl.out.${n}.xsl 2>/dev/null || ret=1
    ${XSLTPROC} curl.out.${n}.xsl - < curl.out.${n}.xml > xsltproc.out.${n} 2>/dev/null || ret=1
    cp curl.out.${n}.xml stats.xml.out || ret=1

    #
    # grep for expected sections.
    #
    grep "<h1>ISC Bind 9 Configuration and Statistics</h1>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Server Status</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Incoming Requests by DNS Opcode</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h3>Incoming Queries by Query Type</h3>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Outgoing Queries per view</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h3>View " xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Server Statistics</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Zone Maintenance Statistics</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Resolver Statistics (Common)</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h3>Resolver Statistics for View " xsltproc.out.${n} >/dev/null || ret=1
    grep "<h3>ADB Statistics for View " xsltproc.out.${n} >/dev/null || ret=1
    grep "<h3>Cache Statistics for View " xsltproc.out.${n} >/dev/null || ret=1
    # grep "<h3>Cache DB RRsets for View " xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Traffic Size Statistics</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h4>UDP Requests Received</h4>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h4>UDP Responses Sent</h4>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h4>TCP Requests Received</h4>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h4>TCP Responses Sent</h4>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Socket I/O Statistics</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h3>Zones for View " xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Received QTYPES per view/zone</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h3>View _default" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h4>Zone example" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Response Codes per view/zone</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h3>View _default" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h4>Zone example" xsltproc.out.${n} >/dev/null || ret=1
    # grep "<h2>Glue cache statistics</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h3>View _default" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h4>Zone example" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Network Status</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Task Manager Configuration</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Tasks</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Memory Usage Summary</h2>" xsltproc.out.${n} >/dev/null || ret=1
    grep "<h2>Memory Contexts</h2>" xsltproc.out.${n} >/dev/null || ret=1
else
    echo_i "skipping test as libxml2 and/or curl and/or xsltproc was not found"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "checking bind9.xml socket statistics ($n)"
if $FEATURETEST --have-libxml2 && [ -x "${CURL}" ] && [ -x "${XSLTPROC}" ]  ; then
    # Socket statistics (expect no errors)
    grep "<counter name=\"TCP4AcceptFail\">0</counter>" stats.xml.out >/dev/null || ret=1
    grep "<counter name=\"TCP4BindFail\">0</counter>" stats.xml.out >/dev/null || ret=1
    grep "<counter name=\"TCP4ConnFail\">0</counter>" stats.xml.out >/dev/null || ret=1
    grep "<counter name=\"TCP4OpenFail\">0</counter>" stats.xml.out >/dev/null || ret=1
    grep "<counter name=\"TCP4RecvErr\">0</counter>" stats.xml.out >/dev/null || ret=1
    grep "<counter name=\"TCP4SendErr\">0</counter>" stats.xml.out >/dev/null || ret=1

    grep "<counter name=\"TCP6AcceptFail\">0</counter>" stats.xml.out >/dev/null || ret=1
    grep "<counter name=\"TCP6BindFail\">0</counter>" stats.xml.out >/dev/null || ret=1
    grep "<counter name=\"TCP6ConnFail\">0</counter>" stats.xml.out >/dev/null || ret=1
    grep "<counter name=\"TCP6OpenFail\">0</counter>" stats.xml.out >/dev/null || ret=1
    grep "<counter name=\"TCP6RecvErr\">0</counter>" stats.xml.out >/dev/null || ret=1
    grep "<counter name=\"TCP6SendErr\">0</counter>" stats.xml.out >/dev/null || ret=1
else
    echo_i "skipping test as libxml2 and/or curl and/or xsltproc was not found"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "Check that 'zone-statistics full;' is processed by 'rndc reconfig' ($n)"
ret=0
# off by default
rndc_stats ns2 10.53.0.2 || ret=1
sed -n '/Per Zone Query Statistics/,/^++/p' $last_stats | grep -F '[example]' > /dev/null && ret=0
# turn on
copy_setports ns2/named2.conf.in ns2/named.conf
rndc_reconfig ns2 10.53.0.2
rndc_stats ns2 10.53.0.2 || ret=1
sed -n '/Per Zone Query Statistics/,/^++/p' $last_stats | grep -F '[example]' > /dev/null || ret=1
# turn off
copy_setports ns2/named.conf.in ns2/named.conf
rndc_reconfig ns2 10.53.0.2
rndc_stats ns2 10.53.0.2 || ret=1
sed -n '/Per Zone Query Statistics/,/^++/p' $last_stats | grep -F '[example]' > /dev/null && ret=0
# turn on
copy_setports ns2/named2.conf.in ns2/named.conf
rndc_reconfig ns2 10.53.0.2
rndc_stats ns2 10.53.0.2 || ret=1
sed -n '/Per Zone Query Statistics/,/^++/p' $last_stats | grep -F '[example]' > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
