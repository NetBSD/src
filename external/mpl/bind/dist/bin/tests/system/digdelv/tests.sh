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

status=0
n=0

sendcmd() {
  send "${1}" "$EXTRAPORT1"
}

dig_with_opts() {
  "$DIG" -p "$PORT" "$@"
}

mdig_with_opts() {
  "$MDIG" -p "$PORT" "$@"
}

# Check if response in file $1 has the correct TTL range.
# The response record must have RRtype $2 and class IN (CLASS1).
# Maximum TTL is given by $3.  This works in most cases where TTL is
# the second word on the line.  TTL position can be adjusted with
# setting the position $4, but that requires updating this function.
check_ttl_range() {
  file=$1
  pos=$4

  case "$pos" in
    "3")
      {
        awk -v rrtype="$2" -v ttl="$3" '($4 == "IN" || $4 == "CLASS1" ) && $5 == rrtype { if ($3 <= ttl) { ok=1 } } END { exit(ok?0:1) }' <$file
        result=$?
      } || true
      ;;
    *)
      {
        awk -v rrtype="$2" -v ttl="$3" '($3 == "IN" || $3 == "CLASS1" ) && $4 == rrtype { if ($2 <= ttl) { ok=1 } } END { exit(ok?0:1) }' <$file
        result=$?
      } || true
      ;;
  esac

  [ $result -eq 0 ] || echo_i "ttl check failed"
  return $result
}

# use delv insecure mode by default, as we're mostly not testing dnssec
delv_with_opts() {
  "$DELV" +noroot -p "$PORT" "$@"
}

KEYID="$(cat ns2/keyid)"
KEYDATA="$(sed <ns2/keydata -e 's/+/[+]/g')"
NOSPLIT="$(sed <ns2/keydata -e 's/+/[+]/g' -e 's/ //g')"

HAS_PYYAML=0
$PYTHON -c "import yaml" 2>/dev/null && HAS_PYYAML=1

#
# test whether ans7/ans.pl will be able to send a UPDATE response.
# if it can't, we will log that below.
#
if "$PERL" -e 'use Net::DNS; use Net::DNS::Packet; my $p = new Net::DNS::Packet; $p->header->opcode(5);' >/dev/null 2>&1; then
  checkupdate=1
else
  checkupdate=0
fi

if [ -x "$NSLOOKUP" -a $checkupdate -eq 1 ]; then

  n=$((n + 1))
  echo_i "check nslookup handles UPDATE response ($n)"
  ret=0
  "$NSLOOKUP" -q=CNAME -timeout=1 "-port=$PORT" foo.bar 10.53.0.7 >nslookup.out.test$n 2>&1 && ret=1
  grep "Opcode mismatch" nslookup.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

fi

if [ -x "$HOST" -a $checkupdate -eq 1 ]; then

  n=$((n + 1))
  echo_i "check host handles UPDATE response ($n)"
  ret=0
  "$HOST" -W 1 -t CNAME -p $PORT foo.bar 10.53.0.7 >host.out.test$n 2>&1 && ret=1
  grep "Opcode mismatch" host.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

fi

if [ -x "$NSUPDATE" -a $checkupdate -eq 1 ]; then

  n=$((n + 1))
  echo_i "check nsupdate handles UPDATE response to QUERY ($n)"
  ret=0
  res=0
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 || res=$?
server 10.53.0.7 ${PORT}
add x.example.com 300 in a 1.2.3.4
send
EOF
  test $res -eq 1 || ret=1
  grep "invalid OPCODE in response to SOA query" nsupdate.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

fi

