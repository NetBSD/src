# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# test response rate limiting

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

#set -x

ns1=10.53.0.1			    # root, defining the others
ns2=10.53.0.2			    # test server
ns3=10.53.0.3			    # secondary test server
ns4=10.53.0.4			    # log-only test server
ns7=10.53.0.7			    # whitelisted client

USAGE="$0: [-x]"
while getopts "x" c; do
    case $c in
	x) set -x;;
	*) echo "$USAGE" 1>&2; exit 1;;
    esac
done
shift `expr $OPTIND - 1 || true`
if test "$#" -ne 0; then
    echo "$USAGE" 1>&2
    exit 1
fi
# really quit on control-C
trap 'exit 1' 1 2 15


ret=0
setret () {
    ret=1
    echo_i "$*"
}


# Wait until soon after the start of a second to make results consistent.
#   The start of a second credits a rate limit.
#   This would be far easier in C or by assuming a modern version of perl.
sec_start () {
    START=`date`
    while true; do
	NOW=`date`
	if test "$START" != "$NOW"; then
	    return
	fi
	$PERL -e 'select(undef, undef, undef, 0.05)' || true
    done
}


# turn off ${HOME}/.digrc
HOME=/dev/null; export HOME

#   $1=number of tests  $2=target domain  $3=dig options
QNUM=1
burst () {
    BURST_LIMIT=$1; shift
    BURST_DOM_BASE="$1"; shift

    XCNT=$CNT
    CNT='XXX'
    eval FILENAME="mdig.out-$BURST_DOM_BASE"
    CNT=$XCNT

    DOMS=""
    CNTS=`$PERL -e 'for ( $i = 0; $i < '$BURST_LIMIT'; $i++) { printf "%03d\n", '$QNUM' + $i; }'`
    for CNT in $CNTS
    do
        eval BURST_DOM="$BURST_DOM_BASE"
        DOMS="$DOMS $BURST_DOM"
    done
    ARGS="+nocookie +continue +time=1 +tries=1 -p ${PORT} $* @$ns2 $DOMS"
    $MDIG $ARGS 2>&1 |                                                  \
        tr -d '\r' |                                                    \
        tee -a full-$FILENAME |                                         \
        sed -n -e '/^;; AUTHORITY/,/^$/d'			        \
		-e '/^;; ADDITIONAL/,/^$/d'				\
		-e 's/^[^;].*	\([^	 ]\{1,\}\)$/\1/p'		\
		-e 's/;; flags.* tc .*/TC/p'				\
		-e 's/;; .* status: NXDOMAIN.*/NXDOMAIN/p'		\
		-e 's/;; .* status: NOERROR.*/NOERROR/p'		\
		-e 's/;; .* status: SERVFAIL.*/SERVFAIL/p'		\
		-e 's/response failed with timed out.*/drop/p'		\
		-e 's/;; communications error to.*/drop/p' >> $FILENAME
    QNUM=`expr $QNUM + $BURST_LIMIT`
}

# compare integers $1 and $2; ensure the difference is no more than $3
range () {
    $PERL -e 'if (abs(int($ARGV[0]) - int($ARGV[1])) > int($ARGV[2])) { exit(1) }' $1 $2 $3
}

#   $1=domain  $2=IP address  $3=# of IP addresses  $4=TC  $5=drop
#	$6=NXDOMAIN  $7=SERVFAIL or other errors
ck_result() {
    BAD=no
    ADDRS=`egrep "^$2$" mdig.out-$1				2>/dev/null | wc -l`
    # count simple truncated and truncated NXDOMAIN as TC
    TC=`egrep "^TC|NXDOMAINTC$" mdig.out-$1			2>/dev/null | wc -l`
    DROP=`egrep "^drop$" mdig.out-$1				2>/dev/null | wc -l`
    # count NXDOMAIN and truncated NXDOMAIN as NXDOMAIN
    NXDOMAIN=`egrep "^NXDOMAIN|NXDOMAINTC$" mdig.out-$1		2>/dev/null | wc -l`
    SERVFAIL=`egrep "^SERVFAIL$" mdig.out-$1			2>/dev/null | wc -l`
    NOERROR=`egrep "^NOERROR$" mdig.out-$1			2>/dev/null | wc -l`
    
    range $ADDRS "$3" 1 ||
    setret "$ADDRS instead of $3 '$2' responses for $1" &&
    BAD=yes
    
    range $TC "$4" 1 ||
    setret "$TC instead of $4 truncation responses for $1" &&
    BAD=yes
    
    range $DROP "$5" 1 ||
    setret "$DROP instead of $5 dropped responses for $1" &&
    BAD=yes
    
    range $NXDOMAIN "$6" 1 ||
    setret "$NXDOMAIN instead of $6 NXDOMAIN responses for $1" &&
    BAD=yes
    
    range $SERVFAIL "$7" 1 ||
    setret "$SERVFAIL instead of $7 error responses for $1" &&
    BAD=yes

    range $NOERROR "$8" 1 ||
    setret "$NOERROR instead of $8 NOERROR responses for $1" &&
    BAD=yes
    
    if test -z "$BAD"; then
	rm -f mdig.out-$1
    fi
}


