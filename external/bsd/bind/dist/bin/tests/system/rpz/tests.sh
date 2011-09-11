# Copyright (C) 2011  Internet Systems Consortium, Inc. ("ISC")
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

# Id: tests.sh,v 1.6 2011-06-09 03:10:17 marka Exp

# test response policy zones (RPZ)

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

root=10.53.0.1
s2=10.53.0.2
s3=10.53.0.3

DIGCMD="$DIG +noadd +nosea +nocmd -p 5300"


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


# set DIGNM=file name for dig output
# $1=target domain  $2=optional query type  $3=optional string
dignm () {
    DIGNM=dig.out-$DIGNM_SUB-$1
    if test -n "$3"; then
	DIGNM=$DIGNM-$3
    fi
    if test -n "$2"; then
	DIGNM=$DIGNM-`expr "x$2" : 'x-t *\(.*\)'`
    fi
}

setret () {
    ret=1
    echo "$*"
}

# check rewrite to NXDOMAIN
# $1=target domain  $2=optional query type
nxdomain () {
    dignm $1 "$2"
    $DIGCMD +noauth $1 $2 @$s3 >$DIGNM
    $PERL ../digcomp.pl dig.out-nxdomain $DIGNM || setret "   in $DIGNM"
}

# check rewrite to NODATA
# $1=target domain  $2=optional query type
nodata () {
    dignm $1 "$2"
    $DIGCMD +noauth $1 $2 @$s3 >$DIGNM
    $PERL ../digcomp.pl dig.out-nodata $DIGNM ||  setret "   in $DIGNM"
}

# check rewrite to "A 12.12.12.12"
#   modify the output so that it is easily matched, but save the original line
# $1=target domain  $2=optional query type
a12 () {
    dignm $1 "$2"
    $DIGCMD +noauth $1 $2 @$s3						\
	| sed -e "/^$1\.	/{"					\
	    -e "s/.*/;xxx &/p" -e "s/^;xxx $1/a12.tld2/" -e '}'		\
	>$DIGNM
    $PERL ../digcomp.pl dig.out-a12 $DIGNM || ret=1
}

# check that a response is not rewritten
# $1=target domain  $2=optional query type
nochange () {
    dignm $1 "$2" ok
    DIGNM_OK=$DIGNM
    dignm $1 "$2"
    $DIGCMD $1 $2 @$s3 >$DIGNM
    $DIGCMD $1 $2 @$s2 >$DIGNM_OK
    $PERL ../digcomp.pl $DIGNM_OK $DIGNM || ret=1
}

flush_db () {
    if $RNDC -c ../common/rndc.conf -s $s3 -p 9953 freeze; then : ; else
	echo "I:failed to freeze policy zone $1"
	exit 1
    fi
    if $RNDC -c ../common/rndc.conf -s $s3 -p 9953 thaw; then : ; else
	echo "I:failed to thaw policy zone $1"
	exit 1
    fi
}

# $1=message  $2=test file
start_test () {
    ret=0
    if test -n "$1"; then
	echo "I:checking $1"
    fi
    PREV_FILE=$2
    if test -n "$2"; then
	DIGNM_SUB=`expr "$2" : 'test\(.\)'`
	if $NSUPDATE -v $PREV_FILE; then : ; else
	    echo "I:failed to update policy zone $1 with $2"
	    exit 1
	fi
	#flush_db
    else
	DIGNM_SUB="${DIGNM_SUB}x"
    fi
}

end_test () {
    if test $ret != 0; then
	echo "I:failed"
    else
	rm -f dig.out-${DIGNM_SUB}*
    fi
    if test -n "$PREV_FILE"; then
	sed -e 's/ add / delete /' $PREV_FILE | $NSUPDATE
	status=`expr $status + $ret`
	#flush_db
    fi
}


# make NXDOMAIN and NODATA prototypes
echo "I:making prototype RPZ NXDOMAIN, NODATA, and CNAME results"
$DIGCMD +noauth nonexistent @$s2 >dig.out-nxdomain
$DIGCMD +noauth nodata.tld2 @$s2 >dig.out-nodata
$DIGCMD +noauth a12.tld2    @$s2 >dig.out-a12

status=0

