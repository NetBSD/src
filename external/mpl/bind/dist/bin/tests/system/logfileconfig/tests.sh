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
THISDIR=`pwd`
CONFDIR="ns1"

PLAINCONF="${THISDIR}/${CONFDIR}/named.plainconf"
PLAINFILE="named_log"
DIRCONF="${THISDIR}/${CONFDIR}/named.dirconf"
DIRFILE="named_dir"
PIPECONF="${THISDIR}/${CONFDIR}/named.pipeconf"
PIPEFILE="named_pipe"
SYMCONF="${THISDIR}/${CONFDIR}/named.symconf"
SYMFILE="named_sym"
VERSCONF="${THISDIR}/${CONFDIR}/named.versconf"
VERSFILE="named_vers"
TSCONF="${THISDIR}/${CONFDIR}/named.tsconf"
TSFILE="named_ts"
UNLIMITEDCONF="${THISDIR}/${CONFDIR}/named.unlimited"
UNLIMITEDFILE="named_unlimited"
ISOCONF="${THISDIR}/${CONFDIR}/named.iso8601"
ISOFILE="named_iso8601"
ISOCONFUTC="${THISDIR}/${CONFDIR}/named.iso8601-utc"
ISOUTCFILE="named_iso8601_utc"
DLFILE="named_deflog"

PIDFILE="${THISDIR}/${CONFDIR}/named.pid"
myRNDC="$RNDC -c ${THISDIR}/${CONFDIR}/rndc.conf"
myNAMED="$NAMED -c ${THISDIR}/${CONFDIR}/named.conf -m record,size,mctx -T clienttest -T nosyslog -d 99 -D logfileconfig-ns1 -X named.lock -U 4"

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

waitforpidfile() {
	for _w in 1 2 3 4 5 6 7 8 9 10
	do
		test -f $PIDFILE && break
		sleep 1
	done
}

status=0
n=0

cd $CONFDIR
export SYSTEMTESTTOP=../..

echo_i "testing log file validity (named -g + only plain files allowed)"

n=`expr $n + 1`
echo_i "testing plain file (named -g) ($n)"
# First run with a known good config.
echo > $PLAINFILE
copy_setports $PLAINCONF named.conf
$myRNDC reconfig > rndc.out.test$n 2>&1
grep "reloading configuration failed" named.run > /dev/null 2>&1
if [ $? -ne 0 ]
then
	echo_i "testing plain file succeeded"
else
	echo_i "testing plain file failed (unexpected)"
	echo_i "exit status: 1"
	exit 1
fi

# Now try directory, expect failure
n=`expr $n + 1`
echo_i "testing directory as log file (named -g) ($n)"
echo > named.run
rm -rf $DIRFILE
mkdir -p $DIRFILE >/dev/null 2>&1
if [ $? -eq 0 ]
then
	copy_setports $DIRCONF named.conf
	echo > named.run
	$myRNDC reconfig > rndc.out.test$n 2>&1
	grep "checking logging configuration failed: invalid file" named.run > /dev/null 2>&1
	if [ $? -ne 0 ]
	then
		echo_i "testing directory as file succeeded (UNEXPECTED)"
		echo_i "exit status: 1"
		exit 1
	else
		echo_i "testing directory as log file failed (expected)"
	fi
else
	echo_i "skipping directory test (unable to create directory)"
fi

# Now try pipe file, expect failure
n=`expr $n + 1`
echo_i "testing pipe file as log file (named -g) ($n)"
echo > named.run
rm -f $PIPEFILE
mkfifo $PIPEFILE >/dev/null 2>&1
if [ $? -eq 0 ]
then
	copy_setports $PIPECONF named.conf
	echo > named.run
	$myRNDC reconfig > rndc.out.test$n 2>&1
	grep "checking logging configuration failed: invalid file" named.run  > /dev/null 2>&1
	if [ $? -ne 0 ]
	then
		echo_i "testing pipe file as log file succeeded (UNEXPECTED)"
		echo_i "exit status: 1"
		exit 1
	else
		echo_i "testing pipe file as log file failed (expected)"
	fi
else
	echo_i "skipping pipe test (unable to create pipe)"
fi

# Now try symlink file to plain file, expect success
n=`expr $n + 1`
echo_i "testing symlink to plain file as log file (named -g) ($n)"
# Assume success
echo > named.run
echo > $PLAINFILE
rm -f  $SYMFILE  $SYMFILE
ln -s $PLAINFILE $SYMFILE >/dev/null 2>&1
if [ $? -eq 0 ]
then
	copy_setports $SYMCONF named.conf
	$myRNDC reconfig > rndc.out.test$n 2>&1
	echo > named.run
	grep "reloading configuration failed" named.run > /dev/null 2>&1
	if [ $? -ne 0 ]
	then
		echo_i "testing symlink to plain file succeeded"
	else
		echo_i "testing symlink to plain file failed (unexpected)"
		echo_i "exit status: 1"
		exit 1
	fi