ckstats () {
    LABEL="$1"; shift
    TYPE="$1"; shift
    EXPECTED="$1"; shift
    C=`tr -d '\r' < ns2/named.stats |
        sed -n -e "s/[	 ]*\([0-9]*\).responses $TYPE for rate limits.*/\1/p" |
        tail -1`
    C=`expr 0$C + 0`
    
    range "$C" $EXPECTED 1 ||
    setret "wrong $LABEL $TYPE statistics of $C instead of $EXPECTED"
}


#########
sec_start

# Tests of referrals to "." must be done before the hints are loaded
#   or with "additional-from-cache no"
burst 5 a1.tld3 +norec
# basic rate limiting
burst 3 a1.tld2
# delay allows an additional response.
sleep 1
burst 10 a1.tld2
# Request 30 different qnames to try a wildcard.
burst 30 'x$CNT.a2.tld2'
# These should be counted and limited but are not.  See RT33138.
burst 10 'y.x$CNT.a2.tld2'

#					IP      TC      drop  NXDOMAIN SERVFAIL NOERROR
# referrals to "."
ck_result   a1.tld3	x		0	1	2	0	0	2
# check 13 results including 1 second delay that allows an additional response
ck_result   a1.tld2	192.0.2.1	3	4	6	0	0	8

# Check the wild card answers.
# The parent name of the 30 requests is counted.
ck_result 'x*.a2.tld2'	192.0.2.2	2	10	18	0	0	12

# These should be limited but are not.  See RT33138.
ck_result 'y.x*.a2.tld2' 192.0.2.2	10	0	0	0	0	10

#########
sec_start

burst 10 'x.a3.tld3'
burst 10 'y$CNT.a3.tld3'
burst 10 'z$CNT.a4.tld2'

# 10 identical recursive responses are limited
ck_result 'x.a3.tld3'	192.0.3.3	2	3	5	0	0	5

# 10 different recursive responses are not limited
ck_result 'y*.a3.tld3'	192.0.3.3	10	0	0	0	0	10

# 10 different NXDOMAIN responses are limited based on the parent name.
#   We count 13 responses because we count truncated NXDOMAIN responses
#   as both truncated and NXDOMAIN.
ck_result 'z*.a4.tld2'	x		0	3	5	5	0	0

$RNDCCMD $ns2 stats
ckstats first dropped 36
ckstats first truncated 21


#########
sec_start

burst 10 a5.tld2 +tcp
burst 10 a6.tld2 -b $ns7
burst 10 a7.tld4
burst 2 a8.tld2 -t AAAA
burst 2 a8.tld2 -t TXT
burst 2 a8.tld2 -t SPF

#					IP      TC      drop  NXDOMAIN SERVFAIL NOERROR
# TCP responses are not rate limited
ck_result a5.tld2	192.0.2.5	10	0	0	0	0	10

# whitelisted client is not rate limited
ck_result a6.tld2	192.0.2.6	10	0	0	0	0	10

# Errors such as SERVFAIL are rate limited.
ck_result a7.tld4	x		0	0	8	0	2	0

# NODATA responses are counted as the same regardless of qtype.
ck_result a8.tld2	x		0	2	2	0	0	4

$RNDCCMD $ns2 stats
ckstats second dropped 46
ckstats second truncated 23


#########
sec_start

#					IP      TC      drop  NXDOMAIN SERVFAIL NOERROR
# all-per-second
#   The qnames are all unique but the client IP address is constant.
QNUM=101
burst 60 'all$CNT.a9.tld2'

ck_result 'a*.a9.tld2'	192.0.2.8	50	0	10	0	0	50

$RNDCCMD $ns2 stats
ckstats final dropped 56
ckstats final truncated 23

#########
sec_start

DIGOPTS="+nocookie +nosearch +time=1 +tries=1 +ignore -p ${PORT}"
$DIG $DIGOPTS @$ns4 A a7.tld4 > /dev/null 2>&1
$DIG $DIGOPTS @$ns4 A a7.tld4 > /dev/null 2>&1
$DIG $DIGOPTS @$ns4 A a7.tld4 > /dev/null 2>&1
$DIG $DIGOPTS @$ns4 A a7.tld4 > /dev/null 2>&1
$DIG $DIGOPTS @$ns4 A a7.tld4 > /dev/null 2>&1
$DIG $DIGOPTS @$ns4 A a7.tld4 > /dev/null 2>&1
$DIG $DIGOPTS @$ns4 A a7.tld4 > /dev/null 2>&1
$DIG $DIGOPTS @$ns4 A a7.tld4 > /dev/null 2>&1
$DIG $DIGOPTS @$ns4 A a7.tld4 > /dev/null 2>&1
$DIG $DIGOPTS @$ns4 A a7.tld4 > /dev/null 2>&1
$DIG $DIGOPTS @$ns4 A a7.tld4 > /dev/null 2>&1

grep "would limit" ns4/named.run >/dev/null 2>&1 ||
setret "\"would limit\" not found in log file."

$NAMED -D rrl-ns5 -gc broken.conf > broken.out 2>&1 &
sleep 2
grep "min-table-size 1" broken.out > /dev/null || setret "min-table-size 0 was not changed to 1"

if [ -f named.pid ]; then
    $KILL `cat named.pid`
    setret "named should not have started, but did"
fi

echo_i "exit status: $ret"
[ $ret -eq 0 ] || exit 1
