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

# set -e
#
# shellcheck source=conf.sh
. ../conf.sh

RNDCCMD="$RNDC -c ../_common/rndc.conf -p ${CONTROLPORT} -s"

set -e

status=0
n=1
synth_default=yes

rm -f dig.out.*

dig_with_opts() {
  "$DIG" +tcp +noadd +nosea +nostat +nocmd +dnssec -p "$PORT" "$@"
}

check_ad_flag() {
  if [ ${1} = yes ]; then
    grep "flags:[^;]* ad[^;]*; QUERY" ${2} >/dev/null || return 1
  else
    grep "flags:[^;]* ad[^;]*; QUERY" ${2} >/dev/null && return 1
  fi
  return 0
}

check_status() {
  grep "status: ${1}," ${2} >/dev/null || return 1
  return 0
}

check_synth_soa() (
  name=$(echo "$1" | sed 's/\./\\./g')
  grep "^${name}.*[0-9]*.IN.SOA" ${2} >/dev/null || return 1
  grep "^${name}.*3600.IN.SOA" ${2} >/dev/null && return 1
  return 0
)

check_nosynth_soa() (
  name=$(echo "$1" | sed 's/\./\\./g')
  grep "^${name}.*3600.IN.SOA" ${2} >/dev/null || return 1
  return 0
)

check_synth_a() (
  name=$(echo "$1" | sed 's/\./\\./g')
  grep "^${name}.*[0-9]*.IN.A.[0-2]" ${2} >/dev/null || return 1
  grep "^${name}.*3600.IN.A.[0-2]" ${2} >/dev/null && return 1
  return 0
)

check_nosynth_a() (
  name=$(echo "$1" | sed 's/\./\\./g')
  grep "^${name}.*3600.IN.A.[0-2]" ${2} >/dev/null || return 1
  return 0
)

check_synth_aaaa() (
  name=$(echo "$1" | sed 's/\./\\./g')
  grep "^${name}.*[0-9]*.IN.AAAA" ${2} >/dev/null || return 1
  grep "^${name}.*3600.IN.A" ${2} >/dev/null && return 1
  return 0
)

check_nosynth_aaaa() (
  name=$(echo "$1" | sed 's/\./\\./g')
  grep "^${name}.*3600.IN.AAAA" ${2} >/dev/null || return 1
  return 0
)

check_synth_cname() (
  name=$(echo "$1" | sed 's/\./\\./g')
  grep "^${name}.*[0-9]*.IN.CNAME" ${2} >/dev/null || return 1
  grep "^${name}.*3600.IN.CNAME" ${2} >/dev/null && return 1
  return 0
)

check_nosynth_cname() (
  name=$(echo "$1" | sed 's/\./\\./g')
  grep "^${name}.*3600.IN.CNAME" ${2} >/dev/null || return 1
  return 0
)

check_auth_count() {
  grep "AUTHORITY: ${1}," ${2} >/dev/null || return 1
  return 0
}

