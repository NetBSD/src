#!/bin/sh
#
# Copyright (C) 2013-2017  Internet Systems Consortium, Inc. ("ISC")
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
echo "I:class list ($n)"
$RRCHECKER -C > classlist.out
$DIFF classlist.out classlist.good || { echo "I:failed"; status=`expr $status + 1`; }

n=`expr $n + 1`
echo "I:type list ($n)"
$RRCHECKER -T > typelist.out
$DIFF typelist.out typelist.good || { echo "I:failed"; status=`expr $status + 1`; }

n=`expr $n + 1`
echo "I:private type list ($n)"
$RRCHECKER -P > privatelist.out
$DIFF privatelist.out privatelist.good || { echo "I:failed"; status=`expr $status + 1`; }

myecho() {
cat << EOF
$*
EOF
}

n=`expr $n + 1`
echo "I:check conversions to canonical format ($n)"
ret=0
$SHELL ../genzone.sh 0 > tempzone
$CHECKZONE -Dq . tempzone | sed '/^;/d' > checkzone.out$n
while read -r name tt cl ty rest
do
 	myecho "$cl $ty $rest" | $RRCHECKER -p > checker.out || {
		ret=1
		echo "I: '$cl $ty $rest' not handled."
	}
	read -r cl0 ty0 rest0 < checker.out
	test "$cl $ty $rest" = "$cl0 $ty0 $rest0" || {
		ret=1
		echo "I: '$cl $ty $rest' != '$cl0 $ty0 $rest0'"
	}
done < checkzone.out$n
test $ret -eq 0 || { echo "I:failed"; status=`expr $status + 1`; }

n=`expr $n + 1`
echo "I:check conversions to and from unknown record format ($n)"
ret=0
$CHECKZONE -Dq . tempzone | sed '/^;/d' > checkzone.out$n
while read -r name tt cl ty rest
do
 	myecho "$cl $ty $rest" | $RRCHECKER -u > checker.out || {
		ret=1
		echo "I: '$cl $ty $rest' not converted to unknown record format"
	}
	read -r clu tyu restu < checker.out
 	myecho "$clu $tyu $restu" | $RRCHECKER -p > checker.out || {
		ret=1
		echo "I: '$cl $ty $rest' not converted back to canonical format"
	}
	read -r cl0 ty0 rest0 < checker.out
	test "$cl $ty $rest" = "$cl0 $ty0 $rest0" || {
		ret=1
		echo "I: '$cl $ty $rest' != '$cl0 $ty0 $rest0'"
	}
done < checkzone.out$n
test $ret -eq 0 || { echo "I:failed"; status=`expr $status + 1`; }

echo "I:exit status: $status"
[ $status -eq 0 ] || exit 1