start_test "RPZ QNAME rewrites" test1
nxdomain a0-1.tld2
nodata a1-1.tld2
nodata a1-2.tld2
nodata sub.a1-2.tld2
a12 a4-1.sub1.tld2
end_test

start_test "RPZ IP rewrites" test2
nodata a3-1.tld2
nochange a3-2.tld2
nxdomain a3-99.tld2
nochange a4-1.tld2
nxdomain a4-2.tld2
nochange a4-2.tld2 -taaaa
nochange a4-2.tld2 -ttxt
nxdomain a4-2.tld2 -tany
nodata a4-3.tld2
nxdomain a3-1.tld2 -tAAAA
nochange a4-1-aaaa.tld2 -tAAAA
end_test

start_test "RPZ radix tree deletions"
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
end_test

if ./rpz nsdname; then
    start_test "RPZ NSDNAME rewrites" test3
    nochange a3-1.tld2
    nxdomain a3-1.sub1.tld2
    nxdomain a3-1.sub2.sub1.tld2
    end_test
else
    echo "I:RPZ NSDNAME not checked; named was not built with --enable-rpz-nsdname"
fi

if ./rpz nsip; then
    start_test "RPZ NSIP rewrites" test4
    nxdomain a3-1.tld2
    nochange .
    end_test
else
    echo "I:RPZ NSIP not checked; named was not built with --enable-rpz-nsip"
fi

start_test "RPZ policy overrides" test5
nxdomain a3-1.tld2
nochange a3-2.tld2
nodata a3-3.tld2
nxdomain a3-4.tld2
dignm a3-5.tld2 -tany
$DIGCMD +noauth a3-5.tld2 -tany @$s3 >$DIGNM
if grep CNAME $DIGNM >/dev/null; then : ; else
    echo "'policy cname' failed"
    ret=1
fi
end_test

ret=0
echo "I:checking RRSIG queries"
# We don't actually care about the query results; the important
# thing is the server handles RRSIG queries okay
$DIGCMD a3-1.tld2 -trrsig @$s3 > /dev/null 2>&1
$DIGCMD a3-2.tld2 -trrsig @$s3 > /dev/null 2>&1
$DIGCMD a3-5.tld2 -trrsig @$s3 > /dev/null 2>&1
$DIGCMD www.redirect -trrsig @$s3 > /dev/null 2>&1
$DIGCMD www.cname-redirect -trrsig @$s3 > /dev/null 2>&1

$RNDC -c ../common/rndc.conf -s $s3 -p 9953 status > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then
    echo "I:failed";
    (cd ..; $PERL start.pl --noclean --restart rpz ns3)
fi
status=`expr $status + $ret`

ret=0
echo "I:checking SIG queries"
# We don't actually care about the query results; the important
# thing is the server handles SIG queries okay
$DIGCMD a3-1.tld2 -tsig @$s3 > /dev/null 2>&1
$DIGCMD a3-2.tld2 -tsig @$s3 > /dev/null 2>&1
$DIGCMD a3-5.tld2 -tsig @$s3 > /dev/null 2>&1
$DIGCMD www.redirect -tsig @$s3 > /dev/null 2>&1
$DIGCMD www.cname-redirect -tsig @$s3 > /dev/null 2>&1

$RNDC -c ../common/rndc.conf -s $s3 -p 9953 status > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then
    echo "I:failed";
    (cd ..; $PERL start.pl --noclean --restart rpz ns3)
fi
status=`expr $status + $ret`

ret=0
echo "I:checking ANY queries"
# We don't actually care about the query results; the important
# thing is the server handles SIG queries okay
$DIGCMD a3-1.tld2 -tany @$s3 > /dev/null 2>&1
$DIGCMD a3-2.tld2 -tany @$s3 > /dev/null 2>&1
$DIGCMD a3-5.tld2 -tany @$s3 > /dev/null 2>&1
$DIGCMD www.redirect -tany @$s3 > /dev/null 2>&1
$DIGCMD www.cname-redirect -tany @$s3 > /dev/null 2>&1

$RNDC -c ../common/rndc.conf -s $s3 -p 9953 status > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then
    echo "I:failed";
    (cd ..; $PERL start.pl --noclean --restart rpz ns3)
fi
status=`expr $status + $ret`


if test "$status" -eq 0; then
    rm -f dig.out*
fi

echo "I:exit status: $status"
exit $status
