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
n=0

n=`expr $n + 1`
echo_i "class list ($n)"
$RRCHECKER -C > classlist.out
$DIFF classlist.out classlist.good || { echo_i "failed"; status=`expr $status + 1`; }

n=`expr $n + 1`
echo_i "type list ($n)"
$RRCHECKER -T > typelist.out
$DIFF typelist.out typelist.good || { echo_i "failed"; status=`expr $status + 1`; }

n=`expr $n + 1`
echo_i "private type list ($n)"
$RRCHECKER -P > privatelist.out
$DIFF privatelist.out privatelist.good || { echo_i "failed"; status=`expr $status + 1`; }

myecho() {
cat << EOF
$*
EOF
}

n=`expr $n + 1`
echo_i "check conversions to canonical format ($n)"
ret=0
$SHELL ../genzone.sh 0 > tempzone
$CHECKZONE -Dq . tempzone | sed '/^;/d' > checkzone.out$n
while read -r name tt cl ty rest
do
	myecho "$cl $ty $rest" | $RRCHECKER -p > checker.out || {
		ret=1
		echo_i "'$cl $ty $rest' not handled."
	}
	read -r cl0 ty0 rest0 < checker.out
	test "$cl $ty $rest" = "$cl0 $ty0 $rest0" || {
		ret=1
		echo_i "'$cl $ty $rest' != '$cl0 $ty0 $rest0'"
	}
done < checkzone.out$n
test $ret -eq 0 || { echo_i "failed"; status=`expr $status + 1`; }

n=`expr $n + 1`
echo_i "check conversions to and from unknown record format ($n)"
ret=0
$CHECKZONE -Dq . tempzone | sed '/^;/d' > checkzone.out$n
while read -r name tt cl ty rest
do
	myecho "$cl $ty $rest" | $RRCHECKER -u > checker.out || {
		ret=1
		echo_i "'$cl $ty $rest' not converted to unknown record format"
	}
	read -r clu tyu restu < checker.out
	myecho "$clu $tyu $restu" | $RRCHECKER -p > checker.out || {
		ret=1
		echo_i "'$cl $ty $rest' not converted back to canonical format"
	}
	read -r cl0 ty0 rest0 < checker.out
	test "$cl $ty $rest" = "$cl0 $ty0 $rest0" || {
		ret=1
		echo_i "'$cl $ty $rest' != '$cl0 $ty0 $rest0'"
	}
done < checkzone.out$n
test $ret -eq 0 || { echo_i "failed"; status=`expr $status + 1`; }

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