for ns in 2 4 5 6; do
  case $ns in
    2)
      ad=yes
      description="<default>"
      ;;
    4)
      ad=yes
      description="no"
      ;;
    5)
      ad=yes
      description="yes"
      ;;
    6)
      ad=no
      description="yes; dnssec-validation no"
      ;;
    *) exit 1 ;;
  esac
  echo_i "prime negative NXDOMAIN response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts a.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NXDOMAIN dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa example. dig.out.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && cp dig.out.ns${ns}.test$n nxdomain.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime negative NXDOMAIN response no-apex-covering (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts a.no-apex-covering. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NXDOMAIN dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa no-apex-covering. dig.out.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && cp dig.out.ns${ns}.test$n no-apex-covering.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime negative NODATA response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts nodata.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa example. dig.out.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && cp dig.out.ns${ns}.test$n nodata.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime wildcard response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts a.wild-a.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_a a.wild-a.example. dig.out.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && sed 's/^a\./b./' dig.out.ns${ns}.test$n >wild.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime wildcard CNAME response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts a.wild-cname.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_cname a.wild-cname.example. dig.out.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && sed 's/^a\./b./' dig.out.ns${ns}.test$n >wildcname.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime wildcard NODATA 1 NSEC response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts a.wild-1-nsec.example. @10.53.0.${ns} TXT >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa example. dig.out.ns${ns}.test$n || ret=1
  check_auth_count 4 dig.out.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && sed 's/^a\./b./' dig.out.ns${ns}.test$n >wildnodata1nsec.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime wildcard NODATA 2 NSEC response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts a.wild-2-nsec.example. @10.53.0.${ns} TXT >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa example. dig.out.ns${ns}.test$n || ret=1
  check_auth_count 6 dig.out.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && sed 's/^a\./b./' dig.out.ns${ns}.test$n >wildnodata2nsec.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime wildcard NODATA 2 NSEC after data response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts a.wild-2-nsec-afterdata.example. @10.53.0.${ns} TXT >dig.out.txt.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.txt.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.txt.ns${ns}.test$n || ret=1
  check_nosynth_soa example. dig.out.txt.ns${ns}.test$n || ret=1
  check_auth_count 6 dig.out.txt.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && sed 's/^a\./b./' dig.out.txt.ns${ns}.test$n >wildnodata2nsecafterdata.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime insecure negative NXDOMAIN response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts a.insecure.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.ns${ns}.test$n || ret=1
  check_status NXDOMAIN dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa insecure.example. dig.out.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && cp dig.out.ns${ns}.test$n insecure.nxdomain.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime insecure negative NODATA response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts nodata.insecure.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa insecure.example. dig.out.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && cp dig.out.ns${ns}.test$n insecure.nodata.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime insecure wildcard response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts a.wild-a.insecure.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_a a.wild-a.insecure.example. dig.out.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && sed 's/^a\./b./' dig.out.ns${ns}.test$n >insecure.wild.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime wildcard CNAME response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts a.wild-cname.insecure.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_cname a.wild-cname.insecure.example. dig.out.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && sed 's/^a\./b./' dig.out.ns${ns}.test$n >insecure.wildcname.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime insecure wildcard NODATA 1 NSEC response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts a.wild-1-nsec.insecure.example. @10.53.0.${ns} TXT >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa insecure.example. dig.out.ns${ns}.test$n || ret=1
  check_auth_count 4 dig.out.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && cp dig.out.ns${ns}.test$n insecure.wildnodata1nsec.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime insecure wildcard NODATA 2 NSEC response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts a.wild-2-nsec.insecure.example. @10.53.0.${ns} TXT >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa insecure.example. dig.out.ns${ns}.test$n || ret=1
  check_auth_count 6 dig.out.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && cp dig.out.ns${ns}.test$n insecure.wildnodata2nsec.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime insecure wildcard NODATA 2 NSEC after data response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts a.wild-2-nsec-afterdata.insecure.example. @10.53.0.${ns} TXT >dig.out.txt.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.txt.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.txt.ns${ns}.test$n || ret=1
  check_nosynth_soa insecure.example. dig.out.txt.ns${ns}.test$n || ret=1
  check_auth_count 6 dig.out.txt.ns${ns}.test$n || ret=1
  [ $ns -eq 2 ] && sed 's/^a\./b./' dig.out.txt.ns${ns}.test$n >insecure.wildnodata2nsecafterdata.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime minimal NXDOMAIN response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts nxdomain.minimal. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NXDOMAIN dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa minimal. dig.out.ns${ns}.test$n || ret=1
  grep "nxdomaia.minimal.*3600.IN.NSEC.nxdomaiz.minimal. RRSIG NSEC" dig.out.ns${ns}.test$n >/dev/null || ret=1
  [ $ns -eq 2 ] && cp dig.out.ns${ns}.test$n minimal.nxdomain.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime black lie NODATA response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts black.minimal. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa minimal. dig.out.ns${ns}.test$n || ret=1
  grep 'black.minimal.*3600.IN.NSEC.\\000.black.minimal. RRSIG NSEC' dig.out.ns${ns}.test$n >/dev/null || ret=1
  [ $ns -eq 2 ] && cp dig.out.ns${ns}.test$n black.out
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime bad type map NODATA response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts badtypemap.minimal. @10.53.0.${ns} TXT >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa minimal. dig.out.ns${ns}.test$n || ret=1
  grep 'badtypemap.minimal.*3600.IN.NSEC.black.minimal. A$' dig.out.ns${ns}.test$n >/dev/null || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "prime SOA without DNSKEY bad type map NODATA response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts soa-without-dnskey. @10.53.0.${ns} TXT >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa soa-without-dnskey. dig.out.ns${ns}.test$n || ret=1
  grep 'soa-without-dnskey.*3600.IN.NSEC.ns1.soa-without-dnskey. NS SOA RRSIG NSEC$' dig.out.ns${ns}.test$n >/dev/null || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

