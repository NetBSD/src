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

set -e

# shellcheck source=conf.sh
. ../conf.sh

DIGCMD="$DIG @10.53.0.2 -p ${PORT}"
RNDCCMD="$RNDC -c ../_common/rndc.conf -p ${CONTROLPORT} -s"
NS_PARAMS="-m record -c named.conf -d 99 -g -T maxcachesize=2097152"

if ! $FEATURETEST --have-json-c; then
  unset PERL_JSON
  echo_i "JSON was not configured; skipping" >&2
elif $PERL -e 'use JSON;' 2>/dev/null; then
  PERL_JSON=1
else
  unset PERL_JSON
  echo_i "JSON tests require JSON library; skipping" >&2
fi

if ! $FEATURETEST --have-libxml2; then
  unset PERL_XML
  echo_i "XML was not configured; skipping" >&2
elif $PERL -e 'use XML::Simple;' 2>/dev/null; then
  PERL_XML=1
else
  unset PERL_XML
  echo_i "XML tests require XML::Simple; skipping" >&2
fi

if [ ! "$PERL_JSON" ] && [ ! "$PERL_XML" ]; then
  echo_i "skipping all tests"
  exit 0
fi

retry_quiet_fast() {
  __retries="${1}"
  shift

  while :; do
    if "$@"; then
      return 0
    fi
    __retries=$((__retries - 1))
    if [ "${__retries}" -gt 0 ]; then
      # sleep for 0.1 seconds
      perl -e 'select(undef, undef, undef, .1)'
    else
      return 1
    fi
  done
}

wait_for_log_fast() (
  timeout="$1"
  msg="$2"
  file="$3"
  retry_quiet_fast "$timeout" _search_log "$msg" "$file" && return 0
  echo_i "exceeded time limit waiting for literal '$msg' in $file"
  return 1
)

getzones() {
  sleep 1
  echo_i "... using $1"
  case $1 in
    xml) path='xml/v3/zones' ;;
    json) path='json/v1/zones' ;;
    *) return 1 ;;
  esac
  file=$($PERL fetch.pl -p ${EXTRAPORT1} $path)
  cp $file $file.$1.$3
  {
    $PERL zones-${1}.pl $file $2 2>/dev/null | sort >zones.out.$3
    result=$?
  } || true
  return $result
}

getxfrins() {
  echo_i "... using $1"
  case $1 in
    xml) path='xml/v3/xfrins' ;;
    json) path='json/v1/xfrins' ;;
    *) return 1 ;;
  esac
  file=$($PERL fetch.pl -s 10.53.0.3 -p ${EXTRAPORT1} $path)
  cp $file $file.$1.$2
  result=$?
  return $result
}

# TODO: Move loadkeys_on to conf.sh.common
loadkeys_on() {
  nsidx=$1
  zone=$2
  nextpart ns${nsidx}/named.run >/dev/null
  $RNDCCMD 10.53.0.${nsidx} loadkeys ${zone} | sed "s/^/ns${nsidx} /" | cat_i
  wait_for_log 20 "next key event" ns${nsidx}/named.run
}

# verify that the http server dropped the connection without replying
check_http_dropped() {
  if [ -x "${NC}" ]; then
    "${NC}" 10.53.0.3 "${EXTRAPORT1}" >nc.out$n || ret=1
    if test -s nc.out$n; then
      ret=1
    fi
  else
    echo_i "skipping test as nc not found"
  fi
}

status=0
n=1

echo_i "check content-length parse error ($n)"
ret=0
check_http_dropped <<EOF
POST /xml/v3/status HTTP/1.0
Content-Length: nah

EOF
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "check negative content-length ($n)"
ret=0
check_http_dropped <<EOF
POST /xml/v3/status HTTP/1.0
Content-Length: -50

EOF
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "check content-length 32-bit overflow ($n)"
check_http_dropped <<EOF
POST /xml/v3/status HTTP/1.0
Content-Length: 4294967239

