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

matchall () {
    match_result=ok
    file=$1
    while IFS="," read expect matchline; do
	[ -z "$matchline" ] && continue
	matches=`grep "$matchline" $file | wc -l`
	[ "$matches" -ne "$expect" ] && {
	    echo "'$matchline': expected $expect found $matches"
	    return 1
	}
    done << EOF
    $2
EOF
    return 0
}

echo_i "checking for DNSSEC key coverage issues"
ret=0
for dir in [0-9][0-9]-*; do
	ret=0
	echo_i "$dir ($n)"
	kargs= cargs= kmatch= cmatch= kret= cret=0 warn= error= ok=
	. $dir/expect

	# use policy.conf if available
	policy=""
	if [ -e "$dir/policy.conf" ]; then
		policy="-c $dir/policy.conf"
		if grep -e "-c policy.conf" $dir/expect > /dev/null
		then
			echo_i "fix $dir/expect: multiple policy files"
			ret=1
		fi
	else
		policy="-c policy.conf"
	fi

	# run keymgr to update keys
	if [ "$CYGWIN" ]; then
	    $KEYMGR $policy -K $dir -g `cygpath -w $KEYGEN` \
		-s `cygpath -w $SETTIME` $kargs > keymgr.$n 2>&1
	else
	    $KEYMGR $policy -K $dir -g $KEYGEN \
		-s $SETTIME $kargs > keymgr.$n 2>&1
	fi
	# check that return code matches expectations
	found=$?
	if [ $found -ne $kret ]; then
	    echo "keymgr retcode was $found expected $kret"
	    ret=1
	fi

	# check for matches in keymgr output
	matchall keymgr.$n "$kmatch" || ret=1

	# now check coverage
	$COVERAGE -K $dir $cargs > coverage.$n 2>&1
	# check that return code matches expectations
	found=$?
	if [ $found -ne $cret ]; then
	    echo "coverage retcode was $found expected $cret"
	    ret=1
	fi

	# check for correct number of errors
	found=`grep ERROR coverage.$n | wc -l`
	if [ $found -ne $error ]; then
	    echo "error count was $found expected $error"
	    ret=1
	fi

	# check for correct number of warnings
	found=`grep WARNING coverage.$n | wc -l`
	if [ $found -ne $warn ]; then
	    echo "warning count was $found expected $warn"
	    ret=1
	fi

	# check for correct number of OKs
	found=`grep "No errors found" coverage.$n | wc -l`
	if [ $found -ne $ok ]; then
	    echo "good count was $found expected $ok"
	    ret=1
	fi

	# check for matches in coverage output
	matchall coverage.$n "$cmatch" || ret=1

	if [ -f $dir/extra.sh ]; then
	   cd $dir
	   . ./extra.sh
	   cd ..
	fi

	n=`expr $n + 1`
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`
done

echo_i "checking domains ending in . ($n)"
ret=0
$KEYMGR -g $KEYGEN -s $SETTIME . > keymgr.1.$n 2>&1
nkeys=`grep dnssec-keygen keymgr.1.$n | wc -l`
[ "$nkeys" -eq 2 ] || ret=1
$KEYMGR -g $KEYGEN -s $SETTIME . > keymgr.2.$n 2>&1
nkeys=`grep dnssec-keygen keymgr.2.$n | wc -l`
[ "$nkeys" -eq 0 ] || ret=1
$KEYMGR -g $KEYGEN -s $SETTIME example.com. > keymgr.3.$n 2>&1
nkeys=`grep dnssec-keygen keymgr.3.$n | wc -l`
[ "$nkeys" -eq 2 ] || ret=1
$KEYMGR -g $KEYGEN -s $SETTIME example.com. > keymgr.4.$n 2>&1
nkeys=`grep dnssec-keygen keymgr.4.$n | wc -l`
[ "$nkeys" -eq 0 ] || ret=1
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "checking policy.conf parser ($n)"
ret=0
${PYTHON} testpolicy.py policy.sample > policy.out
$DOS2UNIX policy.out > /dev/null
cmp -s policy.good policy.out || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
