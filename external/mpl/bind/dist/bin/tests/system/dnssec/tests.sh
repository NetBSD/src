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
. $SYSTEMTESTTOP/conf.sh

status=0
n=1

rm -f dig.out.*

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd +dnssec -p ${PORT}"
ADDITIONALOPTS="+noall +additional +dnssec -p ${PORT}"
ANSWEROPTS="+noall +answer +dnssec -p ${PORT}"
DELVOPTS="-a ns1/trusted.conf -p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

# convert private-type records to readable form
showprivate () {
    echo "-- $@ --"
    $DIG $DIGOPTS +nodnssec +short @$2 -t type65534 $1 | cut -f3 -d' ' |
        while read record; do
            $PERL -e 'my $rdata = pack("H*", @ARGV[0]);
                die "invalid record" unless length($rdata) == 5;
                my ($alg, $key, $remove, $complete) = unpack("CnCC", $rdata);
                my $action = "signing";
                $action = "removing" if $remove;
                my $state = " (incomplete)";
                $state = " (complete)" if $complete;
                print ("$action: alg: $alg, key: $key$state\n");' $record
        done
}

# check that signing records are marked as complete
checkprivate () {
    ret=0
    x=`showprivate "$@"`
    echo $x | grep incomplete >/dev/null 2>&1 && ret=1
    [ $ret = 1 ] && {
        echo "$x"
        echo_i "failed"
    }
    return $ret
}

# check that a zone file is raw format, version 0
israw0 () {
    cat $1 | $PERL -e 'binmode STDIN;
		      read(STDIN, $input, 8);
                      ($style, $version) = unpack("NN", $input);
                      exit 1 if ($style != 2 || $version != 0);'
    return $?
}

# check that a zone file is raw format, version 1
israw1 () {
    cat $1 | $PERL -e 'binmode STDIN;
		      read(STDIN, $input, 8);
                      ($style, $version) = unpack("NN", $input);
                      exit 1 if ($style != 2 || $version != 1);'
    return $?
}

# strip NS and RRSIG NS from input
stripns () {
    awk '($4 == "NS") || ($4 == "RRSIG" && $5 == "NS") { next} { print }' $1
}

# Check that for a query against a validating resolver where the
# authoritative zone is unsigned (insecure delegation), glue is returned
# in the additional section
echo_i "checking that additional glue is returned for unsigned delegation ($n)"
ret=0
$DIG +tcp +dnssec -p ${PORT} a.insecure.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
grep "ANSWER: 1, AUTHORITY: 1, ADDITIONAL: 2" dig.out.ns4.test$n > /dev/null || ret=1
grep "ns\.insecure\.example\..*A.10\.53\.0\.3" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Check the example. domain

echo_i "checking that zone transfer worked ($n)"
for i in 1 2 3 4 5 6 7 8 9
do
	ret=0
	$DIG $DIGOPTS a.example. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
	$DIG $DIGOPTS a.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
	$PERL ../digcomp.pl dig.out.ns2.test$n dig.out.ns3.test$n > /dev/null || ret=1
	[ $ret = 0 ] && break
	sleep 1
done
digcomp dig.out.ns2.test$n dig.out.ns3.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# test AD bit:
#  - dig +adflag asks for authentication (ad in response)
echo_i "checking AD bit asking for validation ($n)"
ret=0
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag a.example. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag a.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# test AD bit:
#  - dig +noadflag
echo_i "checking that AD is not set without +adflag or +dnssec ($n)"
ret=0
$DIG $DIGOPTS +noauth +noadd +nodnssec +noadflag a.example. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth +noadd +nodnssec +noadflag a.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking for AD in authoritative answer ($n)"
ret=0
$DIG $DIGOPTS a.example. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking positive validation NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.example. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth a.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking postive validation NSEC using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a a.example > delv.out$n || ret=1
   grep "a.example..*10.0.0.1" delv.out$n > /dev/null || ret=1
   grep "a.example..*.RRSIG.A 3 2 300 .*" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking positive validation NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth a.nsec3.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.nsec3.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking positive validation NSEC3 using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a a.nsec3.example > delv.out$n || ret=1
   grep "a.nsec3.example..*10.0.0.1" delv.out$n > /dev/null || ret=1
   grep "a.nsec3.example..*RRSIG.A 7 3 300.*" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking positive validation OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth a.optout.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.optout.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking positive validation OPTOUT using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a a.optout.example > delv.out$n || ret=1
   grep "a.optout.example..*10.0.0.1" delv.out$n > /dev/null || ret=1
   grep "a.optout.example..*RRSIG.A 7 3 300.*" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking positive wildcard validation NSEC ($n)"
ret=0
$DIG $DIGOPTS a.wild.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS a.wild.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
stripns dig.out.ns3.test$n > dig.out.ns3.stripped.test$n
stripns dig.out.ns4.test$n > dig.out.ns4.stripped.test$n
digcomp dig.out.ns3.stripped.test$n dig.out.ns4.stripped.test$n || ret=1
grep "\*\.wild\.example\..*RRSIG	NSEC" dig.out.ns4.test$n > /dev/null || ret=1
grep "\*\.wild\.example\..*NSEC	z\.example" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking positive wildcard validation NSEC using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a a.wild.example > delv.out$n || ret=1
   grep "a.wild.example..*10.0.0.27" delv.out$n > /dev/null || ret=1
   grep "a.wild.example..*RRSIG.A 3 2 300.*" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking positive wildcard answer NSEC3 ($n)"
ret=0
$DIG $DIGOPTS a.wild.nsec3.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
grep "AUTHORITY: 4," dig.out.ns3.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking positive wildcard answer NSEC3 ($n)"
ret=0
$DIG $DIGOPTS a.wild.nsec3.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
grep "AUTHORITY: 4," dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking positive wildcard validation NSEC3 ($n)"
ret=0
$DIG $DIGOPTS a.wild.nsec3.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS a.wild.nsec3.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
stripns dig.out.ns3.test$n > dig.out.ns3.stripped.test$n
stripns dig.out.ns4.test$n > dig.out.ns4.stripped.test$n
digcomp dig.out.ns3.stripped.test$n dig.out.ns4.stripped.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking positive wildcard validation NSEC3 using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a a.wild.nsec3.example > delv.out$n || ret=1
   grep "a.wild.nsec3.example..*10.0.0.6" delv.out$n > /dev/null || ret=1
   grep "a.wild.nsec3.example..*RRSIG.A 7 3 300.*" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking positive wildcard validation OPTOUT ($n)"
ret=0
$DIG $DIGOPTS a.wild.optout.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS a.wild.optout.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
stripns dig.out.ns3.test$n > dig.out.ns3.stripped.test$n
stripns dig.out.ns4.test$n > dig.out.ns4.stripped.test$n
digcomp dig.out.ns3.stripped.test$n dig.out.ns4.stripped.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking positive wildcard validation OPTOUT using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a a.wild.optout.example > delv.out$n || ret=1
   grep "a.wild.optout.example..*10.0.0.6" delv.out$n > /dev/null || ret=1
   grep "a.wild.optout.example..*RRSIG.A 7 3 300.*" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking negative validation NXDOMAIN NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth q.example. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth q.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking negative validation NXDOMAIN NSEC using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a q.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: ncache nxdomain" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking negative validation NXDOMAIN NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth q.nsec3.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth q.nsec3.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking negative validation NXDOMAIN NSEC3 using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a q.nsec3.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: ncache nxdomain" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking negative validation NXDOMAIN OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth q.optout.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth q.optout.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking negative validation NXDOMAIN OPTOUT using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a q.optout.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: ncache nxdomain" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking negative validation NODATA NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.example. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth a.example. @10.53.0.4 txt > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking negative validation NODATA OPTOUT using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 txt a.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: ncache nxrrset" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking negative validation NODATA NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth a.nsec3.example. \
	@10.53.0.3 txt > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.nsec3.example. \
	@10.53.0.4 txt > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking negative validation NODATA NSEC3 using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 txt a.nsec3.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: ncache nxrrset" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking negative validation NODATA OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth a.optout.example. \
	@10.53.0.3 txt > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.optout.example. \
	@10.53.0.4 txt > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking negative validation NODATA OPTOUT using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 txt a.optout.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: ncache nxrrset" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking negative wildcard validation NSEC ($n)"
ret=0
$DIG $DIGOPTS b.wild.example. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS b.wild.example. @10.53.0.4 txt > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking negative wildcard validation NSEC using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 txt b.wild.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: ncache nxrrset" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking negative wildcard validation NSEC3 ($n)"
ret=0
$DIG $DIGOPTS b.wild.nsec3.example. @10.53.0.3 txt > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS b.wild.nsec3.example. @10.53.0.4 txt > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking negative wildcard validation NSEC3 using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 txt b.wild.nsec3.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: ncache nxrrset" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking negative wildcard validation OPTOUT ($n)"
ret=0
$DIG $DIGOPTS b.wild.optout.example. \
	@10.53.0.3 txt > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS b.wild.optout.example. \
	@10.53.0.4 txt > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking negative wildcard validation OPTOUT using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 txt b.optout.nsec3.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: ncache nxrrset" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

# Check the insecure.example domain

echo_i "checking 1-server insecurity proof NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.insecure.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.insecure.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking 1-server insecurity proof NSEC using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a a.insecure.example > delv.out$n || ret=1
   grep "a.insecure.example..*10.0.0.1" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking 1-server insecurity proof NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth a.insecure.nsec3.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.insecure.nsec3.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking 1-server insecurity proof NSEC3 using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a a.insecure.nsec3.example > delv.out$n || ret=1
   grep "a.insecure.nsec3.example..*10.0.0.1" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking 1-server insecurity proof OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth a.insecure.optout.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.insecure.optout.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking 1-server insecurity proof OPTOUT using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a a.insecure.optout.example > delv.out$n || ret=1
   grep "a.insecure.optout.example..*10.0.0.1" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking 1-server negative insecurity proof NSEC ($n)"
ret=0
$DIG $DIGOPTS q.insecure.example. a @10.53.0.3 \
	> dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS q.insecure.example. a @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking 1-server negative insecurity proof NSEC using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a q.insecure.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: ncache nxdomain" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking 1-server negative insecurity proof NSEC3 ($n)"
ret=0
$DIG $DIGOPTS q.insecure.nsec3.example. a @10.53.0.3 \
	> dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS q.insecure.nsec3.example. a @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking 1-server negative insecurity proof NSEC3 using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a q.insecure.nsec3.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: ncache nxdomain" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking 1-server negative insecurity proof OPTOUT ($n)"
ret=0
$DIG $DIGOPTS q.insecure.optout.example. a @10.53.0.3 \
	> dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS q.insecure.optout.example. a @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking 1-server negative insecurity proof OPTOUT using dns_client ($n)"
   $DELV $DELVOPTS @10.53.0.4 a q.insecure.optout.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: ncache nxdomain" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking 1-server negative insecurity proof with SOA hack NSEC ($n)"
ret=0
$DIG $DIGOPTS r.insecure.example. soa @10.53.0.3 \
	> dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS r.insecure.example. soa @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
grep "0	IN	SOA" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking 1-server negative insecurity proof with SOA hack NSEC3 ($n)"
ret=0
$DIG $DIGOPTS r.insecure.nsec3.example. soa @10.53.0.3 \
	> dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS r.insecure.nsec3.example. soa @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
grep "0	IN	SOA" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking 1-server negative insecurity proof with SOA hack OPTOUT ($n)"
ret=0
$DIG $DIGOPTS r.insecure.optout.example. soa @10.53.0.3 \
	> dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS r.insecure.optout.example. soa @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
grep "0	IN	SOA" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Check the secure.example domain

echo_i "checking multi-stage positive validation NSEC/NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.secure.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.secure.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking multi-stage positive validation NSEC/NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth a.nsec3.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.nsec3.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking multi-stage positive validation NSEC/OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth a.optout.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.optout.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking multi-stage positive validation NSEC3/NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.secure.nsec3.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.secure.nsec3.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking multi-stage positive validation NSEC3/NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth a.nsec3.nsec3.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.nsec3.nsec3.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking multi-stage positive validation NSEC3/OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth a.optout.nsec3.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.optout.nsec3.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking multi-stage positive validation OPTOUT/NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.secure.optout.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.secure.optout.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking multi-stage positive validation OPTOUT/NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth a.nsec3.optout.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.nsec3.optout.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking multi-stage positive validation OPTOUT/OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth a.optout.optout.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.optout.optout.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking empty NODATA OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth empty.optout.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth empty.optout.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
#grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Check the bogus domain

echo_i "checking failed validation ($n)"
ret=0
$DIG $DIGOPTS a.bogus.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
grep "SERVFAIL" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking failed validation using dns_client ($n)"
   $DELV $DELVOPTS +cd @10.53.0.4 a a.bogus.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: RRSIG failed to verify" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

# Try validating with a bad trusted key.
# This should fail.

