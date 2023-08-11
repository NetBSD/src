#!/bin/sh

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh
THISDIR=`pwd`
CONFDIR="ns1"

# Test given condition.  If true, test again after a second.  Used for testing
# filesystem-dependent conditions in order to prevent false negatives caused by
# directory contents not being synchronized immediately after rename() returns.
test_with_retry() {
	if test "$@"; then
		sleep 1
		if test "$@"; then
			return 0
		fi
	fi
	return 1
}

status=0
n=0

echo_i "testing log file validity (named -g + only plain files allowed)"

# First run with a known good config.
n=$((n+1))
echo_i "testing log file validity (only plain files allowed) ($n)"
ret=0
cat /dev/null > ns1/named_log
copy_setports ns1/named.plainconf.in ns1/named.conf
nextpart ns1/named.run > /dev/null
rndc_reconfig ns1 10.53.0.1 > rndc.out.test$n
wait_for_log 5 "reloading configuration succeeded" ns1/named.run || ret=1
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Now try directory, expect failure
n=$((n+1))
echo_i "testing directory as log file ($n)"
ret=0
nextpart ns1/named.run > /dev/null
copy_setports ns1/named.dirconf.in ns1/named.conf
rndc_reconfig ns1 10.53.0.1 > rndc.out.test$n
wait_for_log 5 "reloading configuration failed: invalid file" ns1/named.run || ret=1
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Now try pipe file, expect failure
n=$((n+1))
echo_i "testing pipe file as log file ($n)"
ret=0
nextpart ns1/named.run > /dev/null
rm -f ns1/named_pipe
if mkfifo ns1/named_pipe >/dev/null 2>&1; then
    copy_setports ns1/named.pipeconf.in ns1/named.conf
    rndc_reconfig ns1 10.53.0.1 > rndc.out.test$n
    wait_for_log 5 "reloading configuration failed: invalid file" ns1/named.run || ret=1
    if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))
else
    echo_i "skipping pipe test (unable to create pipe)"
fi

# Now try symlink file to plain file, expect success
n=$((n+1))
echo_i "testing symlink to plain file as log file ($n)"
ret=0
rm -f ns1/named_log ns1/named_sym
touch ns1/named_log
if ln -s $(pwd)/ns1/named_log $(pwd)/ns1/named_sym >/dev/null 2>&1; then
    nextpart ns1/named.run > /dev/null
    copy_setports ns1/named.symconf.in ns1/named.conf
    rndc_reconfig ns1 10.53.0.1 > rndc.out.test$n
    wait_for_log 5 "reloading configuration succeeded" ns1/named.run || ret=1
    if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))
else
	echo_i "skipping symlink test (unable to create symlink)"
fi

echo_i "repeat previous tests without named -g"
copy_setports ns1/named.plain.in ns1/named.conf
$PERL ../stop.pl --use-rndc --port ${CONTROLPORT} logfileconfig ns1
cp named1.args ns1/named.args
start_server --noclean --restart --port ${PORT} ns1

n=$((n+1))
echo_i "testing log file validity (only plain files allowed) ($n)"
ret=0
cat /dev/null > ns1/named_log
copy_setports ns1/named.plainconf.in ns1/named.conf
nextpart ns1/named.run > /dev/null
rndc_reconfig ns1 10.53.0.1 > rndc.out.test$n
wait_for_log 5 "reloading configuration succeeded" ns1/named.run || ret=1
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Now try directory, expect failure
n=$((n+1))
echo_i "testing directory as log file ($n)"
ret=0
nextpart ns1/named.run > /dev/null
copy_setports ns1/named.dirconf.in ns1/named.conf
rndc_reconfig ns1 10.53.0.1 > rndc.out.test$n
wait_for_log 5 "reloading configuration failed: invalid file" ns1/named.run || ret=1
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Now try pipe file, expect failure
n=$((n+1))
echo_i "testing pipe file as log file ($n)"
ret=0
nextpart ns1/named.run > /dev/null
rm -f ns1/named_pipe
if mkfifo ns1/named_pipe >/dev/null 2>&1; then
    copy_setports ns1/named.pipeconf.in ns1/named.conf
    rndc_reconfig ns1 10.53.0.1 > rndc.out.test$n
    wait_for_log 5 "reloading configuration failed: invalid file" ns1/named.run || ret=1
    if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))
