# Copyright (C) 2011, 2012  Internet Systems Consortium, Inc. ("ISC")
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

# Id: tests.sh,v 1.12 2012/01/07 23:46:53 tbox Exp 

# test response policy zones (RPZ)

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

ns=10.53.0
ns1=$ns.1			    # root, defining the others
ns2=$ns.2			    # server whose answers are rewritten
ns3=$ns.3			    # resolve that does the rewriting
ns4=$ns.4			    # another server that is rewritten
ns5=$ns.5			    # check performance with this server

HAVE_CORE=

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


RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p 9953 -s"

digcmd () {
    digcmd_args="+noadd +nosearch +time=1 +tries=1 -p 5300 $*"
    expr "$digcmd_args" : '.*@' >/dev/null || \
	digcmd_args="$digcmd_args @$ns3"
    expr "$digcmd_args" : '.*+[no]*auth' >/dev/null || \
	digcmd_args="+noauth $digcmd_args"
    #echo I:dig $digcmd_args 1>&2
    $DIG $digcmd_args
}

# set DIGNM=file name for dig output
GROUP_NM=
TEST_NUM=0
make_dignm () {
    TEST_NUM=`expr $TEST_NUM + 1`
    DIGNM=dig.out$GROUP_NM-$TEST_NUM
    while test -f $DIGNM; do
	TEST_NUM="$TEST_NUM+"
	DIGNM=dig.out$GROUP_NM-$TEST_NUM
    done
}

setret () {
    ret=1
    echo "$*"
}

# (re)load the reponse policy zones with the rules in the file $TEST_FILE
load_db () {
    if test -n "$TEST_FILE"; then
	if $NSUPDATE -v $TEST_FILE; then : ; else
	    echo "I:failed to update policy zone with $TEST_FILE"
	    exit 1
	fi
    fi
}

restart () {
    # try to ensure that the server really has stopped
    # and won't mess with ns$1/name.pid
    if test -z "$HAVE_CORE" -a -f ns$1/named.pid; then
	$RNDCCMD $ns$1 halt >/dev/null 2>&1
	if test -f ns$1/named.pid; then
	    sleep 1
	    PID=`cat ns$1/named.pid 2>/dev/null`
	    if test -n "$PID"; then
		echo "I:killing ns$1 server $PID"
		kill -9 $PID
	    fi
	fi
    fi
    rm -f ns$1/*.jnl
    if test -f ns$1/base.db; then
	for NM in ns$1/bl*.db; do
	    cp -f ns$1/base.db $NM
	done
    fi
    $PERL $SYSTEMTESTTOP/start.pl --noclean --restart . ns$1
    load_db
}

# $1=server and irrelevant args  $2=error message
ckalive () {
    CKALIVE_NS=`expr "$1" : '.*@ns\([1-9]\).*'`
    if test -z "$CKALIVE_NS"; then
	CKALIVE_NS=3
    fi
    eval CKALIVE_IP=\$ns$CKALIVE_NS
    $RNDCCMD $CKALIVE_IP status >/dev/null 2>&1 && return 0
    HAVE_CORE=yes
    setret "$2"
    # restart the server to avoid stalling waiting for it to stop
    restart $CKALIVE_NS
    return 1
}

# $1=message  $2=optional test file name
start_group () {
    ret=0
    test -n "$1" && echo "I:checking $1"
    TEST_FILE=$2
    if test -n "$TEST_FILE"; then
	GROUP_NM="-$TEST_FILE"
	load_db
    else
	GROUP_NM=
    fi
    TEST_NUM=0
}

end_group () {
    if test -n "$TEST_FILE"; then
	sed -e 's/[	 ]add[	 ]/ delete /' $TEST_FILE | $NSUPDATE
	TEST_FILE=
    fi
    ckalive $ns3 "I:failed; ns3 server crashed and restarted"
    if test "$status" -eq 0; then
	# look for complaints from rpz.c
	EMSGS=`grep -l 'invalid rpz' */*.run`
	if test -n "$EMSGS"; then
	    setret "I:'invalid rpz' complaints in $EMSGS starting with:"
	    grep 'invalid rpz' */*.run | sed -e '4,$d' -e 's/^/I:    /'
	fi
	# look for complaints from rpz.c and query.c
	EMSGS=`grep -l 'rpz .*failed' */*.run`
	if test -n "$EMSGS"; then
	    setret "I:'rpz failed' complaints in $EMSGS starting with:"
	    grep 'rpz .*failed' */*.run | sed -e '4,$d' -e 's/^/I:    /'
	fi
    fi
    status=`expr $status + $ret`
    GROUP_NM=
}