else
	echo_i "skipping symlink test (unable to create symlink)"
fi
# Stop the server and run through a series of tests with various config
# files while controlling the stop/start of the server.
# Have to stop the stock server because it uses "-g"
#
$PERL ../../stop.pl logfileconfig ns1

$myNAMED > /dev/null 2>&1

if [ $? -ne 0 ]
then
	echo_i "failed to start $myNAMED"
	echo_i "exit status: $status"
	exit $status
fi

status=0

echo_i "testing log file validity (only plain files allowed)"

n=`expr $n + 1`
echo_i "testing plain file (named -g) ($n)"
# First run with a known good config.
echo > $PLAINFILE
copy_setports $PLAINCONF named.conf
$myRNDC reconfig > rndc.out.test$n 2>&1
grep "reloading configuration failed" named.run > /dev/null 2>&1
if [ $? -ne 0 ]
then
	echo_i "testing plain file succeeded"
else
	echo_i "testing plain file failed (unexpected)"
	echo_i "exit status: 1"
	exit 1
fi

# Now try directory, expect failure
n=`expr $n + 1`
echo_i "testing directory as log file ($n)"
echo > named.run
rm -rf $DIRFILE
mkdir -p $DIRFILE >/dev/null 2>&1
if [ $? -eq 0 ]
then
	copy_setports $DIRCONF named.conf
	echo > named.run
	$myRNDC reconfig > rndc.out.test$n 2>&1
	grep "configuring logging: invalid file" named.run > /dev/null 2>&1
	if [ $? -ne 0 ]
	then
		echo_i "testing directory as file succeeded (UNEXPECTED)"
		echo_i "exit status: 1"
		exit 1
	else
		echo_i "testing directory as log file failed (expected)"
	fi
else
	echo_i "skipping directory test (unable to create directory)"
fi

# Now try pipe file, expect failure
n=`expr $n + 1`
echo_i "testing pipe file as log file ($n)"
echo > named.run
rm -f $PIPEFILE
mkfifo $PIPEFILE >/dev/null 2>&1
if [ $? -eq 0 ]
then
	copy_setports $PIPECONF named.conf
	echo > named.run
	$myRNDC reconfig > rndc.out.test$n 2>&1
	grep "configuring logging: invalid file" named.run  > /dev/null 2>&1
	if [ $? -ne 0 ]
	then
		echo_i "testing pipe file as log file succeeded (UNEXPECTED)"
		echo_i "exit status: 1"
		exit 1
	else
		echo_i "testing pipe file as log file failed (expected)"
	fi
else
	echo_i "skipping pipe test (unable to create pipe)"
fi

# Now try symlink file to plain file, expect success
n=`expr $n + 1`
echo_i "testing symlink to plain file as log file ($n)"
# Assume success
status=0
echo > named.run
echo > $PLAINFILE
rm -f $SYMFILE
ln -s $PLAINFILE $SYMFILE >/dev/null 2>&1
if [ $? -eq 0 ]
then
	copy_setports $SYMCONF named.conf
	$myRNDC reconfig > rndc.out.test$n 2>&1
	echo > named.run
	grep "reloading configuration failed" named.run > /dev/null 2>&1
	if [ $? -ne 0 ]
	then
		echo_i "testing symlink to plain file succeeded"
	else
		echo_i "testing symlink to plain file failed (unexpected)"
		echo_i "exit status: 1"
		exit 1
	fi
else
	echo_i "skipping symlink test (unable to create symlink)"
fi

n=`expr $n + 1`
echo_i "testing default logfile using named -L file ($n)"
# Now stop the server again and test the -L option
rm -f $DLFILE
$PERL ../../stop.pl logfileconfig ns1
if ! test -f $PIDFILE; then
	copy_setports $PLAINCONF named.conf
	$myNAMED -L $DLFILE > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo_i "failed to start $myNAMED"
		echo_i "exit status: $status"
		exit $status
	fi

	waitforpidfile

	sleep 1
	if [ -f "$DLFILE" ]; then
		echo_i "testing default logfile using named -L succeeded"
	else
		echo_i "testing default logfile using named -L failed"
		echo_i "exit status: 1"
		exit 1
	fi
else
	echo_i "failed to cleanly stop $myNAMED"
	echo_i "exit status: 1"
	exit 1
fi

echo_i "testing logging functionality"

