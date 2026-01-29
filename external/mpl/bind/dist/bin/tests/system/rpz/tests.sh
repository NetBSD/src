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

# test response policy zones (RPZ)

# touch dnsrps-off to not test with DNSRPS
# touch dnsrps-only to not test with classic RPZ

. ../conf.sh

ns=10.53.0
ns1=$ns.1   # root, defining the others
ns2=$ns.2   # authoritative server whose records are rewritten
ns3=$ns.3   # main rewriting resolver
ns4=$ns.4   # another authoritative server that is rewritten
ns5=$ns.5   # another rewriting resolver
ns6=$ns.6   # a forwarding server
ns7=$ns.7   # another rewriting resolver
ns8=$ns.8   # another rewriting resolver
ns9=$ns.9   # another rewriting resolver
ns10=$ns.10 # authoritative server

HAVE_CORE=

NS_PARAMS="-m record -c named.conf -d 99 -g"

status=0
t=0

export DNSRPS_TEST_UPDATE_FILE=$(pwd)/dnsrps.cache
SAVE_RESULTS=
ARGS=
if grep 'dnsrps-enable yes;' dnsrps.conf >/dev/null; then
  MODE=dnsrps
else
  MODE=native
fi

USAGE="$0: [-S]"
while getopts "S:" c; do
  case $c in
    S)
      SAVE_RESULTS=-S
      ARGS="$ARGS -S"
      ;;
    *)
      echo "$USAGE" 1>&2
      exit 1
      ;;
  esac
done
shift $((OPTIND - 1))
if test "$#" -ne 0; then
  echo "$USAGE" 1>&2
  exit 1
fi
# really quit on control-C
trap 'exit 1' 1 2 15

TS='%H:%M:%S '
TS=
comment() {
  if test -n "$TS"; then
    date "+${TS}$*" | cat_i
  fi
}

DNSRPSCMD=./dnsrps
RNDCCMD="$RNDC -c ../_common/rndc.conf -p ${CONTROLPORT} -s"

if test -x "$DNSRPSCMD"; then
  WAIT_CMD="$DNSRPSCMD -w 0.1"
  TEN_SECS=100
else
  WAIT_CMD="sleep 1"
  TEN_SECS=10
fi

digcmd() {
  if test "$1" = TCP; then
    shift
  fi
  # Default to +noauth and @$ns3
  # Also default to -bX where X is the @value so that OS X will choose
  #	    the right IP source address.
  digcmd_args=$(echo "+nocookie +noadd +time=2 +tries=1 -p ${PORT} $*" \
    | sed -e "/@/!s/.*/& @$ns3/" \
      -e '/-b/!s/@\([^ ]*\)/@\1 -b\1/' \
      -e '/+n?o?auth/!s/.*/+noauth &/')
  #echo_i "dig $digcmd_args 1>&2
  $DIG $digcmd_args || return
}

# set DIGNM=file name for dig output
GROUP_NM=
TEST_NUM=0
make_dignm() {
  TEST_NUM=$((TEST_NUM + 1))
  DIGNM=dig.out$GROUP_NM-$TEST_NUM
  while test -f $DIGNM; do
    DIGNM="$DIGNM+"
  done
}

setret() {
  ret=1
  status=$((status + 1))
  echo_i "$*"
}

# set $SN to the SOA serial number of a zone
# $1=domain
# $2=DNS server and client IP address
get_sn() {
  SOA=$($DIG -p ${PORT} +short +norecurse soa "$1" "@$2" "-b$2")
  SN=$(expr "$SOA" : '[^ ]* [^ ]* \([^ ]*\) .*' || true)
  test "$SN" != "" && return
  echo_i "no serial number from \`dig -p ${PORT} soa $1 @$2\` in \"$SOA\""
  exit 1
}

get_sn_fast() {
  RSN=$($DNSRPSCMD -n "$1")
  #echo "dnsrps serial for $1 is $RSN"
  if test -z "$RSN"; then
    echo_i "dnsrps failed to get SOA serial number for $1"
    exit 1
  fi
}

# check that dnsrps provider has zones loaded
# $1=domain
# $2=DNS server IP address
FZONES=$(sed -n -e 's/^zone "\(.*\)".*\(10.53.0..\).*/Z=\1;M=\2/p' dnsrps.zones)
dnsrps_loaded() {
  test "$MODE" = dnsrps || return 0
  n=0
  for V in $FZONES; do
    eval "$V"
    get_sn $Z $M
    while true; do
      get_sn_fast "$Z"
      if test "$SN" -eq "0$RSN"; then
        #echo "$Z @$M serial=$SN"
        break
      fi
      n=$((n + 1))
      if test "$n" -gt $TEN_SECS; then
        echo_i "dnsrps serial for $Z is $RSN instead of $SN"
        exit 1
      fi
      $WAIT_CMD
    done
  done
}

# check the serial number in an SOA to ensure that a policy zone has
#   been (re)loaded
# $1=serial number
# $2=domain
# $3=DNS server
ck_soa() {
  n=0
  while true; do
    if test "$MODE" = dnsrps; then
      get_sn_fast "$2"
      test "$RSN" -eq "$1" && return
    else
      get_sn "$2" "$3"
      test "$SN" -eq "$1" && return
    fi
    n=$((n + 1))
    if test "$n" -gt $TEN_SECS; then
      echo_i "got serial number \"$SN\" instead of \"$1\" from $2 @$3"
      return
    fi
    $WAIT_CMD
  done
}

