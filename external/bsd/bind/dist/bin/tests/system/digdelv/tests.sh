# Copyright (C) 2015, 2016  Internet Systems Consortium, Inc. ("ISC")
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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0
n=0
# using dig insecure mode as not testing dnssec here
DIGOPTS="-i -p 5300"

if [ -x ${DIG} ] ; then
  n=`expr $n + 1`
  echo "I:checking dig short form works ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 +short a a.example > dig.out.test$n || ret=1
  if test `wc -l < dig.out.test$n` != 1 ; then ret=1 ; fi
  if [ $ret != 0 ]; then echo "I:failed"; fi 
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking dig split width works ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 +split=4 -t sshfp foo.example > dig.out.test$n || ret=1
  grep " 9ABC DEF6 7890 " < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi 
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking dig with reverse lookup works ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 -x 127.0.0.1 > dig.out.test$n 2>&1 || ret=1
  # doesn't matter if has answer
  grep -i "127\.in-addr\.arpa\." < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi 
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking dig over TCP works ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 a a.example > dig.out.test$n || ret=1
  grep "10\.0\.0\.1$" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking dig +multi +norrcomments works for dnskey (when default is rrcomments)($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +multi +norrcomments DNSKEY dnskey.example > dig.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < dig.out.test$n > /dev/null && ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking dig +multi +norrcomments works for soa (when default is rrcomments)($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +multi +norrcomments SOA example > dig.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < dig.out.test$n > /dev/null && ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking dig +rrcomments works for DNSKEY($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +rrcomments DNSKEY dnskey.example > dig.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking dig +short +rrcomments works for DNSKEY ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +short +rrcomments DNSKEY dnskey.example > dig.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking dig +short +nosplit works($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +short +nosplit DNSKEY dnskey.example > dig.out.test$n || ret=1
  grep "Z8plc4Rb9VIE5x7KNHAYTvTO5d4S8M=$" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking dig +short +rrcomments works($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +short +rrcomments DNSKEY dnskey.example > dig.out.test$n || ret=1
  grep "S8M=  ; ZSK; alg = RSAMD5 ; key id = 30795$" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking dig +short +rrcomments works($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.3 +short +rrcomments DNSKEY dnskey.example > dig.out.test$n || ret=1
  grep "S8M=  ; ZSK; alg = RSAMD5 ; key id = 30795$" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

#  n=`expr $n + 1`
#  echo "I:checking dig +zflag works, and that BIND properly ignores it ($n)"
#  ret=0
#  $DIG $DIGOPTS +tcp @10.53.0.3 +zflag +qr A example > dig.out.test$n || ret=1
#  sed -n '/Sending:/,/Got answer:/p' dig.out.test$n | grep "^;; flags: rd ad; MBZ: 0x4;" > /dev/null || ret=1
#  sed -n '/Got answer:/,/AUTHORITY SECTION:/p' dig.out.test$n | grep "^;; flags: qr rd ra; QUERY: 1" > /dev/null || ret=1
#  if [ $ret != 0 ]; then echo "I:failed"; fi
#  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking dig +qr +ednsopt=08 does not cause an INSIST failure ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 +ednsopt=08 +qr a a.example > dig.out.test$n || ret=1
  grep "INSIST" < dig.out.test$n > /dev/null && ret=1
  grep "FORMERR" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking dig +subnet ($n)"
  ret=0
  $DIG $DIGOPTS +tcp @10.53.0.2 +qr +subnet=127.0.0.1 A a.example > dig.out.test$n 2>&1 || ret=1
  grep "CLIENT-SUBNET: 127.0.0.1/32/0" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`
  
  n=`expr $n + 1`
  echo "I:checking dig +sp works as an abbriviated form of split ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 +sp=4 -t sshfp foo.example > dig.out.test$n || ret=1
  grep " 9ABC DEF6 7890 " < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi 
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking dig -c works ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 -c CHAOS -t txt version.bind > dig.out.test$n || ret=1
  grep "version.bind.		0	CH	TXT" < dig.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi 
  status=`expr $status + $ret`

else
  echo "$DIG is needed, so skipping these dig tests"
fi

# using delv insecure mode as not testing dnssec here
DELVOPTS="-i -p 5300"

if [ -x ${DELV} ] ; then
  n=`expr $n + 1`
  echo "I:checking delv short form works ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +short a a.example > delv.out.test$n || ret=1
  if test `wc -l < delv.out.test$n` != 1 ; then ret=1 ; fi
  if [ $ret != 0 ]; then echo "I:failed"; fi 
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking delv split width works ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +split=4 -t sshfp foo.example > delv.out.test$n || ret=1
  grep " 9ABC DEF6 7890 " < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi 
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking delv with IPv6 on IPv4 does not work ($n)"
  if $TESTSOCK6 fd92:7065:b8e:ffff::3
  then
    ret=0
    # following should fail because @IPv4 overrides earlier @IPv6 above
    # and -6 forces IPv6 so this should fail, such as:
    # ;; getaddrinfo failed: hostname nor servname provided, or not known
    # ;; resolution failed: not found
    # note that delv returns success even on lookup failure
    $DELV $DELVOPTS @fd92:7065:b8e:ffff::3 @10.53.0.3 -6 -t txt foo.example > delv.out.test$n 2>&1 || ret=1
    # it should have no results but error output
    grep "testing" < delv.out.test$n > /dev/null && ret=1
    grep "getaddrinfo failed:" < delv.out.test$n > /dev/null || ret=1
    if [ $ret != 0 ]; then echo "I:failed"; fi 
    status=`expr $status + $ret`
  else
    echo "I:IPv6 unavailable; skipping"
  fi

  n=`expr $n + 1`
  echo "I:checking delv with reverse lookup works ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 -x 127.0.0.1 > delv.out.test$n 2>&1 || ret=1
  # doesn't matter if has answer
  grep -i "127\.in-addr\.arpa\." < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi 
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking delv over TCP works ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 a a.example > delv.out.test$n || ret=1
  grep "10\.0\.0\.1$" < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking delv +multi +norrcomments works for dnskey (when default is rrcomments)($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +multi +norrcomments DNSKEY dnskey.example > delv.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < delv.out.test$n > /dev/null && ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking delv +multi +norrcomments works for soa (when default is rrcomments)($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +multi +norrcomments SOA example > delv.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < delv.out.test$n > /dev/null && ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking delv +rrcomments works for DNSKEY($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +rrcomments DNSKEY dnskey.example > delv.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking delv +short +rrcomments works for DNSKEY ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +short +rrcomments DNSKEY dnskey.example > delv.out.test$n || ret=1
  grep "; ZSK; alg = RSAMD5 ; key id = 30795" < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking delv +short +rrcomments works ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +short +rrcomments DNSKEY dnskey.example > delv.out.test$n || ret=1
  grep "S8M=  ; ZSK; alg = RSAMD5 ; key id = 30795$" < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking delv +short +nosplit works ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +short +nosplit DNSKEY dnskey.example > delv.out.test$n || ret=1
  grep "Z8plc4Rb9VIE5x7KNHAYTvTO5d4S8M=" < delv.out.test$n > /dev/null || ret=1
  if test `wc -l < delv.out.test$n` != 1 ; then ret=1 ; fi
  f=`awk '{print NF}' < delv.out.test$n`
  test "${f:-0}" -eq 14 || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking delv +short +nosplit +norrcomments works ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +short +nosplit +norrcomments DNSKEY dnskey.example > delv.out.test$n || ret=1
  grep "Z8plc4Rb9VIE5x7KNHAYTvTO5d4S8M=$" < delv.out.test$n > /dev/null || ret=1
  if test `wc -l < delv.out.test$n` != 1 ; then ret=1 ; fi
  f=`awk '{print NF}' < delv.out.test$n`
  test "${f:-0}" -eq 4 || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi
  status=`expr $status + $ret`
  
  n=`expr $n + 1`
  echo "I:checking delv +sp works as an abbriviated form of split ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +sp=4 -t sshfp foo.example > delv.out.test$n || ret=1
  grep " 9ABC DEF6 7890 " < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi 
  status=`expr $status + $ret`
  
  n=`expr $n + 1`
  echo "I:checking delv +sh works as an abbriviated form of short ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 +sh a a.example > delv.out.test$n || ret=1
  if test `wc -l < delv.out.test$n` != 1 ; then ret=1 ; fi
  if [ $ret != 0 ]; then echo "I:failed"; fi 
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking delv -c IN works ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 -c IN -t a a.example > delv.out.test$n || ret=1
  grep "a.example." < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi 
  status=`expr $status + $ret`

  n=`expr $n + 1`
  echo "I:checking delv -c CH is ignored, and treated like IN ($n)"
  ret=0
  $DELV $DELVOPTS @10.53.0.3 -c CH -t a a.example > delv.out.test$n || ret=1
  grep "a.example." < delv.out.test$n > /dev/null || ret=1
  if [ $ret != 0 ]; then echo "I:failed"; fi 
  status=`expr $status + $ret`

  exit $status
else
  echo "$DELV is needed, so skipping these delv tests"
fi