EOF
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "check content-length 64-bit overflow ($n)"
check_http_dropped <<EOF
POST /xml/v3/status HTTP/1.0
Content-Length: 18446744073709551549

EOF

if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "Prepare for if-modified-since test ($n)"
ret=0
i=0
if $FEATURETEST --have-libxml2 && [ -x "${CURL}" ]; then
  URL="http://10.53.0.3:${EXTRAPORT1}/bind9.xsl"
  ${CURL} --silent --show-error --fail --output bind9.xsl.1 $URL
  ret=$?
else
  echo_i "skipping test: requires libxml2 and curl"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "checking consistency between named.stats and xml/json ($n)"
ret=0
rm -f ns2/named.stats
$DIGCMD +tcp example ns >dig.out.$n || ret=1
$RNDCCMD 10.53.0.2 stats 2>&1 | sed 's/^/I:ns1 /'
query_count=$(awk '/QUERY/ {print $1}' ns2/named.stats)
txt_count=$(awk '/TXT/ {print $1}' ns2/named.stats)
noerror_count=$(awk '/NOERROR/ {print $1}' ns2/named.stats)
if [ "$PERL_XML" ]; then
  file=$($PERL fetch.pl -p ${EXTRAPORT1} xml/v3/server)
  mv $file xml.stats
  $PERL server-xml.pl >xml.fmtstats 2>/dev/null
  xml_query_count=$(awk '/opcode QUERY/ { print $NF }' xml.fmtstats)
  xml_query_count=${xml_query_count:-0}
  [ "$query_count" -eq "$xml_query_count" ] || ret=1
  xml_txt_count=$(awk '/qtype TXT/ { print $NF }' xml.fmtstats)
  xml_txt_count=${xml_txt_count:-0}
  [ "$txt_count" -eq "$xml_txt_count" ] || ret=1
  xml_noerror_count=$(awk '/rcode NOERROR/ { print $NF }' xml.fmtstats)
  xml_noerror_count=${xml_noerror_count:-0}
  [ "$noerror_count" -eq "$xml_noerror_count" ] || ret=1
fi
if [ "$PERL_JSON" ]; then
  file=$($PERL fetch.pl -p ${EXTRAPORT1} json/v1/server)
  mv $file json.stats
  $PERL server-json.pl >json.fmtstats 2>/dev/null
  json_query_count=$(awk '/opcode QUERY/ { print $NF }' json.fmtstats)
  json_query_count=${json_query_count:-0}
  [ "$query_count" -eq "$json_query_count" ] || ret=1
  json_txt_count=$(awk '/qtype TXT/ { print $NF }' json.fmtstats)
  json_txt_count=${json_txt_count:-0}
  [ "$txt_count" -eq "$json_txt_count" ] || ret=1
  json_noerror_count=$(awk '/rcode NOERROR/ { print $NF }' json.fmtstats)
  json_noerror_count=${json_noerror_count:-0}
  [ "$noerror_count" -eq "$json_noerror_count" ] || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

ret=0
echo_i "checking malloced memory statistics xml/json ($n)"
if [ "$PERL_XML" ]; then
  file=$($PERL fetch.pl -p ${EXTRAPORT1} xml/v3/mem)
  mv $file xml.mem
  $PERL mem-xml.pl $file >xml.fmtmem
  grep "'InUse' => '[0-9][0-9]*'" xml.fmtmem >/dev/null || ret=1
  grep "'inuse' => '[0-9][0-9]*'" xml.fmtmem >/dev/null || ret=1
fi
if [ "$PERL_JSON" ]; then
  file=$($PERL fetch.pl -p ${EXTRAPORT1} json/v1/mem)
  mv $file json.mem
  grep '"inuse":[0-9][0-9]*,' json.mem >/dev/null || ret=1
  grep '"InUse":[0-9][0-9]*,' json.mem >/dev/null || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "checking consistency between regular and compressed output ($n)"