# $1=dig args $2=other dig output file
ckresult () {
    #ckalive "$1" "I:server crashed by 'dig $1'" || return 1
    if $PERL $SYSTEMTESTTOP/digcomp.pl $DIGNM $2 >/dev/null; then
	rm -f ${DIGNM}*
	return 0
    fi
    setret "I:'dig $1' wrong; diff $DIGNM $2"
    return 1
}

# check only that the server does not crash
# $1=target domain  $2=optional query type
nocrash () {
    digcmd $* >/dev/null
    ckalive "$*" "I:server crashed by 'dig $*'"
}


# check rewrite to NXDOMAIN
# $1=target domain  $2=optional query type
nxdomain () {
    make_dignm
    digcmd $*								\
	| sed -e 's/^[a-z].*	IN	CNAME	/;xxx &/'		\
		-e 's/^[a-z].*	IN	RRSIG	/;xxx &/'		\
	    >$DIGNM
    ckresult "$*" proto.nxdomain
}

# check rewrite to NODATA
# $1=target domain  $2=optional query type
nodata () {
    make_dignm
    digcmd $*								\
	| sed -e 's/^[a-z].*	IN	CNAME	/;xxx &/' >$DIGNM
    ckresult "$*" proto.nodata
}

# check rewrite to an address
#   modify the output so that it is easily compared, but save the original line
# $1=IPv4 address  $2=digcmd args  $3=optional TTL
addr () {
    ADDR=$1
    make_dignm
    digcmd $2 >$DIGNM
    #ckalive "$2" "I:server crashed by 'dig $2'" || return 1
    ADDR_ESC=`echo "$ADDR" | sed -e 's/\./\\\\./g'`
    ADDR_TTL=`sed -n -e  "s/^[-.a-z0-9]\{1,\}	*\([0-9]*\)	IN	A\{1,4\}	${ADDR_ESC}\$/\1/p" $DIGNM`
    if test -z "$ADDR_TTL"; then
	setret "I:'dig $2' wrong; no address $ADDR record in $DIGNM"
	return 1
    fi
    if test -n "$3" && test "$ADDR_TTL" -ne "$3"; then
	setret "I:'dig $2' wrong; TTL=$ADDR_TTL instead of $3 in $DIGNM"
	return 1
    fi
    rm -f ${DIGNM}*
}

# check that a response is not rewritten
# $1=target domain  $2=optional query type
nochange () {
    make_dignm
    digcmd $* >$DIGNM
    digcmd $* @$ns2 >${DIGNM}_OK
    ckresult "$*" ${DIGNM}_OK && rm -f ${DIGNM}_OK
}

# check against a 'here document'
here () {
    make_dignm
    sed -e 's/^[	 ]*//' >${DIGNM}_OK
    digcmd $* >$DIGNM
    ckresult "$*" ${DIGNM}_OK
}

# make prototype files to check against rewritten results
digcmd nonexistent @$ns2 >proto.nxdomain
digcmd txt-only.tld2 @$ns2 >proto.nodata


status=0

