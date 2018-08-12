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

DIGOPTS="-p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0
n=0

dotests() {
    n=`expr $n + 1`
    echo_i "test with RT, single zone (+rec) ($n)"
    ret=0
    $DIG $DIGOPTS +rec -t RT rt.rt.example @10.53.0.1 > dig.out.$n || ret=1
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi

    n=`expr $n + 1`
    echo_i "test with RT, two zones (+rec) ($n)"
    ret=0
    $DIG $DIGOPTS +rec -t RT rt.rt2.example @10.53.0.1 > dig.out.$n || ret=1
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi

    n=`expr $n + 1`
    echo_i "test with NAPTR, single zone (+rec) ($n)"
    ret=0
    $DIG $DIGOPTS +rec -t NAPTR nap.naptr.example @10.53.0.1 > dig.out.$n || ret=1
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi

    n=`expr $n + 1`
    echo_i "test with NAPTR, two zones (+rec) ($n)"
    ret=0
    $DIG $DIGOPTS +rec -t NAPTR nap.hang3b.example @10.53.0.1 > dig.out.$n || ret=1
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi

    n=`expr $n + 1`
    echo_i "test with LP (+rec) ($n)"
    ret=0
    $DIG $DIGOPTS +rec -t LP nid2.nid.example @10.53.0.1 > dig.out.$n || ret=1
    case $minimal in
    no)
      grep -w "NS" dig.out.$n > /dev/null || ret=1
      grep -w "L64" dig.out.$n > /dev/null || ret=1
      grep -w "L32" dig.out.$n > /dev/null || ret=1
      ;;
    yes)
      grep -w "NS" dig.out.$n > /dev/null && ret=1
      grep -w "L64" dig.out.$n > /dev/null && ret=1
      grep -w "L32" dig.out.$n > /dev/null && ret=1
      ;;
    no-auth)
      grep -w "NS" dig.out.$n > /dev/null && ret=1
      grep -w "L64" dig.out.$n > /dev/null || ret=1
      grep -w "L32" dig.out.$n > /dev/null || ret=1
      ;;
    no-auth-recursive)
      grep -w "NS" dig.out.$n > /dev/null && ret=1
      grep -w "L64" dig.out.$n > /dev/null || ret=1
      grep -w "L32" dig.out.$n > /dev/null || ret=1
      ;;
    esac
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi

    n=`expr $n + 1`
    echo_i "test with NID (+rec) ($n)"
    ret=0
    $DIG $DIGOPTS +rec -t NID ns1.nid.example @10.53.0.1 > dig.out.$n || ret=1
    if [ $minimal = no ] ; then
      # change && to || when we support NID additional processing
      grep -w "L64" dig.out.$n > /dev/null && ret=1
      grep -w "L32" dig.out.$n > /dev/null && ret=1
    else
      grep -w "L64" dig.out.$n > /dev/null && ret=1
      grep -w "L32" dig.out.$n > /dev/null && ret=1
    fi
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi

    n=`expr $n + 1`
    echo_i "test with NID + LP (+rec) ($n)"
    ret=0
    $DIG $DIGOPTS +rec -t NID nid2.nid.example @10.53.0.1 > dig.out.$n || ret=1
    if [ $minimal = no ] ; then
      # change && to || when we support NID additional processing
      grep -w "LP" dig.out.$n > /dev/null && ret=1
      grep -w "L64" dig.out.$n > /dev/null && ret=1
      grep -w "L32" dig.out.$n > /dev/null && ret=1
    else
      grep -w "LP" dig.out.$n > /dev/null && ret=1
      grep -w "L64" dig.out.$n > /dev/null && ret=1
      grep -w "L32" dig.out.$n > /dev/null && ret=1
    fi
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi

    n=`expr $n + 1`
    echo_i "test with RT, single zone (+norec) ($n)"
    ret=0
    $DIG $DIGOPTS +norec -t RT rt.rt.example @10.53.0.1 > dig.out.$n || ret=1
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi

    n=`expr $n + 1`
    echo_i "test with RT, two zones (+norec) ($n)"
    ret=0
    $DIG $DIGOPTS +norec -t RT rt.rt2.example @10.53.0.1 > dig.out.$n || ret=1
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi

    n=`expr $n + 1`
    echo_i "test with NAPTR, single zone (+norec) ($n)"
    ret=0
    $DIG $DIGOPTS +norec -t NAPTR nap.naptr.example @10.53.0.1 > dig.out.$n || ret=1
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi

    n=`expr $n + 1`
    echo_i "test with NAPTR, two zones (+norec) ($n)"
    ret=0
    $DIG $DIGOPTS +norec -t NAPTR nap.hang3b.example @10.53.0.1 > dig.out.$n || ret=1
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi

    n=`expr $n + 1`
    echo_i "test with LP (+norec) ($n)"
    ret=0
    $DIG $DIGOPTS +norec -t LP nid2.nid.example @10.53.0.1 > dig.out.$n || ret=1
    case $minimal in
    no)
      grep -w "NS" dig.out.$n > /dev/null || ret=1
      grep -w "L64" dig.out.$n > /dev/null || ret=1
      grep -w "L32" dig.out.$n > /dev/null || ret=1
      ;;
    yes)
      grep -w "NS" dig.out.$n > /dev/null && ret=1
      grep -w "L64" dig.out.$n > /dev/null && ret=1
      grep -w "L32" dig.out.$n > /dev/null && ret=1
      ;;
    no-auth)
      grep -w "NS" dig.out.$n > /dev/null && ret=1
      grep -w "L64" dig.out.$n > /dev/null || ret=1
      grep -w "L32" dig.out.$n > /dev/null || ret=1
      ;;
    no-auth-recursive)
      grep -w "NS" dig.out.$n > /dev/null || ret=1
      grep -w "L64" dig.out.$n > /dev/null || ret=1
      grep -w "L32" dig.out.$n > /dev/null || ret=1
      ;;
    esac
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi

    n=`expr $n + 1`
    echo_i "test with NID (+norec) ($n)"
    ret=0
    $DIG $DIGOPTS +norec -t NID ns1.nid.example @10.53.0.1 > dig.out.$n || ret=1
    if [ $minimal = no ] ; then
      # change && to || when we support NID additional processing
      grep -w "L64" dig.out.$n > /dev/null && ret=1
      grep -w "L32" dig.out.$n > /dev/null && ret=1
    else
      grep -w "L64" dig.out.$n > /dev/null && ret=1
      grep -w "L32" dig.out.$n > /dev/null && ret=1
    fi
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi

    n=`expr $n + 1`
    echo_i "test with NID + LP (+norec) ($n)"
    ret=0
    $DIG $DIGOPTS +norec -t NID nid2.nid.example @10.53.0.1 > dig.out.$n || ret=1
    if [ $minimal = no ] ; then
      # change && to || when we support NID additional processing
      grep -w "LP" dig.out.$n > /dev/null && ret=1
      grep -w "L64" dig.out.$n > /dev/null && ret=1
      grep -w "L32" dig.out.$n > /dev/null && ret=1
    else
      grep -w "LP" dig.out.$n > /dev/null && ret=1
      grep -w "L64" dig.out.$n > /dev/null && ret=1
      grep -w "L32" dig.out.$n > /dev/null && ret=1
    fi
    if [ $ret -eq 1 ] ; then
            echo_i " failed"; status=1
    fi
}

