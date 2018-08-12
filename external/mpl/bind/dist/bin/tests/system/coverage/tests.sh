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

COVERAGE="$COVERAGE -c ./named-compilezone"

status=0
n=1

matchall () {
    file=$1
    echo "$2" | while read matchline; do
        grep "$matchline" $file > /dev/null 2>&1 || {
            echo "FAIL"
            return
        }
    done
}

echo_i "checking for DNSSEC key coverage issues"
ret=0
for dir in [0-9][0-9]-*; do
        ret=0
        echo_i "$dir"
        args= warn= error= ok= retcode= match=
        . $dir/expect
        $COVERAGE $args -K $dir example.com > coverage.$n 2>&1

        # check that return code matches expectations
        found=$?
        if [ $found -ne $retcode ]; then
            echo "retcode was $found expected $retcode"
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

        found=`matchall coverage.$n "$match"`
        if [ "$found" = "FAIL" ]; then
            echo "no match on '$match'"
            ret=1
        fi

        n=`expr $n + 1`
        if [ $ret != 0 ]; then echo_i "failed"; fi
        status=`expr $status + $ret`
done

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