start_group "QNAME rewrites" test1
nochange .				# 1 do not crash or rewrite root
nxdomain a0-1.tld2			# 2
nodata a3-1.tld2			# 3
nodata a3-2.tld2			# 4 no crash on DNAME
nodata sub.a3-2.tld2
nxdomain a4-2.tld2			# 6 rewrite based on CNAME target
nxdomain a4-2-cname.tld2		# 7
nodata a4-3-cname.tld2			# 8
addr 12.12.12.12  a4-1.sub1.tld2	# 9 A replacement
addr 12.12.12.12  a4-1.sub2.tld2	# 10 A replacement with wildcard
addr 12.12.12.12  nxc1.sub1.tld2	# 11 replace NXDOMAIN with CNAME
addr 12.12.12.12  nxc2.sub1.tld2	# 12 replace NXDOMAIN with CNAME chain
addr 127.4.4.1	  a4-4.tld2		# 13 prefer 1st conflicting QNAME zone
nochange a6-1.tld2			# 14
addr 127.6.2.1	  a6-2.tld2		# 15
addr 56.56.56.56  a3-6.tld2		# 16 wildcard CNAME
addr 57.57.57.57  a3-7.sub1.tld2	# 17 wildcard CNAME
addr 127.0.0.16	  a4-5-cname3.tld2	# 18 CNAME chain
addr 127.0.0.17	  a4-6-cname3.tld2	# 19 stop short in CNAME chain
nochange a0-1.tld2	    +norecurse	# 20 check that RD=1 is required
nochange a3-1.tld2	    +norecurse	# 21
nochange a3-2.tld2	    +norecurse	# 22
nochange sub.a3-2.tld2	    +norecurse	# 23
nxdomain c1.crash2.tld3			# 24 assert in rbtdb.c
nxdomain a0-1.tld2	    +dnssec	# 25 simple DO=1 without signatures
nxdomain a0-1.tld2s			# 26 simple DO=0 with signatures
nochange a0-1.tld2s	    +dnssec	# 27 simple DO=1 with signatures
nxdomain a0-1s-cname.tld2s  +dnssec	# 28 DNSSEC too early in CNAME chain
nochange a0-1-scname.tld2   +dnssec	# 29 DNSSEC on target in CNAME chain
nochange a0-1.tld2s srv +auth +dnssec	# 30 no write for +DNSSEC and no record
nxdomain a0-1.tld2s srv			# 31
end_group

start_group "IP rewrites" test2
nodata a3-1.tld2			# 1 NODATA
nochange a3-2.tld2			# 2 no policy record so no change
nochange a4-1.tld2			# 3 obsolete PASSTHRU record style
nxdomain a4-2.tld2			# 4
nochange a4-2.tld2 -taaaa		# 5 no A => no policy rewrite
nochange a4-2.tld2 -ttxt		# 6 no A => no policy rewrite
nxdomain a4-2.tld2 -tany		# 7 no A => no policy rewrite
nodata a4-3.tld2			# 8
nxdomain a3-1.tld2 -taaaa		# 9 IPv6 policy
nochange a4-1-aaaa.tld2 -taaaa		# 10
addr 127.0.0.1	 a5-1-2.tld2		# 11 prefer smallest policy address
addr 127.0.0.1	 a5-3.tld2		# 12 prefer first conflicting IP zone
addr 14.14.14.14 a5-4.tld2		# 13 prefer QNAME to IP
nochange a5-4.tld2	    +norecurse	# 14 check that RD=1 is required
nochange a4-4.tld2			# 15 PASSTHRU
nxdomain c2.crash2.tld3			# 16 assert in rbtdb.c
end_group

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

if ./rpz nsdname; then
    start_group "NSDNAME rewrites" test3
    nochange a3-1.tld2
    nochange a3-1.tld2	    +dnssec	# 2 this once caused problems
    nxdomain a3-1.sub1.tld2		# 3 NXDOMAIN *.sub1.tld2 by NSDNAME
    nxdomain a3-1.subsub.sub1.tld2
    nxdomain a3-1.subsub.sub1.tld2 -tany
    addr 12.12.12.12 a4-2.subsub.sub2.tld2 # 6 walled garden for *.sub2.tld2
    nochange a3-2.tld2.			# 7 exempt rewrite by name
    nochange a0-1.tld2.			# 8 exempt rewrite by address block
    addr 12.12.12.12 a4-1.tld2		# 9 prefer QNAME policy to NSDNAME
    addr 127.0.0.1 a3-1.sub3.tld2	# 10 prefer policy for largest NSDNAME
    addr 127.0.0.2 a3-1.subsub.sub3.tld2
    nxdomain xxx.crash1.tld2		# 12 dns_db_detachnode() crash
    end_group
else
    echo "I:NSDNAME not checked; named not configured with --enable-rpz-nsdname"
fi

if ./rpz nsip; then
    start_group "NSIP rewrites" test4
    nxdomain a3-1.tld2			# 1 NXDOMAIN for all of tld2 by NSIP
    nochange a3-2.tld2.			# 2 exempt rewrite by name
    nochange a0-1.tld2.			# 3 exempt rewrite by address block
    nochange a3-1.tld4			# 4 different NS IP address
    end_group
else
    echo "I:NSIP not checked; named not configured with --enable-rpz-nsip"
fi

