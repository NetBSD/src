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
# shellcheck source=conf.sh
. "$SYSTEMTESTTOP/conf.sh"

DIGCMD="$DIG @10.53.0.2 -p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

if [ ! "$HAVEJSONSTATS" ]
then
    unset PERL_JSON
    echo_i "JSON was not configured; skipping" >&2
elif $PERL -e 'use JSON;' 2>/dev/null
then
    PERL_JSON=1
else
    unset PERL_JSON
    echo_i "JSON tests require JSON library; skipping" >&2
fi

if [ ! "$HAVEXMLSTATS" ]
then
    unset PERL_XML
    echo_i "XML was not configured; skipping" >&2
elif $PERL -e 'use XML::Simple;' 2>/dev/null
then
    PERL_XML=1
else
    unset PERL_XML
    echo_i "XML tests require XML::Simple; skipping" >&2
fi

if [ ! "$PERL_JSON" -a ! "$PERL_XML" ]; then
    echo_i "skipping all tests"
    exit 0
fi


getzones() {
    sleep 1
    echo_i "... using $1"
    case $1 in
        xml) path='xml/v3/zones' ;;
        json) path='json/v1/zones' ;;
        *) return 1 ;;
    esac
    file=`$PERL fetch.pl -p ${EXTRAPORT1} $path`
    cp $file $file.$1.$3
    $PERL zones-${1}.pl $file $2 2>/dev/null | sort > zones.out.$3
    result=$?
    return $result
}

# TODO: Move loadkeys_on to conf.sh.common
loadkeys_on() {
    nsidx=$1
    zone=$2
    nextpart ns${nsidx}/named.run > /dev/null
    $RNDCCMD 10.53.0.${nsidx} loadkeys ${zone} | sed "s/^/ns${nsidx} /" | cat_i
    wait_for_log 20 "next key event" ns${nsidx}/named.run
}

status=0
n=1
ret=0
echo_i "checking consistency between named.stats and xml/json ($n)"
rm -f ns2/named.stats
$DIGCMD +tcp example ns > dig.out.$n || ret=1
$RNDCCMD 10.53.0.2 stats 2>&1 | sed 's/^/I:ns1 /'
query_count=`awk '/QUERY/ {print $1}' ns2/named.stats`
txt_count=`awk '/TXT/ {print $1}' ns2/named.stats`
noerror_count=`awk '/NOERROR/ {print $1}' ns2/named.stats`
if [ $PERL_XML ]; then
    file=`$PERL fetch.pl -p ${EXTRAPORT1} xml/v3/server`
    mv $file xml.stats
    $PERL server-xml.pl > xml.fmtstats 2> /dev/null
    xml_query_count=`awk '/opcode QUERY/ { print $NF }' xml.fmtstats`
    xml_query_count=${xml_query_count:-0}
    [ "$query_count" -eq "$xml_query_count" ] || ret=1
    xml_txt_count=`awk '/qtype TXT/ { print $NF }' xml.fmtstats`
    xml_txt_count=${xml_txt_count:-0}
    [ "$txt_count" -eq "$xml_txt_count" ] || ret=1
    xml_noerror_count=`awk '/rcode NOERROR/ { print $NF }' xml.fmtstats`
    xml_noerror_count=${xml_noerror_count:-0}
    [ "$noerror_count" -eq "$xml_noerror_count" ] || ret=1
fi
if [ $PERL_JSON ]; then
    file=`$PERL fetch.pl -p ${EXTRAPORT1} json/v1/server`
    mv $file json.stats
    $PERL server-json.pl > json.fmtstats 2> /dev/null
    json_query_count=`awk '/opcode QUERY/ { print $NF }' json.fmtstats`
    json_query_count=${json_query_count:-0}
    [ "$query_count" -eq "$json_query_count" ] || ret=1
    json_txt_count=`awk '/qtype TXT/ { print $NF }' json.fmtstats`
    json_txt_count=${json_txt_count:-0}
    [ "$txt_count" -eq "$json_txt_count" ] || ret=1
    json_noerror_count=`awk '/rcode NOERROR/ { print $NF }' json.fmtstats`
    json_noerror_count=${json_noerror_count:-0}
    [ "$noerror_count" -eq "$json_noerror_count" ] || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "checking malloced memory statistics xml/json ($n)"
if [ $PERL_XML ]; then
    file=`$PERL fetch.pl -p ${EXTRAPORT1} xml/v3/mem`
    mv $file xml.mem
    $PERL mem-xml.pl $file > xml.fmtmem
    grep "'Malloced' => '[0-9][0-9]*'" xml.fmtmem > /dev/null || ret=1
    grep "'malloced' => '[0-9][0-9]*'" xml.fmtmem > /dev/null || ret=1
    grep "'maxmalloced' => '[0-9][0-9]*'" xml.fmtmem > /dev/null || ret=1
fi
if [ $PERL_JSON ]; then
    file=`$PERL fetch.pl -p ${EXTRAPORT1} json/v1/mem`
    mv $file json.mem
    grep '"malloced":[0-9][0-9]*,' json.mem > /dev/null || ret=1
    grep '"maxmalloced":[0-9][0-9]*,' json.mem > /dev/null || ret=1
    grep '"Malloced":[0-9][0-9]*,' json.mem > /dev/null || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "checking consistency between regular and compressed output ($n)"
