# Copyright (C) 2013, 2016, 2017  Internet Systems Consortium, Inc. ("ISC")
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

n=`expr $n + 1`
echo "I:check lookups against TTL=0 records ($n)"
i=0
passes=10
$DIG -p 5300 @10.53.0.2 axfr example | grep -v "^ds0" |
awk '$2 == "0" { print "-q", $1, $4; print "-q", "zzz"$1, $4;}' > query.list
while [ $i -lt $passes ]
do
	ret=0
	$DIG -p 5300 @10.53.0.3 -f query.list > dig.out$i.1.test$n &
	$DIG -p 5300 @10.53.0.3 -f query.list > dig.out$i.2.test$n &
	$DIG -p 5300 @10.53.0.3 -f query.list > dig.out$i.3.test$n &
	$DIG -p 5300 @10.53.0.3 -f query.list > dig.out$i.4.test$n &
	$DIG -p 5300 @10.53.0.3 -f query.list > dig.out$i.5.test$n &
	$DIG -p 5300 @10.53.0.3 -f query.list > dig.out$i.6.test$n &
	wait
	grep "status: SERVFAIL" dig.out$i.1.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.2.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.3.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.4.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.5.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.6.test$n && ret=1
	[ $ret = 1 ] && break
	i=`expr $i + 1`
	echo "I: successfully completed pass $i of $passes"
done
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:check repeated recursive lookups of non recurring TTL=0 responses get new values ($n)"
count=`(
$DIG +short -p 5300 @10.53.0.3 foo.increment
$DIG +short -p 5300 @10.53.0.3 foo.increment
$DIG +short -p 5300 @10.53.0.3 foo.increment
$DIG +short -p 5300 @10.53.0.3 foo.increment
$DIG +short -p 5300 @10.53.0.3 foo.increment
$DIG +short -p 5300 @10.53.0.3 foo.increment
$DIG +short -p 5300 @10.53.0.3 foo.increment
) | sort -u | wc -l `
if [ $count -ne 7 ] ; then echo "I:failed (count=$count)"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:check lookups against TTL=1 records ($n)"
i=0
passes=10
while [ $i -lt $passes ]
do
	ret=0
	$DIG -p 5300 @10.53.0.3 www.one.tld > dig.out$i.1.test$n
	$DIG -p 5300 @10.53.0.3 www.one.tld > dig.out$i.2.test$n
	$DIG -p 5300 @10.53.0.3 www.one.tld > dig.out$i.3.test$n
	$DIG -p 5300 @10.53.0.3 www.one.tld > dig.out$i.4.test$n
	$DIG -p 5300 @10.53.0.3 www.one.tld > dig.out$i.5.test$n
	$DIG -p 5300 @10.53.0.3 www.one.tld > dig.out$i.6.test$n
	grep "status: SERVFAIL" dig.out$i.1.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.2.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.3.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.4.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.5.test$n && ret=1
	grep "status: SERVFAIL" dig.out$i.6.test$n && ret=1
	[ $ret = 1 ] && break
	i=`expr $i + 1`
	echo "I: successfully completed pass $i of $passes"
	$PERL -e 'select(undef, undef, undef, 0.3);'
done
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:exit status: $status"
[ $status -eq 0 ] || exit 1