n=`expr $n + 1`
echo_i "testing iso8601 timestamp ($n)"
copy_setports $ISOCONF named.conf
$myRNDC reconfig > rndc.out.test$n 2>&1
if grep '^....-..-..T..:..:..\.... ' $ISOFILE > /dev/null; then
	echo_i "testing iso8601 timestamp succeeded"
else
	echo_i "testing iso8601 timestamp failed"
	status=`expr $status + 1`
fi

n=`expr $n + 1`
echo_i "testing iso8601-utc timestamp ($n)"
copy_setports $ISOCONFUTC named.conf
$myRNDC reconfig > rndc.out.test$n 2>&1
if grep '^....-..-..T..:..:..\....Z' $ISOUTCFILE > /dev/null; then
	echo_i "testing iso8601-utc timestamp succeeded"
else
	echo_i "testing iso8601-utc timestamp failed"
	status=`expr $status + 1`
fi

n=`expr $n + 1`
echo_i "testing explicit versions ($n)"
copy_setports $VERSCONF named.conf
# a seconds since epoch version number
touch $VERSFILE.1480039317
t1=`$PERL -e 'print time()."\n";'`
$myRNDC reconfig > rndc.out.test$n 2>&1
$DIG version.bind txt ch @10.53.0.1 -p ${PORT} > dig.out.test$n
t2=`$PERL -e 'print time()."\n";'`
t=`expr ${t2:-0} - ${t1:-0}`
if test ${t:-1000} -gt 5
then
        echo_i "testing explicit versions failed: cleanup of old entries took too long ($t secs)"
	status=`expr $status + 1`
fi
if ! grep "status: NOERROR" dig.out.test$n > /dev/null
then
	echo_i "testing explicit versions failed: DiG lookup failed"
	status=`expr $status + 1`
fi
if test_with_retry -f $VERSFILE.1480039317
then
	echo_i "testing explicit versions failed: $VERSFILE.1480039317 not removed"
	status=`expr $status + 1`
fi
if test_with_retry -f $VERSFILE.5
then
	echo_i "testing explicit versions failed: $VERSFILE.5 exists"
	status=`expr $status + 1`
fi
if test_with_retry ! -f $VERSFILE.4
then
	echo_i "testing explicit versions failed: $VERSFILE.4 does not exist"
	status=`expr $status + 1`
fi

n=`expr $n + 1`
echo_i "testing timestamped versions ($n)"
copy_setports $TSCONF named.conf
# a seconds since epoch version number
touch $TSFILE.2015010112000012
t1=`$PERL -e 'print time()."\n";'`
$myRNDC reconfig > rndc.out.test$n 2>&1
$DIG version.bind txt ch @10.53.0.1 -p ${PORT} > dig.out.test$n
t2=`$PERL -e 'print time()."\n";'`
t=`expr ${t2:-0} - ${t1:-0}`
if test ${t:-1000} -gt 5
then
        echo_i "testing timestamped versions failed: cleanup of old entries took too long ($t secs)"
	status=`expr $status + 1`
fi
if ! grep "status: NOERROR" dig.out.test$n > /dev/null
then
	echo_i "testing timestamped versions failed: DiG lookup failed"
	status=`expr $status + 1`
fi
if test_with_retry -f $TSFILE.1480039317
then
	echo_i "testing timestamped versions failed: $TSFILE.1480039317 not removed"
	status=`expr $status + 1`
fi

n=`expr $n + 1`
echo_i "testing unlimited versions ($n)"
copy_setports $UNLIMITEDCONF named.conf
# a seconds since epoch version number
touch $UNLIMITEDFILE.1480039317
t1=`$PERL -e 'print time()."\n";'`
$myRNDC reconfig > rndc.out.test$n 2>&1
$DIG version.bind txt ch @10.53.0.1 -p ${PORT} > dig.out.test$n
t2=`$PERL -e 'print time()."\n";'`
t=`expr ${t2:-0} - ${t1:-0}`
if test ${t:-1000} -gt 5
then
        echo_i "testing unlimited versions failed: took too long ($t secs)"
	status=`expr $status + 1`
fi
if ! grep "status: NOERROR" dig.out.test$n > /dev/null
then
	echo_i "testing unlimited versions failed: DiG lookup failed"
	status=`expr $status + 1`
fi
if test_with_retry ! -f $UNLIMITEDFILE.1480039317
then
	echo_i "testing unlimited versions failed: $UNLIMITEDFILE.1480039317 removed"
	status=`expr $status + 1`
fi
if test_with_retry ! -f $UNLIMITEDFILE.4
then
	echo_i "testing unlimited versions failed: $UNLIMITEDFILE.4 does not exist"
	status=`expr $status + 1`
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