if [ -x "$DIG" ]; then

  if [ $checkupdate -eq 1 ]; then

    n=$((n + 1))
    echo_i "check dig handles UPDATE response ($n)"
    ret=0
    dig_with_opts @10.53.0.7 +tries=1 +timeout=1 cname foo.bar >dig.out.test$n 2>&1 && ret=1
    grep "Opcode mismatch" dig.out.test$n >/dev/null || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  else
    echo_i "Skipped UPDATE handling test"
  fi

  n=$((n + 1))
  echo_i "checking dig short form works ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +short a a.example >dig.out.test$n || ret=1
  test "$(wc -l <dig.out.test$n)" -eq 1 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig split width works ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +split=4 -t sshfp foo.example >dig.out.test$n || ret=1
  grep " 9ABC DEF6 7890 " <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "SSHFP" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +unknownformat works ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +unknownformat a a.example >dig.out.test$n || ret=1
  grep "CLASS1[ 	][ 	]*TYPE1[ 	][ 	]*\\\\# 4 0A000001" <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "TYPE1" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig with reverse lookup works ($n)"
  ret=0
  dig_with_opts @10.53.0.3 -x 127.0.0.1 >dig.out.test$n 2>&1 || ret=1
  # doesn't matter if has answer
  grep -i "127\\.in-addr\\.arpa\\." <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "SOA" 86400 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig over TCP works ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 a a.example >dig.out.test$n || ret=1
  grep "10\\.0\\.0\\.1$" <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +multi +norrcomments works for DNSKEY (when default is rrcomments)($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +multi +norrcomments -t DNSKEY example >dig.out.test$n || ret=1
  grep "; ZSK; alg = $DEFAULT_ALGORITHM ; key id = $KEYID" dig.out.test$n >/dev/null && ret=1
  check_ttl_range dig.out.test$n "DNSKEY" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +multi +norrcomments works for SOA (when default is rrcomments)($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +multi +norrcomments -t SOA example >dig.out.test$n || ret=1
  grep "; serial" dig.out.test$n >/dev/null && ret=1
  check_ttl_range dig.out.test$n "SOA" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +rrcomments works for DNSKEY($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +rrcomments DNSKEY example >dig.out.test$n || ret=1
  grep "; ZSK; alg = $DEFAULT_ALGORITHM ; key id = $KEYID" <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "DNSKEY" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +short +rrcomments works for DNSKEY ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +short +rrcomments DNSKEY example >dig.out.test$n || ret=1
  grep "; ZSK; alg = $DEFAULT_ALGORITHM ; key id = $KEYID" <dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +short +nosplit works($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +short +nosplit DNSKEY example >dig.out.test$n || ret=1
  grep "$NOSPLIT" <dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +short +rrcomments works($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +short +rrcomments DNSKEY example >dig.out.test$n || ret=1
  grep -q "$KEYDATA  ; ZSK; alg = $DEFAULT_ALGORITHM ; key id = $KEYID\$" <dig.out.test$n || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig multi flag is local($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 -t DNSKEY example +nomulti example +nomulti >dig.out.nn.$n || ret=1
  dig_with_opts +tcp @10.53.0.3 -t DNSKEY example +multi example +nomulti >dig.out.mn.$n || ret=1
  dig_with_opts +tcp @10.53.0.3 -t DNSKEY example +nomulti example +multi >dig.out.nm.$n || ret=1
  dig_with_opts +tcp @10.53.0.3 -t DNSKEY example +multi example +multi >dig.out.mm.$n || ret=1
  lcnn=$(wc -l <dig.out.nn.$n)
  lcmn=$(wc -l <dig.out.mn.$n)
  lcnm=$(wc -l <dig.out.nm.$n)
  lcmm=$(wc -l <dig.out.mm.$n)
  test "$lcmm" -ge "$lcnm" || ret=1
  test "$lcmm" -ge "$lcmn" || ret=1
  test "$lcnm" -ge "$lcnn" || ret=1
  test "$lcmn" -ge "$lcnn" || ret=1
  check_ttl_range dig.out.nn.$n "DNSKEY" 300 || ret=1
  check_ttl_range dig.out.mn.$n "DNSKEY" 300 || ret=1
  check_ttl_range dig.out.nm.$n "DNSKEY" 300 || ret=1
  check_ttl_range dig.out.mm.$n "DNSKEY" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +noheader-only works ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +noheader-only A example >dig.out.test$n || ret=1
  grep "Got answer:" <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "SOA" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +short +rrcomments works($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +short +rrcomments DNSKEY example >dig.out.test$n || ret=1
  grep -q "$KEYDATA  ; ZSK; alg = $DEFAULT_ALGORITHM ; key id = $KEYID\$" <dig.out.test$n || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +header-only works ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +header-only example >dig.out.test$n || ret=1
  grep "^;; flags: qr rd; QUERY: 0, ANSWER: 0," <dig.out.test$n >/dev/null || ret=1
  grep "^;; QUESTION SECTION:" <dig.out.test$n >/dev/null && ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +coflag works ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +coflag +qr example >dig.out.test$n || ret=1
  grep "^; EDNS: version: 0, flags: co;" <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "SOA" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "checking dig +coflag +yaml works ($n)"
    ret=0
    dig_with_opts +yaml +tcp @10.53.0.3 +coflag +qr example >dig.out.test$n || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS flags >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "co" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "checking dig +raflag works ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +raflag +qr example >dig.out.test$n || ret=1
  grep "^;; flags: rd ra ad; QUERY: 1, ANSWER: 0," <dig.out.test$n >/dev/null || ret=1
  grep "^;; flags: qr rd ra; QUERY: 1, ANSWER: 0," <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "SOA" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +tcflag works ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +tcflag +qr example >dig.out.test$n || ret=1
  grep "^;; flags: tc rd ad; QUERY: 1, ANSWER: 0" <dig.out.test$n >/dev/null || ret=1
  grep "^;; flags: qr rd ra; QUERY: 1, ANSWER: 0," <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "SOA" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +header-only works (with class and type set) ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +header-only -c IN -t A example >dig.out.test$n || ret=1
  grep "^;; flags: qr rd; QUERY: 0, ANSWER: 0," <dig.out.test$n >/dev/null || ret=1
  grep "^;; QUESTION SECTION:" <dig.out.test$n >/dev/null && ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +zflag works, and that BIND properly ignores it ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.3 +zflag +qr A example >dig.out.test$n || ret=1
  sed -n '/Sending:/,/Got answer:/p' dig.out.test$n | grep "^;; flags: rd ad; MBZ: 0x4;" >/dev/null || ret=1
  sed -n '/Got answer:/,/AUTHORITY SECTION:/p' dig.out.test$n | grep "^;; flags: qr rd ra; QUERY: 1" >/dev/null || ret=1
  check_ttl_range dig.out.test$n "SOA" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +qr +ednsopt=08 does not cause an INSIST failure ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=08 +qr a a.example >dig.out.test$n || ret=1
  grep "INSIST" <dig.out.test$n >/dev/null && ret=1
  grep "FORMERR" <dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +ttlunits works ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.2 +ttlunits A weeks.example >dig.out.test$n || ret=1
  grep "^weeks.example.		3w" <dig.out.test$n >/dev/null || ret=1
  dig_with_opts +tcp @10.53.0.2 +ttlunits A days.example >dig.out.test$n || ret=1
  grep "^days.example.		3d" <dig.out.test$n >/dev/null || ret=1
  dig_with_opts +tcp @10.53.0.2 +ttlunits A hours.example >dig.out.test$n || ret=1
  grep "^hours.example.		3h" <dig.out.test$n >/dev/null || ret=1
  dig_with_opts +tcp @10.53.0.2 +ttlunits A minutes.example >dig.out.test$n || ret=1
  grep "^minutes.example.	45m" <dig.out.test$n >/dev/null || ret=1
  dig_with_opts +tcp @10.53.0.2 +ttlunits A seconds.example >dig.out.test$n || ret=1
  grep "^seconds.example.	45s" <dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig respects precedence of options with +ttlunits ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.2 +ttlunits +nottlid A weeks.example >dig.out.test$n || ret=1
  grep "^weeks.example.		IN" <dig.out.test$n >/dev/null || ret=1
  dig_with_opts +tcp @10.53.0.2 +nottlid +ttlunits A weeks.example >dig.out.test$n || ret=1
  grep "^weeks.example.		3w" <dig.out.test$n >/dev/null || ret=1
  dig_with_opts +tcp @10.53.0.2 +nottlid +nottlunits A weeks.example >dig.out.test$n || ret=1
  grep "^weeks.example.		1814400" <dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig preserves origin on TCP retries ($n)"
  ret=0
  # Ask ans4 to still accept TCP connections, but not respond to queries
  echo "//" | sendcmd 10.53.0.4
  dig_with_opts -d +tcp @10.53.0.4 +retry=1 +time=1 +domain=bar foo >dig.out.test$n 2>&1 && ret=1
  test "$(grep -c "trying origin bar" dig.out.test$n)" -eq 2 || ret=1
  grep "using root origin" <dig.out.test$n >/dev/null && ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig -6 -4 ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.2 -4 -6 A a.example >dig.out.test$n 2>&1 && ret=1
  grep "only one of -4 and -6 allowed" <dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig @IPv6addr -4 A a.example ($n)"
  if testsock6 fd92:7065:b8e:ffff::2 2>/dev/null; then
    ret=0
    dig_with_opts +tcp @fd92:7065:b8e:ffff::2 -4 A a.example >dig.out.test$n 2>&1 && ret=1
    grep "address family not supported" <dig.out.test$n >/dev/null || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  else
    echo_i "IPv6 unavailable; skipping"
  fi

  n=$((n + 1))
  echo_i "checking dig +tcp @IPv4addr -6 A a.example ($n)"
  if testsock6 fd92:7065:b8e:ffff::2 2>/dev/null; then
    ret=0
    dig_with_opts +tcp @10.53.0.2 -6 A a.example >dig.out.test$n 2>&1 || ret=1
    grep "SERVER: ::ffff:10.53.0.2#$PORT" <dig.out.test$n >/dev/null && ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  else
    echo_i "IPv6 unavailable; skipping"
  fi
  n=$((n + 1))

  echo_i "checking dig +notcp @IPv4addr -6 A a.example ($n)"
  if testsock6 fd92:7065:b8e:ffff::2 2>/dev/null; then
    ret=0
    dig_with_opts +notcp @10.53.0.2 -6 A a.example >dig.out.test$n 2>&1 || ret=1
    grep "SERVER: ::ffff:10.53.0.2#$PORT" <dig.out.test$n >/dev/null && ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  else
    echo_i "IPv6 unavailable; skipping"
  fi

  n=$((n + 1))
  echo_i "checking dig +subnet ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.2 +subnet=127.0.0.1 A a.example >dig.out.test$n 2>&1 || ret=1
  grep "CLIENT-SUBNET: 127.0.0.1/32/0" <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +subnet +subnet ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.2 +subnet=127.0.0.0 +subnet=127.0.0.1 A a.example >dig.out.test$n 2>&1 || ret=1
  grep "CLIENT-SUBNET: 127.0.0.1/32/0" <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +subnet with various prefix lengths ($n)"
  ret=0
  for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24; do
    dig_with_opts +tcp @10.53.0.2 +subnet=255.255.255.255/$i A a.example >dig.out.$i.test$n 2>&1 || ret=1
    case $i in
      1 | 9 | 17) octet=128 ;;
      2 | 10 | 18) octet=192 ;;
      3 | 11 | 19) octet=224 ;;
      4 | 12 | 20) octet=240 ;;
      5 | 13 | 21) octet=248 ;;
      6 | 14 | 22) octet=252 ;;
      7 | 15 | 23) octet=254 ;;
      8 | 16 | 24) octet=255 ;;
    esac
    case $i in
      1 | 2 | 3 | 4 | 5 | 6 | 7 | 8) addr="${octet}.0.0.0" ;;
      9 | 10 | 11 | 12 | 13 | 14 | 15 | 16) addr="255.${octet}.0.0" ;;
      17 | 18 | 19 | 20 | 21 | 22 | 23 | 24) addr="255.255.${octet}.0" ;;
    esac
    grep "FORMERR" <dig.out.$i.test$n >/dev/null && ret=1
    grep "CLIENT-SUBNET: $addr/$i/0" <dig.out.$i.test$n >/dev/null || ret=1
    check_ttl_range dig.out.$i.test$n "A" 300 || ret=1
  done
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +subnet=0/0 ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.2 +subnet=0/0 A a.example >dig.out.test$n 2>&1 || ret=1
  grep "status: NOERROR" <dig.out.test$n >/dev/null || ret=1
  grep "CLIENT-SUBNET: 0.0.0.0/0/0" <dig.out.test$n >/dev/null || ret=1
  grep "10.0.0.1" <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +subnet=0 ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.2 +subnet=0 A a.example >dig.out.test$n 2>&1 || ret=1
  grep "status: NOERROR" <dig.out.test$n >/dev/null || ret=1
  grep "CLIENT-SUBNET: 0.0.0.0/0/0" <dig.out.test$n >/dev/null || ret=1
  grep "10.0.0.1" <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "checking dig +subnet=0 +yaml ($n)"
    ret=0
    dig_with_opts +yaml +tcp @10.53.0.2 +subnet=0 A a.example >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message response_message_data OPT_PSEUDOSECTION EDNS CLIENT-SUBNET >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "0.0.0.0/0/0" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "checking dig +subnet=::/0 ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.2 +subnet=::/0 A a.example >dig.out.test$n 2>&1 || ret=1
  grep "status: NOERROR" <dig.out.test$n >/dev/null || ret=1
  grep "CLIENT-SUBNET: ::/0/0" <dig.out.test$n >/dev/null || ret=1
  grep "10.0.0.1" <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "checking dig +subnet=::/0 +yaml ($n)"
    ret=0
    dig_with_opts +yaml +tcp @10.53.0.2 +subnet=::/0 A a.example >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message response_message_data OPT_PSEUDOSECTION EDNS CLIENT-SUBNET >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "::/0/0" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "checking dig +subnet=dead::/16 +yaml ($n)"
    ret=0
    dig_with_opts +yaml +tcp @10.53.0.2 +qr +subnet=dead::/16 A a.example >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS CLIENT-SUBNET >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "dead::/16/0" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "checking dig +ednsopt=8:00000000 (family=0, source=0, scope=0) ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.2 +ednsopt=8:00000000 A a.example >dig.out.test$n 2>&1 || ret=1
  grep "status: NOERROR" <dig.out.test$n >/dev/null || ret=1
  grep "CLIENT-SUBNET: 0/0/0" <dig.out.test$n >/dev/null || ret=1
  grep "10.0.0.1" <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +ednsopt=8:00030000 (family=3, source=0, scope=0) ($n)"
  ret=0
  dig_with_opts +qr +tcp @10.53.0.2 +ednsopt=8:00030000 A a.example >dig.out.test$n 2>&1 || ret=1
  grep "status: FORMERR" <dig.out.test$n >/dev/null || ret=1
  grep "CLIENT-SUBNET: 00 03 00 00" <dig.out.test$n >/dev/null || ret=1
  test "$(grep -c "CLIENT-SUBNET: 00 03 00 00" dig.out.test$n)" -eq 1 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +subnet with prefix lengths between byte boundaries ($n)"
  ret=0
  for p in 9 10 11 12 13 14 15; do
    dig_with_opts +tcp @10.53.0.2 +subnet=10.53/$p A a.example >dig.out.test.$p.$n 2>&1 || ret=1
    grep "FORMERR" <dig.out.test.$p.$n >/dev/null && ret=1
    grep "CLIENT-SUBNET.*/$p/0" <dig.out.test.$p.$n >/dev/null || ret=1
    check_ttl_range dig.out.test.$p.$n "A" 300 || ret=1
  done
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +sp works as an abbreviated form of split ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +sp=4 -t sshfp foo.example >dig.out.test$n || ret=1
  grep " 9ABC DEF6 7890 " <dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "SSHFP" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig -c works ($n)"
  ret=0
  dig_with_opts @10.53.0.3 -c CHAOS -t txt version.bind >dig.out.test$n || ret=1
  grep "version.bind.		0	CH	TXT" <dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +ednsopt with option number ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=3 a.example >dig.out.test$n 2>&1 || ret=1
  grep 'NSID: .* ("ns3")' dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking dig +ednsopt with option name ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=nsid a.example >dig.out.test$n 2>&1 || ret=1
  grep 'NSID: .* ("ns3")' dig.out.test$n >/dev/null || ret=1
  check_ttl_range dig.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking ednsopt UPDATE-LEASE prints as expected (single lease) ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=UPDATE-LEASE:00000e10 +qr a.example >dig.out.test$n 2>&1 || ret=1
  pat='UPDATE-LEASE: 3600 (1 hour)'
  grep "$pat" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
  n=$((n + 1))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "checking ednsopt UPDATE-LEASE prints as expected (single lease) +yaml ($n)"
    ret=0
    dig_with_opts @10.53.0.3 +yaml +ednsopt=UPDATE-LEASE:00000e10 +qr a.example >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS UPDATE-LEASE LEASE >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "3600" ] || ret=1
    grep "LEASE: 3600 # 1 hour" dig.out.test$n >/dev/null || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "checking ednsopt UPDATE-LEASE prints as expected (split lease) ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=UPDATE-LEASE:00000e1000127500 +qr a.example >dig.out.test$n 2>&1 || ret=1
  pat='UPDATE-LEASE: 3600/1209600 (1 hour/2 weeks)'
  grep "$pat" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "checking ednsopt UPDATE-LEASE prints as expected (split lease) +yaml ($n)"
    ret=0
    dig_with_opts @10.53.0.3 +yaml +ednsopt=UPDATE-LEASE:00000e1000127500 +qr a.example >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS UPDATE-LEASE LEASE >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "3600" ] || ret=1
    grep "LEASE: 3600 # 1 hour" dig.out.test$n >/dev/null || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS UPDATE-LEASE KEY-LEASE >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "1209600" ] || ret=1
    grep "KEY-LEASE: 1209600 # 2 weeks" dig.out.test$n >/dev/null || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  echo_i "checking ednsopt LLQ prints as expected ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=llq:0001000200001234567812345678fefefefe +qr a.example >dig.out.test$n 2>&1 || ret=1
  pat='LLQ: Version: 1, Opcode: 2, Error: 0, Identifier: 1311768465173141112, Lifetime: 4278124286$'
  grep "$pat" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "checking ednsopt LLQ prints as expected +yaml ($n)"
    ret=0
    dig_with_opts @10.53.0.3 +yaml +ednsopt=llq:0001000200001234567812345678fefefefe +qr a.example >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS LLQ LLQ-VERSION >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "1" ] || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS LLQ LLQ-OPCODE >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "2" ] || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS LLQ LLQ-ERROR >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "0" ] || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS LLQ LLQ-ID >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "1311768465173141112" ] || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS LLQ LLQ-LEASE >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "4278124286" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "checking that dig warns about .local queries ($n)"
  ret=0
  dig_with_opts @10.53.0.3 local soa >dig.out.test$n 2>&1 || ret=1
  grep ";; WARNING: .local is reserved for Multicast DNS" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig processes +ednsopt=key-tag and FORMERR is returned ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=key-tag a.example +qr >dig.out.test$n 2>&1 || ret=1
  grep "; KEY-TAG: *$" dig.out.test$n >/dev/null || ret=1
  grep "status: FORMERR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig processes +ednsopt=key-tag:<value-list> ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=key-tag:00010002 a.example +qr >dig.out.test$n 2>&1 || ret=1
  grep "; KEY-TAG: 1, 2$" dig.out.test$n >/dev/null || ret=1
  grep "status: FORMERR" dig.out.test$n >/dev/null && ret=1
  check_ttl_range dig.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    ret=0
    echo_i "check that dig processes +ednsopt=key-tag:<value-list> +yaml ($n)"
    dig_with_opts @10.53.0.3 +yaml +ednsopt=key-tag:00010002 a.example +qr >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS KEY-TAG >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "[1, 2]" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "check that dig processes +ednsopt=key-tag:<malformed-value-list> and FORMERR is returned ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=key-tag:0001000201 a.example +qr >dig.out.test$n 2>&1 || ret=1
  grep "; KEY-TAG: 00 01 00 02 01" dig.out.test$n >/dev/null || ret=1
  grep "status: FORMERR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig processes +ednsopt=client-tag:value ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=client-tag:0001 a.example +qr >dig.out.test$n 2>&1 || ret=1
  grep "; CLIENT-TAG: 1$" dig.out.test$n >/dev/null || ret=1
  grep "status: FORMERR" dig.out.test$n >/dev/null && ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "check that dig processes +ednsopt=client-tag:value +yaml ($n)"
    ret=0
    dig_with_opts @10.53.0.3 +yaml +ednsopt=client-tag:0001 a.example +qr >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS CLIENT-TAG >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "1" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "check that FORMERR is returned for a too short client-tag ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=client-tag:01 a.example +qr >dig.out.test$n 2>&1 || ret=1
  grep "; CLIENT-TAG" dig.out.test$n >/dev/null || ret=1
  grep "status: FORMERR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that FORMERR is returned for a too long client-tag ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=client-tag:000001 a.example +qr >dig.out.test$n 2>&1 || ret=1
  grep "; CLIENT-TAG" dig.out.test$n >/dev/null || ret=1
  grep "status: FORMERR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig processes +ednsopt=server-tag:value ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=server-tag:0001 a.example +qr >dig.out.test$n 2>&1 || ret=1
  grep "; SERVER-TAG: 1$" dig.out.test$n >/dev/null || ret=1
  grep "status: FORMERR" dig.out.test$n >/dev/null && ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "check that dig processes +ednsopt=server-tag:value +yaml ($n)"
    ret=0
    dig_with_opts @10.53.0.3 +yaml +ednsopt=server-tag:0001 a.example +qr >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS SERVER-TAG >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "1" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "check that FORMERR is returned for a too short server-tag ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=server-tag:01 a.example +qr >dig.out.test$n 2>&1 || ret=1
  grep "; SERVER-TAG" dig.out.test$n >/dev/null || ret=1
  grep "status: FORMERR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that FORMERR is returned for a too long server-tag ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=server-tag:000001 a.example +qr >dig.out.test$n 2>&1 || ret=1
  grep "; SERVER-TAG" dig.out.test$n >/dev/null || ret=1
  grep "status: FORMERR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig processes +ednsopt=chain:02002200 ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=chain:02002200 'a.\000"' +qr >dig.out.test$n 2>&1 || ret=1
  grep '; CHAIN: "\\000\\""' dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "check that dig processes +ednsopt=chain:02002200 +yaml ($n)"
    ret=0
    dig_with_opts @10.53.0.3 +yaml +ednsopt=chain:02002200 'a.\000"' +qr >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS CHAIN >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = '\000\"' ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "check that dig processes +expire ($n)"
  ret=0
  dig_with_opts @10.53.0.1 +expire . soa >dig.out.test$n 2>&1 || ret=1
  grep '; EXPIRE: 1200 (20 minutes)' dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "check that dig processes +expire +yaml ($n)"
    ret=0
    dig_with_opts @10.53.0.1 +yaml +expire . soa >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message response_message_data OPT_PSEUDOSECTION EDNS EXPIRE >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "1200" ] || ret=1
    grep "EXPIRE: 1200 # 20 minutes" dig.out.test$n >/dev/null || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "check that dig processes +keepalive ($n)"
  ret=0
  dig_with_opts @10.53.0.1 +keepalive . soa +tcp >dig.out.test$n 2>&1 || ret=1
  grep '; TCP-KEEPALIVE: 30.0 secs' dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "check that dig processes +keepalive +yaml ($n)"
    ret=0
    dig_with_opts @10.53.0.1 +yaml +keepalive . soa +tcp >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message response_message_data OPT_PSEUDOSECTION EDNS TCP-KEEPALIVE >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "30.0 secs" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "check that Extended DNS Error 0 is printed correctly ($n)"
  ret=0
  # First defined EDE code, additional text "foo".
  dig_with_opts @10.53.0.3 +ednsopt=ede:0000666f6f a.example +qr >dig.out.test$n 2>&1 || ret=1
  pat='^; EDE: 0 (Other): (foo)$'
  grep "$pat" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "check that Extended DNS Error 0 is printed correctly +yaml ($n)"
    ret=0
    # add specials '"' and '\'
    dig_with_opts @10.53.0.3 +yaml +ednsopt=ede:0000666f6f225c a.example +qr >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS EDE EXTRA-TEXT >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = 'foo"\' ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "check that Extended DNS Error 24 is printed correctly ($n)"
  ret=0
  # Last defined EDE code, no additional text.
  dig_with_opts @10.53.0.3 +ednsopt=ede:0018 a.example +qr >dig.out.test$n 2>&1 || ret=1
  pat='^; EDE: 24 (Invalid Data)$'
  grep "$pat" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that Extended DNS Error 25 is printed correctly ($n)"
  ret=0
  # First undefined EDE code, additional text "foo".
  dig_with_opts @10.53.0.3 +ednsopt=ede:0019666f6f a.example +qr >dig.out.test$n 2>&1 || ret=1
  pat='^; EDE: 25: (foo)$'
  grep "$pat" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that invalid Extended DNS Error (length 0) is printed ($n)"
  ret=0
  # EDE payload is too short
  dig_with_opts @10.53.0.3 +ednsopt=ede a.example +qr >dig.out.test$n 2>&1 || ret=1
  pat='^; EDE:$'
  grep "$pat" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that invalid Extended DNS Error (length 1) is printed ($n)"
  ret=0
  # EDE payload is too short
  dig_with_opts @10.53.0.3 +ednsopt=ede:00 a.example +qr >dig.out.test$n 2>&1 || ret=1
  pat='^; EDE: 00 (".")$'
  grep "$pat" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "check that +yaml Extended DNS Error 0 is printed correctly ($n)"
    ret=0
    # First defined EDE code, additional text "foo".
    dig_with_opts @10.53.0.3 +yaml +ednsopt=ede:0000666f6f a.example +qr >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS EDE INFO-CODE >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "0 (Other)" ] || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS EDE EXTRA-TEXT >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "foo" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "check that +yaml Extended DNS Error 24 is printed correctly ($n)"
    ret=0
    # Last defined EDE code, no additional text.
    dig_with_opts @10.53.0.3 +yaml +ednsopt=ede:0018 a.example +qr >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS EDE INFO-CODE >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "24 (Invalid Data)" ] || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS EDE EXTRA-TEXT >yamlget.out.test$n 2>&1 && ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "check that +yaml Extended DNS Error 25 is printed correctly ($n)"
    ret=0
    # First undefined EDE code, additional text "foo".
    dig_with_opts @10.53.0.3 +yaml +ednsopt=ede:0019666f6f a.example +qr >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS EDE INFO-CODE >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "25" ] || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS EDE EXTRA-TEXT >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "foo" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "check that invalid Extended DNS Error (length 0) is printed ($n)"
    ret=0
    # EDE payload is too short
    dig_with_opts @10.53.0.3 +yaml +ednsopt=ede a.example +qr >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS EDE >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "None" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "check that invalid +yaml Extended DNS Error (length 1) is printed ($n)"
    ret=0
    # EDE payload is too short
    dig_with_opts @10.53.0.3 +yaml +ednsopt=ede:00 a.example +qr >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data OPT_PSEUDOSECTION EDNS EDE >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = '00 (".")' ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "check that dig handles malformed option '+ednsopt=:' gracefully ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +ednsopt=: a.example >dig.out.test$n 2>&1 && ret=1
  grep "ednsopt no code point specified" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig gracefully handles bad escape in domain name ($n)"
  ret=0
  digstatus=0
  dig_with_opts @10.53.0.3 '\0.' >dig.out.test$n 2>&1 || digstatus=$?
  echo digstatus=$digstatus >>dig.out.test$n
  test $digstatus -eq 10 || ret=1
  grep REQUIRE dig.out.test$n >/dev/null && ret=1
  grep "is not a legal name (bad escape)" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig -q -m works ($n)"
  ret=0
  dig_with_opts @10.53.0.3 -q -m >dig.out.test$n 2>&1
  pat='^;-m\..*IN.*A$'
  grep "$pat" dig.out.test$n >/dev/null || ret=1
  grep "Dump of all outstanding memory allocations" dig.out.test$n >/dev/null && ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking exit code for a retry upon TCP EOF (immediate -> immediate) ($n)"
  ret=0
  echo "no_response no_response" | sendcmd 10.53.0.5
  dig_with_opts @10.53.0.5 example AXFR +tries=2 >dig.out.test$n 2>&1 && ret=1
  # Sanity check: ensure ans5 behaves as expected.
  [ $(grep "communications error.*end of file" dig.out.test$n | wc -l) -eq 2 ] || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking exit code for a retry upon TCP EOF (partial AXFR -> partial AXFR) ($n)"
  ret=0
  echo "partial_axfr partial_axfr" | sendcmd 10.53.0.5
  dig_with_opts @10.53.0.5 example AXFR +tries=2 >dig.out.test$n 2>&1 && ret=1
  # Sanity check: ensure ans5 behaves as expected.
  [ $(grep "communications error.*end of file" dig.out.test$n | wc -l) -eq 2 ] || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking exit code for a retry upon TCP EOF (immediate -> partial AXFR) ($n)"
  ret=0
  echo "no_response partial_axfr" | sendcmd 10.53.0.5
  dig_with_opts @10.53.0.5 example AXFR +tries=2 >dig.out.test$n 2>&1 && ret=1
  # Sanity check: ensure ans5 behaves as expected.
  [ $(grep "communications error.*end of file" dig.out.test$n | wc -l) -eq 2 ] || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking exit code for a retry upon TCP EOF (partial AXFR -> immediate) ($n)"
  ret=0
  echo "partial_axfr no_response" | sendcmd 10.53.0.5
  dig_with_opts @10.53.0.5 example AXFR +tries=2 >dig.out.test$n 2>&1 && ret=1
  # Sanity check: ensure ans5 behaves as expected.
  [ $(grep "communications error.*end of file" dig.out.test$n | wc -l) -eq 2 ] || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking exit code for a retry upon TCP EOF (immediate -> complete AXFR) ($n)"
  ret=0
  echo "no_response complete_axfr" | sendcmd 10.53.0.5
  dig_with_opts @10.53.0.5 example AXFR +tries=2 >dig.out.test$n 2>&1 || ret=1
  # Sanity check: ensure ans5 behaves as expected.
  [ $(grep "communications error.*end of file" dig.out.test$n | wc -l) -eq 1 ] || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking exit code for a retry upon TCP EOF (partial AXFR -> complete AXFR) ($n)"
  ret=0
  echo "partial_axfr complete_axfr" | sendcmd 10.53.0.5
  dig_with_opts @10.53.0.5 example AXFR +tries=2 >dig.out.test$n 2>&1 || ret=1
  # Sanity check: ensure ans5 behaves as expected.
  [ $(grep "communications error.*end of file" dig.out.test$n | wc -l) -eq 1 ] || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking +tries=1 won't retry twice upon TCP EOF ($n)"
  ret=0
  echo "no_response no_response" | sendcmd 10.53.0.5
  dig_with_opts @10.53.0.5 example AXFR +tries=1 >dig.out.test$n 2>&1 && ret=1
  # Sanity check: ensure ans5 behaves as expected.
  [ $(grep "communications error.*end of file" dig.out.test$n | wc -l) -eq 1 ] || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking +retry=0 won't retry twice upon TCP EOF ($n)"
  ret=0
  dig_with_opts @10.53.0.5 example AXFR +retry=0 >dig.out.test$n 2>&1 && ret=1
  # Sanity check: ensure ans5 behaves as expected.
  [ $(grep "communications error.*end of file" dig.out.test$n | wc -l) -eq 1 ] || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig +expandaaaa works ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +expandaaaa AAAA ns2.example >dig.out.test$n 2>&1 || ret=1
  grep "ns2.example.*fd92:7065:0b8e:ffff:0000:0000:0000:0002" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig +noexpandaaaa works ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +noexpandaaaa AAAA ns2.example >dig.out.test$n 2>&1 || ret=1
  grep "ns2.example.*fd92:7065:b8e:ffff::2" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig default for +[no]expandaaa (+noexpandaaaa) works ($n)"
  ret=0
  dig_with_opts @10.53.0.3 AAAA ns2.example >dig.out.test$n 2>&1 || ret=1
  grep "ns2.example.*fd92:7065:b8e:ffff::2" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))

  echo_i "check that dig +short +expandaaaa works ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +short +expandaaaa AAAA ns2.example >dig.out.test$n 2>&1 || ret=1
  pat='^fd92:7065:0b8e:ffff:0000:0000:0000:0002$'
  grep "$pat" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "check dig +yaml ANY output ($n)"
    ret=0
    dig_with_opts +qr +yaml @10.53.0.3 any ns2.example >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data status >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "NOERROR" ] || ret=1
    $PYTHON yamlget.py dig.out.test$n 1 message response_message_data status >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "NOERROR" ] || ret=1
    $PYTHON yamlget.py dig.out.test$n 1 message response_message_data QUESTION_SECTION 0 >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "ns2.example. IN ANY" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "check dig +yaml output of an IPv6 address ending in zeroes ($n)"
    ret=0
    dig_with_opts +qr +yaml @10.53.0.3 aaaa d.example >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 1 message response_message_data ANSWER_SECTION 0 >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "d.example. 300 IN AAAA fd92:7065:b8e:ffff::0" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "check that dig +bufsize=0 just sets the buffer size to 0 ($n)"
  ret=0
  dig_with_opts @10.53.0.3 a.example +bufsize=0 +qr >dig.out.test$n 2>&1 || ret=1
  grep "EDNS:" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig +bufsize restores default bufsize ($n)"
  ret=0
  dig_with_opts @10.53.0.3 a.example +bufsize=0 +bufsize +qr >dig.out.test$n 2>&1 || ret=1
  lines=$(grep "EDNS:.* udp:" dig.out.test$n | wc -l)
  lines1232=$(grep "EDNS:.* udp: 1232" dig.out.test$n | wc -l)
  test $lines -eq 2 || ret=1
  test $lines1232 -eq 2 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig without -u displays 'Query time' in millseconds ($n)"
  ret=0
  dig_with_opts @10.53.0.3 a.example >dig.out.test$n 2>&1 || ret=1
  grep ';; Query time: [0-9][0-9]* msec' dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig -u displays 'Query time' in microseconds ($n)"
  ret=0
  dig_with_opts -u @10.53.0.3 a.example >dig.out.test$n 2>&1 || ret=1
  grep ';; Query time: [0-9][0-9]* usec' dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig +yaml without -u displays timestamps in milliseconds ($n)"
  ret=0
  dig_with_opts +yaml @10.53.0.3 a.example >dig.out.test$n 2>&1 || ret=1
  grep 'query_time: !!timestamp ....-..-..T..:..:..\....Z' dig.out.test$n >/dev/null || ret=1
  grep 'response_time: !!timestamp ....-..-..T..:..:..\....Z' dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig -u +yaml displays timestamps in microseconds ($n)"
  ret=0
  dig_with_opts -u +yaml @10.53.0.3 a.example >dig.out.test$n 2>&1 || ret=1
  grep 'query_time: !!timestamp ....-..-..T..:..:..\.......Z' dig.out.test$n >/dev/null || ret=1
  grep 'response_time: !!timestamp ....-..-..T..:..:..\.......Z' dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  # See [GL #3020] for more information
  n=$((n + 1))
  echo_i "check that dig handles UDP timeout followed by a SERVFAIL correctly ($n)"
  # Ask ans8 to be in "unstable" mode (switching between "silent" and "servfail" modes)
  echo "unstable" | sendcmd 10.53.0.8
  ret=0
  dig_with_opts +timeout=1 +nofail @10.53.0.8 a.example >dig.out.test$n 2>&1 || ret=1
  grep -F "status: SERVFAIL" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig handles TCP timeout followed by a SERVFAIL correctly ($n)"
  # Ask ans8 to be in "unstable" mode (switching between "silent" and "servfail" modes)
  echo "unstable" | sendcmd 10.53.0.8
  ret=0
  dig_with_opts +timeout=1 +nofail +tcp @10.53.0.8 a.example >dig.out.test$n 2>&1 || ret=1
  grep -F "status: SERVFAIL" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig tries the next server after a UDP socket network unreachable error ($n)"
  ret=0
  dig_with_opts @192.0.2.128 @10.53.0.3 a.example >dig.out.test$n 2>&1 || ret=1
  test $(grep -F -e "connection refused" -e "timed out" -e "network unreachable" -e "host unreachable" dig.out.test$n | wc -l) -eq 3 || ret=1
  grep -F "status: NOERROR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig tries the next server after a TCP socket network unreachable error ($n)"
  ret=0
  dig_with_opts +tcp @192.0.2.128 @10.53.0.3 a.example >dig.out.test$n 2>&1 || ret=1
  test $(grep -F -e "connection refused" -e "timed out" -e "network unreachable" -e "host unreachable" dig.out.test$n | wc -l) -eq 3 || ret=1
  grep -F "status: NOERROR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig tries the next server after a UDP socket read error ($n)"
  ret=0
  dig_with_opts @10.53.0.99 @10.53.0.3 a.example >dig.out.test$n 2>&1 || ret=1
  grep -F "status: NOERROR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig tries the next server after a TCP socket read error ($n)"
  # Ask ans8 to be in "close" mode, which closes the connection after accepting it
  echo "close" | sendcmd 10.53.0.8
  ret=0
  dig_with_opts +tcp @10.53.0.8 @10.53.0.3 a.example >dig.out.test$n 2>&1 || ret=1
  grep -F "status: NOERROR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  # Note that we combine TCP socket "connection error" and "timeout" cases in
  # one, because it is not trivial to simulate the timeout case in a system test
  # in Linux without a firewall, but the code which handles error cases during
  # the connection establishment time does not differentiate between timeout and
  # other types of errors (unlike during reading), so this one check should be
  # sufficient for both cases.
  n=$((n + 1))
  echo_i "check that dig tries the next server after a TCP socket connection error/timeout ($n)"
  ret=0
  dig_with_opts +tcp @10.53.0.99 @10.53.0.3 a.example >dig.out.test$n 2>&1 || ret=1
  test $(grep -F -e "connection refused" -e "timed out" -e "network unreachable" -e "host unreachable" dig.out.test$n | wc -l) -eq 3 || ret=1
  grep -F "status: NOERROR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig tries the next server after UDP socket read timeouts ($n)"
  # Ask ans8 to be in "silent" mode
  echo "silent" | sendcmd 10.53.0.8
  ret=0
  dig_with_opts +timeout=1 @10.53.0.8 @10.53.0.3 a.example >dig.out.test$n 2>&1 || ret=1
  grep -F "status: NOERROR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig tries the next server after TCP socket read timeouts ($n)"
  # Ask ans8 to be in "silent" mode
  echo "silent" | sendcmd 10.53.0.8
  ret=0
  dig_with_opts +timeout=1 +tcp @10.53.0.8 @10.53.0.3 a.example >dig.out.test$n 2>&1 || ret=1
  grep -F "status: NOERROR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  # See [GL #3248] for more information
  n=$((n + 1))
  echo_i "check that dig correctly refuses to use a server with a IPv4 mapped IPv6 address after failing with a regular IP address ($n)"
  ret=0
  dig_with_opts @10.53.0.8 @::ffff:10.53.0.8 a.example >dig.out.test$n 2>&1 || ret=1
  grep -F ";; Skipping mapped address" dig.out.test$n >/dev/null || ret=1
  grep -F ";; No acceptable nameservers" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  # See [GL #3244] for more information
  n=$((n + 1))
  echo_i "check that dig handles printing query information with +qr and +y when multiple queries are involved (including a failed query) ($n)"
  ret=0
  dig_with_opts +timeout=1 +qr +y @127.0.0.1 @10.53.0.3 a.example >dig.out.test$n 2>&1 || ret=1
  grep -F "IN A 10.0.0.1" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig +noedns +ednsflags=<nonzero> re-enables EDNS ($n)"
  dig_with_opts @10.53.0.3 +qr +noedns +ednsflags=0x70 a.example >dig.out.test$n 2>&1 || ret=1
  grep "; EDNS: version: 0, flags:; MBZ: 0x0070, udp: 1232" dig.out.test$n >/dev/null || ret=1
  grep "; EDNS: version: 0, flags:; udp: 1232" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that dig +showbadvers works ($n)"
  dig_with_opts @10.53.0.3 +edns=1 +qr +showbadvers a.example >dig.out.test$n 2>&1 || ret=1
  grep "; EDNS: version: 1, flags:; udp: 1232" dig.out.test$n >/dev/null || ret=1
  grep "; EDNS: version: 0, flags:; udp: 1232" dig.out.test$n >/dev/null || ret=1
  grep -F "status: BADVERS" dig.out.test$n >/dev/null || ret=1
  grep -F "status: NOERROR" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  # See GL#5609
  n=$((n + 1))
  echo_i "check dig with a IPv4 source address and a server with both IPv4 and IPv6 addresses doesn't crash ($n)"
  ret=0
  dig_with_opts @localhost example -b 10.53.0.1 >dig.out.test$n 2>&1 || ret=1
  # We only care about an assertion failure, otherwise reset 'ret' to 0, because
  # @localhost is't really expected to have an answer for our query.
  grep -F "core dumped" dig.out.test$n >/dev/null || ret=0
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
else
  echo_i "$DIG is needed, so skipping these dig tests"