# policies in ./test5 overridden by response-policy{} in ns3/named.conf
#   and in ns5/named.conf
start_group "policy overrides" test5
addr 127.0.0.1 a3-1.tld2		# 1 bl-given
nochange a3-2.tld2			# 2 bl-passthru
nochange a3-3.tld2			# 3 bl-no-op	obsolete for passthru
nochange a3-4.tld2			# 4 bl-disabled
nodata a3-5.tld2			# 5 bl-nodata
nodata a3-5.tld2    +norecurse		# 6 bl-nodata	    recursive-only no
nodata a3-5.tld2			# 7 bl-nodata
nodata a3-5.tld2    +norecurse	@$ns5	# 8 bl-nodata	    recursive-only no
nodata a3-5.tld2s		@$ns5	# 9 bl-nodata
nodata a3-5.tld2s   +dnssec	@$ns5	# 10 bl-nodata	    break-dnssec
nxdomain a3-6.tld2			# 11 bl-nxdomain
here a3-7.tld2 -tany <<'EOF'
    ;; status: NOERROR, x
    a3-7.tld2.	    x	IN	CNAME   txt-only.tld2.
    txt-only.tld2.  x	IN	TXT     "txt-only-tld2"
EOF
addr 58.58.58.58 a3-8.tld2		# 13 bl_wildcname
addr 59.59.59.59 a3-9.sub9.tld2		# 14 bl_wildcname
addr 12.12.12.12 a3-15.tld2		# 15 bl-garden	via CNAME to a12.tld2
addr 127.0.0.16 a3-16.tld2	    100	# 16 bl		    max-policy-ttl 100
addr 17.17.17.17 "a3-17.tld2 @$ns5" 90	# 17 ns5 bl	    max-policy-ttl 90
end_group

# check that miscellaneous bugs are still absent
start_group "crashes"
for Q in RRSIG SIG ANY 'ANY +dnssec'; do
    nocrash a3-1.tld2 -t$Q
    nocrash a3-2.tld2 -t$Q
    nocrash a3-5.tld2 -t$Q
    nocrash www.redirect -t$Q
    nocrash www.credirect -t$Q
done
end_group


# superficial test for major performance bugs
QPERF=`sh qperf.sh`
if test -n "$QPERF"; then
    perf () {
	echo "I:checking performance $1"
	# don't measure the costs of -d99
	$RNDCCMD $ns5 notrace >/dev/null
	$QPERF -1 -l2 -d ns5/requests -s $ns5 -p 5300 >ns5/$2.perf
	ckalive $ns5 "I:failed; server #5 crashed"
    }
    trim () {
	sed -n -e 's/.*Queries per second: *\([0-9]*\).*/\1/p' ns5/$1.perf
    }

    # Dry run to prime disk cache
    #	Otherwise a first test of either flavor is 25% low
    perf 'to prime disk cache' rpz

    # get queries/second with rpz
    perf 'with rpz' rpz

    # turn off rpz and measure queries/second again
    # Don't wait for a clean stop.  Clean stops of this server need seconds
    # until the sockets are closed.  5 or 10 seconds after that, the
    # server really stops and deletes named.pid.
    echo "# rpz off" >ns5/rpz-switch
    PID=`cat ns5/named.pid`
    test -z "$PID" || kill -9 "$PID"
    $PERL $SYSTEMTESTTOP/start.pl --noclean --restart . ns5
    perf 'without rpz' norpz

    # Don't wait for a clean stop.  Clean stops of this server need seconds
    # until the sockets are closed.  5 or 10 seconds after that, the
    # server really stops and deletes named.pid.
    echo "# rpz off" >ns5/rpz-switch
    PID=`cat ns5/named.pid`
    test -z "$PID" || kill -9 "$PID" && rm -f ns5/named.pid

    NORPZ=`trim norpz`
    RPZ=`trim rpz`
    echo "I:$RPZ qps with RPZ versus $NORPZ qps without"

    # fail if RPZ costs more than 100%
    NORPZ2=`expr "$NORPZ" / 2`
    if test "$RPZ" -le "$NORPZ2"; then
	echo "I:rpz $RPZ qps too far below non-RPZ $NORPZ qps"
	status=`expr $status + 1`
    fi
else
    echo "I:performance not checked; queryperf not available"
fi


# restart the main test RPZ server to see if that creates a core file
if test -z "$HAVE_CORE"; then
    $PERL $SYSTEMTESTTOP/stop.pl . ns3
    restart 3
    HAVE_CORE=`find ns* -name '*core*' -print`
    test -z "$HAVE_CORE" || setret "I:found $HAVE_CORE; memory leak?"
fi


echo "I:exit status: $status"
exit $status