done

echo_i "prime redirect response (+nodnssec) (synth-from-dnssec <default>;) ($n)"
ret=0
dig_with_opts +nodnssec a.redirect. @10.53.0.3 a >dig.out.ns3.test$n || ret=1
check_ad_flag no dig.out.ns3.test$n || ret=1
check_status NOERROR dig.out.ns3.test$n || ret=1
grep 'a\.redirect\..*300.IN.A.100\.100\.100\.2' dig.out.ns3.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

#
# ensure TTL of synthesised answers differs from direct answers.
#
sleep 1

for ns in 2 4 5 6; do
  case $ns in
    2) ad=yes synth=${synth_default} description="<default>" ;;
    4) ad=yes synth=no description="no" ;;
    5) ad=yes synth=yes description="yes" ;;
    6) ad=no synth=no description="yes; dnssec-validation no" ;;
    *) exit 1 ;;
  esac
  echo_i "check synthesized NXDOMAIN response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts b.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NXDOMAIN dig.out.ns${ns}.test$n || ret=1
  if [ ${synth} = yes ]; then
    check_synth_soa example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.example/A >/dev/null && ret=1
  else
    check_nosynth_soa example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.example/A >/dev/null || ret=1
  fi
  digcomp nxdomain.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check synthesized NXDOMAIN response no-apex-covering (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts b.no-apex-covering. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NXDOMAIN dig.out.ns${ns}.test$n || ret=1
  if [ ${synth} = yes ]; then
    check_synth_soa no-apex-covering. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.no-apex-covering/A >/dev/null && ret=1
  else
    check_nosynth_soa no-apex-covering. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.no-apex-covering/A >/dev/null || ret=1
  fi
  digcomp no-apex-covering.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check synthesized NODATA response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts nodata.example. @10.53.0.${ns} aaaa >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  if [ ${synth} = yes ]; then
    check_synth_soa example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep nodata.example/AAAA >/dev/null && ret=1
  else
    check_nosynth_soa example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep nodata.example/AAAA >/dev/null || ret=1
  fi
  digcomp nodata.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check synthesized wildcard response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts b.wild-a.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  if [ ${synth} = yes ]; then
    check_synth_a b.wild-a.example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.wild-a.example/A >/dev/null && ret=1
  else
    check_nosynth_a b.wild-a.example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.wild-a.example/A >/dev/null || ret=1
  fi
  digcomp wild.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check synthesized wildcard CNAME response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts b.wild-cname.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  if [ ${synth} = yes ]; then
    check_synth_cname b.wild-cname.example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.wild-cname.example/A >/dev/null && ret=1
  else
    check_nosynth_cname b.wild-cname.example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.wild-cname.example/A >/dev/null || ret=1
  fi
  grep "ns1.example.*.IN.A" dig.out.ns${ns}.test$n >/dev/null || ret=1
  digcomp wildcname.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check synthesized wildcard NODATA 1 NSEC response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts b.wild-1-nsec.example. @10.53.0.${ns} AAAA >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  if [ ${synth} = yes ]; then
    check_synth_soa example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.wild-1-nsec.example/AAAA >/dev/null && ret=1
  else
    check_nosynth_soa example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.wild-1-nsec.example/AAAA >/dev/null || ret=1
  fi
  digcomp wildnodata1nsec.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check synthesized wildcard NODATA 2 NSEC response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts b.wild-2-nsec.example. @10.53.0.${ns} AAAA >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  if [ ${synth} = yes ]; then
    check_synth_soa example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.wild-2-nsec.example/AAAA >/dev/null && ret=1
  else
    check_nosynth_soa example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.wild-2-nsec.example/AAAA >/dev/null || ret=1
  fi
  digcomp wildnodata2nsec.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check synthesized wildcard NODATA 2 NSEC after data response (synth-from-dnssec ${description};) ($n)"
  ret=0
  # Use AAAA to avoid cached qname minimisation _.wild-2-nsec-afterdata.example A record
  dig_with_opts b.wild-2-nsec-afterdata.example. @10.53.0.${ns} AAAA >dig.out.a.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.a.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.a.ns${ns}.test$n || ret=1
  check_nosynth_aaaa b.wild-2-nsec-afterdata.example. dig.out.a.ns${ns}.test$n || ret=1
  #
  nextpart ns1/named.run >/dev/null
  dig_with_opts b.wild-2-nsec-afterdata.example. @10.53.0.${ns} TLSA >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  if [ ${synth} = yes ]; then
    check_synth_soa example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.wild-2-nsec-afterdata.example/TLSA >/dev/null && ret=1
  else
    check_nosynth_soa example. dig.out.ns${ns}.test$n || ret=1
    nextpart ns1/named.run | grep b.wild-2-nsec-afterdata.example/TLSA >/dev/null || ret=1
  fi
  digcomp wildnodata2nsecafterdata.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check insecure NXDOMAIN response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts b.insecure.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.ns${ns}.test$n || ret=1
  check_status NXDOMAIN dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa insecure.example. dig.out.ns${ns}.test$n || ret=1
  nextpart ns1/named.run | grep b.insecure.example/A >/dev/null || ret=1
  digcomp insecure.nxdomain.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check insecure NODATA response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts nodata.insecure.example. @10.53.0.${ns} aaaa >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa insecure.example. dig.out.ns${ns}.test$n || ret=1
  nextpart ns1/named.run | grep nodata.insecure.example/AAAA >/dev/null || ret=1
  digcomp insecure.nodata.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check insecure wildcard response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts b.wild-a.insecure.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  grep "b\.wild-a\.insecure\.example\..*3600.IN.A" dig.out.ns${ns}.test$n >/dev/null || ret=1
  nextpart ns1/named.run | grep b.wild-a.insecure.example/A >/dev/null || ret=1
  digcomp insecure.wild.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check insecure wildcard CNAME response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts b.wild-cname.insecure.example. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_cname b.wild-cname.insecure.example dig.out.ns${ns}.test$n || ret=1
  nextpart ns1/named.run | grep b.wild-cname.insecure.example/A >/dev/null || ret=1
  grep "ns1.insecure.example.*.IN.A" dig.out.ns${ns}.test$n >/dev/null || ret=1
  digcomp insecure.wildcname.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check insecure wildcard NODATA 1 NSEC response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts b.wild-1-nsec.insecure.example. @10.53.0.${ns} AAAA >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa insecure.example. dig.out.ns${ns}.test$n || ret=1
  digcomp insecure.wildnodata1nsec.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check insecure wildcard NODATA 2 NSEC response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts b.wild-2-nsec.insecure.example. @10.53.0.${ns} AAAA >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa insecure.example. dig.out.ns${ns}.test$n || ret=1
  digcomp insecure.wildnodata2nsec.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check insecure wildcard NODATA 2 NSEC after data response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts b.wild-2-nsec-afterdata.insecure.example. @10.53.0.${ns} AAAA >dig.out.a.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.a.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.a.ns${ns}.test$n || ret=1
  check_nosynth_aaaa b.wild-2-nsec-afterdata.insecure.example. dig.out.a.ns${ns}.test$n || ret=1
  #
  dig_with_opts b.wild-2-nsec-afterdata.insecure.example. @10.53.0.${ns} TLSA >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag no dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa insecure.example. dig.out.ns${ns}.test$n || ret=1
  digcomp insecure.wildnodata2nsecafterdata.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check minimal NXDOMAIN response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts nxdomaic.minimal. @10.53.0.${ns} a >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NXDOMAIN dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa minimal. dig.out.ns${ns}.test$n || ret=1
  nextpart ns1/named.run | grep nxdomaic.minimal/A >/dev/null || ret=1
  digcomp minimal.nxdomain.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check black lie NODATA response (synth-from-dnssec ${description};) ($n)"
  ret=0
  nextpart ns1/named.run >/dev/null
  dig_with_opts black.minimal. @10.53.0.${ns} aaaa >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa minimal. dig.out.ns${ns}.test$n || ret=1
  nextpart ns1/named.run | grep black.minimal/AAAA >/dev/null || ret=1
  digcomp black.out dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check bad type map NODATA response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts badtypemap.minimal. @10.53.0.${ns} HINFO >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa minimal. dig.out.ns${ns}.test$n || ret=1
  grep 'badtypemap.minimal.*3600.IN.NSEC.black.minimal. A$' dig.out.ns${ns}.test$n >/dev/null || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check bad type map NODATA response with existent data (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts badtypemap.minimal. @10.53.0.${ns} AAAA >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_aaaa badtypemap.minimal. dig.out.ns${ns}.test$n || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check SOA without DNSKEY bad type map NODATA response (synth-from-dnssec ${description};) ($n)"
  ret=0
  dig_with_opts soa-without-dnskey. @10.53.0.${ns} A >dig.out.ns${ns}.test$n || ret=1
  check_ad_flag $ad dig.out.ns${ns}.test$n || ret=1
  check_status NOERROR dig.out.ns${ns}.test$n || ret=1
  check_nosynth_soa soa-without-dnskey. dig.out.ns${ns}.test$n || ret=1
  grep 'soa-without-dnskey.*3600.IN.NSEC.ns1.soa-without-dnskey. NS SOA RRSIG NSEC$' dig.out.ns${ns}.test$n >/dev/null || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check 'rndc stats' output for 'covering nsec returned' (synth-from-dnssec ${description};) ($n)"
  ret=0
  ${RNDCCMD} 10.53.0.${ns} stats 2>&1 | sed 's/^/ns6 /' | cat_i
  # 2 views, _bind should always be '0 covering nsec returned'
  count=$(grep "covering nsec returned" ns${ns}/named.stats | wc -l)
  test $count = 2 || ret=1
  zero=$(grep " 0 covering nsec returned" ns${ns}/named.stats | wc -l)
  if [ ${synth} = yes ]; then
    test $zero = 1 || ret=1
  else
    test $zero = 2 || ret=1
  fi
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  echo_i "check 'rndc stats' output for 'cache NSEC auxiliary database nodes' (synth-from-dnssec ${description};) ($n)"
  ret=0
  # 2 views, _bind should always be '0 cache NSEC auxiliary database nodes'
  count=$(grep "cache NSEC auxiliary database nodes" ns${ns}/named.stats | wc -l)
  test $count = 2 || ret=1
  zero=$(grep "0 cache NSEC auxiliary database nodes" ns${ns}/named.stats | wc -l)
  if [ ${ad} = yes ]; then
    test $zero = 1 || ret=1
  else
    test $zero = 2 || ret=1
  fi
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  for synthesized in NXDOMAIN no-data wildcard; do
    case $synthesized in
      NXDOMAIN) count=2 ;;
      no-data) count=4 ;;
      wildcard) count=2 ;;
    esac
    echo_i "check 'rndc stats' output for 'synthesized a ${synthesized} response' (synth-from-dnssec ${description};) ($n)"
    ret=0
    if [ ${synth} = yes ]; then
      grep "$count synthesized a ${synthesized} response" ns${ns}/named.stats >/dev/null || ret=1
    else
      grep "synthesized a ${synthesized} response" ns${ns}/named.stats >/dev/null && ret=1
    fi
    n=$((n + 1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  done

  if ${FEATURETEST} --have-libxml2 && [ -x "${CURL}" ] && [ -x "${XMLLINT}" ]; then
    echo_i "getting XML statisistcs for (synth-from-dnssec ${description};) ($n)"
    ret=0
    xml=xml.out$n
    ${CURL} http://10.53.0.${ns}:${EXTRAPORT1}/xml/v3/server >$xml 2>/dev/null || ret=1
    n=$((n + 1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    echo_i "check XML for 'CoveringNSEC' with (synth-from-dnssec ${description};) ($n)"
    ret=0
    count=$("${XMLLINT}" --xpath 'count(/statistics/views/view[@name="_default"]/counters[@type="cachestats"]/counter[@name="CoveringNSEC"])' $xml)
    test $count = 1 || ret=1
    zero=$("${XMLLINT}" --xpath 'count(/statistics/views/view[@name="_default"]/counters[@type="cachestats"]/counter[@name="CoveringNSEC" and text()="0"])' $xml)
    if [ ${synth} = yes ]; then
      test $zero = 0 || ret=1
    else
      test $zero = 1 || ret=1
    fi
    n=$((n + 1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    echo_i "check XML for 'CacheNSECNodes' with (synth-from-dnssec ${description};) ($n)"
    ret=0
    count=$("${XMLLINT}" --xpath 'count(/statistics/views/view[@name="_default"]/counters[@type="cachestats"]/counter[@name="CacheNSECNodes"])' $xml)
    test $count = 1 || ret=1
    zero=$("${XMLLINT}" --xpath 'count(/statistics/views/view[@name="_default"]/counters[@type="cachestats"]/counter[@name="CacheNSECNodes" and text()="0"])' $xml)
    if [ ${ad} = yes ]; then
      test $zero = 0 || ret=1
    else
      test $zero = 1 || ret=1
    fi
    n=$((n + 1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    for synthesized in SynthNXDOMAIN SynthNODATA SynthWILDCARD; do
      case $synthesized in
        SynthNXDOMAIN) count=2 ;;
        SynthNODATA) count=4 ;;
        SynthWILDCARD) count=2 ;;
      esac

      echo_i "check XML for '$synthesized}' with (synth-from-dnssec ${description};) ($n)"
      ret=0
      if [ ${synth} != yes ]; then
        count=0
      fi
      test $("${XMLLINT}" --xpath '/statistics/server/counters[@type="nsstat"]/counter[@name="'"${synthesized}"'"]/text()' $xml) -eq $count || ret=1
      n=$((n + 1))
      if [ $ret != 0 ]; then echo_i "failed"; fi
      status=$((status + ret))
    done
  else
    echo_i "Skipping XML statistics checks"
  fi

  if $FEATURETEST --have-json-c && [ -x "${CURL}" ] && [ -x "${JQ}" ]; then
    echo_i "getting JSON statisistcs for (synth-from-dnssec ${description};) ($n)"
    ret=0
    json=json.out$n
    ${CURL} http://10.53.0.${ns}:${EXTRAPORT1}/json/v1/server >$json 2>/dev/null || ret=1
    n=$((n + 1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    echo_i "check JSON for 'CoveringNSEC' with (synth-from-dnssec ${description};) ($n)"
    ret=0
    count=$("${JQ}" '.views | map(select(.resolver.cachestats | has("CoveringNSEC"))) | length' <$json)
    test $count = 2 || ret=1
    zero=$("${JQ}" '.views | map(select(.resolver.cachestats.CoveringNSEC == 0)) | length' <$json)
    if [ ${synth} = yes ]; then
      test $zero = 1 || ret=1
    else
      test $zero = 2 || ret=1
    fi
    n=$((n + 1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    echo_i "check JSON for 'CacheNSECNodes' with (synth-from-dnssec ${description};) ($n)"
    ret=0
    count=$("${JQ}" '.views | map(select(.resolver.cachestats | has("CacheNSECNodes"))) | length' <$json)
    test $count = 2 || ret=1
    zero=$("${JQ}" '.views | map(select(.resolver.cachestats.CacheNSECNodes == 0)) | length' <$json)
    if [ ${ad} = yes ]; then
      test $zero = 1 || ret=1
    else
      test $zero = 2 || ret=1
    fi
    n=$((n + 1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    for synthesized in SynthNXDOMAIN SynthNODATA SynthWILDCARD; do
      case $synthesized in
        SynthNXDOMAIN) count=2 ;;
        SynthNODATA) count=4 ;;
        SynthWILDCARD) count=2 ;;
      esac

      echo_i "check JSON for '$synthesized}' with (synth-from-dnssec ${description};) ($n)"
      ret=0
      if [ ${synth} = yes ]; then
        test $("${JQ}" ".nsstats.${synthesized}" <$json) -eq $count || ret=1
      else
        "${JQ}" -e '.nsstats | has("'"${synthesized}"'")' <$json >/dev/null && ret=1
      fi
      n=$((n + 1))
      if [ $ret != 0 ]; then echo_i "failed"; fi
      status=$((status + ret))
    done
  else
    echo_i "Skipping JSON statistics checks"
  fi
done

echo_i "check redirect response (+dnssec) (synth-from-dnssec <default>;) ($n)"
ret=0
synth=${synth_default}
dig_with_opts b.redirect. @10.53.0.3 a >dig.out.ns3.test$n || ret=1
check_ad_flag yes dig.out.ns3.test$n || ret=1
check_status NXDOMAIN dig.out.ns3.test$n || ret=1
if [ ${synth} = yes ]; then
  check_synth_soa . dig.out.ns3.test$n || ret=1
else
  check_nosynth_soa . dig.out.ns3.test$n || ret=1
fi
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check redirect response (+nodnssec) (synth-from-dnssec <default>;) ($n)"
ret=0
dig_with_opts +nodnssec b.redirect. @10.53.0.3 a >dig.out.ns3.test$n || ret=1
check_ad_flag no dig.out.ns3.test$n || ret=1
check_status NOERROR dig.out.ns3.test$n || ret=1
grep 'b\.redirect\..*300.IN.A.100\.100\.100\.2' dig.out.ns3.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check DNAME handling (synth-from-dnssec yes;) ($n)"
ret=0
dig_with_opts dnamed.example. ns @10.53.0.5 >dig.out.ns5.test$n || ret=1
dig_with_opts a.dnamed.example. a @10.53.0.5 >dig.out.ns5-1.test$n || ret=1
check_status NOERROR dig.out.ns5-1.test$n || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "regression test for CVE-2022-0635 ($n)"
ret=0
# add DNAME to cache
dig_with_opts dname.dnamed. dname @10.53.0.5 >dig.out.ns5-1.test$n || ret=1
grep "status: NOERROR" dig.out.ns5-1.test$n >/dev/null || ret=1
# add A record to cache at name before DNAME owner
dig_with_opts a.dnamed. a @10.53.0.5 >dig.out.ns5-2.test$n || ret=1
grep "status: NOERROR" dig.out.ns5-2.test$n >/dev/null || ret=1
# add NSEC record to cache at name before DNAME owner
dig_with_opts a.dnamed. aaaa @10.53.0.5 >dig.out.ns5-3.test$n || ret=1
grep "status: NOERROR" dig.out.ns5-3.test$n >/dev/null || ret=1
# wait for NSEC to timeout
sleep 6
# use DNAME for lookup
dig_with_opts b.dname.dnamed a @10.53.0.5 >dig.out.ns5-4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns5-4.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check synth-from-dnssec with grafted zone (forward only) ($n)"
ret=0
#prime cache with NXDOMAIN NSEC covering 'fun' to 'minimal'
dig_with_opts internal @10.53.0.5 >dig.out.ns5-1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns5-1.test$n >/dev/null || ret=1
grep '^fun\..*NSEC.minimal\. ' dig.out.ns5-1.test$n >/dev/null || ret=1
#perform lookup in grafted zone
dig_with_opts example.internal @10.53.0.5 >dig.out.ns5-2.test$n || ret=1
grep "status: NOERROR" dig.out.ns5-2.test$n >/dev/null || ret=1
grep '^example\.internal\..*A.1.2.3.4$' dig.out.ns5-2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check synth-from-dnssec with grafted zone (primary zone) ($n)"
ret=0
#prime cache with NXDOMAIN NSEC covering 'fun' to 'minimal'
dig_with_opts internal @10.53.0.5 >dig.out.ns5-1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns5-1.test$n >/dev/null || ret=1
grep '^fun\..*NSEC.minimal\. ' dig.out.ns5-1.test$n >/dev/null || ret=1
#perform lookup in grafted zone
dig_with_opts example.internal2 @10.53.0.5 >dig.out.ns5-2.test$n || ret=1
grep "status: NOERROR" dig.out.ns5-2.test$n >/dev/null || ret=1
grep '^example\.internal2\..*A.1.2.3.4$' dig.out.ns5-2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