fi

if [ -x "$MDIG" ]; then
  n=$((n + 1))
  echo_i "checking mdig +tcp works with a source address and port ($n)"
  ret=0
  # When running more than once in quick succession with a source address#port,
  # we can get a "response failed with address not available" error because
  # the address#port is still busy, but we are not interested in that error,
  # as we are only looking for the unexpected error case, that's why we ignore
  # the return code from mdig, but we check for the unexpected error message
  # using grep. See GitLab #4969.
  mdig_with_opts -b "10.53.0.3#${EXTRAPORT8}" +tcp @10.53.0.3 example >dig.out.test$n 2>&1 || true
  grep -F "unexpected error" dig.out.test$n >/dev/null && ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that mdig handles malformed option '+ednsopt=:' gracefully ($n)"
  ret=0
  mdig_with_opts @10.53.0.3 +ednsopt=: a.example >dig.out.test$n 2>&1 && ret=1
  grep "ednsopt no code point specified" dig.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking mdig +multi +norrcomments works for DNSKEY (when default is rrcomments)($n)"
  ret=0
  mdig_with_opts +tcp @10.53.0.3 +multi +norrcomments -t DNSKEY example >dig.out.test$n || ret=1
  grep "; ZSK; alg = $DEFAULT_ALGORITHM ; key id = $KEYID" dig.out.test$n && ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking mdig +multi +norrcomments works for SOA (when default is rrcomments)($n)"
  ret=0
  mdig_with_opts +tcp @10.53.0.3 +multi +norrcomments -t SOA example >dig.out.test$n || ret=1
  grep "; serial" <dig.out.test$n >/dev/null && ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "check mdig +yaml output ($n)"
    ret=0
    mdig_with_opts +yaml @10.53.0.3 -t any ns2.example >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message response_message_data status >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "NOERROR" ] || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message response_message_data QUESTION_SECTION 0 >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "ns2.example. IN ANY" ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi
else
  echo_i "$MDIG is needed, so skipping these mdig tests"
fi

if [ -x "$DELV" ]; then
  n=$((n + 1))
  echo_i "checking delv short form works ($n)"
  ret=0
  delv_with_opts @10.53.0.3 +short a a.example >delv.out.test$n || ret=1
  test "$(wc -l <delv.out.test$n)" -eq 1 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv split width works ($n)"
  ret=0
  delv_with_opts @10.53.0.3 +split=4 -t sshfp foo.example >delv.out.test$n || ret=1
  grep " 9ABC DEF6 7890 " <delv.out.test$n >/dev/null || ret=1
  check_ttl_range delv.out.test$n "SSHFP" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +unknownformat works ($n)"
  ret=0
  delv_with_opts @10.53.0.3 +unknownformat a a.example >delv.out.test$n || ret=1
  grep "CLASS1[ 	][ 	]*TYPE1[ 	][ 	]*\\\\# 4 0A000001" <delv.out.test$n >/dev/null || ret=1
  check_ttl_range delv.out.test$n "TYPE1" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv -4 -6 ($n)"
  ret=0
  delv_with_opts @10.53.0.3 -4 -6 A a.example >delv.out.test$n 2>&1 && ret=1
  grep "only one of -4 and -6 allowed" <delv.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv with IPv6 on IPv4 does not work ($n)"
  if testsock6 fd92:7065:b8e:ffff::3 2>/dev/null; then
    ret=0
    # following should fail because @IPv4 overrides earlier @IPv6 above
    # and -6 forces IPv6 so this should fail, with a message
    # "Use of IPv4 disabled by -6"
    delv_with_opts @fd92:7065:b8e:ffff::3 @10.53.0.3 -6 -t txt foo.example >delv.out.test$n 2>&1 && ret=1
    # it should have no results but error output
    grep "testing" <delv.out.test$n >/dev/null && ret=1
    grep "Use of IPv4 disabled by -6" delv.out.test$n >/dev/null || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  else
    echo_i "IPv6 unavailable; skipping"
  fi

  n=$((n + 1))
  echo_i "checking delv with IPv4 on IPv6 does not work ($n)"
  if testsock6 fd92:7065:b8e:ffff::3 2>/dev/null; then
    ret=0
    # following should fail because @IPv6 overrides earlier @IPv4 above
    # and -4 forces IPv4 so this should fail, with a message
    # "Use of IPv6 disabled by -4"
    delv_with_opts @10.53.0.3 @fd92:7065:b8e:ffff::3 -4 -t txt foo.example >delv.out.test$n 2>&1 && ret=1
    # it should have no results but error output
    grep "testing" delv.out.test$n >/dev/null && ret=1
    grep "Use of IPv6 disabled by -4" delv.out.test$n >/dev/null || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  else
    echo_i "IPv6 unavailable; skipping"
  fi

  n=$((n + 1))
  echo_i "checking delv with reverse lookup works ($n)"
  ret=0
  delv_with_opts @10.53.0.3 -x 127.0.0.1 >delv.out.test$n 2>&1 || ret=1
  # doesn't matter if has answer
  grep -i "127\\.in-addr\\.arpa\\." <delv.out.test$n >/dev/null || ret=1
  check_ttl_range delv.out.test$n '\\-ANY' 10800 3 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv over TCP works ($n)"
  ret=0
  delv_with_opts +tcp @10.53.0.3 a a.example >delv.out.test$n || ret=1
  grep "10\\.0\\.0\\.1$" <delv.out.test$n >/dev/null || ret=1
  check_ttl_range delv.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +multi +norrcomments works for DNSKEY (when default is rrcomments)($n)"
  ret=0
  delv_with_opts +tcp @10.53.0.3 +multi +norrcomments DNSKEY example >delv.out.test$n || ret=1
  grep "; ZSK; alg = $DEFAULT_ALGORITHM ; key id = $KEYID" <delv.out.test$n >/dev/null && ret=1
  check_ttl_range delv.out.test$n "DNSKEY" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +multi +norrcomments works for SOA (when default is rrcomments)($n)"
  ret=0
  delv_with_opts +tcp @10.53.0.3 +multi +norrcomments SOA example >delv.out.test$n || ret=1
  grep "; ZSK; alg = $DEFAULT_ALGORITHM ; key id = $KEYID" <delv.out.test$n >/dev/null && ret=1
  check_ttl_range delv.out.test$n "SOA" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +rrcomments works for DNSKEY($n)"
  ret=0
  delv_with_opts +tcp @10.53.0.3 +rrcomments DNSKEY example >delv.out.test$n || ret=1
  grep "; ZSK; alg = $DEFAULT_ALGORITHM ; key id = $KEYID" <delv.out.test$n >/dev/null || ret=1
  check_ttl_range delv.out.test$n "DNSKEY" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +short +rrcomments works for DNSKEY ($n)"
  ret=0
  delv_with_opts +tcp @10.53.0.3 +short +rrcomments DNSKEY example >delv.out.test$n || ret=1
  grep "; ZSK; alg = $DEFAULT_ALGORITHM ; key id = $KEYID" <delv.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +short +rrcomments works ($n)"
  ret=0
  delv_with_opts +tcp @10.53.0.3 +short +rrcomments DNSKEY example >delv.out.test$n || ret=1
  grep -q "$KEYDATA  ; ZSK; alg = $DEFAULT_ALGORITHM ; key id = $KEYID" <delv.out.test$n || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +short +nosplit works ($n)"
  ret=0
  delv_with_opts +tcp @10.53.0.3 +short +nosplit DNSKEY example >delv.out.test$n || ret=1
  grep -q "$NOSPLIT" <delv.out.test$n || ret=1
  test "$(wc -l <delv.out.test$n)" -eq 1 || ret=1
  test "$(awk '{print NF}' <delv.out.test$n)" -eq 14 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +short +nosplit +norrcomments works ($n)"
  ret=0
  delv_with_opts +tcp @10.53.0.3 +short +nosplit +norrcomments DNSKEY example >delv.out.test$n || ret=1
  grep -q "$NOSPLIT\$" <delv.out.test$n || ret=1
  test "$(wc -l <delv.out.test$n)" -eq 1 || ret=1
  test "$(awk '{print NF}' <delv.out.test$n)" -eq 4 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +sp works as an abbriviated form of split ($n)"
  ret=0
  delv_with_opts @10.53.0.3 +sp=4 -t sshfp foo.example >delv.out.test$n || ret=1
  grep " 9ABC DEF6 7890 " <delv.out.test$n >/dev/null || ret=1
  check_ttl_range delv.out.test$n "SSHFP" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +sh works as an abbriviated form of short ($n)"
  ret=0
  delv_with_opts @10.53.0.3 +sh a a.example >delv.out.test$n || ret=1
  test "$(wc -l <delv.out.test$n)" -eq 1 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv -c IN works ($n)"
  ret=0
  delv_with_opts @10.53.0.3 -c IN -t a a.example >delv.out.test$n || ret=1
  grep "a.example." <delv.out.test$n >/dev/null || ret=1
  check_ttl_range delv.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv -c CH is ignored, and treated like IN ($n)"
  ret=0
  delv_with_opts @10.53.0.3 -c CH -t a a.example >delv.out.test$n || ret=1
  grep "a.example." <delv.out.test$n >/dev/null || ret=1
  check_ttl_range delv.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv -c CH is ignored, and treated like IN ($n)"
  ret=0
  delv_with_opts @10.53.0.3 -c CH -t a a.example >delv.out.test$n || ret=1
  grep "a.example." <delv.out.test$n >/dev/null || ret=1
  check_ttl_range delv.out.test$n "A" 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that delv -q -m works ($n)"
  ret=0
  delv_with_opts @10.53.0.3 -q -m >delv.out.test$n 2>&1 || ret=1
  grep '^; -m\..*[0-9]*.*IN.*ANY.*;' delv.out.test$n >/dev/null || ret=1
  grep "^add " delv.out.test$n >/dev/null && ret=1
  grep "^del " delv.out.test$n >/dev/null && ret=1
  check_ttl_range delv.out.test$n '\\-ANY' 300 3 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that delv -t ANY works ($n)"
  ret=0
  delv_with_opts @10.53.0.3 -t ANY example >delv.out.test$n 2>&1 || ret=1
  grep "^example." <delv.out.test$n >/dev/null || ret=1
  check_ttl_range delv.out.test$n NS 300 || ret=1
  check_ttl_range delv.out.test$n SOA 300 || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that delv loads key-style trust anchors ($n)"
  ret=0
  delv_with_opts -a ns3/anchor.dnskey +root=example @10.53.0.3 -t DNSKEY example >delv.out.test$n 2>&1 || ret=1
  grep "fully validated" delv.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check that delv loads DS-style trust anchors ($n)"
  ret=0
  delv_with_opts -a ns3/anchor.ds +root=example @10.53.0.3 -t DNSKEY example >delv.out.test$n 2>&1 || ret=1
  grep "fully validated" delv.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if [ $HAS_PYYAML -ne 0 ]; then
    n=$((n + 1))
    echo_i "check delv +yaml ANY output ($n)"
    ret=0
    delv_with_opts +yaml @10.53.0.3 any ns2.example >delv.out.test$n || ret=1
    $PYTHON yamlget.py delv.out.test$n status >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "success" ] || ret=1
    $PYTHON yamlget.py delv.out.test$n query_name >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "ns2.example" ] || ret=1
    $PYTHON yamlget.py delv.out.test$n records 0 answer_not_validated 0 >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    count=$(echo $value | wc -w)
    [ ${count:-0} -eq 5 ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "check delv +yaml NODATA output ($n)"
    ret=0
    delv_with_opts +yaml @10.53.0.3 type500 ns2.example >delv.out.test$n || ret=1
    $PYTHON yamlget.py delv.out.test$n status >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "ncache nxrrset" ] || ret=1
    $PYTHON yamlget.py delv.out.test$n query_name >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "ns2.example" ] || ret=1
    $PYTHON yamlget.py delv.out.test$n records 0 negative_response_answer_not_validated 0 >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    count=$(echo $value | wc -w)
    [ ${count:-0} -eq 5 ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "check delv +yaml NXDOMAIN output ($n)"
    ret=0
    delv_with_opts +yaml @10.53.0.3 a this-does-not-exist.ns2.example >delv.out.test$n || ret=1
    $PYTHON yamlget.py delv.out.test$n status >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "ncache nxdomain" ] || ret=1
    $PYTHON yamlget.py delv.out.test$n query_name >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "this-does-not-exist.ns2.example" ] || ret=1
    $PYTHON yamlget.py delv.out.test$n records 0 negative_response_answer_not_validated 0 >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    count=$(echo $value | wc -w)
    [ ${count:-0} -eq 5 ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

  n=$((n + 1))
  echo_i "check that delv handles REFUSED when chasing DS records ($n)"
  delv_with_opts @10.53.0.2 +root xxx.example.tld A >delv.out.test$n 2>&1 || ret=1
  grep ";; resolution failed: broken trust chain" delv.out.test$n >/dev/null || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check NS output from delv +ns ($n)"
  ret=0
  delv_with_opts -i +ns +nortrace +nostrace +nomtrace +novtrace +hint=root.hint ns example >delv.out.test$n || ret=1
  lines=$(awk '$1 == "example." && $4 == "NS" {print}' delv.out.test$n | wc -l)
  [ $lines -eq 2 ] || ret=1
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +ns (no validation) ($n)"
  ret=0
  delv_with_opts -i +ns +hint=root.hint a a.example >delv.out.test$n || ret=1
  grep -q '; authoritative' delv.out.test$n || ret=1
  grep -q '_.example' delv.out.test$n && ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +ns +qmin (no validation) ($n)"
  ret=0
  delv_with_opts -i +ns +qmin +hint=root.hint a a.example >delv.out.test$n || ret=1
  grep -q '; authoritative' delv.out.test$n || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +ns (with validation) ($n)"
  ret=0
  delv_with_opts -a ns1/anchor.dnskey +root +ns +hint=root.hint a a.example >delv.out.test$n || ret=1
  grep -q '; fully validated' delv.out.test$n || ret=1
  grep -q '_.example' delv.out.test$n && ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking delv +ns +qmin (with validation) ($n)"
  ret=0
  delv_with_opts -a ns1/anchor.dnskey +root +ns +qmin +hint=root.hint a a.example >delv.out.test$n || ret=1
  grep -q '; fully validated' delv.out.test$n || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  if testsock6 fd92:7065:b8e:ffff::2 2>/dev/null; then
    n=$((n + 1))
    echo_i "checking delv -4 +ns uses only IPv4 ($n)"
    ret=0
    delv_with_opts -a ns1/anchor.dnskey +root -4 +ns +hint=root.hint a a.example >delv.out.test$n || ret=1
    grep -qF 'sending packet to 10.53' delv.out.test$n >/dev/null || ret=1
    grep -qF 'sending packet to fd92:7065' delv.out.test$n >/dev/null && ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "checking delv -6 +ns uses only IPv6 ($n)"
    ret=0
    delv_with_opts -a ns1/anchor.dnskey +root -6 +ns +hint=root.hint a a.example >delv.out.test$n || ret=1
    grep -qF 'sending packet to 10.53' delv.out.test$n >/dev/null && ret=1
    grep -qF 'sending packet to fd92:7065' delv.out.test$n >/dev/null || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  fi

else
  echo_i "$DELV is needed, so skipping these delv tests"
fi

if [ $HAS_PYYAML -ne 0 ]; then
  for qname in "yaml" "'.yaml" "[.yaml" "{.yaml" "&.yaml" "#.yaml"; do
    n=$((n + 1))
    echo_i "check yaml special '${yaml}.example' ($n)"
    ret=0
    dig_with_opts @10.53.0.3 +yaml "${qname}.example" TXT +qr >dig.out.test$n 2>&1 || ret=1
    $PYTHON yamlget.py dig.out.test$n 0 message query_message_data QUESTION_SECTION 0 >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "${qname}.example. IN TXT" ] || ret=1
    $PYTHON yamlget.py dig.out.test$n 1 message response_message_data ANSWER_SECTION 0 >yamlget.out.test$n 2>&1 || ret=1
    read -r value <yamlget.out.test$n
    [ "$value" = "${qname}"'.example. 300 IN TXT "a: b"' ] || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  done

  n=$((n + 1))
  echo_i "check yaml character values ($n)"
  ret=0
  dig_with_opts @10.53.0.3 +yaml "all.yaml.example" TXT +qr >dig.out.test$n 2>&1 || ret=1
  $PYTHON yamlget.py dig.out.test$n 1 message response_message_data ANSWER_SECTION 0 >yamlget.out.test$n 2>&1 || ret=1
  read -r value <yamlget.out.test$n
  expected='all.yaml.example. 300 IN TXT'
  expected="$expected "'"\000" "\001" "\002" "\003" "\004" "\005" "\006" "\007"'
  expected="$expected "'"\008" "\009" "\010" "\011" "\012" "\013" "\014" "\015"'
  expected="$expected "'"\016" "\017" "\018" "\019" "\020" "\021" "\022" "\023"'
  expected="$expected "'"\024" "\025" "\026" "\027" "\028" "\029" "\030" "\031"'
  expected="$expected "'" " "!" "\"" "#" "$" "%" "&" "'"'"'" "(" ")" "*" "+" ","'
  expected="$expected "'"-" "." "/" "0" "1" "2" "3" "4" "5" "6" "7" "8" "9" ":"'
  expected="$expected "'";" "<" "=" ">" "?" "@" "A" "B" "C" "D" "E" "F" "G" "H"'
  expected="$expected "'"I" "J" "K" "L" "M" "N" "O" "P" "Q" "R" "S" "T" "U" "V"'
  expected="$expected "'"W" "X" "Y" "Z" "[" "\\" "]" "^" "_" "`" "a" "b" "c" "d"'
  expected="$expected "'"e" "f" "g" "h" "i" "j" "k" "l" "m" "n" "o" "p" "q" "r"'
  expected="$expected "'"s" "t" "u" "v" "w" "x" "y" "z" "{" "|" "}" "~" "\127"'
  expected="$expected "'"\128" "\129" "\130" "\131" "\132" "\133" "\134" "\135"'
  expected="$expected "'"\136" "\137" "\138" "\139" "\140" "\141" "\142" "\143"'
  expected="$expected "'"\144" "\145" "\146" "\147" "\148" "\149" "\150" "\151"'
  expected="$expected "'"\152" "\153" "\154" "\155" "\156" "\157" "\158" "\159"'
  expected="$expected "'"\160" "\161" "\162" "\163" "\164" "\165" "\166" "\167"'
  expected="$expected "'"\168" "\169" "\170" "\171" "\172" "\173" "\174" "\175"'
  expected="$expected "'"\176" "\177" "\178" "\179" "\180" "\181" "\182" "\183"'
  expected="$expected "'"\184" "\185" "\186" "\187" "\188" "\189" "\190" "\191"'
  expected="$expected "'"\192" "\193" "\194" "\195" "\196" "\197" "\198" "\199"'
  expected="$expected "'"\200" "\201" "\202" "\203" "\204" "\205" "\206" "\207"'
  expected="$expected "'"\208" "\209" "\210" "\211" "\212" "\213" "\214" "\215"'
  expected="$expected "'"\216" "\217" "\218" "\219" "\220" "\221" "\222" "\223"'
  expected="$expected "'"\224" "\225" "\226" "\227" "\228" "\229" "\230" "\231"'
  expected="$expected "'"\232" "\233" "\234" "\235" "\236" "\237" "\238" "\239"'
  expected="$expected "'"\240" "\241" "\242" "\243" "\244" "\245" "\246" "\247"'
  expected="$expected "'"\248" "\249" "\250" "\251" "\252" "\253" "\254" "\255"'
  [ "$value" = "$expected" ] || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