echo_i "testing with 'minimal-responses yes;'"
minimal=yes
dotests

echo_i "reconfiguring server: minimal-responses no"
copy_setports ns1/named2.conf.in ns1/named.conf
$RNDCCMD 10.53.0.1 reconfig 2>&1 | sed 's/^/ns1 /' | cat_i
sleep 2

echo_i "testing with 'minimal-responses no;'"
minimal=no
dotests

n=`expr $n + 1`
echo_i "testing with 'minimal-any no;' ($n)"
ret=0
$DIG $DIGOPTS -t ANY www.rt.example @10.53.0.1 > dig.out.$n || ret=1
grep "ANSWER: 3, AUTHORITY: 1, ADDITIONAL: 2" dig.out.$n > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
    echo_i " failed"; status=1
fi

echo_i "reconfiguring server: minimal-any yes"
copy_setports ns1/named3.conf.in ns1/named.conf
$RNDCCMD 10.53.0.1 reconfig 2>&1 | sed 's/^/ns1 /' | cat_i
sleep 2

n=`expr $n + 1`
echo_i "testing with 'minimal-any yes;' over UDP ($n)"
ret=0
$DIG $DIGOPTS -t ANY +notcp www.rt.example @10.53.0.1 > dig.out.$n || ret=1
grep "ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 1" dig.out.$n > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
    echo_i " failed"; status=1