for i in 1 2 3 4 5; do
	ret=0
	if [ "$HAVEXMLSTATS" ];
	then
		URL=http://10.53.0.2:${EXTRAPORT1}/xml/v3/server
		filter_str='s#<current-time>.*</current-time>##g'
	else
		URL=http://10.53.0.2:${EXTRAPORT1}/json/v1/server
		filter_str='s#"current-time.*",##g'
	fi
	$CURL -D regular.headers $URL 2>/dev/null | \
		sed -e "$filter_str" > regular.out
	$CURL -D compressed.headers --compressed $URL 2>/dev/null | \
		sed -e "$filter_str" > compressed.out
	diff regular.out compressed.out >/dev/null || ret=1
	if [ $ret != 0 ]; then
		echo_i "failed on try $i, probably a timing issue, trying again"
		sleep 1
	else
		break
	fi
done

status=`expr $status + $ret`
n=`expr $n + 1`

ret=0
echo_i "checking if compressed output is really compressed ($n)"
if [ "$HAVEZLIB" ];
then
    REGSIZE=`cat regular.headers | \
	grep -i Content-Length | sed -e "s/.*: \([0-9]*\).*/\1/"`
    COMPSIZE=`cat compressed.headers | \
	grep -i Content-Length | sed -e "s/.*: \([0-9]*\).*/\1/"`
    if [ ! `expr $REGSIZE / $COMPSIZE` -gt 2 ]; then
	ret=1
    fi
else
    echo_i "skipped"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

# Test dnssec sign statistics.
zone="dnssec"
sign_prefix="dnssec-sign operations"
refresh_prefix="dnssec-refresh operations"
ksk_id=`cat ns2/$zone.ksk.id`
zsk_id=`cat ns2/$zone.zsk.id`

# 1. Test sign operations for scheduled resigning.
ret=0
# The dnssec zone has 10 RRsets to sign (including NSEC) with the ZSK and one
# RRset (DNSKEY) with the KSK. So starting named with signatures that expire
# almost right away, this should trigger 10 zsk and 1 ksk sign operations.
# However, the DNSSEC maintenance assumes when we see the SOA record we have
# walked the whole zone, since the SOA record should always have the most
# recent signature.
echo "${refresh_prefix} ${zsk_id}: 10" > zones.expect
echo "${refresh_prefix} ${ksk_id}: 1" >> zones.expect
echo "${sign_prefix} ${zsk_id}: 10" >> zones.expect
echo "${sign_prefix} ${ksk_id}: 1" >> zones.expect
cat zones.expect | sort > zones.expect.$n
rm -f zones.expect
# Fetch and check the dnssec sign statistics.
echo_i "fetching zone stats data after zone maintenance at startup ($n)"
if [ $PERL_XML ]; then
    getzones xml $zone x$n || ret=1
    cmp zones.out.x$n zones.expect.$n || ret=1
fi
if [ $PERL_JSON ]; then
    getzones json $zone j$n || ret=1
    cmp zones.out.j$n zones.expect.$n || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

# 2. Test sign operations after dynamic update.
ret=0
(
# Update dnssec zone to trigger signature creation.
echo zone $zone
echo server 10.53.0.2 "$PORT"
echo update add $zone. 300 in txt "nsupdate added me"
echo send
) | $NSUPDATE
# This should trigger the resign of SOA, TXT and NSEC (+3 zsk).
echo "${refresh_prefix} ${zsk_id}: 10" > zones.expect
echo "${refresh_prefix} ${ksk_id}: 1" >> zones.expect
echo "${sign_prefix} ${zsk_id}: 13" >> zones.expect
echo "${sign_prefix} ${ksk_id}: 1" >> zones.expect
cat zones.expect | sort > zones.expect.$n
rm -f zones.expect
# Fetch and check the dnssec sign statistics.
echo_i "fetching zone stats data after dynamic update ($n)"
if [ $PERL_XML ]; then
    getzones xml $zone x$n || ret=1
    cmp zones.out.x$n zones.expect.$n || ret=1
fi
if [ $PERL_JSON ]; then
    getzones json $zone j$n || ret=1
    cmp zones.out.j$n zones.expect.$n || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

# 3. Test sign operations of KSK.
ret=0
echo_i "fetch zone stats data after updating DNSKEY RRset ($n)"
# Add a standby DNSKEY, this triggers resigning the DNSKEY RRset.
zsk=$("$KEYGEN" -K ns2 -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
$SETTIME -K ns2 -P now -A never $zsk.key > /dev/null
loadkeys_on 2 $zone || ret=1
# This should trigger the resign of SOA (+1 zsk) and DNSKEY (+1 ksk).
echo "${refresh_prefix} ${zsk_id}: 11" > zones.expect
echo "${refresh_prefix} ${ksk_id}: 2" >> zones.expect
echo "${sign_prefix} ${zsk_id}: 14" >> zones.expect
echo "${sign_prefix} ${ksk_id}: 2" >> zones.expect
cat zones.expect | sort > zones.expect.$n
rm -f zones.expect
# Fetch and check the dnssec sign statistics.
if [ $PERL_XML ]; then
    getzones xml $zone x$n || ret=1
    cmp zones.out.x$n zones.expect.$n || ret=1
fi
if [ $PERL_JSON ]; then
    getzones json $zone j$n || ret=1
    cmp zones.out.j$n zones.expect.$n || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
