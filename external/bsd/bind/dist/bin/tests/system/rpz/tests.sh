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

ns1=10.53.0.1			    # root, defining the other two
ns2=10.53.0.2			    # server whose answers are rewritten
ns3=10.53.0.3			    # resolve that does the rewriting
ns4=10.53.0.4			    # another server that is rewritten

RNDCCMD="$RNDC -c ../common/rndc.conf -s $ns3 -p 9953"

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


digcmd () {
    #echo I:dig +noadd +noauth +nosearch +time=1 +tries=1 -p 5300 $* 1>&2
    $DIG +noadd +noauth +nosearch +time=1 +tries=1 -p 5300 $*
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
    $RNDCCMD stop >/dev/null 2>&1
    rm -f ns3/*.jnl
    for NM in ns3/bl*.db; do
	cp -f ns3/base.db $NM
    done
    (cd ..; $PERL start.pl --noclean --restart rpz ns3)
    load_db
}

ckalive () {
    $RNDCCMD status >/dev/null 2>&1 && return 0
    HAVE_CORE=yes
    setret "$1"
    restart
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
    status=`expr $status + $ret`
    ckalive "I:failed; server crashed"
    GROUP_NM=
}

# $1=dig args $2=other dig output file
ckresult () {
    #ckalive "I:server crashed by 'dig $1'" || return 1
    if $PERL ../digcomp.pl $DIGNM $2 >/dev/null; then
	rm -f ${DIGNM}*
	return 0
    fi
    setret "I:'dig $1' wrong; diff $DIGNM $2"
    return 1
}

# check only that the server does not crash
# $1=target domain  $2=optional query type
nocrash () {
    digcmd $* @$ns3 >/dev/null
    ckalive "I:server crashed by 'dig $*'"
}


# check rewrite to NXDOMAIN
# $1=target domain  $2=optional query type
nxdomain () {
    make_dignm
    digcmd +noauth $* @$ns3						\
	| sed -e 's/^[a-z].*	IN	CNAME	/;xxx &/' >$DIGNM
    ckresult "$*" proto.nxdomain
}

# check rewrite to NODATA
# $1=target domain  $2=optional query type
nodata () {
    make_dignm
    digcmd +noauth $* @$ns3						\
	| sed -e 's/^[a-z].*	IN	CNAME	/;xxx &/' >$DIGNM
    ckresult "$*" proto.nodata
}

# check rewrite to an address
#   modify the output so that it is easily compared, but save the original line
# $1=IPv4 address, $2=target domain  $3=optional query type
addr () {
    ADDR=$1
    shift
    ADDR_ESC=`echo "$ADDR" | sed -e 's/\./\\\\./g'`
    make_dignm
    digcmd +noauth $* @$ns3 >$DIGNM
    #ckalive "I:server crashed by 'dig $*'" || return
    if grep -i '^[a-z].*	A	'"$ADDR_ESC"'$' $DIGNM >/dev/null; then
	rm -f ${DIGNM}*
	return 0
    fi
    setret "I:'dig $*' wrong; no A $ADDR record in $DIGNM $2"
}

# check that a response is not rewritten
# $1=target domain  $2=optional query type
nochange () {
    make_dignm
    digcmd $* @$ns3 >$DIGNM
    digcmd $* @$ns2 >${DIGNM}_OK
    ckresult "$*" ${DIGNM}_OK && rm -f ${DIGNM}_OK
}

# check against a 'here document'
here () {
    make_dignm
    sed -e 's/^[	 ]*//' >${DIGNM}_OK
    digcmd $* @$ns3 >$DIGNM
    ckresult "$*" ${DIGNM}_OK
}

# make prototype files to check against rewritten results
digcmd +noauth nonexistent @$ns2 >proto.nxdomain
digcmd +noauth txt-only.tld2 @$ns2 >proto.nodata


status=0

start_group "QNAME rewrites" test1
nochange .
nxdomain a0-1.tld2
nodata a3-1.tld2
nodata a3-2.tld2
nodata sub.a3-2.tld2			# 5 no crash on DNAME
nxdomain a4-2.tld2			# 6 rewrite based on CNAME target
nxdomain a4-2-cname.tld2		# 7
nodata a4-3-cname.tld2			# 8
addr 12.12.12.12  a4-1.sub1.tld2	# 9 A replacement
addr 12.12.12.12  a4-1.sub2.tld2	# 10 A replacement with wildcard
addr 12.12.12.12  nxc1.sub1.tld2	# 11 replace NXDOMAIN with CNAME
addr 12.12.12.12  nxc2.sub1.tld2	# 12 replace NXDOMAIN with CNAME chain
addr 127.0.0.1	  a4-4.tld2		# 13 prefer 1st conflicting QNAME zone
addr 56.56.56.56  a3-6.tld2		# 14 wildcard CNAME
addr 57.57.57.57  a3-7.sub1.tld2	# 15 wildcard CNAME
addr 127.0.0.16	  a4-5-cname3.tld2	# 16 CNAME chain
addr 127.0.0.17	  a4-6-cname3.tld2	# 17 stop short in CNAME chain
nxdomain c1.crash2.tld3			# 18 assert in rbtdb.c
nochange a0-1.tld2      +norecurse
nxdomain a0-1.tld2      +dnssec
nxdomain a0-1.signed-tld2
nochange a0-1.signed-tld2       +dnssec
end_group

start_group "IP rewrites" test2
nodata a3-1.tld2
nochange a3-2.tld2
nochange a4-1.tld2
nxdomain a4-2.tld2
nochange a4-2.tld2 -taaaa
nochange a4-2.tld2 -ttxt
nxdomain a4-2.tld2 -tany
nodata a4-3.tld2
nxdomain a3-1.tld2 -tAAAA
nochange a4-1-aaaa.tld2 -tAAAA
addr 127.0.0.1	 a5-1-2.tld2		# 11 prefer smallest policy address
addr 127.0.0.1	 a5-3.tld2		# 12 prefer first conflicting IP zone
addr 14.14.14.14 a5-4.tld2		# 13 prefer QNAME to IP
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
    nochange a3-1.tld2 +dnssec		# 2 this once caused problems
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
start_group "policy overrides" test5
addr 127.0.0.1 a3-1.tld2		# 1 bl-given
nochange a3-2.tld2			# 2 bl-passthru
nochange a3-3.tld2			# 3 bl-no-op	obsolete for passthru
nochange a3-4.tld2			# 4 bl-disabled
nodata a3-5.tld2			# 5 bl-nodata
nxdomain a3-6.tld2			# 6 bl-nxdomain
here +noauth a3-7.tld2 -tany <<'EOF'	# 7 bl_cname
    ;; status: NOERROR, x
    a3-7.tld2.	    300 IN	CNAME   txt-only.tld2.
    txt-only.tld2.  120 IN	TXT     "txt-only-tld2"
EOF
addr 58.58.58.58 a3-8.tld2		# 8 bl_wildcname
addr 59.59.59.59 a3-9.sub9.tld2		# 9 bl_wildcname
addr 12.12.12.12 a3-10.tld2		# 10 bl-garden
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

# restart the server to see if that creates a core file
if test -z "$HAVE_CORE"; then
    $RNDCCMD halt
    restart
    test -s ns3/named.core && setret "I:found stray core file; memory leak?"
fi

echo "I:exit status: $status"
exit $status
