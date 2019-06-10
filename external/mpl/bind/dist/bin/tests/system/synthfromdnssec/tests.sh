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

# shellcheck source=conf.sh
SYSTEMTESTTOP=..
. "$SYSTEMTESTTOP/conf.sh"

set -e

status=0
n=1

rm -f dig.out.*

dig_with_opts() {
    "$DIG" +tcp +noadd +nosea +nostat +nocmd +dnssec -p "$PORT" "$@"
}

for ns in 2 4 5
do
    case $ns in
    2) description="<default>";;
    4) description="no";;
    5) description="yes";;
    *) exit 1;;
    esac
    echo_i "prime negative NXDOMAIN response (synth-from-dnssec ${description};) ($n)"
    ret=0
    dig_with_opts a.example. @10.53.0.${ns} a > dig.out.ns${ns}.test$n || ret=1
    grep "flags:[^;]* ad[ ;]" dig.out.ns${ns}.test$n > /dev/null || ret=1
    grep "status: NXDOMAIN," dig.out.ns${ns}.test$n > /dev/null || ret=1
    grep "example.*3600.IN.SOA" dig.out.ns${ns}.test$n > /dev/null || ret=1
    [ $ns -eq ${ns} ] && nxdomain=dig.out.ns${ns}.test$n
    n=$((n+1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    echo_i "prime negative NODATA response (synth-from-dnssec ${description};) ($n)"
    ret=0
    dig_with_opts nodata.example. @10.53.0.${ns} a > dig.out.ns${ns}.test$n || ret=1
    grep "flags:[^;]* ad[ ;]" dig.out.ns${ns}.test$n > /dev/null || ret=1
    grep "status: NOERROR," dig.out.ns${ns}.test$n > /dev/null || ret=1
    grep "example.*3600.IN.SOA" dig.out.ns${ns}.test$n > /dev/null || ret=1
    [ $ns -eq 2 ] && nodata=dig.out.ns${ns}.test$n
    n=$((n+1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    echo_i "prime wildcard response (synth-from-dnssec ${description};) ($n)"
    ret=0
    dig_with_opts a.wild-a.example. @10.53.0.${ns} a > dig.out.ns${ns}.test$n || ret=1
    grep "flags:[^;]* ad[ ;]" dig.out.ns${ns}.test$n > /dev/null || ret=1
    grep "status: NOERROR," dig.out.ns${ns}.test$n > /dev/null || ret=1
    grep "a.wild-a.example.*3600.IN.A" dig.out.ns${ns}.test$n > /dev/null || ret=1
    n=$((n+1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    echo_i "prime wildcard CNAME response (synth-from-dnssec ${description};) ($n)"
    ret=0
    dig_with_opts a.wild-cname.example. @10.53.0.${ns} a > dig.out.ns${ns}.test$n || ret=1
    grep "flags:[^;]* ad[ ;]" dig.out.ns${ns}.test$n > /dev/null || ret=1
    grep "status: NOERROR," dig.out.ns${ns}.test$n > /dev/null || ret=1
    grep "a.wild-cname.example.*3600.IN.CNAME" dig.out.ns${ns}.test$n > /dev/null || ret=1
    n=$((n+1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status+ret))
done

echo_i "prime redirect response (+nodnssec) (synth-from-dnssec <default>;) ($n)"
ret=0
dig_with_opts +nodnssec a.redirect. @10.53.0.3 a > dig.out.ns2.test$n || ret=1
grep "flags:[^;]* ad[ ;]" dig.out.ns2.test$n > /dev/null && ret=1
grep "status: NOERROR," dig.out.ns2.test$n > /dev/null || ret=1
grep 'a\.redirect\..*300.IN.A.100\.100\.100\.2' dig.out.ns2.test$n > /dev/null || ret=1
n=$((n+1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

#
# ensure TTL of synthesised answers differs from direct answers.
#
sleep 1

for ns in 2 4 5
do
    case $ns in
    2) synth=yes description="<default>";;
    4) synth=no  description="no";;
    5) synth=yes description="yes";;
    *) exit 1;;
    esac
    echo_i "check synthesized NXDOMAIN response (synth-from-dnssec ${description};) ($n)"
    ret=0
    dig_with_opts b.example. @10.53.0.${ns} a > dig.out.ns${ns}.test$n || ret=1
    grep "flags:[^;]* ad[ ;]" dig.out.ns${ns}.test$n > /dev/null || ret=1
    grep "status: NXDOMAIN," dig.out.ns${ns}.test$n > /dev/null || ret=1
    if [ ${synth} = yes ]
    then
	grep "example.*IN.SOA" dig.out.ns${ns}.test$n > /dev/null || ret=1
	grep "example.*3600.IN.SOA" dig.out.ns${ns}.test$n > /dev/null && ret=1
    else
	grep "example.*3600.IN.SOA" dig.out.ns${ns}.test$n > /dev/null || ret=1
    fi
    digcomp $nxdomain dig.out.ns${ns}.test$n || ret=1
    n=$((n+1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    echo_i "check synthesized NODATA response (synth-from-dnssec ${description};) ($n)"
    ret=0
    dig_with_opts nodata.example. @10.53.0.${ns} aaaa > dig.out.ns${ns}.test$n || ret=1
    grep "flags:[^;]* ad[ ;]" dig.out.ns${ns}.test$n > /dev/null || ret=1
    grep "status: NOERROR," dig.out.ns${ns}.test$n > /dev/null || ret=1
    if [ ${synth} = yes ]
    then
	grep "example.*IN.SOA" dig.out.ns${ns}.test$n > /dev/null || ret=1
	grep "example.*3600.IN.SOA" dig.out.ns${ns}.test$n > /dev/null && ret=1
    else
	grep "example.*3600.IN.SOA" dig.out.ns${ns}.test$n > /dev/null || ret=1
    fi
    digcomp $nodata dig.out.ns${ns}.test$n || ret=1
    n=$((n+1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    echo_i "check synthesized wildcard response (synth-from-dnssec ${description};) ($n)"
    ret=0
    dig_with_opts b.wild-a.example. @10.53.0.${ns} a > dig.out.ns${ns}.test$n || ret=1
    grep "flags:[^;]* ad[ ;]" dig.out.ns${ns}.test$n > /dev/null || ret=1
    grep "status: NOERROR," dig.out.ns${ns}.test$n > /dev/null || ret=1
    if [ ${synth} = yes ]
    then
	    grep "b\.wild-a\.example\..*IN.A" dig.out.ns${ns}.test$n > /dev/null || ret=1
	    grep "b\.wild-a\.example\..*3600.IN.A" dig.out.ns${ns}.test$n > /dev/null && ret=1
    else
	    grep "b\.wild-a\.example\..*3600.IN.A" dig.out.ns${ns}.test$n > /dev/null || ret=1
    fi
    n=$((n+1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    echo_i "check synthesized wildcard CNAME response (synth-from-dnssec ${description};) ($n)"
    ret=0
    dig_with_opts b.wild-cname.example. @10.53.0.${ns} a > dig.out.ns${ns}.test$n || ret=1
    grep "flags:[^;]* ad[ ;]" dig.out.ns${ns}.test$n > /dev/null || ret=1
    grep "status: NOERROR," dig.out.ns${ns}.test$n > /dev/null || ret=1
    if [ ${synth} = yes ]
    then
	grep "b.wild-cname.example.*IN.CNAME" dig.out.ns${ns}.test$n > /dev/null || ret=1
	grep "b.wild-cname.example.*3600.IN.CNAME" dig.out.ns${ns}.test$n > /dev/null && ret=1
    else
	grep "b.wild-cname.example.*3600.IN.CNAME" dig.out.ns${ns}.test$n > /dev/null || ret=1
    fi
    grep "ns1.example.*.IN.A" dig.out.ns${ns}.test$n > /dev/null || ret=1
    n=$((n+1))
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status+ret))
done

echo_i "check redirect response (+dnssec) (synth-from-dnssec <default>;) ($n)"
ret=0
dig_with_opts b.redirect. @10.53.0.3 a > dig.out.ns2.test$n || ret=1
grep "flags:[^;]* ad[ ;]" dig.out.ns2.test$n > /dev/null || ret=1
grep "status: NXDOMAIN," dig.out.ns2.test$n > /dev/null || ret=1
grep "\..*3600.IN.SOA" dig.out.ns2.test$n > /dev/null && ret=1
n=$((n+1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "check redirect response (+nodnssec) (synth-from-dnssec <default>;) ($n)"
ret=0
dig_with_opts +nodnssec b.redirect. @10.53.0.3 a > dig.out.ns2.test$n || ret=1
grep "flags:[^;]* ad[ ;]" dig.out.ns2.test$n > /dev/null && ret=1
grep "status: NOERROR," dig.out.ns2.test$n > /dev/null || ret=1
grep 'b\.redirect\..*300.IN.A.100\.100\.100\.2' dig.out.ns2.test$n > /dev/null || ret=1
n=$((n+1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))


echo_i "check DNAME handling (synth-from-dnssec yes;) ($n)"
ret=0
dig_with_opts dnamed.example. ns @10.53.0.5 > dig.out.ns5.test$n || ret=1
dig_with_opts a.dnamed.example. a @10.53.0.5 > dig.out.ns5-1.test$n || ret=1
grep "status: NOERROR," dig.out.ns5-1.test$n > /dev/null || ret=1
n=$((n+1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