echo_i "checking that validation fails with a misconfigured trusted key ($n)"
ret=0
$DIG $DIGOPTS example. soa @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "SERVFAIL" dig.out.ns5.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that negative validation fails with a misconfigured trusted key ($n)"
ret=0
$DIG $DIGOPTS example. ptr @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "SERVFAIL" dig.out.ns5.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that insecurity proofs fail with a misconfigured trusted key ($n)"
ret=0
$DIG $DIGOPTS a.insecure.example. a @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "SERVFAIL" dig.out.ns5.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that validation fails when key record is missing ($n)"
ret=0
$DIG $DIGOPTS a.b.keyless.example. a @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "SERVFAIL" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking that validation fails when key record is missing using dns_client ($n)"
   $DELV $DELVOPTS +cd @10.53.0.4 a a.b.keyless.example > delv.out$n 2>&1 || ret=1
   grep "resolution failed: broken trust chain" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "checking that validation succeeds when a revoked key is encountered ($n)"
ret=0
$DIG $DIGOPTS revkey.example soa @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags: .* ad" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${DELV} ] ; then
   ret=0
   echo_i "checking that validation succeeds when a revoked key is encountered using dns_client ($n)"
   $DELV $DELVOPTS +cd @10.53.0.4 soa revkey.example > delv.out$n 2>&1 || ret=1
   grep "fully validated" delv.out$n > /dev/null || ret=1
   n=`expr $n + 1`
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

echo_i "Checking that a bad CNAME signature is caught after a +CD query ($n)"
ret=0
#prime
$DIG $DIGOPTS +cd bad-cname.example. @10.53.0.4 > dig.out.ns4.prime$n || ret=1
#check: requery with +CD.  pending data should be returned even if it's bogus
expect="a.example.
10.0.0.1"
ans=`$DIG $DIGOPTS +cd +nodnssec +short bad-cname.example. @10.53.0.4` || ret=1
test "$ans" = "$expect" || ret=1
test $ret = 0 || echo_i "failed, got '$ans', expected '$expect'"
#check: requery without +CD.  bogus cached data should be rejected.
$DIG $DIGOPTS +nodnssec bad-cname.example. @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "SERVFAIL" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "Checking that a bad DNAME signature is caught after a +CD query ($n)"
ret=0
#prime
$DIG $DIGOPTS +cd a.bad-dname.example. @10.53.0.4 > dig.out.ns4.prime$n || ret=1
#check: requery with +CD.  pending data should be returned even if it's bogus
expect="example.
a.example.
10.0.0.1"
ans=`$DIG $DIGOPTS +cd +nodnssec +short a.bad-dname.example. @10.53.0.4` || ret=1
test "$ans" = "$expect" || ret=1
test $ret = 0 || echo_i "failed, got '$ans', expected '$expect'"
#check: requery without +CD.  bogus cached data should be rejected.
$DIG $DIGOPTS +nodnssec a.bad-dname.example. @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "SERVFAIL" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Check the insecure.secure.example domain (insecurity proof)

echo_i "checking 2-server insecurity proof ($n)"
ret=0
$DIG $DIGOPTS +noauth a.insecure.secure.example. @10.53.0.2 a \
	> dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth a.insecure.secure.example. @10.53.0.4 a \
	> dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Check a negative response in insecure.secure.example

echo_i "checking 2-server insecurity proof with a negative answer ($n)"
ret=0
$DIG $DIGOPTS q.insecure.secure.example. @10.53.0.2 a > dig.out.ns2.test$n \
	|| ret=1
$DIG $DIGOPTS q.insecure.secure.example. @10.53.0.4 a > dig.out.ns4.test$n \
	|| ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking 2-server insecurity proof with a negative answer and SOA hack ($n)"
ret=0
$DIG $DIGOPTS r.insecure.secure.example. @10.53.0.2 soa > dig.out.ns2.test$n \
	|| ret=1
$DIG $DIGOPTS r.insecure.secure.example. @10.53.0.4 soa > dig.out.ns4.test$n \
	|| ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Check that the query for a security root is successful and has ad set

echo_i "checking security root query ($n)"
ret=0
$DIG $DIGOPTS . @10.53.0.4 key > dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Check that the setting the cd bit works

echo_i "checking cd bit on a positive answer ($n)"
ret=0
$DIG $DIGOPTS +noauth example. soa @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
$DIG $DIGOPTS +noauth +cdflag example. soa @10.53.0.5 \
	> dig.out.ns5.test$n || ret=1
digcomp dig.out.ns4.test$n dig.out.ns5.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking cd bit on a negative answer ($n)"
ret=0
$DIG $DIGOPTS q.example. soa @10.53.0.4 > dig.out.ns4.test$n || ret=1
$DIG $DIGOPTS +cdflag q.example. soa @10.53.0.5 > dig.out.ns5.test$n || ret=1
digcomp dig.out.ns4.test$n dig.out.ns5.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking positive validation RSASHA256 NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.rsasha256.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.rsasha256.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking positive validation RSASHA512 NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.rsasha512.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.rsasha512.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking positive validation with KSK-only DNSKEY signature ($n)"
ret=0
$DIG $DIGOPTS +noauth a.kskonly.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.kskonly.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking cd bit on a query that should fail ($n)"
ret=0
$DIG $DIGOPTS a.bogus.example. soa @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
$DIG $DIGOPTS +cdflag a.bogus.example. soa @10.53.0.5 \
	> dig.out.ns5.test$n || ret=1
digcomp dig.out.ns4.test$n dig.out.ns5.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking cd bit on an insecurity proof ($n)"
ret=0
$DIG $DIGOPTS +noauth a.insecure.example. soa @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
$DIG $DIGOPTS +noauth +cdflag a.insecure.example. soa @10.53.0.5 \
	> dig.out.ns5.test$n || ret=1
digcomp dig.out.ns4.test$n dig.out.ns5.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
# Note - these are looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking cd bit on a negative insecurity proof ($n)"
ret=0
$DIG $DIGOPTS q.insecure.example. a @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
$DIG $DIGOPTS +cdflag q.insecure.example. a @10.53.0.5 \
	> dig.out.ns5.test$n || ret=1
digcomp dig.out.ns4.test$n dig.out.ns5.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
# Note - these are looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that validation of an ANY query works ($n)"
ret=0
$DIG $DIGOPTS +noauth foo.example. any @10.53.0.2 > dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth foo.example. any @10.53.0.4 > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
# 2 records in the zone, 1 NXT, 3 SIGs
grep "ANSWER: 6" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that validation of a query returning a CNAME works ($n)"
ret=0
$DIG $DIGOPTS +noauth cname1.example. txt @10.53.0.2 \
	> dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth cname1.example. txt @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
# the CNAME & its sig, the TXT and its SIG
grep "ANSWER: 4" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that validation of a query returning a DNAME works ($n)"
ret=0
$DIG $DIGOPTS +noauth foo.dname1.example. txt @10.53.0.2 \
	> dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth foo.dname1.example. txt @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
# The DNAME & its sig, the TXT and its SIG, and the synthesized CNAME.
# It would be nice to test that the CNAME is being synthesized by the
# recursive server and not cached, but I don't know how.
grep "ANSWER: 5" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that validation of an ANY query returning a CNAME works ($n)"
ret=0
$DIG $DIGOPTS +noauth cname2.example. any @10.53.0.2 \
	> dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth cname2.example. any @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
# The CNAME, NXT, and their SIGs
grep "ANSWER: 4" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that validation of an ANY query returning a DNAME works ($n)"
ret=0
$DIG $DIGOPTS +noauth foo.dname2.example. any @10.53.0.2 \
	> dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth foo.dname2.example. any @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that positive validation in a privately secure zone works ($n)"
ret=0
$DIG $DIGOPTS +noauth a.private.secure.example. a @10.53.0.2 \
	> dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth a.private.secure.example. a @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that negative validation in a privately secure zone works ($n)"
ret=0
$DIG $DIGOPTS +noauth q.private.secure.example. a @10.53.0.2 \
	> dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth q.private.secure.example. a @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that lookups succeed after disabling a algorithm works ($n)"
ret=0
$DIG $DIGOPTS +noauth example. SOA @10.53.0.2 \
	> dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth example. SOA @10.53.0.6 \
	> dig.out.ns6.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns6.test$n || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns6.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking privately secure to nxdomain works ($n)"
ret=0
$DIG $DIGOPTS +noauth private2secure-nxdomain.private.secure.example. SOA @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
grep "NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking privately secure wildcard to nxdomain works ($n)"
ret=0
$DIG $DIGOPTS +noauth a.wild.private.secure.example. SOA @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
grep "NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking a non-cachable NODATA works ($n)"
ret=0
$DIG $DIGOPTS +noauth a.nosoa.secure.example. txt @10.53.0.7 \
	> dig.out.ns7.test$n || ret=1
grep "AUTHORITY: 0" dig.out.ns7.test$n > /dev/null || ret=1
$DIG $DIGOPTS +noauth a.nosoa.secure.example. txt @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking a non-cachable NXDOMAIN works ($n)"
ret=0
$DIG $DIGOPTS +noauth b.nosoa.secure.example. txt @10.53.0.7 \
	> dig.out.ns7.test$n || ret=1
grep "AUTHORITY: 0" dig.out.ns7.test$n > /dev/null || ret=1
$DIG $DIGOPTS +noauth b.nosoa.secure.example. txt @10.53.0.4 \
	> dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

#
# private.secure.example is served by the same server as its
# grand parent and there is not a secure delegation from secure.example
# to private.secure.example.  In addition secure.example is using a
# algorithm which the validation does not support.
#
echo_i "checking dnssec-lookaside-validation works ($n)"
ret=0
$DIG $DIGOPTS private.secure.example. SOA @10.53.0.6 \
	> dig.out.ns6.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns6.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that we can load a rfc2535 signed zone ($n)"
ret=0
$DIG $DIGOPTS rfc2535.example. SOA @10.53.0.2 \
	> dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that we can transfer a rfc2535 signed zone ($n)"
ret=0
$DIG $DIGOPTS rfc2535.example. SOA @10.53.0.3 \
	> dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "basic dnssec-signzone checks:"
echo_i " two DNSKEYs ($n)"
ret=0
(
cd signer/general
rm -f signed.zone
$SIGNER -f signed.zone -o example.com. test1.zone > signer.out.$n 2>&1
test -f signed.zone
) || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i " one non-KSK DNSKEY ($n)"
ret=0
(
cd signer/general
rm -f signed.zone
$SIGNER -f signed.zone -o example.com. test2.zone > signer.out.$n 2>&1
test -f signed.zone
) && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i " one KSK DNSKEY ($n)"
ret=0
(
cd signer/general
rm -f signed.zone
$SIGNER -f signed.zone -o example.com. test3.zone > signer.out.$n 2>&1
test -f signed.zone
) && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i " three DNSKEY ($n)"
ret=0
(
cd signer/general
rm -f signed.zone
$SIGNER -f signed.zone -o example.com. test4.zone > signer.out.$n 2>&1
test -f signed.zone
) || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i " three DNSKEY, one private key missing ($n)"
ret=0
(
cd signer/general
rm -f signed.zone
$SIGNER -f signed.zone -o example.com. test5.zone > signer.out.$n 2>&1
test -f signed.zone
) || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i " four DNSKEY ($n)"
ret=0
(
cd signer/general
rm -f signed.zone
$SIGNER -f signed.zone -o example.com. test6.zone > signer.out.$n 2>&1
test -f signed.zone
) || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i " two DNSKEY, both private keys missing ($n)"
ret=0
(
cd signer/general
rm -f signed.zone
$SIGNER -f signed.zone -o example.com. test7.zone > signer.out.$n 2>&1
test -f signed.zone
) && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i " two DNSKEY, one private key missing ($n)"
ret=0
(
cd signer/general
rm -f signed.zone
$SIGNER -f signed.zone -o example.com. test8.zone > signer.out.$n 2>&1
test -f signed.zone
) && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that we can sign a zone with out-of-zone records ($n)"
ret=0
zone=example
key1=`$KEYGEN -K signer -q -r $RANDFILE -a NSEC3RSASHA1 -b 1024 -n zone $zone`
key2=`$KEYGEN -K signer -q -r $RANDFILE -f KSK -a NSEC3RSASHA1 -b 1024 -n zone $zone`
(
cd signer
cat example.db.in $key1.key $key2.key > example.db
$SIGNER -o example -f example.db example.db > /dev/null 2>&1
) || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that we can sign a zone (NSEC3) with out-of-zone records ($n)"
ret=0
zone=example
key1=`$KEYGEN -K signer -q -r $RANDFILE -a NSEC3RSASHA1 -b 1024 -n zone $zone`
key2=`$KEYGEN -K signer -q -r $RANDFILE -f KSK -a NSEC3RSASHA1 -b 1024 -n zone $zone`
(
cd signer
cat example.db.in $key1.key $key2.key > example.db
$SIGNER -3 - -H 10 -o example -f example.db example.db > /dev/null 2>&1
awk '/^IQF9LQTLK/ {
		printf("%s", $0);
		while (!index($0, ")")) {
			if (getline <= 0)
				break;
			printf (" %s", $0);
		}
		printf("\n");
	}' example.db | sed 's/[ 	][ 	]*/ /g' > nsec3param.out