fi
n=`expr $n + 1`

echo_i "testing with 'minimal-any yes;' over TCP ($n)"
ret=0
$DIG $DIGOPTS -t ANY +tcp www.rt.example @10.53.0.1 > dig.out.$n || ret=1
grep "ANSWER: 3, AUTHORITY: 0, ADDITIONAL: 1" dig.out.$n > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
    echo_i " failed"; status=1
fi

n=`expr $n + 1`
echo_i "testing with 'minimal-any yes;' over UDP ($n)"
ret=0
$DIG $DIGOPTS -t ANY +notcp www.rt.example @10.53.0.1 > dig.out.$n || ret=1
grep "ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 1" dig.out.$n > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
    echo_i " failed"; status=1
fi

echo_i "testing with 'minimal-responses no-auth;'"
minimal=no-auth
dotests

echo_i "reconfiguring server: minimal-responses no-auth-recursive"
copy_setports ns1/named4.conf.in ns1/named.conf
$RNDCCMD 10.53.0.1 reconfig 2>&1 | sed 's/^/ns1 /' | cat_i
sleep 2

echo_i "testing with 'minimal-responses no-auth-recursive;'"
minimal=no-auth-recursive
dotests

n=`expr $n + 1`
echo_i "testing returning TLSA records with MX query ($n)"
ret=0
$DIG $DIGOPTS -t mx mx.example @10.53.0.1 > dig.out.$n || ret=1
grep "mx\.example\..*MX.0 mail\.mx\.example" dig.out.$n > /dev/null || ret=1
grep "mail\.mx\.example\..*A.1\.2\.3\.4" dig.out.$n > /dev/null || ret=1
grep "_25\._tcp\.mail\.mx\.example\..*TLSA.3 0 1 5B30F9602297D558EB719162C225088184FAA32CA45E1ED15DE58A21 D9FCE383" dig.out.$n > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
    echo_i " failed"; status=1
fi

n=`expr $n + 1`
echo_i "testing returning TLSA records with SRV query ($n)"
ret=0
$DIG $DIGOPTS -t srv _xmpp-client._tcp.srv.example @10.53.0.1 > dig.out.$n || ret=1
grep "_xmpp-client\._tcp\.srv\.example\..*SRV.1 0 5222 server\.srv\.example" dig.out.$n > /dev/null || ret=1
grep "server\.srv\.example\..*A.1\.2\.3\.4" dig.out.$n > /dev/null || ret=1
grep "_5222\._tcp\.server\.srv\.example\..*TLSA.3 0 1 5B30F9602297D558EB719162C225088184FAA32CA45E1ED15DE58A21 D9FCE383" dig.out.$n > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
    echo_i " failed"; status=1
fi

echo_i "reconfiguring server: minimal-responses no"
copy_setports ns1/named2.conf.in ns1/named.conf
$RNDCCMD 10.53.0.1 reconfig 2>&1 | sed 's/^/ns1 /' | cat_i
sleep 2

n=`expr $n + 1`
echo_i "testing NS handling in ANY responses (authoritative) ($n)"
ret=0
$DIG $DIGOPTS -t ANY rt.example @10.53.0.1 > dig.out.$n || ret=1
grep "AUTHORITY: 0" dig.out.$n  > /dev/null || ret=1
grep "NS[ 	]*ns" dig.out.$n  > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
    echo_i " failed"; status=1
fi

n=`expr $n + 1`
echo_i "testing NS handling in ANY responses (recursive) ($n)"
ret=0
$DIG $DIGOPTS -t ANY rt.example @10.53.0.3 > dig.out.$n || ret=1
grep "AUTHORITY: 0" dig.out.$n  > /dev/null || ret=1
grep "NS[ 	]*ns" dig.out.$n  > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
    echo_i " failed"; status=1
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
