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

status=0
n=0
# using dig insecure mode as not testing dnssec here
DIGOPTS="-i -p ${PORT}"
SENDCMD="$PERL $SYSTEMTESTTOP/send.pl 10.53.0.4 ${EXTRAPORT1}"

if [ -x ${DIG} ] ; then
  n=`expr $n + 1`
  echo_i "checking dig short form works ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 +short a a.example > dig.out.test$n || ret=1
  if test `wc -l < dig.out.test$n` != 1 ; then ret=1 ; fi
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig split width works ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 +split=4 -t sshfp foo.example > dig.out.test$n || ret=1
  grep " 9ABC DEF6 7890 " < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +unknownformat works ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 +unknownformat a a.example > dig.out.test$n || ret=1
  grep "CLASS1[ 	][ 	]*TYPE1[ 	][ 	]*\\\\# 4 0A000001" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig with reverse lookup works ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 -x 127.0.0.1 > dig.out.test$n 2>&1 || ret=1
  # doesn't matter if has answer
  grep -i "127\.in-addr\.arpa\." < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig over TCP works ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 a a.example > dig.out.test$n || ret=1
  grep "10\.0\.0\.1$" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +multi +norrcomments works for dnskey (when default is rrcomments)($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +multi +norrcomments DNSKEY dnskey.example > dig.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < dig.out.test$n > /dev/null && ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +multi +norrcomments works for soa (when default is rrcomments)($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +multi +norrcomments SOA example > dig.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < dig.out.test$n > /dev/null && ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +rrcomments works for DNSKEY($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +rrcomments DNSKEY dnskey.example > dig.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +short +rrcomments works for DNSKEY ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +short +rrcomments DNSKEY dnskey.example > dig.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +short +nosplit works($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +short +nosplit DNSKEY dnskey.example > dig.out.test$n || ret=1
  grep "Z8plc4Rb9VIE5x7KNHAYTvTO5d4S8M=$" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +short +rrcomments works($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +short +rrcomments DNSKEY dnskey.example > dig.out.test$n || ret=1
  grep "S8M=  ; ZSK; alg = RSAMD5 ; key id = 30795$" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig multi flag is local($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 -t DNSKEY dnskey.example +nomulti dnskey.example +nomulti > dig.out.nn.$n || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.3 -t DNSKEY dnskey.example +multi dnskey.example +nomulti > dig.out.mn.$n || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.3 -t DNSKEY dnskey.example +nomulti dnskey.example +multi > dig.out.nm.$n || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.3 -t DNSKEY dnskey.example +multi dnskey.example +multi > dig.out.mm.$n || ret=1
  lcnn=`wc -l < dig.out.nn.$n`
  lcmn=`wc -l < dig.out.mn.$n`
  lcnm=`wc -l < dig.out.nm.$n`
  lcmm=`wc -l < dig.out.mm.$n`
  test $lcmm -ge $lcnm || ret=1
  test $lcmm -ge $lcmn || ret=1
  test $lcnm -ge $lcnn || ret=1
  test $lcmn -ge $lcnn || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +noheader-only works ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +noheader-only A example > dig.out.test$n || ret=1
  grep "Got answer:" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +short +rrcomments works($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +short +rrcomments DNSKEY dnskey.example > dig.out.test$n || ret=1
  grep "S8M=  ; ZSK; alg = RSAMD5 ; key id = 30795$" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  echo_i "checking dig +header-only works ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +header-only example > dig.out.test$n || ret=1
  grep "^;; flags: qr rd; QUERY: 0, ANSWER: 0," < dig.out.test$n > /dev/null || ret=1
  grep "^;; QUESTION SECTION:" < dig.out.test$n > /dev/null && ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +header-only works (with class and type set) ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +header-only -c IN -t A example > dig.out.test$n || ret=1
  grep "^;; flags: qr rd; QUERY: 0, ANSWER: 0," < dig.out.test$n > /dev/null || ret=1
  grep "^;; QUESTION SECTION:" < dig.out.test$n > /dev/null && ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +zflag works, and that BIND properly ignores it ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +zflag +qr A example > dig.out.test$n || ret=1
  sed -n '/Sending:/,/Got answer:/p' dig.out.test$n | grep "^;; flags: rd ad; MBZ: 0x4;" > /dev/null || ret=1
  sed -n '/Got answer:/,/AUTHORITY SECTION:/p' dig.out.test$n | grep "^;; flags: qr rd ra; QUERY: 1" > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +qr +ednsopt=08 does not cause an INSIST failure ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 +ednsopt=08 +qr a a.example > dig.out.test$n || ret=1
  grep "INSIST" < dig.out.test$n > /dev/null && ret=1
  grep "FORMERR" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  echo_i "checking dig +ttlunits works ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.2 +ttlunits A weeks.example > dig.out.test$n || ret=1
  grep "^weeks.example.		3w" < dig.out.test$n > /dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.2 +ttlunits A days.example > dig.out.test$n || ret=1
  grep "^days.example.		3d" < dig.out.test$n > /dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.2 +ttlunits A hours.example > dig.out.test$n || ret=1
  grep "^hours.example.		3h" < dig.out.test$n > /dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.2 +ttlunits A minutes.example > dig.out.test$n || ret=1
  grep "^minutes.example.	45m" < dig.out.test$n > /dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.2 +ttlunits A seconds.example > dig.out.test$n || ret=1
  grep "^seconds.example.	45s" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig respects precedence of options with +ttlunits ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.2 +ttlunits +nottlid A weeks.example > dig.out.test$n || ret=1
  grep "^weeks.example.		IN" < dig.out.test$n > /dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.2 +nottlid +ttlunits A weeks.example > dig.out.test$n || ret=1
  grep "^weeks.example.		3w" < dig.out.test$n > /dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.2 +nottlid +nottlunits A weeks.example > dig.out.test$n || ret=1
  grep "^weeks.example.		1814400" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig preserves origin on TCP retries ($n)"
  ret=0
  # Ask ans4 to still accept TCP connections, but not respond to queries
  echo "//" | $SENDCMD
  $DIG $DIGOPTS -d +tcp @10.53.0.4 +retry=1 +time=1 +domain=bar foo > dig.out.test$n 2>&1 && ret=1
  l=`grep "trying origin bar" dig.out.test$n | wc -l`
  [ ${l:-0} -eq 2 ] || ret=1
  grep "using root origin" < dig.out.test$n > /dev/null && ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig -6 -4 ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.2 -4 -6 A a.example > dig.out.test$n 2>&1 && ret=1
  grep "only one of -4 and -6 allowed" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig @IPv6addr -4 A a.example ($n)"
  if $TESTSOCK6 fd92:7065:b8e:ffff::2 2>/dev/null
  then
    ret=0
    $DIG $DIGOPTS +tcp @fd92:7065:b8e:ffff::2 -4 A a.example > dig.out.test$n 2>&1 && ret=1
    grep "address family not supported" < dig.out.test$n > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
  else
    echo_i "IPv6 unavailable; skipping"
  fi

  n=`expr $n + 1`
  echo_i "checking dig @IPv4addr -6 +mapped A a.example ($n)"
  if $TESTSOCK6 fd92:7065:b8e:ffff::2 2>/dev/null && [ `uname -s` != "OpenBSD" ]
  then
    ret=0
    ret=0
    $DIG $DIGOPTS +tcp @10.53.0.2 -6 +mapped A a.example > dig.out.test$n 2>&1 || ret=1
    grep "SERVER: ::ffff:10.53.0.2#${PORT}" < dig.out.test$n > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
  else
    echo_i "IPv6 or IPv4-to-IPv6 mapping unavailable; skipping"
  fi

  n=`expr $n + 1`
  echo_i "checking dig +tcp @IPv4addr -6 +nomapped A a.example ($n)"
  if $TESTSOCK6 fd92:7065:b8e:ffff::2 2>/dev/null
  then
    ret=0
    ret=0
    $DIG $DIGOPTS +tcp @10.53.0.2 -6 +nomapped A a.example > dig.out.test$n 2>&1 || ret=1
    grep "SERVER: ::ffff:10.53.0.2#${PORT}" < dig.out.test$n > /dev/null && ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
  else
    echo_i "IPv6 unavailable; skipping"
  fi
  n=`expr $n + 1`

  echo_i "checking dig +notcp @IPv4addr -6 +nomapped A a.example ($n)"
  if $TESTSOCK6 fd92:7065:b8e:ffff::2 2>/dev/null
  then
    ret=0
    ret=0
    $DIG $DIGOPTS +notcp @10.53.0.2 -6 +nomapped A a.example > dig.out.test$n 2>&1 || ret=1
    grep "SERVER: ::ffff:10.53.0.2#${PORT}" < dig.out.test$n > /dev/null && ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
  else
    echo_i "IPv6 unavailable; skipping"
  fi

  n=`expr $n + 1`
  echo_i "checking dig +subnet ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.2 +subnet=127.0.0.1 A a.example > dig.out.test$n 2>&1 || ret=1
  grep "CLIENT-SUBNET: 127.0.0.1/32/0" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +subnet +subnet ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.2 +subnet=127.0.0.0 +subnet=127.0.0.1 A a.example > dig.out.test$n 2>&1 || ret=1
  grep "CLIENT-SUBNET: 127.0.0.1/32/0" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +subnet with various prefix lengths ($n)"
  ret=0
  for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24; do
      $DIG $DIGOPTS +tcp @10.53.0.2 +subnet=255.255.255.255/$i A a.example > dig.out.$i.test$n 2>&1 || ret=1
      case $i in
      1|9|17) octet=128 ;;
      2|10|18) octet=192 ;;
      3|11|19) octet=224 ;;
      4|12|20) octet=240 ;;
      5|13|21) octet=248 ;;
      6|14|22) octet=252 ;;
      7|15|23) octet=254 ;;
      8|16|24) octet=255 ;;
      esac
      case $i in
      1|2|3|4|5|6|7|8) addr="${octet}.0.0.0";;
      9|10|11|12|13|14|15|16) addr="255.${octet}.0.0";;
      17|18|19|20|21|22|23|24) addr="255.255.${octet}.0" ;;
      esac
      grep "FORMERR" < dig.out.$i.test$n > /dev/null && ret=1
      grep "CLIENT-SUBNET: $addr/$i/0" < dig.out.$i.test$n > /dev/null || ret=1
  done
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +subnet=0/0 ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.2 +subnet=0/0 A a.example > dig.out.test$n 2>&1 || ret=1
  grep "status: NOERROR" < dig.out.test$n > /dev/null || ret=1
  grep "CLIENT-SUBNET: 0.0.0.0/0/0" < dig.out.test$n > /dev/null || ret=1
  grep "10.0.0.1" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +subnet=0 ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.2 +subnet=0 A a.example > dig.out.test$n 2>&1 || ret=1
  grep "status: NOERROR" < dig.out.test$n > /dev/null || ret=1
  grep "CLIENT-SUBNET: 0.0.0.0/0/0" < dig.out.test$n > /dev/null || ret=1
  grep "10.0.0.1" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +subnet=::/0 ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.2 +subnet=::/0 A a.example > dig.out.test$n 2>&1 || ret=1
  grep "status: NOERROR" < dig.out.test$n > /dev/null || ret=1
  grep "CLIENT-SUBNET: ::/0/0" < dig.out.test$n > /dev/null || ret=1
  grep "10.0.0.1" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +ednsopt=8:00000000 (family=0, source=0, scope=0) ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.2 +ednsopt=8:00000000 A a.example > dig.out.test$n 2>&1 || ret=1
  grep "status: NOERROR" < dig.out.test$n > /dev/null || ret=1
  grep "CLIENT-SUBNET: 0/0/0" < dig.out.test$n > /dev/null || ret=1
  grep "10.0.0.1" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +ednsopt=8:00030000 (family=3, source=0, scope=0) ($n)"
  ret=0
  $DIG $DIGOPTS +qr +tcp @10.53.0.2 +ednsopt=8:00030000 A a.example > dig.out.test$n 2>&1 || ret=1
  grep "status: FORMERR" < dig.out.test$n > /dev/null || ret=1
  grep "CLIENT-SUBNET: 00 03 00 00" < dig.out.test$n > /dev/null || ret=1
  lines=`grep "CLIENT-SUBNET: 00 03 00 00" dig.out.test$n | wc -l`
  [ ${lines:-0} -eq 1 ] || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +subnet with prefix lengths between byte boundaries ($n)"
  ret=0
  for p in 9 10 11 12 13 14 15; do
    $DIG $DIGOPTS +tcp @10.53.0.2 +subnet=10.53/$p A a.example > dig.out.test.$p.$n 2>&1 || ret=1
    grep "FORMERR" < dig.out.test.$p.$n > /dev/null && ret=1
    grep "CLIENT-SUBNET.*/$p/0" < dig.out.test.$p.$n > /dev/null || ret=1
  done
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +sp works as an abbreviated form of split ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 +sp=4 -t sshfp foo.example > dig.out.test$n || ret=1
  grep " 9ABC DEF6 7890 " < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig -c works ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 -c CHAOS -t txt version.bind > dig.out.test$n || ret=1
  grep "version.bind.		0	CH	TXT" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +dscp ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 +dscp=32 a a.example > /dev/null 2>&1 || ret=1
  $DIG $DIGOPTS @10.53.0.3 +dscp=-1 a a.example > /dev/null 2>&1 && ret=1
  $DIG $DIGOPTS @10.53.0.3 +dscp=64 a a.example > /dev/null 2>&1 && ret=1
  #TODO add a check to make sure dig is actually setting the dscp on the query
  #we might have to add better logging to named for this
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +ednsopt with option number ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 +ednsopt=3 a.example > dig.out.test$n 2>&1 || ret=1
  grep 'NSID: .* ("ns3")' dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking dig +ednsopt with option name ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 +ednsopt=nsid a.example > dig.out.test$n 2>&1 || ret=1
  grep 'NSID: .* ("ns3")' dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  if $FEATURETEST --with-idn
  then
    echo_i "checking dig +idnout ($n)"
    ret=0
    $DIG $DIGOPTS @10.53.0.3 +noidnout xn--caf-dma.example. > dig.out.1.test$n 2>&1 || ret=1
    $DIG $DIGOPTS @10.53.0.3 +idnout xn--caf-dma.example. > dig.out.2.test$n 2>&1 || ret=1
    grep "^xn--caf-dma.example" dig.out.1.test$n > /dev/null || ret=1
    grep "^xn--caf-dma.example" dig.out.2.test$n > /dev/null && ret=1
    grep 10.1.2.3 dig.out.1.test$n > /dev/null || ret=1
    grep 10.1.2.3 dig.out.2.test$n > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
  else
    echo_i "skipping 'dig +idnout' as IDN support is not enabled ($n)"
  fi

  n=`expr $n + 1`
  echo_i "checking that dig warns about .local queries ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 local soa > dig.out.test$n 2>&1 || ret=1
  grep ";; WARNING: .local is reserved for Multicast DNS" dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "check that dig processes +ednsopt=key-tag and FORMERR is returned ($n)"
  $DIG $DIGOPTS @10.53.0.3 +ednsopt=key-tag a.example +qr > dig.out.test$n 2>&1 || ret=1
  grep "; KEY-TAG$" dig.out.test$n > /dev/null || ret=1
  grep "status: FORMERR" dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "check that dig processes +ednsopt=key-tag:<value-list> ($n)"
  $DIG $DIGOPTS @10.53.0.3 +ednsopt=key-tag:00010002 a.example +qr > dig.out.test$n 2>&1 || ret=1
  grep "; KEY-TAG: 1, 2$" dig.out.test$n > /dev/null || ret=1
  grep "status: FORMERR" dig.out.test$n > /dev/null && ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "check that dig processes +ednsopt=key-tag:<malformed-value-list> and FORMERR is returned ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 +ednsopt=key-tag:0001000201 a.example +qr > dig.out.test$n 2>&1 || ret=1
  grep "; KEY-TAG: 00 01 00 02 01" dig.out.test$n > /dev/null || ret=1
  grep "status: FORMERR" dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "check that dig gracefully handles bad escape in domain name ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 '\0.' > dig.out.test$n 2>&1
  digstatus=$?
  echo digstatus=$digstatus >> dig.out.test$n
  test $digstatus -eq 10 || ret=1
  grep REQUIRE dig.out.test$n > /dev/null && ret=1
  grep "is not a legal name (bad escape)" dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