grep "IQF9LQTLKKNFK0KVIFELRAK4IC4QLTMG.example. 0 IN NSEC3 1 0 10 - ( IQF9LQTLKKNFK0KVIFELRAK4IC4QLTMG A NS SOA RRSIG DNSKEY NSEC3PARAM )" nsec3param.out > /dev/null
) || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking NSEC3 signing with empty nonterminals above a delegation ($n)"
ret=0
zone=example
key1=`$KEYGEN -K signer -q -r $RANDFILE -a NSEC3RSASHA1 -b 1024 -n zone $zone`
key2=`$KEYGEN -K signer -q -r $RANDFILE -f KSK -a NSEC3RSASHA1 -b 1024 -n zone $zone`
(
cd signer
cat example.db.in $key1.key $key2.key > example3.db
echo "some.empty.nonterminal.nodes.example 60 IN NS ns.example.tld" >> example3.db
$SIGNER -3 - -A -H 10 -o example -f example3.db example3.db > /dev/null 2>&1
awk '/^IQF9LQTLK/ {
		printf("%s", $0);
		while (!index($0, ")")) {
			if (getline <= 0)
				break;
			printf (" %s", $0);
		}
		printf("\n");
	}' example.db | sed 's/[ 	][ 	]*/ /g' > nsec3param.out

grep "IQF9LQTLKKNFK0KVIFELRAK4IC4QLTMG.example. 0 IN NSEC3 1 0 10 - ( IQF9LQTLKKNFK0KVIFELRAK4IC4QLTMG A NS SOA RRSIG DNSKEY NSEC3PARAM )" nsec3param.out > /dev/null
) || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that dnsssec-signzone updates originalttl on ttl changes ($n)"
ret=0
zone=example
key1=`$KEYGEN -K signer -q -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone`
key2=`$KEYGEN -K signer -q -r $RANDFILE -f KSK -a RSASHA1 -b 1024 -n zone $zone`
(
cd signer
cat example.db.in $key1.key $key2.key > example.db
$SIGNER -o example -f example.db.before example.db > /dev/null 2>&1
sed 's/60.IN.SOA./50 IN SOA /' example.db.before > example.db.changed
$SIGNER -o example -f example.db.after example.db.changed > /dev/null 2>&1
)
grep "SOA 5 1 50" signer/example.db.after > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking dnssec-signzone keeps valid signatures from removed keys ($n)"
ret=0
zone=example
key1=`$KEYGEN -K signer -q -r $RANDFILE -f KSK -a RSASHA1 -b 1024 -n zone $zone`
key2=`$KEYGEN -K signer -q -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone`
keyid2=`echo $key2 | sed 's/^Kexample.+005+0*\([0-9]\)/\1/'`
key3=`$KEYGEN -K signer -q -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone`
keyid3=`echo $key3 | sed 's/^Kexample.+005+0*\([0-9]\)/\1/'`
(
cd signer
cat example.db.in $key1.key $key2.key > example.db
$SIGNER -D -o example example.db > /dev/null 2>&1

# now switch out key2 for key3 and resign the zone
cat example.db.in $key1.key $key3.key > example.db
echo '$INCLUDE "example.db.signed"' >> example.db
$SIGNER -D -o example example.db > /dev/null 2>&1
) || ret=1
grep " $keyid2 " signer/example.db.signed > /dev/null 2>&1 || ret=1
grep " $keyid3 " signer/example.db.signed > /dev/null 2>&1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking dnssec-signzone -R purges signatures from removed keys ($n)"
ret=0
(
cd signer
$SIGNER -RD -o example example.db > /dev/null 2>&1
) || ret=1
grep " $keyid2 " signer/example.db.signed > /dev/null 2>&1 && ret=1
grep " $keyid3 " signer/example.db.signed > /dev/null 2>&1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking dnssec-signzone keeps valid signatures from inactive keys ($n)"
ret=0
zone=example
(
cd signer
cp -f example.db.in example.db
$SIGNER -SD -o example example.db > /dev/null 2>&1
echo '$INCLUDE "example.db.signed"' >> example.db
# now retire key2 and resign the zone
$SETTIME -I now $key2 > /dev/null 2>&1
$SIGNER -SD -o example example.db > /dev/null 2>&1
) || ret=1
grep " $keyid2 " signer/example.db.signed > /dev/null 2>&1 || ret=1
grep " $keyid3 " signer/example.db.signed > /dev/null 2>&1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking dnssec-signzone -Q purges signatures from inactive keys ($n)"
ret=0
(
cd signer
$SIGNER -SDQ -o example example.db > /dev/null 2>&1
) || ret=1
grep " $keyid2 " signer/example.db.signed > /dev/null 2>&1 && ret=1
grep " $keyid3 " signer/example.db.signed > /dev/null 2>&1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking dnssec-signzone retains unexpired signatures ($n)"
ret=0
(
cd signer
$SIGNER -Sxt -o example example.db > signer.out.1 2>&1
$SIGNER -Sxt -o example -f example.db.signed example.db.signed > signer.out.2 2>&1
) || ret=1
gen1=`awk '/generated/ {print $3}' signer/signer.out.1`
retain1=`awk '/retained/ {print $3}' signer/signer.out.1`
drop1=`awk '/dropped/ {print $3}' signer/signer.out.1`
gen2=`awk '/generated/ {print $3}' signer/signer.out.2`
retain2=`awk '/retained/ {print $3}' signer/signer.out.2`
drop2=`awk '/dropped/ {print $3}' signer/signer.out.2`
[ "$retain2" -eq `expr "$gen1" + "$retain1"` ] || ret=1
[ "$gen2" -eq 0 ] || ret=1
[ "$drop2" -eq 0 ] || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking dnssec-signzone purges RRSIGs from formerly-owned glue (nsec) ($n)"
ret=0
(
cd signer
# remove NSEC-only keys
rm -f Kexample.+005*
cp -f example.db.in example2.db
cat << EOF >> example2.db
sub1.example. IN A 10.53.0.1
ns.sub2.example. IN A 10.53.0.2
EOF
echo '$INCLUDE "example2.db.signed"' >> example2.db
touch example2.db.signed
$SIGNER -DS -O full -f example2.db.signed -o example example2.db > /dev/null 2>&1
) || ret=1
grep "^sub1\.example\..*RRSIG[ 	]A[ 	]" signer/example2.db.signed > /dev/null 2>&1 || ret=1
grep "^ns\.sub2\.example\..*RRSIG[ 	]A[ 	]" signer/example2.db.signed > /dev/null 2>&1 || ret=1
(
cd signer
cp -f example.db.in example2.db
cat << EOF >> example2.db
sub1.example. IN NS sub1.example.
sub1.example. IN A 10.53.0.1
sub2.example. IN NS ns.sub2.example.
ns.sub2.example. IN A 10.53.0.2
EOF
echo '$INCLUDE "example2.db.signed"' >> example2.db
$SIGNER -DS -O full -f example2.db.signed -o example example2.db > /dev/null 2>&1
) || ret=1
grep "^sub1\.example\..*RRSIG[ 	]A[ 	]" signer/example2.db.signed > /dev/null 2>&1 && ret=1
grep "^ns\.sub2\.example\..*RRSIG[ 	]A[ 	]" signer/example2.db.signed > /dev/null 2>&1 && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking dnssec-signzone purges RRSIGs from formerly-owned glue (nsec3) ($n)"
ret=0
(
cd signer
rm -f example2.db.signed
cp -f example.db.in example2.db
cat << EOF >> example2.db
sub1.example. IN A 10.53.0.1
ns.sub2.example. IN A 10.53.0.2
EOF
echo '$INCLUDE "example2.db.signed"' >> example2.db
touch example2.db.signed
$SIGNER -DS -3 feedabee -O full -f example2.db.signed -o example example2.db > /dev/null 2>&1
) || ret=1
grep "^sub1\.example\..*RRSIG[ 	]A[ 	]" signer/example2.db.signed > /dev/null 2>&1 || ret=1
grep "^ns\.sub2\.example\..*RRSIG[ 	]A[ 	]" signer/example2.db.signed > /dev/null 2>&1 || ret=1
(
cd signer
cp -f example.db.in example2.db
cat << EOF >> example2.db
sub1.example. IN NS sub1.example.
sub1.example. IN A 10.53.0.1
sub2.example. IN NS ns.sub2.example.
ns.sub2.example. IN A 10.53.0.2
EOF
echo '$INCLUDE "example2.db.signed"' >> example2.db
$SIGNER -DS -3 feedabee -O full -f example2.db.signed -o example example2.db > /dev/null 2>&1
) || ret=1
grep "^sub1\.example\..*RRSIG[ 	]A[ 	]" signer/example2.db.signed > /dev/null 2>&1 && ret=1
grep "^ns\.sub2\.example\..*RRSIG[ 	]A[ 	]" signer/example2.db.signed > /dev/null 2>&1 && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking dnssec-signzone output format ($n)"
ret=0
(
cd signer
$SIGNER -O full -f - -Sxt -o example example.db > signer.out.3 2> /dev/null
$SIGNER -O text -f - -Sxt -o example example.db > signer.out.4 2> /dev/null
$SIGNER -O raw -f signer.out.5 -Sxt -o example example.db > /dev/null 2>&1
$SIGNER -O raw=0 -f signer.out.6 -Sxt -o example example.db > /dev/null 2>&1
$SIGNER -O raw -f - -Sxt -o example example.db > signer.out.7 2> /dev/null
) || ret=1
awk '/IN *SOA/ {if (NF != 11) exit(1)}' signer/signer.out.3 || ret=1
awk '/IN *SOA/ {if (NF != 7) exit(1)}' signer/signer.out.4 || ret=1
israw1 signer/signer.out.5 || ret=1
israw0 signer/signer.out.6 || ret=1
israw1 signer/signer.out.7 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking TTLs are capped by dnssec-signzone -M ($n)"
ret=0
(
cd signer
$SIGNER -O full -f signer.out.8 -S -M 30 -o example example.db > /dev/null 2>&1
) || ret=1
awk '/^;/ { next; } $2 > 30 { exit 1; }' signer/signer.out.8 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking dnssec-signzone -N date ($n)"
ret=0
(
cd signer
$SIGNER -O full -f signer.out.9 -S -N date -o example example2.db > /dev/null 2>&1
) || ret=1
now=`$PERL -e '@lt=localtime(); printf "%.4d%0.2d%0.2d00\n",$lt[5]+1900,$lt[4]+1,$lt[3];'`
serial=`awk '/^;/ { next; } $4 == "SOA" { print $7 }' signer/signer.out.9`
[ "$now" -eq "$serial" ] || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking validated data are not cached longer than originalttl ($n)"
ret=0
$DIG $DIGOPTS +ttl +noauth a.ttlpatch.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +ttl +noauth a.ttlpatch.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
grep "3600.IN" dig.out.ns3.test$n > /dev/null || ret=1
grep "300.IN" dig.out.ns3.test$n > /dev/null && ret=1
grep "300.IN" dig.out.ns4.test$n > /dev/null || ret=1
grep "3600.IN" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test that "rndc secroots" is able to dump trusted keys
echo_i "checking rndc secroots ($n)"
ret=0
$RNDCCMD 10.53.0.4 secroots 2>&1 | sed 's/^/ns4 /' | cat_i
keyid=`cat ns1/managed.key.id`
cp ns4/named.secroots named.secroots.test$n
linecount=`grep "./RSAMD5/$keyid ; trusted" named.secroots.test$n | wc -l`
[ "$linecount" -eq 1 ] || ret=1
linecount=`cat named.secroots.test$n | wc -l`
[ "$linecount" -eq 10 ] || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Check direct query for RRSIG.  If we first ask for normal (non RRSIG)
# record, the corresponding RRSIG should be cached and subsequent query
# for RRSIG will be returned with the cached record.
echo_i "checking RRSIG query from cache ($n)"
ret=0
$DIG $DIGOPTS normalthenrrsig.secure.example. @10.53.0.4 a > /dev/null || ret=1
ans=`$DIG $DIGOPTS +short normalthenrrsig.secure.example. @10.53.0.4 rrsig` || ret=1
expect=`$DIG $DIGOPTS +short normalthenrrsig.secure.example. @10.53.0.3 rrsig | grep '^A' ` || ret=1
test "$ans" = "$expect" || ret=1
# also check that RA is set
$DIG $DIGOPTS normalthenrrsig.secure.example. @10.53.0.4 rrsig > dig.out.ns4.test$n || ret=1
grep "flags:.*ra.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Check direct query for RRSIG: If it's not cached with other records,
# it should result in an empty response.
echo_i "checking RRSIG query not in cache ($n)"
ret=0
ans=`$DIG $DIGOPTS +short rrsigonly.secure.example. @10.53.0.4 rrsig` || ret=1
test -z "$ans" || ret=1
# also check that RA is cleared
$DIG $DIGOPTS rrsigonly.secure.example. @10.53.0.4 rrsig > dig.out.ns4.test$n || ret=1
grep "flags:.*ra.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

#
# RT21868 regression test.
#
echo_i "checking NSEC3 zone with mismatched NSEC3PARAM / NSEC parameters ($n)"
ret=0
$DIG $DIGOPTS non-exist.badparam. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

#
# RT22007 regression test.
#
echo_i "checking optout NSEC3 referral with only insecure delegations ($n)"
ret=0
$DIG $DIGOPTS +norec delegation.single-nsec3. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "3KL3NK1HKQ4IUEEHBEF12VGFKUETNBAN.*NSEC3 1 1 1 - 3KL3NK1HKQ4IUEEHBEF12VGFKUETNBAN" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking optout NSEC3 NXDOMAIN with only insecure delegations ($n)"
ret=0
$DIG $DIGOPTS +norec nonexist.single-nsec3. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
grep "3KL3NK1HKQ4IUEEHBEF12VGFKUETNBAN.*NSEC3 1 1 1 - 3KL3NK1HKQ4IUEEHBEF12VGFKUETNBAN" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi

status=`expr $status + $ret`
echo_i "checking optout NSEC3 nodata with only insecure delegations ($n)"
ret=0
$DIG $DIGOPTS +norec single-nsec3. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "3KL3NK1HKQ4IUEEHBEF12VGFKUETNBAN.*NSEC3 1 1 1 - 3KL3NK1HKQ4IUEEHBEF12VGFKUETNBAN" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that a zone finishing the transition from RSASHA1 to RSASHA256 validates secure ($n)"
ret=0
$DIG $DIGOPTS ns algroll. @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking positive and negative validation with negative trust anchors ($n)"
ret=0

#
# check correct initial behavior
#
$DIG $DIGOPTS a.bogus.example. a @10.53.0.4 > dig.out.ns4.test$n.1 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.1 > /dev/null || ret=1
$DIG $DIGOPTS badds.example. soa @10.53.0.4 > dig.out.ns4.test$n.2 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.2 > /dev/null || ret=1
$DIG $DIGOPTS a.secure.example. a @10.53.0.4 > dig.out.ns4.test$n.3 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.3 > /dev/null && ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.3 > /dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed - checking initial state"; fi
status=`expr $status + $ret`
ret=0

#
# add negative trust anchors
#
$RNDCCMD 10.53.0.4 nta -f -l 20s bogus.example 2>&1 | sed 's/^/ns4 /' | cat_i
$RNDCCMD 10.53.0.4 nta badds.example 2>&1 | sed 's/^/ns4 /' | cat_i
# reconfig should maintain NTAs
$RNDCCMD 10.53.0.4 reconfig 2>&1 | sed 's/^/ns4 /' | cat_i
$RNDCCMD 10.53.0.4 nta -d > rndc.out.ns4.test$n.1
lines=`wc -l < rndc.out.ns4.test$n.1`
[ "$lines" -eq 2 ] || ret=1
$RNDCCMD 10.53.0.4 nta secure.example 2>&1 | sed 's/^/ns4 /' | cat_i
$RNDCCMD 10.53.0.4 nta fakenode.secure.example 2>&1 | sed 's/^/ns4 /' | cat_i
# reload should maintain NTAs
$RNDCCMD 10.53.0.4 reload 2>&1 | sed 's/^/ns4 /' | cat_i
$RNDCCMD 10.53.0.4 nta -d > rndc.out.ns4.test$n.2
lines=`wc -l < rndc.out.ns4.test$n.2`
[ "$lines" -eq 4 ] || ret=1
start=`$PERL -e 'print time()."\n";'`

if [ $ret != 0 ]; then echo_i "failed - adding NTA's failed"; fi
status=`expr $status + $ret`
ret=0

#
# check behavior with NTA's in place
#
$DIG $DIGOPTS a.bogus.example. a @10.53.0.4 > dig.out.ns4.test$n.4 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.4 > /dev/null && ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.4 > /dev/null && ret=1
$DIG $DIGOPTS badds.example. soa @10.53.0.4 > dig.out.ns4.test$n.5 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.5 > /dev/null && ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.5 > /dev/null && ret=1
$DIG $DIGOPTS a.secure.example. a @10.53.0.4 > dig.out.ns4.test$n.6 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.6 > /dev/null && ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.6 > /dev/null && ret=1
$DIG $DIGOPTS a.fakenode.secure.example. a @10.53.0.4 > dig.out.ns4.test$n.7 || ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.7 > /dev/null && ret=1
echo_i "dumping secroots"
$RNDCCMD 10.53.0.4 secroots | sed 's/^/ns4 /' | cat_i
grep "bogus.example: expiry" ns4/named.secroots > /dev/null || ret=1
grep "badds.example: expiry" ns4/named.secroots > /dev/null || ret=1
grep "secure.example: expiry" ns4/named.secroots > /dev/null || ret=1
grep "fakenode.secure.example: expiry" ns4/named.secroots > /dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed - with NTA's in place failed"; fi
status=`expr $status + $ret`
ret=0

echo_i "waiting for NTA rechecks/expirations"

#
# secure.example and badds.example used default nta-duration
# (configured as 10s in ns4/named1.conf), but nta recheck interval
# is configured to 7s, so at t=8 the NTAs for secure.example and
# fakenode.secure.example should both be lifted, but badds.example
# should still be going.
#
$PERL -e 'my $delay =  '$start' + 10 - time(); select(undef, undef, undef, $delay) if ($delay > 0);'
$DIG $DIGOPTS b.secure.example. a @10.53.0.4 > dig.out.ns4.test$n.8 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.8 > /dev/null && ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.8 > /dev/null || ret=1
$DIG $DIGOPTS b.fakenode.secure.example. a @10.53.0.4 > dig.out.ns4.test$n.9 || ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.9 > /dev/null || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n.9 > /dev/null || ret=1
$DIG $DIGOPTS badds.example. soa @10.53.0.4 > dig.out.ns4.test$n.10 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.10 > /dev/null && ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.10 > /dev/null && ret=1

if [ $ret != 0 ]; then echo_i "failed - checking that default nta's were lifted due to recheck"; fi
status=`expr $status + $ret`
ret=0

#
# bogus.example was set to expire in 20s, so at t=11
# it should still be NTA'd, but badds.example used the default
# lifetime of 10s, so it should revert to SERVFAIL now.
#
$PERL -e 'my $delay = '$start' + 13 - time(); select(undef, undef, undef, $delay) if ($delay > 0);'
# check nta table
$RNDCCMD 10.53.0.4 nta -d > rndc.out.ns4.test$n._11
lines=`grep " expiry " rndc.out.ns4.test$n._11 | wc -l`
[ "$lines" -le 2 ] || ret=1
grep "bogus.example: expiry" rndc.out.ns4.test$n._11 > /dev/null || ret=1
grep "badds.example: expiry" rndc.out.ns4.test$n._11 > /dev/null && ret=1
$DIG $DIGOPTS b.bogus.example. a @10.53.0.4 > dig.out.ns4.test$n.11 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.11 > /dev/null && ret=1
$DIG $DIGOPTS a.badds.example. a @10.53.0.4 > dig.out.ns4.test$n.12 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.12 > /dev/null || ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.12 > /dev/null && ret=1
$DIG $DIGOPTS c.secure.example. a @10.53.0.4 > dig.out.ns4.test$n.13 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.13 > /dev/null && ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.13 > /dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed - checking that default nta's were lifted due to lifetime"; fi
status=`expr $status + $ret`
ret=0

#
# at t=21, all the NTAs should have expired.
#
$PERL -e 'my $delay = '$start' + 21 - time(); select(undef, undef, undef, $delay) if ($delay > 0);'
# check correct behavior after bogus.example expiry
$DIG $DIGOPTS d.secure.example. a @10.53.0.4 > dig.out.ns4.test$n.14 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.14 > /dev/null && ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.14 > /dev/null || ret=1
$DIG $DIGOPTS c.bogus.example. a @10.53.0.4 > dig.out.ns4.test$n.15 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.15 > /dev/null || ret=1
# check nta table has been cleaned up now
$RNDCCMD 10.53.0.4 nta -d > rndc.out.ns4.test$n.3
lines=`grep " expiry " rndc.out.ns4.test$n.3 | wc -l`
[ "$lines" -eq 0 ] || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed - checking that all nta's have been lifted"; fi
status=`expr $status + $ret`
ret=0

echo_i "testing NTA removals ($n)"
$RNDCCMD 10.53.0.4 nta badds.example 2>&1 | sed 's/^/ns4 /' | cat_i
$RNDCCMD 10.53.0.4 nta -d > rndc.out.ns4.test$n.1
grep "badds.example: expiry" rndc.out.ns4.test$n.1 > /dev/null || ret=1
$DIG $DIGOPTS a.badds.example. a @10.53.0.4 > dig.out.ns4.test$n.1 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.1 > /dev/null && ret=1
grep "^a.badds.example." dig.out.ns4.test$n.1 > /dev/null || ret=1
$RNDCCMD 10.53.0.4 nta -remove badds.example > rndc.out.ns4.test$n.2
grep "Negative trust anchor removed: badds.example/_default" rndc.out.ns4.test$n.2 > /dev/null || ret=1
$RNDCCMD 10.53.0.4 nta -d > rndc.out.ns4.test$n.3
grep "badds.example: expiry" rndc.out.ns4.test$n.3 > /dev/null && ret=1
$DIG $DIGOPTS a.badds.example. a @10.53.0.4 > dig.out.ns4.test$n.2 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.2 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
ret=0

echo_i "remove non-existent NTA three times"
$RNDCCMD 10.53.0.4 nta -r foo > rndc.out.ns4.test$n.4 2>&1
$RNDCCMD 10.53.0.4 nta -remove foo > rndc.out.ns4.test$n.5 2>&1
$RNDCCMD 10.53.0.4 nta -r foo > rndc.out.ns4.test$n.6 2>&1
grep "'nta' failed: not found" rndc.out.ns4.test$n.6 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
ret=0

n=`expr $n + 1`
echo_i "testing NTA with bogus lifetimes ($n)"
echo_i "check with no nta lifetime specified"
$RNDCCMD 10.53.0.4 nta -l "" foo > rndc.out.ns4.test$n.1 2>&1
grep "'nta' failed: bad ttl" rndc.out.ns4.test$n.1 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
ret=0

echo_i "check with bad nta lifetime"
$RNDCCMD 10.53.0.4 nta -l garbage foo > rndc.out.ns4.test$n.2 2>&1
grep "'nta' failed: bad ttl" rndc.out.ns4.test$n.2 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
ret=0

echo_i "check with too long nta lifetime"
$RNDCCMD 10.53.0.4 nta -l 7d1h foo > rndc.out.ns4.test$n.3 2>&1
grep "'nta' failed: out of range" rndc.out.ns4.test$n.3 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
ret=0

#
# check NTA persistence across restarts
#
n=`expr $n + 1`
echo_i "testing NTA persistence across restarts ($n)"
$RNDCCMD 10.53.0.4 nta -d > rndc.out.ns4.test$n.1
lines=`grep " expiry " rndc.out.ns4.test$n.1 | wc -l`
[ "$lines" -eq 0 ] || ret=1
$RNDCCMD 10.53.0.4 nta -f -l 30s bogus.example 2>&1 | sed 's/^/ns4 /' | cat_i
$RNDCCMD 10.53.0.4 nta -f -l 10s badds.example 2>&1 | sed 's/^/ns4 /' | cat_i
$RNDCCMD 10.53.0.4 nta -d > rndc.out.ns4.test$n.2
lines=`grep " expiry " rndc.out.ns4.test$n.2 | wc -l`
[ "$lines" -eq 2 ] || ret=1
start=`$PERL -e 'print time()."\n";'`

if [ $ret != 0 ]; then echo_i "failed - NTA persistence: adding NTA's failed"; fi
status=`expr $status + $ret`
ret=0

echo_i "killing ns4 with SIGTERM"
cd ns4
$KILL -TERM `cat named.pid`
rm -f named.pid
cd ..

#
# ns4 has now shutdown. wait until t=14 when badds.example's NTA
# (lifetime=10s) would have expired, and then restart ns4.
#
echo_i "waiting till 14s have passed since NTAs were added before restarting ns4"
$PERL -e 'my $delay = '$start' + 14 - time(); select(undef, undef, undef, $delay) if ($delay > 0);'

if
        $PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} . ns4
then
        echo_i "restarted server ns4"
else
        echo_i "could not restart server ns4"
        exit 1
fi

echo_i "sleeping for an additional 4 seconds for ns4 to fully startup"
sleep 4

#
# ns4 should be back up now. The NTA for bogus.example should still be
# valid, whereas badds.example should not have been added during named
# startup (as it had already expired), the fact that it's ignored should
# be logged.
#
$RNDCCMD 10.53.0.4 nta -d > rndc.out.ns4.test$n.3
lines=`wc -l < rndc.out.ns4.test$n.3`
[ "$lines" -eq 1 ] || ret=1
grep "bogus.example: expiry" rndc.out.ns4.test$n.3 > /dev/null || ret=1
$DIG $DIGOPTS b.bogus.example. a @10.53.0.4 > dig.out.ns4.test$n.4 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.4 > /dev/null && ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.4 > /dev/null && ret=1
$DIG $DIGOPTS a.badds.example. a @10.53.0.4 > dig.out.ns4.test$n.5 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.5 > /dev/null || ret=1
grep "ignoring expired NTA at badds.example" ns4/named.run > /dev/null || ret=1

# cleanup
$RNDCCMD 10.53.0.4 nta -remove bogus.example > rndc.out.ns4.test$n.6

if [ $ret != 0 ]; then echo_i "failed - NTA persistence: restoring NTA failed"; fi
status=`expr $status + $ret`
ret=0