# (re)load the response policy zones with the rules in the file $TEST_FILE
load_db() {
  if test -n "$TEST_FILE"; then
    for ZONE in bl0 bl1 bl2 bl3 bl4 bl5 bl6 bl7 bl8 bl9 bl10 bl11 bl12 bl13 bl14 bl15 bl16 bl17 bl18 bl19; do
      produce_librpz_rules ns5 $ZONE bl
    done

    produce_librpz_rules ns2 bl.tld2 bl.tld2
    cat $TEST_FILE >>$DNSRPS_TEST_UPDATE_FILE

    if $NSUPDATE -v $TEST_FILE; then
      :
      $RNDCCMD $ns3 sync
    else
      echo_i "failed to update policy zone with $TEST_FILE"
      $RNDCCMD $ns3 sync
      exit 1
    fi
  fi
}

# restart name server
# $1 ns number
# $2 rebuild bl rpz zones if "rebuild-bl-rpz"
restart() {
  # try to ensure that the server really has stopped
  # and won't mess with ns$1/name.pid
  if test -z "$HAVE_CORE" -a -f ns$1/named.pid; then
    $RNDCCMD $ns$1 halt >/dev/null 2>&1
    if test -f ns$1/named.pid; then
      sleep 1
      PID=$(cat ns$1/named.pid 2>/dev/null)
      if test -n "$PID"; then
        echo_i "killing ns$1 server $PID"
        kill -9 $PID
      fi
    fi
  fi
  rm -f ns$1/*.jnl
  if [ "$2" = "rebuild-bl-rpz" ]; then
    if test -f ns$1/base.db; then
      for NM in ns$1/bl*.db; do
        cp -f ns$1/base.db $NM
      done
    fi
  fi
  start_server --noclean --restart --port ${PORT} ns$1
  load_db
  dnsrps_loaded
  sleep 1
}

# $1=server and irrelevant args
# $2=error message
ckalive() {
  CKALIVE_NS=$(expr "$1" : '.*@ns\([1-9]\).*' || true)
  if test -z "$CKALIVE_NS"; then
    CKALIVE_NS=3
  fi
  eval CKALIVE_IP=\$ns$CKALIVE_NS
  $RNDCCMD $CKALIVE_IP status >/dev/null 2>&1 && return 0
  HAVE_CORE=yes
  setret "$2"
  # restart the server to avoid stalling waiting for it to stop
  restart $CKALIVE_NS "rebuild-bl-rpz"
  return 0
}

ckstats() {
  HOST=$1
  LABEL="$2"
  NSDIR="$3"
  EXPECTED="$4"
  $RNDCCMD $HOST stats
  NEW_CNT=$(sed -n -e 's/[	 ]*\([0-9]*\).response policy.*/\1/p' \
    $NSDIR/named.stats | tail -1)
  eval "OLD_CNT=\$${NSDIR}_CNT"
  NEW_CNT=$((NEW_CNT))
  OLD_CNT=$((OLD_CNT))
  GOT=$((NEW_CNT - OLD_CNT))
  if test "$GOT" -ne "$EXPECTED"; then
    setret "wrong $LABEL $NSDIR statistics of $GOT instead of $EXPECTED"
  fi
  eval "${NSDIR}_CNT=$NEW_CNT"
}

ckstatsrange() {
  HOST=$1
  LABEL="$2"
  NSDIR="$3"
  MIN="$4"
  MAX="$5"
  $RNDCCMD $HOST stats
  NEW_CNT=$(sed -n -e 's/[	 ]*\([0-9]*\).response policy.*/\1/p' \
    $NSDIR/named.stats | tail -1)
  eval "OLD_CNT=\$${NSDIR}_CNT"
  NEW_CNT=$((NEW_CNT))
  OLD_CNT=$((OLD_CNT))
  GOT=$((NEW_CNT - OLD_CNT))
  if test "$GOT" -lt "$MIN" -o "$GOT" -gt "$MAX"; then
    setret "wrong $LABEL $NSDIR statistics of $GOT instead of ${MIN}..${MAX}"
  fi
  eval "${NSDIR}_CNT=$NEW_CNT"
}

add_librpz_rule() {
  echo $1 >>$DNSRPS_TEST_UPDATE_FILE
}

produce_librpz_rules() {
  # echo "Producing rules for $1"
  ZONEFILE=$1/$3.db
  cat $ZONEFILE | grep -E -v '^;' | grep -E '\<(A|CNAME)\>' | awk -v zone=$2 '{ if (NF == 4) {print "static add "$1"."zone" "$2" "$3" "$4} else if (NF == 3) {print "static add "$1"."zone" 300 "$2" "$3}}' >>$DNSRPS_TEST_UPDATE_FILE
}

# $1=message
# $2=optional test file name
start_group() {
  ret=0
  t=$((t + 1))
  test -n "$1" && date "+${TS}checking $1 (${t})" | cat_i
  TEST_FILE=$2
  if test -n "$TEST_FILE"; then
    GROUP_NM="-$TEST_FILE"
    load_db
  else
    GROUP_NM=
  fi
  dnsrps_loaded
  TEST_NUM=0
}

end_group() {
  if test -n "$TEST_FILE"; then
    # remove the previous set of test rules
    cp $TEST_FILE tmp
    add_librpz_rule "rollback"
    sed -e 's/[	 ]add[	 ]/ delete /' tmp | $NSUPDATE
    rm -f tmp
    TEST_FILE=
  fi
  ckalive $ns3 "failed; ns3 server crashed and restarted"
  dnsrps_loaded
  GROUP_NM=
}

