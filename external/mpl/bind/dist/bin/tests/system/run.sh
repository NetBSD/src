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

#
# Run a system test.
#

SYSTEMTESTTOP=.
. $SYSTEMTESTTOP/conf.sh

stopservers=true
baseport=5300

if [ ${SYSTEMTEST_NO_CLEAN:-0} -eq 1 ]; then
	clean=false
else
	clean=true
fi

while getopts "knp:r-:" flag; do
    case "$flag" in
	-) case "${OPTARG}" in
               keep) stopservers=false ;;
               noclean) clean=false ;;
           esac
           ;;
	k) stopservers=false ;;
	n) clean=false ;;
	p) baseport=$OPTARG ;;
	r) runall="-r" ;;
    esac
done
shift `expr $OPTIND - 1`

if [ $# -eq 0 ]; then
    echofail "Usage: $0 [-k] [-n] [-p <PORT>] [-r] test-directory [test-options]" >&2;
    exit 1
fi

systest=$1
shift

if [ ! -d $systest ]; then
    echofail "$0: $systest: no such test" >&2
    exit 1
fi

# Define the number of ports allocated for each test, and the lowest and
# highest valid values for the "-p" option.
#
# The lowest valid value is one more than the highest privileged port number
# (1024).
#
# The highest valid value is calculated by noting that the value passed on the
# command line is the lowest port number in a block of "numports" consecutive
# ports and that the highest valid port number is 65,535.
numport=100
minvalid=`expr 1024 + 1`
maxvalid=`expr 65535 - $numport + 1`

test "$baseport" -eq "$baseport" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echofail "$0: $systest: must specify a numeric value for the port" >&2
    exit 1
elif [ $baseport -lt $minvalid -o $baseport -gt $maxvalid  ]; then
    echofail "$0: $systest: the specified port must be in the range $minvalid to $maxvalid" >&2
    exit 1
fi

# Name the first 10 ports in the set (it is assumed that each test has access
# to ten or more ports): the query port, the control port and eight extra
# ports.  Since the lowest numbered port (specified in the command line)
# will usually be a multiple of 10, the names are chosen so that if this is
# true, the last digit of EXTRAPORTn is "n".
PORT=$baseport
EXTRAPORT1=`expr $baseport + 1`
EXTRAPORT2=`expr $baseport + 2`
EXTRAPORT3=`expr $baseport + 3`
EXTRAPORT4=`expr $baseport + 4`
EXTRAPORT5=`expr $baseport + 5`
EXTRAPORT6=`expr $baseport + 6`
EXTRAPORT7=`expr $baseport + 7`
EXTRAPORT8=`expr $baseport + 8`
CONTROLPORT=`expr $baseport + 9`

LOWPORT=$baseport
HIGHPORT=`expr $baseport + $numport - 1`

export PORT
export EXTRAPORT1
export EXTRAPORT2
export EXTRAPORT3
export EXTRAPORT4
export EXTRAPORT5
export EXTRAPORT6
export EXTRAPORT7
export EXTRAPORT8
export CONTROLPORT

export LOWPORT
export HIGHPORT

echostart "S:$systest:`date`"
echoinfo  "T:$systest:1:A"
echoinfo  "A:$systest:System test $systest"
echoinfo  "I:$systest:PORTRANGE:${LOWPORT} - ${HIGHPORT}"

if [ x${PERL:+set} = x ]
then
    echowarn "I:$systest:Perl not available.  Skipping test."
    echowarn "R:$systest:UNTESTED"
    echoend  "E:$systest:`date $dateargs`"
    exit 0;
fi

$PERL testsock.pl -p $PORT  || {
    echowarn "I:$systest:Network interface aliases not set up.  Skipping test."
    echowarn "R:$systest:UNTESTED"
    echoend  "E:$systest:`date $dateargs`"
    exit 0;
}

# Check for test-specific prerequisites.
test ! -f $systest/prereq.sh || ( cd $systest && $SHELL prereq.sh "$@" )
result=$?

if [ $result -eq 0 ]; then
    : prereqs ok
else
    echowarn "I:$systest:Prerequisites missing, skipping test."
    [ $result -eq 255 ] && echowarn "R:$systest:SKIPPED" || echowarn "R:$systest:UNTESTED"
    echoend "E:$systest:`date $dateargs`"
    exit 0
fi

# Check for PKCS#11 support
if
    test ! -f $systest/usepkcs11 || $SHELL cleanpkcs11.sh
then
    : pkcs11 ok
else
    echowarn "I:$systest:Need PKCS#11, skipping test."
    echowarn "R:$systest:PKCS11ONLY"
    echoend  "E:$systest:`date $dateargs`"
    exit 0
fi

# Set up any dynamically generated test data
if test -f $systest/setup.sh
then
   ( cd $systest && $SHELL setup.sh "$@" )
fi

# Start name servers running
$PERL start.pl --port $PORT $systest
if [ $? -ne 0 ]; then
    echofail "R:$systest:FAIL"
    echoend  "E:$systest:`date $dateargs`"
    exit 1
fi

# Run the tests
( cd $systest ; $SHELL tests.sh "$@" )
status=$?

if $stopservers
then
    :
else
    exit $status
fi

# Shutdown
$PERL stop.pl $systest

status=`expr $status + $?`

if [ $status != 0 ]; then
    echofail "R:$systest:FAIL"
    # Do not clean up - we need the evidence.
    find . -name core -exec chmod 0644 '{}' \;
else
    echopass "R:$systest:PASS"
    if $clean
    then
	$SHELL clean.sh $runall $systest "$@"
	if test -d ../../../.git
	then
	    git status -su --ignored $systest | \
	    sed -n -e 's|^?? \(.*\)|I:file \1 not removed|p' \
	    -e 's|^!! \(.*/named.run\)$|I:file \1 not removed|p' \
	    -e 's|^!! \(.*/named.memstats\)$|I:file \1 not removed|p'
	fi
    fi
fi

echoend "E:$systest:`date $dateargs`"

exit $status