#
# check "regular" attribute in NTA file works as expected at named
# startup.
#
n=`expr $n + 1`
echo_i "testing loading regular attribute from NTA file ($n)"
$RNDCCMD 10.53.0.4 nta -d > rndc.out.ns4.test$n.1 2>/dev/null
lines=`wc -l < rndc.out.ns4.test$n.1`
[ "$lines" -eq 0 ] || ret=1
# initially, secure.example. validates with AD=1
$DIG $DIGOPTS a.secure.example. a @10.53.0.4 > dig.out.ns4.test$n.2 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.2 > /dev/null && ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.2 > /dev/null || ret=1

echo_i "killing ns4 with SIGTERM"
cd ns4
$KILL -TERM `cat named.pid`
rm -f named.pid
cd ..

echo_i "sleeping for an additional 4 seconds for ns4 to fully shutdown"
sleep 4

#
# ns4 has now shutdown. add NTA for secure.example. directly into the
# _default.nta file with the regular attribute and some future timestamp.
#
year=`date +%Y`
future="`expr 20 + ${year}`0101010000"
echo "secure.example. regular $future" > ns4/_default.nta
start=`$PERL -e 'print time()."\n";'`

if
        $PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} . ns4
then
        echo_i "restarted server ns4"
else
        echo_i "could not restart server ns4"
        exit 1
fi

# nta-recheck is configured as 7s, so at t=10 the NTAs for
# secure.example. should be lifted as it is not a forced NTA.
echo_i "waiting till 10s have passed after ns4 was restarted"
$PERL -e 'my $delay = '$start' + 10 - time(); select(undef, undef, undef, $delay) if ($delay > 0);'

# secure.example. should now return an AD=1 answer (still validates) as
# the NTA has been lifted.
$DIG $DIGOPTS a.secure.example. a @10.53.0.4 > dig.out.ns4.test$n.3 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.3 > /dev/null && ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.3 > /dev/null || ret=1

# cleanup
$RNDCCMD 10.53.0.4 nta -remove secure.example > rndc.out.ns4.test$n.4 2>/dev/null

if [ $ret != 0 ]; then echo_i "failed - NTA persistence: loading regular NTAs failed"; fi
status=`expr $status + $ret`
ret=0

#
# check "forced" attribute in NTA file works as expected at named
# startup.
#
n=`expr $n + 1`
echo_i "testing loading forced attribute from NTA file ($n)"
$RNDCCMD 10.53.0.4 nta -d > rndc.out.ns4.test$n.1 2>/dev/null
lines=`wc -l < rndc.out.ns4.test$n.1`
[ "$lines" -eq 0 ] || ret=1
# initially, secure.example. validates with AD=1
$DIG $DIGOPTS a.secure.example. a @10.53.0.4 > dig.out.ns4.test$n.2 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.2 > /dev/null && ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.2 > /dev/null || ret=1

echo_i "killing ns4 with SIGTERM"
cd ns4
$KILL -TERM `cat named.pid`
rm -f named.pid
cd ..

echo_i "sleeping for an additional 4 seconds for ns4 to fully shutdown"
sleep 4

#
# ns4 has now shutdown. add NTA for secure.example. directly into the
# _default.nta file with the forced attribute and some future timestamp.
#
echo "secure.example. forced $future" > ns4/_default.nta
start=`$PERL -e 'print time()."\n";'`

if
        $PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} . ns4
then
        echo_i "restarted server ns4"
else
        echo_i "could not restart server ns4"
        exit 1
fi

# nta-recheck is configured as 7s, but even at t=10 the NTAs for
# secure.example. should not be lifted as it is a forced NTA.
echo_i "waiting till 10s have passed after ns4 was restarted"
$PERL -e 'my $delay = '$start' + 10 - time(); select(undef, undef, undef, $delay) if ($delay > 0);'

# secure.example. should now return an AD=0 answer (non-authenticated)
# as the NTA is still there.
$DIG $DIGOPTS a.secure.example. a @10.53.0.4 > dig.out.ns4.test$n.3 || ret=1
grep "status: SERVFAIL" dig.out.ns4.test$n.3 > /dev/null && ret=1
grep "flags:[^;]* ad[^;]*;" dig.out.ns4.test$n.3 > /dev/null && ret=1

# cleanup
$RNDCCMD 10.53.0.4 nta -remove secure.example > rndc.out.ns4.test$n.4 2>/dev/null

if [ $ret != 0 ]; then echo_i "failed - NTA persistence: loading forced NTAs failed"; fi
status=`expr $status + $ret`
ret=0

#
# check that NTA lifetime read from file is clamped to 1 week.
#
n=`expr $n + 1`
echo_i "testing loading out of bounds lifetime from NTA file ($n)"

echo_i "killing ns4 with SIGTERM"
cd ns4
$KILL -TERM `cat named.pid`
rm -f named.pid
cd ..

echo_i "sleeping for an additional 4 seconds for ns4 to fully shutdown"
sleep 4

#
# ns4 has now shutdown. add NTA for secure.example. directly into the
# _default.nta file with a lifetime well into the future.
#
echo "secure.example. forced $future" > ns4/_default.nta
added=`$PERL -e 'print time()."\n";'`

if
        $PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} . ns4
then
        echo_i "restarted server ns4"
else
        echo_i "could not restart server ns4"
        exit 1
fi

echo_i "sleeping for an additional 4 seconds for ns4 to fully startup"
sleep 4

# dump the NTA to a file
$RNDCCMD 10.53.0.4 nta -d > rndc.out.ns4.test$n.1 2>/dev/null
lines=`wc -l < rndc.out.ns4.test$n.1`
[ "$lines" -eq 1 ] || ret=1
ts=`awk '{print $3" "$4}' < rndc.out.ns4.test$n.1`
# rndc nta outputs localtime, so append the timezone
ts_with_zone="$ts `date +%z`"
echo "ts=$ts" > rndc.out.ns4.test$n.2
echo "ts_with_zone=$ts_with_zone" >> rndc.out.ns4.test$n.2
echo "added=$added" >> rndc.out.ns4.test$n.2
if $PERL -e 'use Time::Piece; use Time::Seconds;' 2>/dev/null
then
    # ntadiff.pl computes $ts_with_zone - ($added + 1week)
    d=`$PERL ./ntadiff.pl "$ts_with_zone" "$added"`
    echo "d=$d" >> rndc.out.ns4.test$n.2
    # diff from $added(now) + 1week to the clamped NTA lifetime should be
    # less than a few seconds (handle daylight saving changes by adding 3600).
    [ $d -lt 3610 ] || ret=1
else
    echo_i "skipped ntadiff test; install PERL module Time::Piece"
fi

# cleanup
$RNDCCMD 10.53.0.4 nta -remove secure.example > rndc.out.ns4.test$n.3 2>/dev/null

if [ $ret != 0 ]; then echo_i "failed - NTA lifetime clamping failed"; fi
status=`expr $status + $ret`
ret=0

echo_i "completed NTA tests"

# Run a minimal update test if possible.  This is really just
# a regression test for RT #2399; more tests should be added.

if $PERL -e 'use Net::DNS;' 2>/dev/null
then
    echo_i "running DNSSEC update test"
    ret=0
    {
      $PERL dnssec_update_test.pl -s 10.53.0.3 -p ${PORT} dynamic.example.  || ret=1
    } | cat_i
    [ $ret -eq 1 ] && status=1
else
    echo_i "The DNSSEC update test requires the Net::DNS library." >&2
fi

n=`expr $n + 1`
echo_i "checking managed key maintenance has not started yet ($n)"
ret=0
[ -f "ns4/managed-keys.bind.jnl" ] && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Reconfigure caching server to use "dnssec-validation auto", and repeat
# some of the DNSSEC validation tests to ensure that it works correctly.
echo_i "switching to automatic root key configuration"
copy_setports ns4/named2.conf.in ns4/named.conf
$RNDCCMD 10.53.0.4 reconfig 2>&1 | sed 's/^/ns4 /' | cat_i
sleep 5

echo_i "checking managed key maintenance timer has now started ($n)"
ret=0
[ -f "ns4/managed-keys.bind.jnl" ] || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking positive validation NSEC ($n)"
ret=0
$DIG $DIGOPTS +noauth a.example. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth a.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking positive validation NSEC3 ($n)"
ret=0
$DIG $DIGOPTS +noauth a.nsec3.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.nsec3.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking positive validation OPTOUT ($n)"
ret=0
$DIG $DIGOPTS +noauth a.optout.example. \
	@10.53.0.3 a > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth a.optout.example. \
	@10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking negative validation ($n)"
ret=0
$DIG $DIGOPTS +noauth q.example. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth q.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that root DS queries validate ($n)"
ret=0
$DIG $DIGOPTS +noauth . @10.53.0.1 ds > dig.out.ns1.test$n || ret=1
$DIG $DIGOPTS +noauth . @10.53.0.4 ds > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns1.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that DS at a RFC 1918 empty zone lookup succeeds ($n)"
ret=0
$DIG $DIGOPTS +noauth 10.in-addr.arpa ds @10.53.0.2 >dig.out.ns2.test$n || ret=1
$DIG $DIGOPTS +noauth 10.in-addr.arpa ds @10.53.0.6 >dig.out.ns6.test$n || ret=1
digcomp dig.out.ns2.test$n dig.out.ns6.test$n || ret=1
grep "status: NOERROR" dig.out.ns6.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking expired signatures remain with "'"allow-update { none; };"'" and no keys available ($n)"
ret=0
$DIG $DIGOPTS +noauth expired.example. +dnssec @10.53.0.3 soa > dig.out.ns3.test$n || ret=1
grep "RRSIG.SOA" dig.out.ns3.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi

status=`expr $status + $ret`
echo_i "checking expired signatures do not validate ($n)"
ret=0
$DIG $DIGOPTS +noauth expired.example. +dnssec @10.53.0.4 soa > dig.out.ns4.test$n || ret=1
grep "SERVFAIL" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
grep "expired.example/.*: RRSIG has expired" ns4/named.run > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that the NSEC3 record for the apex is properly signed when a DNSKEY is added via UPDATE ($n)"
ret=0
(
cd ns3
kskname=`$KEYGEN -q -3 -a RSASHA1 -r $RANDFILE -fk update-nsec3.example`
(
echo zone update-nsec3.example
echo server 10.53.0.3 ${PORT}
grep DNSKEY ${kskname}.key | sed -e 's/^/update add /' -e 's/IN/300 IN/'
echo send
) | $NSUPDATE
)
$DIG $DIGOPTS +dnssec a update-nsec3.example. @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.* ad[ ;]" dig.out.ns4.test$n > /dev/null || ret=1
grep "NSEC3 .* TYPE65534" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that the NSEC record is properly generated when DNSKEY are added via auto-dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec a auto-nsec.example. @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.* ad[ ;]" dig.out.ns4.test$n > /dev/null || ret=1
grep "IN.NSEC[^3].* DNSKEY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that the NSEC3 record is properly generated when DNSKEY are added via auto-dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec a auto-nsec3.example. @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.* ad[ ;]" dig.out.ns4.test$n > /dev/null || ret=1
grep "IN.NSEC3 .* DNSKEY" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that signing records have been marked as complete ($n)"
ret=0
checkprivate dynamic.example 10.53.0.3 || ret=1
checkprivate update-nsec3.example 10.53.0.3 || ret=1
checkprivate auto-nsec3.example 10.53.0.3 || ret=1
checkprivate expiring.example 10.53.0.3 || ret=1
checkprivate auto-nsec.example 10.53.0.3 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing' without arguments is handled ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing > /dev/null 2>&1 && ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing -list' without zone is handled ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -list > /dev/null 2>&1 && ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing -clear' without additional arguments is handled ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -clear > /dev/null 2>&1 && ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing -clear all' without zone is handled ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -clear all > /dev/null 2>&1 && ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing -nsec3param' without additional arguments is handled ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -nsec3param > /dev/null 2>&1 && ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing -nsec3param none' without zone is handled ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -nsec3param none > /dev/null 2>&1 && ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing -nsec3param 1' without additional arguments is handled ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -nsec3param 1 > /dev/null 2>&1 && ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing -nsec3param 1 0' without additional arguments is handled ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -nsec3param 1 0 > /dev/null 2>&1 && ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing -nsec3param 1 0 0' without additional arguments is handled ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -nsec3param 1 0 0 > /dev/null 2>&1 && ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing -nsec3param 1 0 0 -' without zone is handled ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -nsec3param 1 0 0 - > /dev/null 2>&1 && ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing -nsec3param' works with salt ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -nsec3param 1 0 0 ffff inline.example > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
for i in 1 2 3 4 5 6 7 8 9 10 ; do
        salt=`$DIG $DIGOPTS +nodnssec +short nsec3param inline.example. @10.53.0.3 | awk '{print $4}'`
	if [ "$salt" = "FFFF" ]; then
		break;
	fi
	echo_i "sleeping ...."
	sleep 1
done;
[ "$salt" = "FFFF" ] || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing -nsec3param' works without salt ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -nsec3param 1 0 0 - inline.example > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
for i in 1 2 3 4 5 6 7 8 9 10 ; do
	salt=`$DIG $DIGOPTS +nodnssec +short nsec3param inline.example. @10.53.0.3 | awk '{print $4}'`
	if [ "$salt" = "-" ]; then
		break;
	fi
	echo_i "sleeping ...."
	sleep 1