clean_result() {
  if test -z "$SAVE_RESULTS"; then
    rm -f $*
  fi
}

# $1=dig args
# $2=other dig output file
ckresult() {
  #ckalive "$1" "server crashed by 'dig $1'" || return 1
  expr "$1" : 'TCP ' >/dev/null && tcp=1 || tcp=0
  digarg=${1#TCP }

  if grep "flags:.* aa .*ad;" $DIGNM; then
    setret "'dig $digarg' AA and AD set;"
  elif grep "flags:.* aa .*ad;" $DIGNM; then
    setret "'dig $digarg' AD set;"
  fi

  if $PERL ../digcomp.pl $DIGNM $2 >/dev/null; then
    grep -q 'Truncated, retrying in TCP' $DIGNM && trunc=1 || trunc=0
    if [ "$tcp" -ne "$trunc" ]; then
      setret "'dig $digarg' wrong; no or unexpected truncation in $DIGNM"
    else
      clean_result ${DIGNM}*
    fi
    return 0
  fi
  setret "'dig $digarg' wrong; diff $DIGNM $2"
  return 0
}

# check only that the server does not crash
# $1=target domain
# $2=optional query type
nocrash() {
  digcmd $* >/dev/null
  ckalive "$*" "server crashed by 'dig $*'"
}

# check rewrite to NXDOMAIN
# $1=target domain
# $2=optional query type
nxdomain() {
  make_dignm
  digcmd $* \
    | sed -e 's/^[a-z].*	IN	CNAME	/;xxx &/' \
      -e 's/^[a-z].*	IN	RRSIG	/;xxx &/' \
      >$DIGNM
  ckresult "$*" proto.nxdomain
}

# check rewrite to NODATA
# $1=target domain
# $2=optional query type
nodata() {
  make_dignm
  digcmd $* \
    | sed -e 's/^[a-z].*	IN	CNAME	/;xxx &/' >$DIGNM
  ckresult "$*" proto.nodata
}

# check rewrite to an address
#   modify the output so that it is easily compared, but save the original line
# $1=IPv4 address
# $2=digcmd args
# $3=optional TTL
addr() {
  ADDR=$1
  make_dignm
  digcmd $2 >$DIGNM
  #ckalive "$2" "server crashed by 'dig $2'" || return 1
  ADDR_ESC=$(echo "$ADDR" | sed -e 's/\./\\./g')
  ADDR_TTL=$(sed -n -e "s/^[-.a-z0-9]\{1,\}[	 ]*\([0-9]*\)	IN	AA*	${ADDR_ESC}\$/\1/p" $DIGNM)
  if test -z "$ADDR_TTL"; then
    setret "'dig $2' wrong; no address $ADDR record in $DIGNM"
    return 0
  fi
  if test -n "$3" && test "$ADDR_TTL" -ne "$3"; then
    setret "'dig $2' wrong; TTL=$ADDR_TTL instead of $3 in $DIGNM"
    return 0
  fi
  clean_result ${DIGNM}*
}

# Check that a response is not rewritten
#   Use $ns1 instead of the authority for most test domains, $ns2 to prevent
#   spurious differences for `dig +norecurse`
# $1=optional "TCP"
# remaining args for dig
nochange() {
  make_dignm
  digcmd $* >$DIGNM
  digcmd $* @$ns1 >${DIGNM}_OK
  ckresult "$*" ${DIGNM}_OK && clean_result ${DIGNM}_OK
}

nochange_ns10() {
  make_dignm
  digcmd $* >$DIGNM
  digcmd $* @$ns10 >${DIGNM}_OK
  ckresult "$*" ${DIGNM}_OK && clean_result ${DIGNM}_OK
}

# check against a 'here document'
here() {
  make_dignm
  sed -e 's/^[	 ]*//' >${DIGNM}_OK
  digcmd $* >$DIGNM
  ckresult "$*" ${DIGNM}_OK
}

# check dropped response
DROPPED='^;; no servers could be reached'
drop() {
  make_dignm
  digcmd $* >$DIGNM || true
  if grep "$DROPPED" $DIGNM >/dev/null; then
    clean_result ${DIGNM}*
    return 0
  fi
  setret "'dig $1' wrong; response in $DIGNM"
  return 0
}

nsd() {
  $NSUPDATE -p ${PORT} <<EOF
  server $1
  ttl 300
  update $2 $3 IN CNAME .
  update $2 $4 IN CNAME .
  send
EOF
  sleep 2
}

#
# generate prototype NXDOMAIN response to compare against.
#
make_proto_nxdomain() {
  digcmd nonexistent @$ns2 >proto.nxdomain || return 1
  grep "status: NXDOMAIN" proto.nxdomain >/dev/null || return 1
  return 0
}

#
# generate prototype NODATA response to compare against.
#
make_proto_nodata() {
  digcmd txt-only.tld2 @$ns2 >proto.nodata || return 1
  grep "status: NOERROR" proto.nodata >/dev/null || return 1
  return 0
}

# ensure that the fast-expire zone is populated before we begin testing
$RNDCCMD $ns3 retransfer fast-expire

# make prototype files to check against rewritten results
retry_quiet 10 make_proto_nxdomain
retry_quiet 10 make_proto_nodata

start_group "QNAME rewrites" test1

nochange .                            # 1 do not crash or rewrite root
nxdomain a0-1.tld2                    # 2
nodata a3-1.tld2                      # 3
nodata a3-2.tld2                      # 4 nodata at DNAME itself
nochange sub.a3-2.tld2                # 5 miss where DNAME might work
nxdomain a4-2.tld2                    # 6 rewrite based on CNAME target
nxdomain a4-2-cname.tld2              # 7
nodata a4-3-cname.tld2                # 8
addr 12.12.12.12 a4-1.sub1.tld2       # 9 A replacement
addr 12.12.12.12 a4-1.sub2.tld2       # 10 A replacement with wildcard
addr 12.12.12.12 nxc1.sub1.tld2       # 11 replace NXDOMAIN with CNAME
addr 12.12.12.12 nxc2.sub1.tld2       # 12 replace NXDOMAIN with CNAME chain
addr 127.4.4.1 a4-4.tld2              # 13 prefer 1st conflicting QNAME zone
nochange a6-1.tld2                    # 14
addr 127.6.2.1 a6-2.tld2              # 15
addr 56.56.56.56 a3-6.tld2            # 16 wildcard CNAME
addr 57.57.57.57 a3-7.sub1.tld2       # 17 wildcard CNAME
addr 127.0.0.16 a4-5-cname3.tld2      # 18 CNAME chain
addr 127.0.0.17 a4-6-cname3.tld2      # 19 stop short in CNAME chain
nochange a5-2.tld2 +norecurse         # 20 check that RD=1 is required
nochange a5-3.tld2 +norecurse         # 21
nochange a5-4.tld2 +norecurse         # 22
nochange sub.a5-4.tld2 +norecurse     # 23
nxdomain c1.crash2.tld3               # 24 assert in rbtdb.c
nxdomain a0-1.tld2 +dnssec            # 25 simple DO=1 without signatures
nxdomain a0-1.tld2s +nodnssec         # 26 simple DO=0 with signatures
nochange a0-1.tld2s +dnssec           # 27 simple DO=1 with signatures
nxdomain a0-1s-cname.tld2s +dnssec    # 28 DNSSEC too early in CNAME chain
nochange a0-1-scname.tld2 +dnssec     # 29 DNSSEC on target in CNAME chain
nochange a0-1.tld2s srv +auth +dnssec # 30 no write for DNSSEC and no record
nxdomain a0-1.tld2s srv +nodnssec     # 31
drop a3-8.tld2 any                    # 32 drop
nochange TCP a3-9.tld2                # 33 tcp-only
here x.servfail <<'EOF'               # 34 qname-wait-recurse yes
    ;; status: SERVFAIL, x
EOF
addr 35.35.35.35 "x.servfail @$ns5" # 35 qname-wait-recurse no
end_group
ckstats $ns3 test1 ns3 22
ckstats $ns5 test1 ns5 1
ckstats $ns6 test1 ns6 0

start_group "NXDOMAIN/NODATA action on QNAME trigger" test1
nxdomain a0-1.tld2 @$ns6                 # 1
nodata a3-1.tld2 @$ns6                   # 2
nodata a3-2.tld2 @$ns6                   # 3 nodata at DNAME itself
nxdomain a4-2.tld2 @$ns6                 # 4 rewrite based on CNAME target
nxdomain a4-2-cname.tld2 @$ns6           # 5
nodata a4-3-cname.tld2 @$ns6             # 6
addr 12.12.12.12 "a4-1.sub1.tld2 @$ns6"  # 7 A replacement
addr 12.12.12.12 "a4-1.sub2.tld2 @$ns6"  # 8 A replacement with wildcard
addr 127.4.4.1 "a4-4.tld2 @$ns6"         # 9 prefer 1st conflicting QNAME zone
addr 12.12.12.12 "nxc1.sub1.tld2 @$ns6"  # 10 replace NXDOMAIN w/ CNAME
addr 12.12.12.12 "nxc2.sub1.tld2 @$ns6"  # 11 replace NXDOMAIN w/ CNAME chain
addr 127.6.2.1 "a6-2.tld2 @$ns6"         # 12
addr 56.56.56.56 "a3-6.tld2 @$ns6"       # 13 wildcard CNAME
addr 57.57.57.57 "a3-7.sub1.tld2 @$ns6"  # 14 wildcard CNAME
addr 127.0.0.16 "a4-5-cname3.tld2 @$ns6" # 15 CNAME chain
addr 127.0.0.17 "a4-6-cname3.tld2 @$ns6" # 16 stop short in CNAME chain
nxdomain c1.crash2.tld3 @$ns6            # 17 assert in rbtdb.c
nxdomain a0-1.tld2 +dnssec @$ns6         # 18 simple DO=1 without sigs
nxdomain a0-1s-cname.tld2s +dnssec @$ns6 # 19
drop a3-8.tld2 any @$ns6                 # 20 drop
end_group
ckstatsrange $ns3 test1 ns3 22 30
ckstats $ns5 test1 ns5 0
ckstats $ns6 test1 ns6 0

start_group "IP rewrites" test2
msg='rpz IP address "128.2.0.0.0.0.3.2.2001" is not the canonical "128.2.zz.3.2.2001"'
grep "$msg" ns3/named.run >/dev/null || setret "expected 'is not the canonical' message not logged"
nodata a3-1.tld2                    # 1 NODATA
nochange a3-2.tld2                  # 2 no policy record so no change
nochange a4-1.tld2                  # 3 obsolete PASSTHRU record style
nxdomain a4-2.tld2                  # 4
nochange a4-2.tld2 -taaaa           # 5 no A => no policy rewrite
nochange a4-2.tld2 -ttxt            # 6 no A => no policy rewrite
nxdomain a4-2.tld2 -tany            # 7 no A => no policy rewrite
nodata a4-3.tld2                    # 8
nxdomain a3-1.tld2 -taaaa           # 9 IPv6 policy
nochange a4-1-aaaa.tld2 -taaaa      # 10
addr 127.0.0.1 a5-1-2.tld2          # 11 prefer smallest policy address
addr 127.0.0.1 a5-3.tld2            # 12 prefer first conflicting IP zone
nochange a5-4.tld2 +norecurse       # 13 check that RD=1 is required for #14
addr 14.14.14.14 a5-4.tld2          # 14 prefer QNAME to IP
nochange a4-4.tld2                  # 15 PASSTHRU
nxdomain c2.crash2.tld3             # 16 assert in rbtdb.c
addr 127.0.0.17 "a4-4.tld2 -b $ns1" # 17 client-IP address trigger
nxdomain a7-1.tld2                  # 18 secondary policy zone (RT34450)
# updating an response zone policy
cp ns2/blv2.tld2.db.in ns2/bl.tld2.db
rndc_reload ns2 $ns2 bl.tld2
add_librpz_rule "update zone bl.tld2 1 inc"
ck_soa 2 bl.tld2 $ns3
add_librpz_rule "wipe"
produce_librpz_rules ns2 bl.tld2 bl.tld2
nochange a7-1.tld2 # 19 PASSTHRU
# ensure that a clock tick has occurred so that named will do the reload
sleep 1
cp ns2/blv3.tld2.db.in ns2/bl.tld2.db
rndc_reload ns2 $ns2 bl.tld2
add_librpz_rule "update zone bl.tld2 1 inc"
ck_soa 3 bl.tld2 $ns3
produce_librpz_rules ns2 bl.tld2 bl.tld2
nxdomain a7-1.tld2 # 20 secondary policy zone (RT34450)
end_group
ckstats $ns3 test2 ns3 12

# check that IP addresses for previous group were deleted from the radix tree
start_group "radix tree deletions"
nochange a3-1.tld2
nochange a3-2.tld2
nochange a4-1.tld2
nochange a4-2.tld2
nochange a4-2.tld2 -taaaa
nochange a4-2.tld2 -ttxt
nochange a4-2.tld2 -tany
nochange a4-3.tld2
nochange a3-1.tld2 -tAAAA
nochange a4-1-aaaa.tld2 -tAAAA
nochange a5-1-2.tld2
end_group
ckstats $ns3 'radix tree deletions' ns3 0

# these tests assume "min-ns-dots 0"
start_group "NSDNAME rewrites" test3
nextpart ns3/named.run >/dev/null
nochange a3-1.tld2                     # 1
nochange a3-1.tld2 +dnssec             # 2 this once caused problems
nxdomain a3-1.sub1.tld2                # 3 NXDOMAIN *.sub1.tld2 by NSDNAME
nxdomain a3-1.subsub.sub1.tld2         # 4
nxdomain a3-1.subsub.sub1.tld2 -tany   # 5
addr 12.12.12.12 a4-2.subsub.sub2.tld2 # 6 walled garden for *.sub2.tld2
nochange a3-2.tld2.                    # 7 exempt rewrite by name
nochange a0-1.tld2.                    # 8 exempt rewrite by address block
addr 12.12.12.12 a4-1.tld2             # 9 prefer QNAME policy to NSDNAME
addr 127.0.0.1 a3-1.sub3.tld2          # 10 prefer policy for largest NSDNAME
addr 127.0.0.2 a3-1.subsub.sub3.tld2   # 11
nxdomain xxx.crash1.tld2               # 12 dns_db_detachnode() crash

nxdomain a3-1.stub                     # 13
nxdomain a3-1.static-stub              # 14
nochange_ns10 a3-1.stub-nomatch        # 15
nochange_ns10 a3-1.static-stub-nomatch # 16
nextpart ns3/named.run | grep -F "unrecognized NS rpz_rrset_find() failed: glue" >/dev/null \
  && setret "seen: unrecognized NS rpz_rrset_find() failed: glue"
end_group
ckstats $ns3 test3 ns3 9

# these tests assume "min-ns-dots 0"
start_group "NSIP rewrites" test4
nextpart ns3/named.run >/dev/null
nxdomain a3-1.tld2                     # 1 NXDOMAIN for all of tld2
nochange a3-2.tld2.                    # 2 exempt rewrite by name
nochange a0-1.tld2.                    # 3 exempt rewrite by address block
nochange a3-1.tld4                     # 4 different NS IP address
nxdomain a4-1.stub                     # 5
nxdomain a4-1.static-stub              # 6
nochange_ns10 a4-1.stub-nomatch        # 7
nochange_ns10 a4-1.static-stub-nomatch # 8
nextpart ns3/named.run | grep -F "unrecognized NS rpz_rrset_find() failed: glue" >/dev/null \
  && setret "seen: unrecognized NS rpz_rrset_find() failed: glue"
end_group

start_group "walled garden NSIP rewrites" test4a
addr 41.41.41.41 a3-1.tld2     # 1 walled garden for all of tld2
addr 2041::41 'a3-1.tld2 AAAA' # 2 walled garden for all of tld2
here a3-1.tld2 TXT <<'EOF'     # 3 text message for all of tld2
    ;; status: NOERROR, x
    a3-1.tld2.	    x	IN	TXT   "NSIP walled garden"
EOF
end_group
ckstats $ns3 test4 ns3 6

# policies in ./test5 overridden by response-policy{} in ns3/named.conf
# and in ns5/named.conf
start_group "policy overrides" test5
addr 127.0.0.1 a3-1.tld2            # 1 bl-given
nochange a3-2.tld2                  # 2 bl-passthru
nochange a3-3.tld2                  # 3 bl-no-op (obsolete for passthru)
nochange a3-4.tld2                  # 4 bl-disabled
nodata a3-5.tld2                    # 5 bl-nodata zone recursive-only no
nodata a3-5.tld2 +norecurse         # 6 bl-nodata zone recursive-only no
nodata a3-5.tld2                    # 7 bl-nodata not needed
nxdomain a3-5.tld2 +norecurse @$ns5 # 8 bl-nodata global recursive-only no
nxdomain a3-5.tld2s @$ns5           # 9 bl-nodata global break-dnssec
nxdomain a3-5.tld2s +dnssec @$ns5   # 10 bl-nodata global break-dnssec
nxdomain a3-6.tld2                  # 11 bl-nxdomain
here a3-7.tld2 -tany <<'EOF'        # 12
    ;; status: NOERROR, x
    a3-7.tld2.	    x	IN	CNAME   txt-only.tld2.
    txt-only.tld2.  x	IN	TXT     "txt-only-tld2"
EOF
addr 58.58.58.58 a3-8.tld2             # 13 bl_wildcname
addr 59.59.59.59 a3-9.sub9.tld2        # 14 bl_wildcname
addr 12.12.12.12 a3-15.tld2            # 15 bl-garden via CNAME to a12.tld2
addr 127.0.0.16 a3-16.tld2 100         # 16 bl	max-policy-ttl 100
addr 17.17.17.17 "a3-17.tld2 @$ns5" 90 # 17 ns5 bl max-policy-ttl 90
drop a3-18.tld2 any                    # 18 bl-drop
nxdomain TCP a3-19.tld2                # 19 bl-tcp-only
end_group
ckstats $ns3 test5 ns3 12
ckstats $ns5 test5 ns5 4

# check that miscellaneous bugs are still absent
add_librpz_rule "wipe"
start_group "crashes" test6
for Q in RRSIG SIG ANY 'ANY +dnssec'; do
  nocrash a3-1.tld2 -t$Q
  nocrash a3-2.tld2 -t$Q
  nocrash a3-5.tld2 -t$Q
  nocrash www.redirect -t$Q
  nocrash www.credirect -t$Q
done

# This is not a bug, because any data leaked by writing 24.4.3.2.10.rpz-ip
# (or whatever) is available by publishing "foo A 10.2.3.4" and then
# resolving foo.
# nxdomain 32.3.2.1.127.rpz-ip
end_group
ckstats $ns3 bugs ns3 8

# Ensure ns3 manages to transfer the fast-expire zone before shutdown.
nextpartreset ns3/named.run
wait_for_log 20 "zone fast-expire/IN: transferred serial 1" ns3/named.run

# reconfigure the ns5 primary server without the fast-expire zone, so
# it can't be refreshed on ns3, and will expire in 5 seconds.
cat /dev/null >ns5/expire.conf
rndc_reconfig ns5 10.53.0.5

# restart the main test RPZ server to see if that creates a core file
if test -z "$HAVE_CORE"; then
  stop_server --use-rndc --port ${CONTROLPORT} ns3
  add_librpz_rule "restart"
  restart 3 "rebuild-bl-rpz"
  HAVE_CORE=$(find ns* -name '*core*' -print)
  test -z "$HAVE_CORE" || setret "found $HAVE_CORE; memory leak?"
fi

# look for complaints from lib/dns/rpz.c and bin/name/query.c
for runfile in ns*/named.run; do
  EMSGS=$(nextpart $runfile | grep -E -l 'invalid rpz|rpz.*failed' || true)
  if test -n "$EMSGS"; then
    setret "error messages in $runfile starting with:"
    grep -E 'invalid rpz|rpz.*failed' ns*/named.run \
      | sed -e '10,$d' -e 's/^//' | cat_i
  fi
done

if [ native = "$MODE" ]; then
  # restart the main test RPZ server with a bad zone.
  t=$((t + 1))
  echo_i "checking that ns3 with broken rpz does not crash (${t})"
  stop_server --use-rndc --port ${CONTROLPORT} ns3
  cp ns3/broken.db.in ns3/bl.db
  restart 3 # do not rebuild rpz zones
  nocrash a3-1.tld2 -tA
  stop_server --use-rndc --port ${CONTROLPORT} ns3
  restart 3 "rebuild-bl-rpz"

  t=$((t + 1))
  echo_i "checking if rpz survives a certain class of failed reconfiguration attempts (${t})"
  cp ns3/named2.conf ns3/named.conf
  $RNDCCMD $ns3 reconfig >/dev/null 2>&1 && setret "failed"
  sleep 1
  cp ns3/named1.conf ns3/named.conf
  $RNDCCMD $ns3 reconfig || setret "failed"

  t=$((t + 1))
  echo_i "checking the configured extended DNS error code (EDE) (${t})"
  $DIG -p ${PORT} @$ns3 walled.tld2 >dig.out.$t || setret "failed"
  grep -F "EDE: 4 (Forged Answer)" dig.out.$t >/dev/null || setret "failed"

  t=$((t + 1))
  echo_i "checking the configured extended DNS error code, CNAME override (EDE) (${t})"
  $DIG -p ${PORT} @$ns3 evil.tld2 >dig.out.$t || setret "failed"
  grep -F "EDE: 15 (Blocked)" dig.out.$t >/dev/null || setret "failed"

  t=$((t + 1))
  echo_i "checking the configured extended DNS error code, wildcard CNAME override (EDE) (${t})"
  $DIG -p ${PORT} @$ns3 foo.evil.tld2 >dig.out.$t || setret "failed"
  grep -F "EDE: 15 (Blocked)" dig.out.$t >/dev/null || setret "failed"

  # reload a RPZ zone that is now deliberately broken.
  t=$((t + 1))
  echo_i "checking rpz failed update will keep previous rpz rules (${t})"
  $DIG -p ${PORT} @$ns3 walled.tld2 >dig.out.$t.before || setret "failed"
  grep "walled\.tld2\..*IN.*A.*10\.0\.0\.1" dig.out.$t.before >/dev/null || setret "failed"
  cp ns3/broken.db.in ns3/manual-update-rpz.db
  rndc_reload ns3 $ns3 manual-update-rpz
  sleep 1
  # ensure previous RPZ rules still apply.
  $DIG -p ${PORT} @$ns3 walled.tld2 >dig.out.$t.after || setret "failed"
  grep "walled\.tld2\..*IN.*A.*10\.0\.0\.1" dig.out.$t.after >/dev/null || setret "failed"

  t=$((t + 1))
  echo_i "checking the default (unset) extended DNS error code (EDE) (${t})"
  $DIG -p ${PORT} @$ns3 a6-2.tld2. A >dig.out.$t || setret "failed"
  grep -F "EDE: " dig.out.$t >/dev/null && setret "failed"

  t=$((t + 1))
  echo_i "checking reload of a mixed-case RPZ zone (${t})"
  # First, a sanity check: the A6-2.TLD2.mixed-case-rpz RPZ record should
  # cause a6-2.tld2 NOERROR answers to be rewritten to NXDOMAIN answers.
  $DIG -p ${PORT} @$ns3 a6-2.tld2. A >dig.out.$t.before || setret "failed"
  grep "status: NXDOMAIN" dig.out.$t.before >/dev/null || setret "failed"
  # Add a sibling name (a6-1.tld2.mixed-case-rpz, with "tld2" in lowercase
  # rather than uppercase) before A6-2.TLD.mixed-case-rpz.
  nextpart ns3/named.run >/dev/null
  cp ns3/mixed-case-rpz-2.db.in ns3/mixed-case-rpz.db
  rndc_reload ns3 $ns3 mixed-case-rpz
  wait_for_log 20 "rpz: mixed-case-rpz: reload done" ns3/named.run
  # a6-2.tld2 NOERROR answers should still be rewritten to NXDOMAIN answers.
  # (The bug we try to trigger here caused a6-2.tld2.mixed-case-rpz to be
  # erroneously removed from the summary RPZ database after reload.)
  $DIG -p ${PORT} @$ns3 a6-2.tld2. A >dig.out.$t.after || setret "failed"
  grep "status: NXDOMAIN" dig.out.$t.after >/dev/null || setret "failed"
fi

t=$((t + 1))
echo_i "checking that ttl values are not zeroed when qtype is '*' (${t})"
$DIG +noall +answer -p ${PORT} @$ns3 any a3-2.tld2 >dig.out.$t || setret "failed"
ttl=$(awk '/a3-2 tld2 text/ {print $2}' dig.out.$t)
if test ${ttl:=0} -eq 0; then setret "failed"; fi

t=$((t + 1))
echo_i "checking rpz updates/transfers with parent nodes added after children (${t})"
# regression test for RT #36272: the success condition
# is the secondary server not crashing.
for i in 1 2 3 4 5; do
  nsd $ns5 add example.com.policy1. '*.example.com.policy1.'
  nsd $ns5 delete example.com.policy1. '*.example.com.policy1.'
done
for i in 1 2 3 4 5; do
  nsd $ns5 add '*.example.com.policy1.' example.com.policy1.
  nsd $ns5 delete '*.example.com.policy1.' example.com.policy1.
done

t=$((t + 1))
echo_i "checking that going from an empty policy zone works (${t})"
nsd $ns5 add '*.x.servfail.policy2.' x.servfail.policy2.
add_librpz_rule "update add *.x.servfail.policy2 300 CNAME ."
sleep 1
rndc_reload ns7 $ns7 policy2
$DIG z.x.servfail -p ${PORT} @$ns7 >dig.out.${t} || setret "failed"
grep NXDOMAIN dig.out.${t} >/dev/null || setret "failed"

t=$((t + 1))
echo_i "checking that 'ede none' works same way as when \"ede\" is unset (${t})"
$DIG z.x.servfail -p ${PORT} @$ns7 >dig.out.${t} || setret "failed"
grep -F "EDE: " dig.out.${t} >/dev/null && setret "failed"

t=$((t + 1))
echo_i "checking that 'add-soa no' at rpz zone level works (${t})"
$DIG z.x.servfail -p ${PORT} @$ns7 >dig.out.${t} || setret "failed"
grep "SOA" dig.out.${t} >/dev/null && setret "failed"

if [ native = "$MODE" ]; then
  t=$((t + 1))
  echo_i "checking that 'add-soa yes' at response-policy level works (${t})"
  $DIG walled.tld2 -p ${PORT} +noall +add @$ns3 >dig.out.${t} || setret "failed"
  grep "^manual-update-rpz\..*60.*SOA" dig.out.${t} >/dev/null || setret "failed"
fi

if [ native = "$MODE" ]; then
  t=$((t + 1))
  echo_i "reconfiguring server with 'add-soa no' (${t})"
  cp ns3/named.conf ns3/named.conf.tmp
  sed -e "s/add-soa yes/add-soa no/g" <ns3/named.conf.tmp >ns3/named.conf
  rndc_reconfig ns3 $ns3
  echo_i "checking that 'add-soa no' at response-policy level works (${t})"
  $DIG walled.tld2 -p ${PORT} +noall +add @$ns3 >dig.out.${t} || setret "failed"
  grep "^manual-update-rpz\..*SOA" dig.out.${t} >/dev/null && setret "failed"
fi

if [ native = "$MODE" ]; then
  t=$((t + 1))
  echo_i "checking that 'add-soa unset' works (${t})"
  $DIG walled.tld2 -p ${PORT} +noall +add @$ns8 >dig.out.${t} || setret "failed"
  grep "^manual-update-rpz\..*60.*SOA" dig.out.${t} >/dev/null || setret "failed"
fi

# dnsrps does not allow NS RRs in policy zones, so this check
# with dnsrps results in no rewriting.
if [ native = "$MODE" ]; then
  t=$((t + 1))
  echo_i "checking rpz with delegation fails correctly (${t})"
  $DIG -p ${PORT} @$ns3 ns example.com >dig.out.$t || setret "failed"
  grep "status: SERVFAIL" dig.out.$t >/dev/null || setret "failed"

  t=$((t + 1))
  echo_i "checking policies from expired zone are no longer in effect ($t)"
  $DIG -p ${PORT} @$ns3 a expired >dig.out.$t || setret "failed"
  grep "expired.*10.0.0.10" dig.out.$t >/dev/null && setret "failed"
  grep "fast-expire/IN: response-policy zone expired" ns3/named.run >/dev/null || setret "failed"
fi

# RPZ 'CNAME *.' (NODATA) trumps DNS64.  Test against various DNS64 scenarios.
produce_librpz_rules ns9 rpz rpz
for label in a-only no-a-no-aaaa a-plus-aaaa; do
  for type in AAAA A; do
    t=$((t + 1))
    case $label in
      a-only)
        echo_i "checking rpz 'CNAME *.' (NODATA) with dns64, $type lookup with A-only (${t})"
        ;;
      no-a-no-aaaa)
        echo_i "checking rpz 'CNAME *.' (NODATA) with dns64, $type lookup with no A or AAAA (${t})"
        ;;
      a-plus-aaaa)
        echo_i "checking rpz 'CNAME *.' (NODATA) with dns64, $type lookup with A and AAAA (${t})"
        ;;
    esac
    ret=0
    $DIG ${label}.example -p ${PORT} $type @10.53.0.9 >dig.out.${t} || setret "failed"
    grep "status: NOERROR" dig.out.$t >/dev/null || ret=1
    grep "ANSWER: 0, AUTHORITY: 0, ADDITIONAL: 2$" dig.out.$t >/dev/null || ret=1
    grep "^rpz" dig.out.$t >/dev/null || ret=1
    [ $ret -eq 0 ] || echo_i "failed"
    status=$((status + ret))
  done
