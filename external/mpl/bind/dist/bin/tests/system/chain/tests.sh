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
SEND="$PERL $SYSTEMTESTTOP/send.pl 10.53.0.4 ${EXTRAPORT1}"
status=0
n=0

n=`expr $n + 1`
echo_i "checking short DNAME from authoritative ($n)"
ret=0
$DIG $DIGOPTS a.short-dname.example @10.53.0.2 a > dig.out.ns2.short || ret=1
grep "status: NOERROR" dig.out.ns2.short > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking short DNAME from recursive ($n)"
ret=0
$DIG $DIGOPTS a.short-dname.example @10.53.0.7 a > dig.out.ns4.short || ret=1
grep "status: NOERROR" dig.out.ns4.short > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking long DNAME from authoritative ($n)"
ret=0
$DIG $DIGOPTS a.long-dname.example @10.53.0.2 a > dig.out.ns2.long || ret=1
grep "status: NOERROR" dig.out.ns2.long > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking long DNAME from recursive ($n)"
ret=0
$DIG $DIGOPTS a.long-dname.example @10.53.0.7 a > dig.out.ns4.long || ret=1
grep "status: NOERROR" dig.out.ns4.long > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking (too) long DNAME from authoritative ($n)"
ret=0
$DIG $DIGOPTS 01234567890123456789012345678901234567890123456789.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.long-dname.example @10.53.0.2 a > dig.out.ns2.toolong || ret=1
grep "status: YXDOMAIN" dig.out.ns2.toolong > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking (too) long DNAME from recursive with cached DNAME ($n)"
ret=0
$DIG $DIGOPTS 01234567890123456789012345678901234567890123456789.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.long-dname.example @10.53.0.7 a > dig.out.ns4.cachedtoolong || ret=1
grep "status: YXDOMAIN" dig.out.ns4.cachedtoolong > /dev/null || ret=1
grep '^long-dname\.example\..*DNAME.*long' dig.out.ns4.cachedtoolong > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking (too) long DNAME from recursive without cached DNAME ($n)"
ret=0
$DIG $DIGOPTS 01234567890123456789012345678901234567890123456789.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.longlonglonglonglonglonglonglonglonglonglonglonglonglong.toolong-dname.example @10.53.0.7 a > dig.out.ns4.uncachedtoolong || ret=1
grep "status: YXDOMAIN" dig.out.ns4.uncachedtoolong > /dev/null || ret=1
grep '^toolong-dname\.example\..*DNAME.*long' dig.out.ns4.uncachedtoolong > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME to DNAME from authoritative ($n)"
ret=0
$DIG $DIGOPTS cname.example @10.53.0.2 a > dig.out.ns2.cname
grep "status: NOERROR" dig.out.ns2.cname > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME to DNAME from recursive"
ret=0
$DIG $DIGOPTS cname.example @10.53.0.7 a > dig.out.ns4.cname
grep "status: NOERROR" dig.out.ns4.cname > /dev/null || ret=1
grep '^cname.example.' dig.out.ns4.cname > /dev/null || ret=1
grep '^cnamedname.example.' dig.out.ns4.cname > /dev/null || ret=1
grep '^a.cnamedname.example.' dig.out.ns4.cname > /dev/null || ret=1
grep '^a.target.example.' dig.out.ns4.cname > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking DNAME is returned with synthesized CNAME before DNAME ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.7 name.synth-then-dname.example.broken A > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep '^name.synth-then-dname\.example\.broken\..*CNAME.*name.$' dig.out.test$n > /dev/null || ret=1
grep '^synth-then-dname\.example\.broken\..*DNAME.*\.$' dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking DNAME is returned with CNAME to synthesized CNAME before DNAME ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.7 cname-to-synth2-then-dname.example.broken A > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep '^cname-to-synth2-then-dname\.example\.broken\..*CNAME.*name\.synth2-then-dname\.example\.broken.$' dig.out.test$n > /dev/null || ret=1
grep '^name\.synth2-then-dname\.example\.broken\..*CNAME.*name.$' dig.out.test$n > /dev/null || ret=1
grep '^synth2-then-dname\.example\.broken\..*DNAME.*\.$' dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME loops are detected ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.7 loop.example > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 17" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME to external delegated zones is handled ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.7 a.example > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 2" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME to internal delegated zones is handled ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.7 b.example > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 2" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME to signed external delgation is handled ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.7 c.example > dig.out.$n
grep "status: NOERROR" dig.out.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i " failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME to signed internal delgation is handled ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.7 d.example > dig.out.$n
grep "status: NOERROR" dig.out.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i " failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME chains in various orders ($n)"
ret=0
echo "cname,cname,cname|1,2,3,4,s1,s2,s3,s4" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.1.$n 2>&1
grep 'status: NOERROR' dig.out.1.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.1.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "cname,cname,cname|1,1,2,2,3,4,s4,s3,s1" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.2.$n 2>&1
grep 'status: NOERROR' dig.out.2.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.2.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "cname,cname,cname|2,1,3,4,s3,s1,s2,s4" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.3.$n 2>&1
grep 'status: NOERROR' dig.out.3.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.3.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "cname,cname,cname|4,3,2,1,s4,s3,s2,s1" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.4.$n 2>&1
grep 'status: NOERROR' dig.out.4.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.4.$n > /dev/null 2>&1 || ret=1
echo "cname,cname,cname|4,3,2,1,s4,s3,s2,s1" | $SEND
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.5.$n 2>&1
grep 'status: NOERROR' dig.out.5.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.5.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "cname,cname,cname|4,3,3,3,s1,s1,1,3,4" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.6.$n 2>&1
grep 'status: NOERROR' dig.out.6.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.6.$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that only the initial CNAME is cached ($n)"
ret=0
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "cname,cname,cname|1,2,3,4,s1,s2,s3,s4" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.1.$n 2>&1
sleep 1
$DIG $DIGOPTS +noall +answer @10.53.0.7 cname1.domain.nil > dig.out.2.$n 2>&1
ttl=`awk '{print $2}' dig.out.2.$n`
[ "$ttl" -eq 86400 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking DNAME chains in various orders ($n)"
ret=0
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "dname,dname|5,4,3,2,1,s5,s4,s3,s2,s1" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.1.$n 2>&1
grep 'status: NOERROR' dig.out.1.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 3' dig.out.1.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "dname,dname|5,4,3,2,1,s5,s4,s3,s2,s1" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.2.$n 2>&1
grep 'status: NOERROR' dig.out.2.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 3' dig.out.2.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "dname,dname|2,3,s1,s2,s3,s4,1" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.3.$n 2>&1
grep 'status: NOERROR' dig.out.3.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 3' dig.out.3.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking external CNAME/DNAME chains in various orders ($n)"
ret=0
echo "xname,dname|1,2,3,4,s1,s2,s3,s4" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.1.$n 2>&1
grep 'status: NOERROR' dig.out.1.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.1.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "xname,dname|s2,2,s1,1,4,s4,3" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.2.$n 2>&1
grep 'status: NOERROR' dig.out.2.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.2.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "xname,dname|s2,2,2,2" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.3.$n 2>&1
grep 'status: SERVFAIL' dig.out.3.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking explicit DNAME query ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.7 dname short-dname.example > dig.out.7.$n 2>&1
grep 'status: NOERROR' dig.out.7.$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking DNAME via ANY query ($n)"
ret=0
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS @10.53.0.7 any short-dname.example > dig.out.7.$n 2>&1
grep 'status: NOERROR' dig.out.7.$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