done;
[ "$salt" = "-" ] || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing -nsec3param' works with 'auto' as salt ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -nsec3param 1 0 0 auto inline.example > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
for i in 1 2 3 4 5 6 7 8 9 10 ; do
	salt=`$DIG $DIGOPTS +nodnssec +short nsec3param inline.example. @10.53.0.3 | awk '{print $4}'`
	[ -n "$salt" -a "$salt" != "-" ] && break
	echo_i "sleeping ...."
	sleep 1
done;
[ "$salt" != "-" ] || ret=1
[ `expr "${salt}" : ".*"` -eq 16 ] || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'rndc signing -nsec3param' with 'auto' as salt again generates a different salt ($n)"
ret=0
oldsalt=$salt
$RNDCCMD 10.53.0.3 signing -nsec3param 1 0 0 auto inline.example > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 status > /dev/null || ret=1
for i in 1 2 3 4 5 6 7 8 9 10 ; do
	salt=`$DIG $DIGOPTS +nodnssec +short nsec3param inline.example. @10.53.0.3 | awk '{print $4}'`
	[ -n "$salt" -a "$salt" != "$oldsalt" ] && break
	echo_i "sleeping ...."
	sleep 1
done;
[ "$salt" != "$oldsalt" ] || ret=1
[ `expr "$salt" : ".*"` -eq 16 ] || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check rndc signing -list output ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -list dynamic.example 2>&1 > signing.out
grep "No signing records found" signing.out > /dev/null 2>&1 || {
        ret=1
        sed 's/^/ns3 /' signing.out | cat_i
}
$RNDCCMD 10.53.0.3 signing -list update-nsec3.example 2>&1 > signing.out
grep "Done signing with key .*/NSEC3RSASHA1" signing.out > /dev/null 2>&1 || {
        ret=1
        sed 's/^/ns3 /' signing.out | cat_i
}
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "clear signing records ($n)"
$RNDCCMD 10.53.0.3 signing -clear all update-nsec3.example > /dev/null || ret=1
sleep 1
$RNDCCMD 10.53.0.3 signing -list update-nsec3.example 2>&1 > signing.out
grep "No signing records found" signing.out > /dev/null 2>&1 || {
        ret=1
        sed 's/^/ns3 /' signing.out | cat_i
}
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that a insecure zone beneath a cname resolves ($n)"
ret=0
$DIG $DIGOPTS soa insecure.below-cname.example. @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that a secure zone beneath a cname resolves ($n)"
ret=0
$DIG $DIGOPTS soa secure.below-cname.example. @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "ANSWER: 2," dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.* ad[ ;]" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking dnskey query with no data still gets put in cache ($n)"
ret=0
myDIGOPTS="+noadd +nosea +nostat +noquest +nocomm +nocmd -p ${PORT} @10.53.0.4"
firstVal=`$DIG $myDIGOPTS insecure.example. dnskey| awk '$1 != ";;" { print $2 }'`
sleep 1
secondVal=`$DIG $myDIGOPTS insecure.example. dnskey| awk '$1 != ";;" { print $2 }'`
if [ ${firstVal:-0} -eq ${secondVal:-0} ]
then
	sleep 1
	thirdVal=`$DIG $myDIGOPTS insecure.example. dnskey|awk '$1 != ";;" { print $2 }'`
	if [ ${firstVal:-0} -eq ${thirdVal:-0} ]
	then
		echo_i "cannot confirm query answer still in cache"
		ret=1
	fi
fi
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that a split dnssec dnssec-signzone work ($n)"
ret=0
$DIG $DIGOPTS soa split-dnssec.example. @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "ANSWER: 2," dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.* ad[ ;]" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that a smart split dnssec dnssec-signzone work ($n)"
ret=0
$DIG $DIGOPTS soa split-smart.example. @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "ANSWER: 2," dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.* ad[ ;]" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that NOTIFY is sent at the end of NSEC3 chain generation ($n)"
ret=0
(
echo zone nsec3chain-test
echo server 10.53.0.2 ${PORT}
echo update add nsec3chain-test. 0 nsec3param 1 0 1 123456
echo send
) | $NSUPDATE
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18
do
	$DIG $DIGOPTS nsec3param nsec3chain-test @10.53.0.2 > dig.out.ns2.test$n || ret=1
	if grep "ANSWER: 3," dig.out.ns2.test$n >/dev/null
	then
		break;
	fi
	echo_i "sleeping ...."
	sleep 3
done;
grep "ANSWER: 3," dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "nsec3 chain generation not complete"; fi
$DIG $DIGOPTS +noauth +nodnssec soa nsec3chain-test @10.53.0.2 > dig.out.ns2.test$n || ret=1
s2=`awk '$4 == "SOA" { print $7}' dig.out.ns2.test$n`
for i in 1 2 3 4 5 6 7 8 9 10
do
	$DIG $DIGOPTS +noauth +nodnssec soa nsec3chain-test @10.53.0.3 > dig.out.ns3.test$n || ret=1
	s3=`awk '$4 == "SOA" { print $7}' dig.out.ns3.test$n`
	test "$s2" = "$s3" && break
	sleep 1
done
digcomp dig.out.ns2.test$n dig.out.ns3.test$n || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check dnssec-dsfromkey from stdin ($n)"
ret=0
$DIG $DIGOPTS dnskey algroll. @10.53.0.2 | \
        $DSFROMKEY -f - algroll. > dig.out.ns2.test$n || ret=1
NF=`awk '{print NF}' dig.out.ns2.test$n | sort -u`
[ "${NF}" = 7 ] || ret=1
# make canonical
awk '{
	for (i=1;i<7;i++) printf("%s ", $i);
	for (i=7;i<=NF;i++) printf("%s", $i);
	printf("\n");
}' < dig.out.ns2.test$n > canonical1.$n || ret=1
awk '{
	for (i=1;i<7;i++) printf("%s ", $i);
	for (i=7;i<=NF;i++) printf("%s", $i);
	printf("\n");
}' < ns1/dsset-algroll$TP > canonical2.$n || ret=1
diff -b canonical1.$n canonical2.$n > /dev/null 2>&1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Intentionally strip ".key" from keyfile name to ensure the error message
# includes it anyway to avoid confusion (RT #21731)
echo_i "check dnssec-dsfromkey error message when keyfile is not found ($n)"
ret=0
key=`$KEYGEN -a RSASHA1 -q -r $RANDFILE example.` || ret=1
mv $key.key $key
$DSFROMKEY $key > dsfromkey.out.$n 2>&1 && ret=1
grep "$key.key: file not found" dsfromkey.out.$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing soon-to-expire RRSIGs without a replacement private key ($n)"
ret=0
$DIG $ANSWEROPTS +nottlid expiring.example ns @10.53.0.3 | grep RRSIG > dig.out.ns3.test$n 2>&1
# there must be a signature here
[ -s dig.out.ns3.test$n ] || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing new records are signed with 'no-resign' ($n)"
ret=0
(
echo zone nosign.example
echo server 10.53.0.3 ${PORT}
echo update add new.nosign.example 300 in txt "hi there"
echo send
) | $NSUPDATE
sleep 1
$DIG $ANSWEROPTS +nottlid txt new.nosign.example @10.53.0.3 \
        > dig.out.ns3.test$n 2>&1
grep RRSIG dig.out.ns3.test$n > /dev/null 2>&1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing expiring records aren't resigned with 'no-resign' ($n)"
ret=0
$DIG $ANSWEROPTS +nottlid nosign.example ns @10.53.0.3 | \
        grep RRSIG | sed 's/[ 	][ 	]*/ /g' > dig.out.ns3.test$n 2>&1
# the NS RRSIG should not be changed
cmp -s nosign.before dig.out.ns3.test$n || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing updates fail with no private key ($n)"
ret=0
rm -f ns3/Knosign.example.*.private
(
echo zone nosign.example
echo server 10.53.0.3 ${PORT}
echo update add fail.nosign.example 300 in txt "reject me"
echo send
) | $NSUPDATE > /dev/null 2>&1 && ret=1
$DIG $ANSWEROPTS +nottlid fail.nosign.example txt @10.53.0.3 \
        > dig.out.ns3.test$n 2>&1
[ -s dig.out.ns3.test$n ] && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing legacy upper case signer name validation ($n)"
ret=0
$DIG +tcp +noadd +noauth +dnssec -p ${PORT} soa upper.example @10.53.0.4 \
        > dig.out.ns4.test$n 2>&1
grep 'flags:.* ad;' dig.out.ns4.test$n > /dev/null || ret=1
grep 'RRSIG.*SOA.* UPPER\.EXAMPLE\. ' dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing that we lower case signer name ($n)"
ret=0
$DIG +tcp +noadd +noauth +dnssec -p ${PORT} soa LOWER.EXAMPLE @10.53.0.4 \
        > dig.out.ns4.test$n 2>&1
grep 'flags:.* ad;' dig.out.ns4.test$n > /dev/null || ret=1
grep 'RRSIG.*SOA.* lower\.example\. ' dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing TTL is capped at RRSIG expiry time ($n)"
ret=0
$RNDCCMD 10.53.0.3 freeze expiring.example 2>&1 | sed 's/^/ns3 /' | cat_i
(
cd ns3
for file in K*.moved; do
  mv $file `basename $file .moved`
done
$SIGNER -S -r $RANDFILE -N increment -e now+1mi -o expiring.example expiring.example.db > /dev/null 2>&1
) || ret=1
$RNDCCMD 10.53.0.3 reload expiring.example 2>&1 | sed 's/^/ns3 /' | cat_i

$RNDCCMD 10.53.0.4 flush 2>&1 | sed 's/^/ns4 /' | cat_i
$DIG $ANSWEROPTS +cd expiring.example soa @10.53.0.4 > dig.out.ns4.1.$n
$DIG $ANSWEROPTS expiring.example soa @10.53.0.4 > dig.out.ns4.2.$n
ttls=`awk '$1 != ";;" {print $2}' dig.out.ns4.1.$n`
ttls2=`awk '$1 != ";;" {print $2}' dig.out.ns4.2.$n`
for ttl in ${ttls:-0}; do
    [ ${ttl:-0} -eq 300 ] || ret=1
done
for ttl in ${ttls2:-0}; do
    [ ${ttl:-0} -le 60 ] || ret=1
done
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing TTL is capped at RRSIG expiry time for records in the additional section (NS) ($n)"
ret=0
$RNDCCMD 10.53.0.4 flush 2>&1 | sed 's/^/ns4 /' | cat_i
sleep 1
$DIG $ADDITIONALOPTS +cd expiring.example ns @10.53.0.4 > dig.out.ns4.1.$n
$DIG $ADDITIONALOPTS expiring.example ns @10.53.0.4 > dig.out.ns4.2.$n
ttls=`awk '$1 != ";;" {print $2}' dig.out.ns4.1.$n`
ttls2=`awk '$1 != ";;" {print $2}' dig.out.ns4.2.$n`
for ttl in ${ttls:-300}; do
    [ ${ttl:-0} -eq 300 ] || ret=1
done
for ttl in ${ttls2:-0}; do
    [ ${ttl:-0} -le 60 ] || ret=1
done
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing TTL is capped at RRSIG expiry time for records in the additional section (MX) ($n)"
ret=0
$RNDCCMD 10.53.0.4 flush 2>&1 | sed 's/^/ns4 /' | cat_i
sleep 1
$DIG $ADDITIONALOPTS +cd expiring.example mx @10.53.0.4 > dig.out.ns4.1.$n
$DIG $ADDITIONALOPTS expiring.example mx @10.53.0.4 > dig.out.ns4.2.$n
ttls=`awk '$1 != ";;" {print $2}' dig.out.ns4.1.$n`
ttls2=`awk '$1 != ";;" {print $2}' dig.out.ns4.2.$n`
for ttl in ${ttls:-300}; do
    [ ${ttl:-0} -eq 300 ] || ret=1
done
for ttl in ${ttls2:-0}; do
    [ ${ttl:-0} -le 60 ] || ret=1
done
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

copy_setports ns4/named3.conf.in ns4/named.conf
$RNDCCMD 10.53.0.4 reconfig 2>&1 | sed 's/^/ns4 /' | cat_i
sleep 3

echo_i "testing TTL of about to expire RRsets with dnssec-accept-expired yes; ($n)"
ret=0
$RNDCCMD 10.53.0.4 flush 2>&1 | sed 's/^/ns4 /' | cat_i
$DIG $ANSWEROPTS +cd expiring.example soa @10.53.0.4 > dig.out.ns4.1.$n
$DIG $ANSWEROPTS expiring.example soa @10.53.0.4 > dig.out.ns4.2.$n
ttls=`awk '$1 != ";;" {print $2}' dig.out.ns4.1.$n`
ttls2=`awk '$1 != ";;" {print $2}' dig.out.ns4.2.$n`
for ttl in ${ttls:-0}; do
    [ $ttl -eq 300 ] || ret=1
done
for ttl in ${ttls2:-0}; do
    [ $ttl -le 120 -a $ttl -gt 60 ] || ret=1
done
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing TTL of expired RRsets with dnssec-accept-expired yes; ($n)"
ret=0
$DIG $ANSWEROPTS +cd expired.example soa @10.53.0.4 > dig.out.ns4.1.$n
$DIG $ANSWEROPTS expired.example soa @10.53.0.4 > dig.out.ns4.2.$n
ttls=`awk '$1 != ";;" {print $2}' dig.out.ns4.1.$n`
ttls2=`awk '$1 != ";;" {print $2}' dig.out.ns4.2.$n`
for ttl in ${ttls:-0}; do
    [ $ttl -eq 300 ] || ret=1
