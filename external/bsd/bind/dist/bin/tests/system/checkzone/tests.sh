# Copyright (C) 2011-2015  Internet Systems Consortium, Inc. ("ISC")
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

# Id: tests.sh,v 1.2 2011/03/02 04:20:33 marka Exp 

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0
n=1

for db in zones/good*.db
do
	echo "I:checking $db ($n)"
	ret=0
	case $db in
	zones/good-gc-msdcs.db)
		$CHECKZONE -k fail -i local example $db > test.out.$n 2>&1 || ret=1
		;;
	zones/good-dns-sd-reverse.db)
		$CHECKZONE -k fail -i local 0.0.0.0.in-addr.arpa $db > test.out.$n 2>&1 || ret=1
		;;
	*)
		$CHECKZONE -i local example $db > test.out.$n 2>&1 || ret=1
		;;
	esac
	n=`expr $n + 1`
	if [ $ret != 0 ]; then echo "I:failed"; fi
	status=`expr $status + $ret`
done

for db in zones/bad*.db
do
	echo "I:checking $db ($n)"
	ret=0
	case $db in
	zones/bad-dns-sd-reverse.db)
		$CHECKZONE -k fail -i local 0.0.0.0.in-addr.arpa $db > test.out.$n 2>&1 && ret=1
		;;
	*)
                $CHECKZONE -i local example $db > test.out.$n 2>&1 && ret=1
		;;
	esac
	n=`expr $n + 1`
	if [ $ret != 0 ]; then echo "I:failed"; fi
	status=`expr $status + $ret`
done

echo "I:checking with spf warnings ($n)"
ret=0
$CHECKZONE example zones/spf.db > test.out1.$n 2>&1 || ret=1
$CHECKZONE -T ignore example zones/spf.db > test.out2.$n 2>&1 || ret=1
grep "'x.example' found type SPF" test.out1.$n > /dev/null && ret=1
grep "'y.example' found type SPF" test.out1.$n > /dev/null || ret=1
grep "'example' found type SPF" test.out1.$n > /dev/null && ret=1
grep "'x.example' found type SPF" test.out2.$n > /dev/null && ret=1
grep "'y.example' found type SPF" test.out2.$n > /dev/null && ret=1
grep "'example' found type SPF" test.out2.$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking for no 'inherited owner' warning on '\$INCLUDE file' with no new \$ORIGIN ($n)"
ret=0
$CHECKZONE example zones/nowarn.inherited.owner.db > test.out1.$n 2>&1 || ret=1
grep "inherited.owner" test.out1.$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking for 'inherited owner' warning on '\$ORIGIN + \$INCLUDE file' ($n)"
ret=0
$CHECKZONE example zones/warn.inherit.origin.db > test.out1.$n 2>&1 || ret=1
grep "inherited.owner" test.out1.$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking for 'inherited owner' warning on '\$INCLUDE file origin' ($n)"
ret=0
$CHECKZONE example zones/warn.inherited.owner.db > test.out1.$n 2>&1 || ret=1
grep "inherited.owner" test.out1.$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking that raw zone with bad class is handled ($n)"
ret=0
$CHECKZONE -f raw example zones/bad-badclass.raw > test.out.$n 2>&1 && ret=1
grep "failed: bad class" test.out.$n >/dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:exit status: $status"
exit $status