else
  echo_i "$DIG is needed, so skipping these dig tests"
fi

# using delv insecure mode as not testing dnssec here
DELVOPTS="-i -p ${PORT}"

if [ -x ${DELV} ] ; then
  n=`expr $n + 1`
  echo_i "checking delv short form works ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +short a a.example > delv.out.test$n || ret=1
  if test `wc -l < delv.out.test$n` != 1 ; then ret=1 ; fi
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv split width works ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +split=4 -t sshfp foo.example > delv.out.test$n || ret=1
  grep " 9ABC DEF6 7890 " < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv +unknownformat works ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +unknownformat a a.example > delv.out.test$n || ret=1
  grep "CLASS1[ 	][ 	]*TYPE1[ 	][ 	]*\\\\# 4 0A000001" < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv -4 -6 ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 -4 -6 A a.example > delv.out.test$n 2>&1 && ret=1
  grep "only one of -4 and -6 allowed" < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv with IPv6 on IPv4 does not work ($n)"
  if $TESTSOCK6 fd92:7065:b8e:ffff::3 2>/dev/null
  then
    ret=0
    # following should fail because @IPv4 overrides earlier @IPv6 above
    # and -6 forces IPv6 so this should fail, with a message
    # "Use of IPv4 disabled by -6"
    $DELV $DELVOPTS @fd92:7065:b8e:ffff::3 @10.53.0.3 -6 -t txt foo.example > delv.out.test$n 2>&1
    # it should have no results but error output
    grep "testing" < delv.out.test$n > /dev/null && ret=1
    grep "Use of IPv4 disabled by -6" delv.out.test$n > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
  else
    echo_i "IPv6 unavailable; skipping"
  fi

  n=`expr $n + 1`
  echo_i "checking delv with IPv4 on IPv6 does not work ($n)"
  if $TESTSOCK6 fd92:7065:b8e:ffff::3 2>/dev/null
  then
    ret=0
    # following should fail because @IPv6 overrides earlier @IPv4 above
    # and -4 forces IPv4 so this should fail, with a message
    # "Use of IPv6 disabled by -4"
    $DELV $DELVOPTS @10.53.0.3 @fd92:7065:b8e:ffff::3 -4 -t txt foo.example > delv.out.test$n 2>&1
    # it should have no results but error output
    grep "testing" delv.out.test$n > /dev/null && ret=1
    grep "Use of IPv6 disabled by -4" delv.out.test$n > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
  else
    echo_i "IPv6 unavailable; skipping"
  fi

  n=`expr $n + 1`
  echo_i "checking delv with reverse lookup works ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 -x 127.0.0.1 > delv.out.test$n 2>&1 || ret=1
  # doesn't matter if has answer
  grep -i "127\.in-addr\.arpa\." < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv over TCP works ($n)"
  ret=0
  $DELV $DELVOPTS +tcp @10.53.0.3 a a.example > delv.out.test$n || ret=1
  grep "10\.0\.0\.1$" < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv +multi +norrcomments works for dnskey (when default is rrcomments)($n)"
  ret=0
  $DELV $DELVOPTS +tcp @10.53.0.3 +multi +norrcomments DNSKEY dnskey.example > delv.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < delv.out.test$n > /dev/null && ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv +multi +norrcomments works for soa (when default is rrcomments)($n)"
  ret=0
  $DELV $DELVOPTS +tcp @10.53.0.3 +multi +norrcomments SOA example > delv.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < delv.out.test$n > /dev/null && ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv +rrcomments works for DNSKEY($n)"
  ret=0
  $DELV $DELVOPTS +tcp @10.53.0.3 +rrcomments DNSKEY dnskey.example > delv.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv +short +rrcomments works for DNSKEY ($n)"
  ret=0
  $DELV $DELVOPTS +tcp @10.53.0.3 +short +rrcomments DNSKEY dnskey.example > delv.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv +short +rrcomments works ($n)"
  ret=0
  $DELV $DELVOPTS +tcp @10.53.0.3 +short +rrcomments DNSKEY dnskey.example > delv.out.test$n || ret=1
  grep "S8M=  ; ZSK; alg = RSAMD5 ; key id = 30795$" < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv +short +nosplit works ($n)"
  ret=0
  $DELV $DELVOPTS +tcp @10.53.0.3 +short +nosplit DNSKEY dnskey.example > delv.out.test$n || ret=1
  grep "Z8plc4Rb9VIE5x7KNHAYTvTO5d4S8M=" < delv.out.test$n > /dev/null || ret=1
  if test `wc -l < delv.out.test$n` != 1 ; then ret=1 ; fi
  f=`awk '{print NF}' < delv.out.test$n`
  test "${f:-0}" -eq 14 || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv +short +nosplit +norrcomments works ($n)"
  ret=0
  $DELV $DELVOPTS +tcp @10.53.0.3 +short +nosplit +norrcomments DNSKEY dnskey.example > delv.out.test$n || ret=1
  grep "Z8plc4Rb9VIE5x7KNHAYTvTO5d4S8M=$" < delv.out.test$n > /dev/null || ret=1
  if test `wc -l < delv.out.test$n` != 1 ; then ret=1 ; fi
  f=`awk '{print NF}' < delv.out.test$n`
  test "${f:-0}" -eq 4 || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv +sp works as an abbriviated form of split ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +sp=4 -t sshfp foo.example > delv.out.test$n || ret=1
  grep " 9ABC DEF6 7890 " < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv +sh works as an abbriviated form of short ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +sh a a.example > delv.out.test$n || ret=1
  if test `wc -l < delv.out.test$n` != 1 ; then ret=1 ; fi
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv -c IN works ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 -c IN -t a a.example > delv.out.test$n || ret=1
  grep "a.example." < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo_i "checking delv -c CH is ignored, and treated like IN ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 -c CH -t a a.example > delv.out.test$n || ret=1
  grep "a.example." < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=`expr $status + $ret`
else
  echo_i "$DELV is needed, so skipping these delv tests"
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