done
for ttl in ${ttls2:-0}; do
    [ $ttl -le 120 -a $ttl -gt 60 ] || ret=1
done
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing TTL is capped at RRSIG expiry time for records in the additional section with dnssec-accept-expired yes; ($n)"
ret=0
$RNDCCMD 10.53.0.4 flush 2>&1 | sed 's/^/ns4 /' | cat_i
$DIG $ANSWEROPTS +cd expiring.example mx @10.53.0.4 > dig.out.ns4.1.$n
$DIG $ANSWEROPTS expiring.example mx @10.53.0.4 > dig.out.ns4.2.$n
ttls=`awk '$1 != ";;" {print $2}' dig.out.ns4.1.$n`
ttls2=`awk '$1 != ";;" {print $2}' dig.out.ns4.2.$n`
for ttl in ${ttls:-300}; do
    [ $ttl -eq 300 ] || ret=1
done
for ttl in ${ttls2:-0}; do
    [ $ttl -le 120  -a $ttl -gt 60 ] || ret=1
done
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing DNSKEY lookup via CNAME ($n)"
ret=0
$DIG $DIGOPTS +noauth cnameandkey.secure.example. \
	@10.53.0.3 dnskey > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth cnameandkey.secure.example. \
	@10.53.0.4 dnskey > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "CNAME" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing KEY lookup at CNAME (present) ($n)"
ret=0
$DIG $DIGOPTS +noauth cnameandkey.secure.example. \
	@10.53.0.3 key > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth cnameandkey.secure.example. \
	@10.53.0.4 key > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "CNAME" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing KEY lookup at CNAME (not present) ($n)"
ret=0
$DIG $DIGOPTS +noauth cnamenokey.secure.example. \
	@10.53.0.3 key > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS +noauth cnamenokey.secure.example. \
	@10.53.0.4 key > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "CNAME" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing DNSKEY lookup via DNAME ($n)"
ret=0
$DIG $DIGOPTS a.dnameandkey.secure.example. \
	@10.53.0.3 dnskey > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS a.dnameandkey.secure.example. \
	@10.53.0.4 dnskey > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "CNAME" dig.out.ns4.test$n > /dev/null || ret=1
grep "DNAME" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "testing KEY lookup via DNAME ($n)"
ret=0
$DIG $DIGOPTS b.dnameandkey.secure.example. \
	@10.53.0.3 key > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS b.dnameandkey.secure.example. \
	@10.53.0.4 key > dig.out.ns4.test$n || ret=1
digcomp dig.out.ns3.test$n dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "DNAME" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that named doesn't loop when all private keys are not available ($n)"
ret=0
lines=`grep "reading private key file expiring.example" ns3/named.run | wc -l`
test ${lines:-1000} -lt 15 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check against against missing nearest provable proof ($n)"
$DIG $DIGOPTS +norec b.c.d.optout-tld. \
	@10.53.0.6 ds > dig.out.ds.ns6.test$n || ret=1
nsec3=`grep "IN.NSEC3" dig.out.ds.ns6.test$n | wc -l`
[ $nsec3 -eq 2 ] || ret=1
$DIG $DIGOPTS +norec b.c.d.optout-tld. \
	@10.53.0.6 A > dig.out.ns6.test$n || ret=1
nsec3=`grep "IN.NSEC3" dig.out.ns6.test$n | wc -l`
[ $nsec3 -eq 1 ] || ret=1
$DIG $DIGOPTS optout-tld. \
	@10.53.0.4 SOA > dig.out.soa.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.soa.ns4.test$n > /dev/null || ret=1
$DIG $DIGOPTS b.c.d.optout-tld. \
	@10.53.0.4 A > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that key id are logged when dumping the cache ($n)"
ret=0
$RNDCCMD 10.53.0.4 dumpdb 2>&1 | sed 's/^/ns4 /' | cat_i
sleep 1
grep "; key id = " ns4/named_dump.db > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check KEYDATA records are printed in human readable form in key zone ($n)"
# force the managed-keys zone to be written out
$RNDCCMD 10.53.0.4 managed-keys sync 2>&1 | sed 's/^/ns4 /' | cat_i
for i in 1 2 3 4 5 6 7 8 9
do
    ret=0
    if test -f ns4/managed-keys.bind
    then
	grep KEYDATA ns4/managed-keys.bind > /dev/null &&
	grep "next refresh:" ns4/managed-keys.bind > /dev/null &&
	break
    fi
    ret=1
    sleep 1
done
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check dig's +nocrypto flag ($n)"
ret=0
$DIG $DIGOPTS +norec +nocrypto DNSKEY . \
	@10.53.0.1 > dig.out.dnskey.ns1.test$n || ret=1
grep '256 3 1 \[key id = [1-9][0-9]*]' dig.out.dnskey.ns1.test$n > /dev/null || ret=1
grep 'RRSIG.* \[omitted]' dig.out.dnskey.ns1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +norec +nocrypto DS example \
	@10.53.0.1 > dig.out.ds.ns1.test$n || ret=1
grep 'DS.* 3 [12] \[omitted]' dig.out.ds.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check simultaneous inactivation and publishing of dnskeys removes inactive signature ($n)"
ret=0
cnt=0
while :
do
$DIG $DIGOPTS publish-inactive.example @10.53.0.3 dnskey > dig.out.ns3.test$n
keys=`awk '$5 == 257 { print; }' dig.out.ns3.test$n | wc -l`
test $keys -gt 2 && break
cnt=`expr $cnt + 1`
test $cnt -gt 120 && break
sleep 1
done
test $keys -gt 2 || ret=1
sigs=`grep RRSIG dig.out.ns3.test$n | wc -l`
sigs=`expr $sigs + 0`
n=`expr $n + 1`
test $sigs -eq 2 || ret=1
if test $ret != 0 ; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that increasing the sig-validity-interval resigning triggers re-signing ($n)"
ret=0
before=`$DIG axfr siginterval.example -p ${PORT} @10.53.0.3 | grep RRSIG.SOA`
cp ns3/siginterval2.conf ns3/siginterval.conf
$RNDCCMD 10.53.0.3 reconfig 2>&1 | sed 's/^/ns3 /' | cat_i
for i in 1 2 3 4 5 6 7 8 9 0
do
after=`$DIG axfr siginterval.example -p ${PORT} @10.53.0.3 | grep RRSIG.SOA`
test "$before" != "$after" && break
sleep 1
done
n=`expr $n + 1`
if test "$before" = "$after" ; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

copy_setports ns4/named4.conf.in ns4/named.conf
$RNDCCMD 10.53.0.4 reconfig 2>&1 | sed 's/^/ns4 /' | cat_i
sleep 3

echo_i "check insecure delegation between static-stub zones ($n)"
ret=0
$DIG $DIGOPTS ns insecure.secure.example \
	@10.53.0.4 > dig.out.ns4.1.test$n || ret=1
grep "SERVFAIL" dig.out.ns4.1.test$n > /dev/null && ret=1
$DIG $DIGOPTS ns secure.example \
	@10.53.0.4 > dig.out.ns4.2.test$n || ret=1
grep "SERVFAIL" dig.out.ns4.2.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check the acceptance of seconds as inception and expiration times ($n)"
ret=0
in="NSEC 8 0 86400 1390003200 1389394800 33655 . NYWjZYBV1b+h4j0yu/SmPOOylR8P4IXKDzHX3NwEmU1SUp27aJ91dP+i+UBcnPmBib0hck4DrFVvpflCEpCnVQd2DexcN0GX+3PM7XobxhtDlmnU X1L47zJlbdHNwTqHuPaMM6Xy9HGMXps7O5JVyfggVhTz2C+G5OVxBdb2rOo="

exp="NSEC 8 0 86400 20140118000000 20140110230000 33655 . NYWjZYBV1b+h4j0yu/SmPOOylR8P4IXKDzHX3NwEmU1SUp27aJ91dP+i +UBcnPmBib0hck4DrFVvpflCEpCnVQd2DexcN0GX+3PM7XobxhtDlmnU X1L47zJlbdHNwTqHuPaMM6Xy9HGMXps7O5JVyfggVhTz2C+G5OVxBdb2 rOo="

out=`echo "IN RRSIG $in" | $RRCHECKER -p | sed 's/^IN.RRSIG.//'`
[ "$out" = "$exp" ] || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check the correct resigning time is reported in zonestatus ($n)"
ret=0
$RNDCCMD 10.53.0.3 \
		zonestatus secure.example > rndc.out.ns3.test$n
# next resign node: secure.example/DNSKEY
name=`awk '/next resign node:/ { print $4 }' rndc.out.ns3.test$n | sed 's;/; ;'`
# next resign time: Thu, 24 Apr 2014 10:38:16 GMT
time=`awk 'BEGIN { m["Jan"] = "01"; m["Feb"] = "02"; m["Mar"] = "03";
		   m["Apr"] = "04"; m["May"] = "05"; m["Jun"] = "06";
		   m["Jul"] = "07"; m["Aug"] = "08"; m["Sep"] = "09";
		   m["Oct"] = "10"; m["Nov"] = "11"; m["Dec"] = "12";}
	 /next resign time:/ { printf "%d%s%02d%s\n", $7, m[$6], $5, $8 }' rndc.out.ns3.test$n | sed 's/://g'`
$DIG $DIGOPTS +noall +answer $name @10.53.0.3 > dig.out.test$n
expire=`awk '$4 == "RRSIG" { print $9 }' dig.out.test$n`
inception=`awk '$4 == "RRSIG" { print $10 }' dig.out.test$n`
$PERL -e 'exit(0) if ("'"$time"'" lt "'"$expire"'" && "'"$time"'" gt "'"$inception"'"); exit(1);' || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that split rrsigs are handled ($n)"
ret=0
$DIG $DIGOPTS split-rrsig soa @10.53.0.7 > dig.out.test$n || ret=1
awk 'BEGIN { ok=0; } $4 == "SOA" { if ($7 > 1) ok=1; } END { if (!ok) exit(1); }' dig.out.test$n || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that 'dnssec-keygen -S' works for all supported algorithms ($n)"
ret=0
alg=1
until test $alg = 256
do
	size=
	case $alg in
	1) # RSA/MD5
	   size="-b 1024";;
	2) # Diffie Helman
	   alg=`expr $alg + 1`
	   continue;;
	3) # DSA/SHA1
	   size="-b 512";;
	5) # RSA/SHA-1
	   size="-b 1024";;
	6) # DSA-NSEC3-SHA1
	   size="-b 512";;
	7) # RSASHA1-NSEC3-SHA1
	   size="-b 1024";;
	8) # RSA/SHA-256
	   size="-b 1024";;
	10) # RSA/SHA-512
	   size="-b 1024";;
	157|160|161|162|163|164|165) # private - non standard
	   alg=`expr $alg + 1`
	   continue;;
	esac
	key1=`$KEYGEN -a $alg $size -n zone -r $RANDFILE example 2> keygen.err`
	if grep "unsupported algorithm" keygen.err > /dev/null
	then
		alg=`expr $alg + 1`
		continue
	fi
	if test -z "$key1"
	then
		echo_i "'$KEYGEN -a $alg': failed"
		cat keygen.err
		ret=1
		alg=`expr $alg + 1`
		continue
	fi
	$SETTIME -I now+4d $key1.private > /dev/null
	key2=`$KEYGEN -v 10 -r $RANDFILE -i 3d -S $key1.private 2> /dev/null`
	test -f $key2.key -a -f $key2.private || {
		ret=1
		echo_i "'dnssec-keygen -S' failed for algorithm: $alg"
	}
	alg=`expr $alg + 1`
