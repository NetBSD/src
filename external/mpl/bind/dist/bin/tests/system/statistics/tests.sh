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

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd"
DIGCMD="$DIG $DIGOPTS -p ${PORT}"
RNDCCMD="$RNDC -p ${CONTROLPORT} -c ../common/rndc.conf"

status=0

ret=0
n=1

echo_i "fetching a.example from ns2's initial configuration ($n)"
$DIGCMD +noauth a.example. @10.53.0.2 any > dig.out.ns2.1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "dumping initial stats for ns2 ($n)"
$RNDCCMD -s 10.53.0.2 stats > /dev/null 2>&1
[ -f ns2/named.stats ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "verifying adb records in named.stats ($n)"
grep "ADB stats" ns2/named.stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "checking for 1 entry in adb hash table in named.stats ($n)"
grep "1 Addresses in hash table" ns2/named.stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "verifying cache statistics in named.stats ($n)"
grep "Cache Statistics" ns2/named.stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "checking for 2 entries in adb hash table in named.stats ($n)"
$DIGCMD a.example.info. @10.53.0.2 any > /dev/null 2>&1
$RNDCCMD -s 10.53.0.2 stats > /dev/null 2>&1
grep "2 Addresses in hash table" ns2/named.stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "dumping initial stats for ns3 ($n)"
rm -f ns3/named.stats
$RNDCCMD -s 10.53.0.3 stats > /dev/null 2>&1
[ -f ns3/named.stats ] || ret=1
if [ ! "$CYGWIN" ]; then
    nsock0nstat=`grep "UDP/IPv4 sockets active" ns3/named.stats | awk '{print $1}'`
    [ 0 -ne ${nsock0nstat:-0} ] || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "sending queries to ns3"
$DIGCMD +tries=2 +time=1 +recurse @10.53.0.3 foo.info. any > /dev/null 2>&1

ret=0
echo_i "dumping updated stats for ns3 ($n)"
rm -f ns3/named.stats
$RNDCCMD -s 10.53.0.3 stats > /dev/null 2>&1
[ -f ns3/named.stats ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "verifying recursing clients output in named.stats ($n)"
grep "2 recursing clients" ns3/named.stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "verifying active fetches output in named.stats ($n)"
grep "1 active fetches" ns3/named.stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

if [ ! "$CYGWIN" ]; then
    ret=0
    echo_i "verifying active sockets output in named.stats ($n)"
    nsock1nstat=`grep "UDP/IPv4 sockets active" ns3/named.stats | awk '{print $1}'`
    [ `expr ${nsock1nstat:-0} - ${nsock0nstat:-0}` -eq 1 ] || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
    n=`expr $n + 1`
fi

# there should be 1 UDP and no TCP queries.  As the TCP counter is zero
# no status line is emitted.
ret=0
echo_i "verifying queries in progress in named.stats ($n)"
grep "1 UDP queries in progress" ns3/named.stats > /dev/null || ret=1
grep "TCP queries in progress" ns3/named.stats > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "verifying bucket size output ($n)"
grep "bucket size" ns3/named.stats > /dev/null || ret=1
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
echo_i "checking priming queries are counted ($n)"
grep "1 priming queries" ns3/named.stats
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