else
    echo_i "skipping pipe test (unable to create pipe)"
fi

# Now try symlink file to plain file, expect success
n=$((n+1))
echo_i "testing symlink to plain file as log file ($n)"
ret=0
rm -f ns1/named_log ns1/named_sym
touch ns1/named_log
if ln -s $(pwd)/ns1/named_log $(pwd)/ns1/named_sym >/dev/null 2>&1; then
    nextpart ns1/named.run > /dev/null
    copy_setports ns1/named.symconf.in ns1/named.conf
    rndc_reconfig ns1 10.53.0.1 > rndc.out.test$n
    wait_for_log 5 "reloading configuration succeeded" ns1/named.run || ret=1
    if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))
else
	echo_i "skipping symlink test (unable to create symlink)"
fi

echo_i "testing logging functionality"
n=$((n+1))
ret=0
echo_i "testing iso8601 timestamp ($n)"
copy_setports ns1/named.iso8601.in ns1/named.conf
rndc_reconfig ns1 10.53.0.1 > rndc.out.test$n
grep '^....-..-..T..:..:..\.... ' ns1/named_iso8601 > /dev/null || ret=1
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "testing iso8601-utc timestamp ($n)"
ret=0
copy_setports ns1/named.iso8601-utc.in ns1/named.conf
rndc_reconfig ns1 10.53.0.1 > rndc.out.test$n
grep '^....-..-..T..:..:..\....Z' ns1/named_iso8601_utc > /dev/null || ret=1
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "testing explicit versions ($n)"
ret=0
copy_setports ns1/named.versconf.in ns1/named.conf
# a seconds since epoch version number
touch ns1/named_vers.1480039317
rndc_reconfig ns1 10.53.0.1 > rndc.out.test$n
$DIG version.bind txt ch @10.53.0.1 -p ${PORT} > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
# we are configured to retain five logfiles (a current file
# and 4 backups). so files with version number 5 or higher
# should be removed.
test_with_retry -f ns1/named_vers.1480039317 && ret=1
test_with_retry -f ns1/named_vers.5 && ret=1
test_with_retry -f ns1/named_vers.4 || ret=1
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "testing timestamped versions ($n)"
ret=0
copy_setports ns1/named.tsconf.in ns1/named.conf
# a seconds since epoch version number
touch ns1/named_ts.1480039317
# a timestamp version number
touch ns1/named_ts.20150101120000120
rndc_reconfig ns1 10.53.0.1 > rndc.out.test$n
_found2() (
        $DIG version.bind txt ch @10.53.0.1 -p ${PORT} > dig.out.test$n
        grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1

        # we are configured to keep three versions, so the oldest
        # timestamped versions should be gone, and there should
        # be two or three backup ones.
        [ -f ns1/named_ts.1480039317 ] && return 1
        [ -f ns1/named_ts.20150101120000120 ] && return 1
        set -- ns1/named_ts.*
        [ "$#" -eq 2 -o "$#" -eq 3 ] || return 1
)
retry_quiet 5 _found2 || ret=1
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "testing unlimited versions ($n)"
ret=0
copy_setports ns1/named.unlimited.in ns1/named.conf
# a seconds since epoch version number
touch ns1/named_unlimited.1480039317
rndc_reconfig ns1 10.53.0.1 > rndc.out.test$n
$DIG version.bind txt ch @10.53.0.1 -p ${PORT} > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
test_with_retry -f ns1/named_unlimited.1480039317 || ret=1
test_with_retry -f ns1/named_unlimited.4 || ret=1
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "testing default logfile using named -L file ($n)"
ret=0
$PERL ../stop.pl logfileconfig ns1
cp named2.args ns1/named.args
test -f ns1/named.pid && ret=1
rm -f ns1/named_deflog
copy_setports ns1/named.plainconf.in ns1/named.conf
start_server --noclean --restart --port ${PORT} ns1
[ -f "ns1/named_deflog" ] || ret=1
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