done
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that CDS records are signed using KSK by dnssec-signzone ($n)"
ret=0
$DIG $DIGOPTS +noall +answer @10.53.0.2 cds cds.secure > dig.out.test$n
lines=`awk '$4 == "RRSIG" && $5 == "CDS" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 2 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that CDS records are not signed using ZSK by dnssec-signzone -x ($n)"
ret=0
$DIG $DIGOPTS +noall +answer @10.53.0.2 cds cds-x.secure > dig.out.test$n
lines=`awk '$4 == "RRSIG" && $5 == "CDS" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that positive unknown NSEC3 hash algorithm does validate ($n)"
ret=0
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag @10.53.0.3 nsec3-unknown.example SOA > dig.out.ns3.test$n
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag @10.53.0.4 nsec3-unknown.example SOA > dig.out.ns4.test$n
grep "status: NOERROR," dig.out.ns3.test$n > /dev/null || ret=1
grep "status: NOERROR," dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that CDS records are signed using KSK by with dnssec-auto ($n)"
ret=0
$DIG $DIGOPTS +noall +answer @10.53.0.2 cds cds-auto.secure > dig.out.test$n
lines=`awk '$4 == "RRSIG" && $5 == "CDS" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 2 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that a lone non matching CDS record is rejected ($n)"
ret=0
(
echo zone cds-update.secure
echo server 10.53.0.2 ${PORT}
echo update delete cds-update.secure CDS
$DIG $DIGOPTS +noall +answer @10.53.0.2 dnskey cds-update.secure |
grep "DNSKEY.257" | sed 's/DNSKEY.257/DNSKEY 258/' |
$DSFROMKEY -C -A -f - -T 1 cds-update.secure |
sed "s/^/update add /"
echo send
) | $NSUPDATE > nsupdate.out.test$n 2>&1
grep "update failed: REFUSED" nsupdate.out.test$n > /dev/null || ret=1
$DIG $DIGOPTS +noall +answer @10.53.0.2 cds cds-update.secure > dig.out.test$n
lines=`awk '$4 == "CDS" {print}' dig.out.test$n | wc -l`
test ${lines:-10} -eq 0 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that CDS records are signed using KSK when added by nsupdate ($n)"
ret=0
(
echo zone cds-update.secure
echo server 10.53.0.2 ${PORT}
echo update delete cds-update.secure CDS
echo send
$DIG $DIGOPTS +noall +answer @10.53.0.2 dnskey cds-update.secure |
grep "DNSKEY.257" |
$DSFROMKEY -C -f - -T 1 cds-update.secure |
sed "s/^/update add /"
echo send
) | $NSUPDATE
$DIG $DIGOPTS +noall +answer @10.53.0.2 cds cds-update.secure > dig.out.test$n
lines=`awk '$4 == "RRSIG" && $5 == "CDS" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 2 || ret=1
lines=`awk '$4 == "CDS" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 2 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that CDS records are signed only using KSK when added by"
echo_i "   nsupdate when dnssec-dnskey-kskonly is yes ($n)"
ret=0
(
echo zone cds-kskonly.secure
echo server 10.53.0.2 ${PORT}
echo update delete cds-kskonly.secure CDS
echo send
$DIG $DIGOPTS +noall +answer @10.53.0.2 dnskey cds-kskonly.secure |
grep "DNSKEY.257" |
$DSFROMKEY -C -f - -T 1 cds-kskonly.secure |
sed "s/^/update add /"
echo send
) | $NSUPDATE
$DIG $DIGOPTS +noall +answer @10.53.0.2 cds cds-kskonly.secure > dig.out.test$n
lines=`awk '$4 == "RRSIG" && $5 == "CDS" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 1 || ret=1
lines=`awk '$4 == "CDS" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 2 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that positive unknown NSEC3 hash algorithm with OPTOUT does validate ($n)"
ret=0
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag @10.53.0.3 optout-unknown.example SOA > dig.out.ns3.test$n
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag @10.53.0.4 optout-unknown.example SOA > dig.out.ns4.test$n
grep "status: NOERROR," dig.out.ns3.test$n > /dev/null || ret=1
grep "status: NOERROR," dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that a non matching CDS record is accepted with a matching CDS record ($n)"
ret=0
(
echo zone cds-update.secure
echo server 10.53.0.2 ${PORT}
echo update delete cds-update.secure CDS
echo send
$DIG $DIGOPTS +noall +answer @10.53.0.2 dnskey cds-update.secure |
grep "DNSKEY.257" |
$DSFROMKEY -C -f - -T 1 cds-update.secure |
sed "s/^/update add /"
$DIG $DIGOPTS +noall +answer @10.53.0.2 dnskey cds-update.secure |
grep "DNSKEY.257" | sed 's/DNSKEY.257/DNSKEY 258/' |
$DSFROMKEY -C -A -f - -T 1 cds-update.secure |
sed "s/^/update add /"
echo send
) | $NSUPDATE
$DIG $DIGOPTS +noall +answer @10.53.0.2 cds cds-update.secure > dig.out.test$n
lines=`awk '$4 == "RRSIG" && $5 == "CDS" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 2 || ret=1
lines=`awk '$4 == "CDS" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 4 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that negative unknown NSEC3 hash algorithm does not validate ($n)"
ret=0
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag @10.53.0.3 nsec3-unknown.example A > dig.out.ns3.test$n
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag @10.53.0.4 nsec3-unknown.example A > dig.out.ns4.test$n
grep "status: NOERROR," dig.out.ns3.test$n > /dev/null || ret=1
grep "status: SERVFAIL," dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that CDNSKEY records are signed using KSK by dnssec-signzone ($n)"
ret=0
$DIG $DIGOPTS +noall +answer @10.53.0.2 cdnskey cdnskey.secure > dig.out.test$n
lines=`awk '$4 == "RRSIG" && $5 == "CDNSKEY" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 2 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that CDNSKEY records are not signed using ZSK by dnssec-signzone -x ($n)"
ret=0
$DIG $DIGOPTS +noall +answer @10.53.0.2 cdnskey cdnskey-x.secure > dig.out.test$n
lines=`awk '$4 == "RRSIG" && $5 == "CDNSKEY" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that negative unknown NSEC3 hash algorithm with OPTOUT does not validate ($n)"
ret=0
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag @10.53.0.3 optout-unknown.example A > dig.out.ns3.test$n
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag @10.53.0.4 optout-unknown.example A > dig.out.ns4.test$n
grep "status: NOERROR," dig.out.ns3.test$n > /dev/null || ret=1
grep "status: SERVFAIL," dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that CDNSKEY records are signed using KSK by with dnssec-auto ($n)"
ret=0
$DIG $DIGOPTS +noall +answer @10.53.0.2 cdnskey cdnskey-auto.secure > dig.out.test$n
lines=`awk '$4 == "RRSIG" && $5 == "CDNSKEY" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 2 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that unknown DNSKEY algorithm validates as insecure ($n)"
ret=0
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag @10.53.0.3 dnskey-unknown.example A > dig.out.ns3.test$n
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag @10.53.0.4 dnskey-unknown.example A > dig.out.ns4.test$n
grep "status: NOERROR," dig.out.ns3.test$n > /dev/null || ret=1
grep "status: NOERROR," dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that a lone non matching CDNSKEY record is rejected ($n)"
ret=0
(
echo zone cdnskey-update.secure
echo server 10.53.0.2 ${PORT}
echo update delete cdnskey-update.secure CDNSKEY
echo send
$DIG $DIGOPTS +noall +answer @10.53.0.2 dnskey cdnskey-update.secure |
sed -n -e "s/^/update add /" -e 's/DNSKEY.257/CDNSKEY 258/p'
echo send
) | $NSUPDATE > nsupdate.out.test$n 2>&1
grep "update failed: REFUSED" nsupdate.out.test$n > /dev/null || ret=1
$DIG $DIGOPTS +noall +answer @10.53.0.2 cdnskey cdnskey-update.secure > dig.out.test$n
lines=`awk '$4 == "CDNSKEY" {print}' dig.out.test$n | wc -l`
test ${lines:-10} -eq 0 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that unknown DNSKEY algorithm + unknown NSEC3 has algorithm validates as insecure ($n)"
ret=0
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag @10.53.0.3 dnskey-nsec3-unknown.example A > dig.out.ns3.test$n
$DIG $DIGOPTS +noauth +noadd +nodnssec +adflag @10.53.0.4 dnskey-nsec3-unknown.example A > dig.out.ns4.test$n
grep "status: NOERROR," dig.out.ns3.test$n > /dev/null || ret=1
grep "status: NOERROR," dig.out.ns4.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that CDNSKEY records are signed using KSK when added by nsupdate ($n)"
ret=0
(
echo zone cdnskey-update.secure
echo server 10.53.0.2 ${PORT}
echo update delete cdnskey-update.secure CDNSKEY
$DIG $DIGOPTS +noall +answer @10.53.0.2 dnskey cdnskey-update.secure |
sed -n -e "s/^/update add /" -e 's/DNSKEY.257/CDNSKEY 257/p'
echo send
) | $NSUPDATE
$DIG $DIGOPTS +noall +answer @10.53.0.2 cdnskey cdnskey-update.secure > dig.out.test$n
lines=`awk '$4 == "RRSIG" && $5 == "CDNSKEY" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 2 || ret=1
lines=`awk '$4 == "CDNSKEY" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that CDNSKEY records are signed only using KSK when added by"
echo_i "   nsupdate when dnssec-dnskey-kskonly is yes ($n)"
ret=0
(
echo zone cdnskey-kskonly.secure
echo server 10.53.0.2 ${PORT}
echo update delete cdnskey-kskonly.secure CDNSKEY
$DIG $DIGOPTS +noall +answer @10.53.0.2 dnskey cdnskey-kskonly.secure |
sed -n -e "s/^/update add /" -e 's/DNSKEY.257/CDNSKEY 257/p'
echo send
) | $NSUPDATE
$DIG $DIGOPTS +noall +answer @10.53.0.2 cdnskey cdnskey-kskonly.secure > dig.out.test$n
lines=`awk '$4 == "RRSIG" && $5 == "CDNSKEY" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 1 || ret=1
lines=`awk '$4 == "CDNSKEY" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking initialization with a revoked managed key ($n)"
ret=0
copy_setports ns5/named2.conf.in ns5/named.conf
$RNDCCMD 10.53.0.5 reconfig 2>&1 | sed 's/^/ns5 /' | cat_i
sleep 3
$DIG $DIGOPTS +dnssec @10.53.0.5 SOA . > dig.out.ns5.test$n
grep "status: SERVFAIL" dig.out.ns5.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that a non matching CDNSKEY record is accepted with a matching CDNSKEY record ($n)"
ret=0
(
echo zone cdnskey-update.secure
echo server 10.53.0.2 ${PORT}
echo update delete cdnskey-update.secure CDNSKEY
$DIG $DIGOPTS +noall +answer @10.53.0.2 dnskey cdnskey-update.secure |
sed -n -e "s/^/update add /" -e 's/DNSKEY.257/CDNSKEY 257/p'
$DIG $DIGOPTS +noall +answer @10.53.0.2 dnskey cdnskey-update.secure |
sed -n -e "s/^/update add /" -e 's/DNSKEY.257/CDNSKEY 258/p'
echo send
) | $NSUPDATE
$DIG $DIGOPTS +noall +answer @10.53.0.2 cdnskey cdnskey-update.secure > dig.out.test$n
lines=`awk '$4 == "RRSIG" && $5 == "CDNSKEY" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 2 || ret=1
lines=`awk '$4 == "CDNSKEY" {print}' dig.out.test$n | wc -l`
test ${lines:-0} -eq 2 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that RRSIGs are correctly removed from apex when RRset is removed  NSEC ($n)"
ret=0
# generate signed zone with MX and AAAA records at apex.
(
cd signer
$KEYGEN -q -r $RANDFILE -a RSASHA1 -3 -fK remove > /dev/null
$KEYGEN -q -r $RANDFILE -a RSASHA1 -33 remove > /dev/null
echo > remove.db.signed
$SIGNER -S -o remove -D -f remove.db.signed remove.db.in > signer.out.1.$n 2>&1
)
grep "RRSIG MX" signer/remove.db.signed > /dev/null || {
	ret=1 ; cp signer/remove.db.signed signer/remove.db.signed.pre$n;
}
# re-generate signed zone without MX and AAAA records at apex.
(
cd signer
$SIGNER -S -o remove -D -f remove.db.signed remove2.db.in > signer.out.2.$n 2>&1
)
grep "RRSIG MX" signer/remove.db.signed > /dev/null &&  {
	ret=1 ; cp signer/remove.db.signed signer/remove.db.signed.post$n;
}
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that RRSIGs are correctly removed from apex when RRset is removed  NSEC3 ($n)"
ret=0
# generate signed zone with MX and AAAA records at apex.
(
cd signer
echo > remove.db.signed
$SIGNER -3 - -S -o remove -D -f remove.db.signed remove.db.in > signer.out.1.$n 2>&1
)
grep "RRSIG MX" signer/remove.db.signed > /dev/null || {
	ret=1 ; cp signer/remove.db.signed signer/remove.db.signed.pre$n;
}
# re-generate signed zone without MX and AAAA records at apex.
(
cd signer
$SIGNER -3 - -S -o remove -D -f remove.db.signed remove2.db.in > signer.out.2.$n 2>&1
)
grep "RRSIG MX" signer/remove.db.signed > /dev/null &&  {
	ret=1 ; cp signer/remove.db.signed signer/remove.db.signed.post$n;
}
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that a named managed zone that was signed 'in-the-future' is re-signed when loaded ($n)"
ret=0
$DIG $DIGOPTS managed-future.example. @10.53.0.4 a > dig.out.ns4.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that trust-anchor-telemetry queries are logged ($n)"
ret=0
grep "sending trust-anchor-telemetry query '_ta-[0-9a-f]*/NULL" ns6/named.run > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that _ta-XXXX trust-anchor-telemetry queries are logged ($n)"
ret=0
grep "trust-anchor-telemetry '_ta-[0-9a-f]*/IN' from" ns1/named.run > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that _ta-AAAA trust-anchor-telemetry are not sent when disabled ($n)"
ret=0
grep "sending trust-anchor-telemetry query '_ta-[0-9a-f]*/IN" ns1/named.run > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that KEY-TAG trust-anchor-telemetry queries are logged ($n)"
ret=0
$DIG $DIGOPTS . dnskey +ednsopt=KEY-TAG:ffff @10.53.0.1 > dig.out.ns4.test$n || ret=1
grep "trust-anchor-telemetry './IN' from .* 65535" ns1/named.run > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check that the view is logged in messages from the validator when using views ($n)"
ret=0
grep "view rec: *validat" ns4/named.run > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