done

if [ native = "$MODE" ]; then
  t=$((t + 1))
  echo_i "checking that rewriting CD=1 queries handles pending data correctly (${t})"
  $RNDCCMD $ns3 flush
  $RNDCCMD $ns6 flush
  $DIG a7-2.tld2s -p ${PORT} @$ns6 +cd >dig.out.${t} || setret "failed"
  grep -w "1.1.1.1" dig.out.${t} >/dev/null || setret "failed"

  t=$((t + 1))
  echo_i "checking that 'servfail-until-ready yes' works (part 1) (${t})"
  # Restart ns3 with '-T rpzslow'
  stop_server ns3
  nextpart ns3/named.run >/dev/null
  start_server --noclean --restart --port ${PORT} ns3 -- "-D rpz-ns3 $NS_PARAMS -T rpzslow"
  wait_for_log 10 "all zones loaded" ns3/named.run
  # Just any query that is expected to success normally, but should return
  # SERVFAIL because RPZ is still processing.
  $DIG tld2. NS -p ${PORT} @$ns3 >dig.out.${t} || setret "failed"
  grep "status: SERVFAIL" dig.out.${t} >/dev/null || setret "failed"

  t=$((t + 1))
  echo_i "checking that 'servfail-until-ready yes' works (part 2) (${t})"
  # The 'slow-rpz.' zone has 30 records (RPZ rules), and '-T rpzslow' forces a
  # 100ms delay for each rule. Wait enough time for processing to finish.
  wait_for_log 10 "slow-rpz: reload done" ns3/named.run
  # Now the same request as in the previous test should return NOERROR
  $DIG tld2. NS -p ${PORT} @$ns3 >dig.out.${t} || setret "failed"
  grep "status: NOERROR" dig.out.${t} >/dev/null || setret "failed"
fi

[ $status -eq 0 ] || exit 1