ret=0
if [ -x "${CURL}" ]; then
  for i in 1 2 3 4 5; do
    ret=0
    if $FEATURETEST --have-libxml2; then
      URL="http://10.53.0.2:${EXTRAPORT1}/xml/v3/server"
      filter_str='s#<current-time>.*</current-time>##g'
    else
      URL="http://10.53.0.2:${EXTRAPORT1}/json/v1/server"
      filter_str='s#"current-time.*",##g'
    fi
    "${CURL}" -D regular.headers "$URL" 2>/dev/null \
      | sed -e "$filter_str" >regular.out || ret=1
    "${CURL}" -D compressed.headers --compressed "$URL" 2>/dev/null \
      | sed -e "$filter_str" >compressed.out || ret=1
    diff regular.out compressed.out >/dev/null || ret=1
    if [ $ret != 0 ]; then
      echo_i "failed on try $i, probably a timing issue, trying again"
      sleep 1
    else
      break
    fi
  done
else
  echo_i "skipping test as curl not found"
fi
status=$((status + ret))
n=$((n + 1))

ret=0
echo_i "checking if compressed output is really compressed ($n)"
if $FEATURETEST --with-zlib; then
  REGSIZE=$(cat regular.headers \
    | grep -i Content-Length | sed -e "s/.*: \([0-9]*\).*/\1/")
  COMPSIZE=$(cat compressed.headers \
    | grep -i Content-Length | sed -e "s/.*: \([0-9]*\).*/\1/")
  if [ ! $((REGSIZE / COMPSIZE)) -gt 2 ]; then
    ret=1
  fi
else
  echo_i "skipped"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

# Test dnssec sign statistics.
zone="dnssec"
sign_prefix="dnssec-sign operations"
refresh_prefix="dnssec-refresh operations"
ksk_id=$(cat ns2/$zone.ksk.id)
zsk_id=$(cat ns2/$zone.zsk.id)

# Test sign operations for scheduled resigning.
ret=0
# The dnssec zone has 10 RRsets to sign (including NSEC) with the ZSK and one
# RRset (DNSKEY) with the KSK. So starting named with signatures that expire
# almost right away, this should trigger 10 zsk and 1 ksk sign operations.
echo "${refresh_prefix} ${zsk_id}: 10" >zones.expect
echo "${refresh_prefix} ${ksk_id}: 1" >>zones.expect
echo "${sign_prefix} ${zsk_id}: 10" >>zones.expect
echo "${sign_prefix} ${ksk_id}: 1" >>zones.expect
cat zones.expect | sort >zones.expect.$n
rm -f zones.expect
# Fetch and check the dnssec sign statistics.
echo_i "fetching zone '$zone' stats data after zone maintenance at startup ($n)"
if [ "$PERL_XML" ]; then
  getzones xml $zone x$n || ret=1
  cmp zones.out.x$n zones.expect.$n || ret=1
fi
if [ "$PERL_JSON" ]; then
  getzones json 0 j$n || ret=1
  cmp zones.out.j$n zones.expect.$n || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

# Test sign operations after dynamic update.
ret=0
(
  # Update dnssec zone to trigger signature creation.
  echo zone $zone
  echo server 10.53.0.2 "$PORT"
  echo update add $zone. 300 in txt "nsupdate added me"
  echo send
) | $NSUPDATE
# This should trigger the resign of SOA, TXT and NSEC (+3 zsk).
echo "${refresh_prefix} ${zsk_id}: 10" >zones.expect
echo "${refresh_prefix} ${ksk_id}: 1" >>zones.expect
echo "${sign_prefix} ${zsk_id}: 13" >>zones.expect
echo "${sign_prefix} ${ksk_id}: 1" >>zones.expect
cat zones.expect | sort >zones.expect.$n
rm -f zones.expect
# Fetch and check the dnssec sign statistics.
echo_i "fetching zone '$zone' stats data after dynamic update ($n)"
if [ "$PERL_XML" ]; then
  getzones xml $zone x$n || ret=1
  cmp zones.out.x$n zones.expect.$n || ret=1
fi
if [ "$PERL_JSON" ]; then
  getzones json 0 j$n || ret=1
  cmp zones.out.j$n zones.expect.$n || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

# Test sign operations of KSK.
ret=0
echo_i "fetch zone '$zone' stats data after updating DNSKEY RRset ($n)"
id=$(echo "${zsk_id}" | cut -d+ -f2 -)
# Add a DNSKEY, this triggers resigning the DNSKEY RRset.
zsk=$("$KEYGEN" -L 3600 -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
(
  echo zone $zone
  echo server 10.53.0.2 "$PORT"
  echo update add $(cat "${zsk}.key" | grep -v ";.*")
  echo send
) | $NSUPDATE
# This should trigger the resign of SOA (+1 zsk) and DNSKEY (+1 ksk).
echo "${refresh_prefix} ${zsk_id}: 10" >zones.expect
echo "${refresh_prefix} ${ksk_id}: 1" >>zones.expect
echo "${sign_prefix} ${zsk_id}: 14" >>zones.expect
echo "${sign_prefix} ${ksk_id}: 2" >>zones.expect
cat zones.expect | sort >zones.expect.$n
rm -f zones.expect
# Fetch and check the dnssec sign statistics.
if [ "$PERL_XML" ]; then
  getzones xml $zone x$n || ret=1
  cmp zones.out.x$n zones.expect.$n || ret=1
fi
if [ "$PERL_JSON" ]; then
  getzones json 0 j$n || ret=1
  cmp zones.out.j$n zones.expect.$n || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

# Test sign operations for scheduled resigning (many keys).
ret=0
zone="manykeys"
ksk8_id=$(cat ns2/$zone.ksk8.id)
zsk8_id=$(cat ns2/$zone.zsk8.id)
ksk13_id=$(cat ns2/$zone.ksk13.id)
zsk13_id=$(cat ns2/$zone.zsk13.id)
ksk14_id=$(cat ns2/$zone.ksk14.id)
zsk14_id=$(cat ns2/$zone.zsk14.id)
# The dnssec zone has 10 RRsets to sign (including NSEC) with the ZSKs and the
# DNSKEY, CDS, and CDNSKEY RRsets with the KSKs. So starting named with
# signatures that expire almost right away, this should trigger 10 zsk and 3
# ksk sign operations per key.
echo "${refresh_prefix} ${zsk8_id}: 10" >zones.expect
echo "${refresh_prefix} ${zsk13_id}: 10" >>zones.expect
echo "${refresh_prefix} ${zsk14_id}: 10" >>zones.expect
echo "${refresh_prefix} ${ksk8_id}: 3" >>zones.expect
echo "${refresh_prefix} ${ksk13_id}: 3" >>zones.expect
echo "${refresh_prefix} ${ksk14_id}: 3" >>zones.expect
echo "${sign_prefix} ${zsk8_id}: 10" >>zones.expect
echo "${sign_prefix} ${zsk13_id}: 10" >>zones.expect
echo "${sign_prefix} ${zsk14_id}: 10" >>zones.expect
echo "${sign_prefix} ${ksk8_id}: 3" >>zones.expect
echo "${sign_prefix} ${ksk13_id}: 3" >>zones.expect
echo "${sign_prefix} ${ksk14_id}: 3" >>zones.expect
cat zones.expect | sort >zones.expect.$n
rm -f zones.expect
# Fetch and check the dnssec sign statistics.
echo_i "fetching zone '$zone' stats data after zone maintenance at startup ($n)"
if [ "$PERL_XML" ]; then
  getzones xml $zone x$n || ret=1
  cmp zones.out.x$n zones.expect.$n || ret=1
fi
if [ "$PERL_JSON" ]; then
  getzones json 2 j$n || ret=1
  cmp zones.out.j$n zones.expect.$n || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

# Test sign operations after dynamic update (many keys).
ret=0
(
  # Update dnssec zone to trigger signature creation.
  echo zone $zone
  echo server 10.53.0.2 "$PORT"
  echo update add $zone. 300 in txt "nsupdate added me"
  echo send
) | $NSUPDATE
# This should trigger the resign of SOA, TXT and NSEC (+3 zsk).
echo "${refresh_prefix} ${zsk8_id}: 10" >zones.expect
echo "${refresh_prefix} ${zsk13_id}: 10" >>zones.expect
echo "${refresh_prefix} ${zsk14_id}: 10" >>zones.expect
echo "${refresh_prefix} ${ksk8_id}: 3" >>zones.expect
echo "${refresh_prefix} ${ksk13_id}: 3" >>zones.expect
echo "${refresh_prefix} ${ksk14_id}: 3" >>zones.expect
echo "${sign_prefix} ${zsk8_id}: 13" >>zones.expect
echo "${sign_prefix} ${zsk13_id}: 13" >>zones.expect
echo "${sign_prefix} ${zsk14_id}: 13" >>zones.expect
echo "${sign_prefix} ${ksk8_id}: 3" >>zones.expect
echo "${sign_prefix} ${ksk13_id}: 3" >>zones.expect
echo "${sign_prefix} ${ksk14_id}: 3" >>zones.expect
cat zones.expect | sort >zones.expect.$n
rm -f zones.expect
# Fetch and check the dnssec sign statistics.
echo_i "fetching zone '$zone' stats data after dynamic update ($n)"
if [ "$PERL_XML" ]; then
  getzones xml $zone x$n || ret=1
  cmp zones.out.x$n zones.expect.$n || ret=1
fi
if [ "$PERL_JSON" ]; then
  getzones json 2 j$n || ret=1
  cmp zones.out.j$n zones.expect.$n || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

# Test sign operations after dnssec-policy change (removing keys).
ret=0
cp ns2/named2.conf ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/I:ns2 /'
# This should trigger the resign of DNSKEY, CDS, and CDNSKEY (+3 ksk),
# and SOA, NSEC, TYPE65534 (+3 zsk). The dnssec-sign statistics for the
# removed keys should be cleared and thus no longer visible. But NSEC and SOA
# are (mistakenly) counted double, one time because of zone_resigninc and one
# time because of zone_nsec3chain. So +5 zsk in total.
echo "${refresh_prefix} ${zsk8_id}: 15" >zones.expect
echo "${refresh_prefix} ${ksk8_id}: 6" >>zones.expect
echo "${sign_prefix} ${zsk8_id}: 18" >>zones.expect
echo "${sign_prefix} ${ksk8_id}: 6" >>zones.expect
cat zones.expect | sort >zones.expect.$n
rm -f zones.expect
# Fetch and check the dnssec sign statistics.
echo_i "fetching zone '$zone' stats data after dnssec-policy change ($n)"
if [ "$PERL_XML" ]; then
  getzones xml $zone x$n || ret=1
  cmp zones.out.x$n zones.expect.$n || ret=1
fi
if [ "$PERL_JSON" ]; then
  getzones json 2 j$n || ret=1
  cmp zones.out.j$n zones.expect.$n || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "Check HTTP/1.1 client-side pipelined requests are handled (GET) ($n)"
ret=0
if [ -x "${NC}" ]; then
  "${NC}" 10.53.0.3 "${EXTRAPORT1}" <<EOF >nc.out$n || ret=1
GET /xml/v3/status HTTP/1.1
Host: 10.53.0.3:${EXTRAPORT1}

GET /xml/v3/status HTTP/1.1
Host: 10.53.0.3:${EXTRAPORT1}
Connection: close

EOF
  lines=$(grep -c "^<statistics version" nc.out$n)
  test "$lines" = 2 || ret=1
  # keep-alive not needed in HTTP/1.1, second response has close
  lines=$(grep -c "^Connection: Keep-Alive" nc.out$n || true)
  test "$lines" = 0 || ret=1
  lines=$(grep -c "^Connection: close" nc.out$n)
  test "$lines" = 1 || ret=1
else
  echo_i "skipping test as nc not found"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "Check HTTP/1.1 client-side pipelined requests are handled (POST) ($n)"
ret=0
if [ -x "${NC}" ]; then
  "${NC}" 10.53.0.3 "${EXTRAPORT1}" <<EOF >nc.out$n || ret=1
POST /xml/v3/status HTTP/1.1
Host: 10.53.0.3:${EXTRAPORT1}
Content-Type: application/json
Content-Length: 3

{}
POST /xml/v3/status HTTP/1.1
Host: 10.53.0.3:${EXTRAPORT1}
Content-Type: application/json
Content-Length: 3
Connection: close

{}
EOF
  lines=$(grep -c "^<statistics version" nc.out$n)
  test "$lines" = 2 || ret=1
  # keep-alive not needed in HTTP/1.1, second response has close
  lines=$(grep -c "^Connection: Keep-Alive" nc.out$n || true)
  test "$lines" = 0 || ret=1
  lines=$(grep -c "^Connection: close" nc.out$n)
  test "$lines" = 1 || ret=1
else
  echo_i "skipping test as nc not found"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "Check HTTP/1.0 keep-alive ($n)"
ret=0
if [ -x "${NC}" ]; then
  "${NC}" 10.53.0.3 "${EXTRAPORT1}" <<EOF >nc.out$n || ret=1
GET /xml/v3/status HTTP/1.0
Connection: keep-alive

GET /xml/v3/status HTTP/1.0

EOF
  # should be two responses
  lines=$(grep -c "^<statistics version" nc.out$n)
  test "$lines" = 2 || ret=1
  # first response has keep-alive, second has close
  lines=$(grep -c "^Connection: Keep-Alive" nc.out$n || true)
  test "$lines" = 1 || ret=1
  lines=$(grep -c "^Connection: close" nc.out$n)
  test "$lines" = 1 || ret=1
else
  echo_i "skipping test as nc not found"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "Check inconsistent Connection: headers ($n)"
ret=0
if [ -x "${NC}" ]; then
  "${NC}" 10.53.0.3 "${EXTRAPORT1}" <<EOF >nc.out$n || ret=1
GET /xml/v3/status HTTP/1.0
Connection: keep-alive
Connection: close

GET /xml/v3/status HTTP/1.0

EOF
  # should be one response (second is ignored)
  lines=$(grep -c "^<statistics version" nc.out$n)
  test "$lines" = 1 || ret=1
  # no keep-alive, one close
  lines=$(grep -c "^Connection: Keep-Alive" nc.out$n || true)
  test "$lines" = 0 || ret=1
  lines=$(grep -c "^Connection: close" nc.out$n)
  test "$lines" = 1 || ret=1
else
  echo_i "skipping test as nc not found"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

if [ -x "${CURL}" ] && ! ("${CURL}" --next 2>&1 | grep 'option --next: is unknown'); then
  CURL_NEXT="${CURL}"
fi

echo_i "Check HTTP with more than 100 headers ($n)"
ret=0
i=0
if [ -x "${CURL_NEXT}" ]; then
  # build input stream.
  : >header.in$n
  while test $i -lt 101; do
    printf 'X-Bloat%d: VGhlIG1vc3QgY29tbW9uIHJlYXNvbiBmb3IgYmxvYXRpbmcgaXMgaGF2aW5nIGEgbG90IG9mIGdhcyBpbiB5b3VyIGd1dC4gCg==\r\n' $i >>header.in$n
    i=$((i + 1))
  done
  printf '\r\n' >>header.in$n

  # send the requests then wait for named to close the socket.
  URL="http://10.53.0.3:${EXTRAPORT1}/xml/v3/status"
  "${CURL}" --silent --include --get "$URL" --next --get --header @header.in$n "$URL" >curl.out$n && ret=1
  # we expect 1 request to be processed.
  lines=$(grep -c "^<statistics version" curl.out$n)
  test "$lines" = 1 || ret=1
else
  echo_i "skipping test as curl with --next support not found"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "Check HTTP/1.1 keep-alive with truncated stream ($n)"
ret=0
i=0
if [ -x "${CURL_NEXT}" ]; then
  # build input stream.
  printf 'X-Bloat: ' >header.in$n
  # curl 8.13 and newer rejects to read line larger than 100KB
  while test $i -lt 1023; do
    printf '%s' "VGhlIG1vc3QgY29tbW9uIHJlYXNvbiBmb3IgYmxvYXRpbmcgaXMgaGF2aW5nIGEgbG90IG9mIGdhcyBpbiB5b3VyIGd1dC4gCg==" >>header.in$n
    i=$((i + 1))
  done
  printf '\r\n' >>header.in$n

  # send the requests then wait for named to close the socket.
  URL="http://10.53.0.3:${EXTRAPORT1}/xml/v3/status"
  "${CURL}" --silent --include --get "$URL" --next --get --header @header.in$n "$URL" >curl.out$n && ret=1
  # we expect 1 request to be processed.
  lines=$(grep -c "^<statistics version" curl.out$n)
  test "$lines" = 1 || ret=1
else
  echo_i "skipping test as curl with --next support not found"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "Check that consequtive responses do not grow excessively ($n)"
ret=0
i=0
if [ -x "${CURL}" ]; then
  URL="http://10.53.0.3:${EXTRAPORT1}/json/v1"
  "${CURL}" --silent --include --header "Accept-Encoding: deflate, gzip, br, zstd" "$URL" "$URL" "$URL" "$URL" "$URL" "$URL" "$URL" "$URL" "$URL" "$URL" >curl.out$n || ret=1
  grep -a Content-Length curl.out$n | awk 'BEGIN { prev=0; } { if (prev != 0 && $2 - prev > 100) { exit(1); } prev = $2; }' || ret=1
else
  echo_i "skipping test as curl not found"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "Check if-modified-since works ($n)"
ret=0
if $FEATURETEST --have-libxml2 && [ -x "${CURL}" ]; then
  URL="http://10.53.0.3:${EXTRAPORT1}/bind9.xsl"
  # ensure over-long time stamps are ignored
  ${CURL} --silent --show-error --fail --output bind9.xsl.2 $URL \
    --header 'If-Modified-Since: 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789'
  if ! [ bind9.xsl.2 -nt bind9.xsl.1 ] \
    || ! ${CURL} --silent --show-error --fail \
      --output bind9.xsl.3 $URL \
      --time-cond bind9.xsl.1 \
    || [ -f bind9.xsl.3 ]; then
    ret=1
  fi
else
  echo_i "skipping test: requires libxml2 and curl"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "Checking that there are no 'first refresh' zones in ns3 ($n)"
ret=0
$RNDCCMD 10.53.0.3 status | grep -E '^xfers first refresh: 0$' >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "Transfering zones from ns1 to ns3 in slow mode ($n)"
ret=0
i=0
# Restart ns1 with '-T transferslowly' to see the xfrins information in ns3's statschannel while it's ongoing
stop_server ns1
start_server --noclean --restart --port ${PORT} ns1 -- "-D statschannel-ns1 $NS_PARAMS -T transferslowly"
# Request a retransfer of the secondary zones
nextpart ns3/named.run >/dev/null
$RNDCCMD 10.53.0.3 retransfer example | sed "s/^/ns3 /" | cat_i
$RNDCCMD 10.53.0.3 retransfer example-tcp | sed "s/^/ns3 /" | cat_i
$RNDCCMD 10.53.0.3 retransfer example-tls | sed "s/^/ns3 /" | cat_i
$RNDCCMD 10.53.0.3 addzone 'example-new { type secondary; primaries { 10.53.0.1; }; file "example-new.db"; };' 2>&1 | sed "s/^/ns3 /" | cat_i
wait_for_log_fast 200 "zone example/IN: Transfer started" ns3/named.run || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

_wait_for_transfers() {
  if [ "$PERL_XML" ] && [ -x "$XMLLINT" ]; then
    getxfrins xml x$n || return 1

    # We expect 4 transfers
    count=$("$XMLLINT" --xpath 'count(/statistics/views/view[@name="_default"]/xfrins/xfrin)' xfrins.xml.x$n)
    if [ $count != 4 ]; then return 1; fi

    # We expect 3 of 4 to be retransfers
    count=$("$XMLLINT" --xpath 'count(/statistics/views/view[@name="_default"]/xfrins/xfrin[firstrefresh[text()="No"]])' xfrins.xml.x$n)
    if [ $count != 3 ]; then return 1; fi

    # We expect 1 of 4 to be a new transfer
    count=$("$XMLLINT" --xpath 'count(/statistics/views/view[@name="_default"]/xfrins/xfrin[firstrefresh[text()="Yes"]])' xfrins.xml.x$n)
    if [ $count != 1 ]; then return 1; fi
  fi

  if [ "$PERL_JSON" ] && [ -x "$JQ" ]; then
    getxfrins json j$n || return 1

    # We expect 4 transfers
    count=$("$JQ" '.views._default.xfrins | length' <xfrins.json.j$n)
    if [ $count != 4 ]; then return 1; fi

    # We expect 3 of 4 to be retransfers
    count=$("$JQ" '.views._default.xfrins | map(select(.firstrefresh == "No")) | length' <xfrins.json.j$n)
    if [ $count != 3 ]; then return 1; fi

    # We expect 1 of 4 to be a new transfer
    count=$("$JQ" '.views._default.xfrins | map(select(.firstrefresh == "Yes")) | length' <xfrins.json.j$n)
    if [ $count != 1 ]; then return 1; fi
  fi
}

# We have now less than one second to catch the zone transfers in progress
echo_i "Checking zone transfer information in the statistics channel ($n)"
ret=0
retry_quiet_fast 200 _wait_for_transfers || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "Checking that there is one 'first refresh' zone in ns3 ($n)"
ret=0
$RNDCCMD 10.53.0.3 status | grep -E '^xfers first refresh: 1$' >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

if [ "$PERL_JSON" ] && [ -x "$JQ" ]; then
  echo_i "Checking zone transfer transports ($n)"
  ret=0
  cp xfrins.json.j$((n - 2)) xfrins.json.j$n
  $PERL xfrins-json.pl xfrins.json.j$n example >xfrins.example.format$n
  echo "soatransport: UDP" >xfrins.example.expect$n
  echo "transport: TCP" >>xfrins.example.expect$n
  cmp xfrins.example.format$n xfrins.example.expect$n || ret=1
  $PERL xfrins-json.pl xfrins.json.j$n example-tcp >xfrins.example-tcp.format$n
  echo "soatransport: TCP" >xfrins.example-tcp.expect$n
  echo "transport: TCP" >>xfrins.example-tcp.expect$n
  cmp xfrins.example-tcp.format$n xfrins.example-tcp.expect$n || ret=1
  $PERL xfrins-json.pl xfrins.json.j$n example-tls >xfrins.example-tls.format$n
  echo "soatransport: TLS" >xfrins.example-tls.expect$n
  echo "transport: TLS" >>xfrins.example-tls.expect$n
  cmp xfrins.example-tls.format$n xfrins.example-tls.expect$n || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
  n=$((n + 1))
fi

echo_i "Wait for slow zone transfer to complete ($n)"
ret=0
wait_for_log 20 "zone example/IN: zone transfer finished: success" ns3/named.run || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
