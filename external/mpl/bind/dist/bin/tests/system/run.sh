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

SYSTEMTESTTOP="$(cd -P -- "$(dirname -- "$0")" && pwd -P)"
. $SYSTEMTESTTOP/conf.sh

export SYSTEMTESTTOP

date_with_args() (
    date "+%Y-%m-%dT%T%z"
)

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
    esac
done
shift `expr $OPTIND - 1`

if [ $# -eq 0 ]; then
    echofail "Usage: $0 [-k] [-n] [-p <PORT>] test-directory [test-options]" >&2;
    exit 1
fi

systest=${1%%/}
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

restart=false

start_servers_failed() {
    echoinfo "I:$systest:starting servers failed"
    echofail "R:$systest:FAIL"
    echoend  "E:$systest:$(date_with_args)"
    exit 1
}

start_servers() {
    echoinfo "I:$systest:starting servers"
    if $restart; then
        $PERL start.pl --restart --port "$PORT" "$systest" || start_servers_failed
    else
        restart=true
        $PERL start.pl --port "$PORT" "$systest" || start_servers_failed
    fi
}

stop_servers_failed() {
    echoinfo "I:$systest:stopping servers failed"
    echofail "R:$systest:FAIL"
    echoend  "E:$systest:$(date_with_args)"
    exit 1
}

stop_servers() {
    if $stopservers; then
        echoinfo "I:$systest:stopping servers"
        $PERL stop.pl "$systest" || stop_servers_failed
    fi
}

echostart "S:$systest:$(date_with_args)"
echoinfo  "T:$systest:1:A"
echoinfo  "A:$systest:System test $systest"
echoinfo  "I:$systest:PORTRANGE:${LOWPORT} - ${HIGHPORT}"

if [ x${PERL:+set} = x ]
then
    echowarn "I:$systest:Perl not available.  Skipping test."
    echowarn "R:$systest:UNTESTED"
    echoend  "E:$systest:$(date_with_args)"
    exit 0;
fi

$PERL testsock.pl -p $PORT  || {
    echowarn "I:$systest:Network interface aliases not set up.  Skipping test."
    echowarn "R:$systest:UNTESTED"
    echoend  "E:$systest:$(date_with_args)"
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
    echoend "E:$systest:$(date_with_args)"
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
    echoend  "E:$systest:$(date_with_args)"
    exit 0
fi

# Clean up files left from any potential previous runs
if test -f $systest/clean.sh
then
    if ! ( cd "${systest}" && $SHELL clean.sh "$@" ); then
	echowarn "I:$systest:clean.sh script failed"
	echofail "R:$systest:FAIL"
	echoend  "E:$systest:$(date_with_args)"
	exit 1
    fi
fi

# Set up any dynamically generated test data
if test -f $systest/setup.sh
then
    if ! ( cd "${systest}" && $SHELL setup.sh "$@" ); then
	echowarn "I:$systest:setup.sh script failed"
	echofail "R:$systest:FAIL"
	echoend  "E:$systest:$(date_with_args)"
	exit 1
    fi
fi

status=0
run=0
# Run the tests
if [ -r "$systest/tests.sh" ]; then
    start_servers
    ( cd "$systest" && $SHELL tests.sh "$@" )
    status=$?
    run=$((run+1))
    stop_servers
fi

if [ -n "$PYTEST" ]; then
    run=$((run+1))
    for test in $(cd "${systest}" && find . -name "tests*.py"); do
	start_servers
	rm -f "$systest/$test.status"
	test_status=0
	(cd "$systest" && "$PYTEST" -v "$test" "$@" || echo "$?" > "$test.status") | SYSTESTDIR="$systest" cat_d
	if [ -f "$systest/$test.status" ]; then
	    echo_i "FAILED"
	    test_status=$(cat "$systest/$test.status")
	fi
	status=$((status+test_status))
	stop_servers
    done
else
    echoinfo "I:$systest:pytest not installed, skipping python tests"
fi

if [ "$run" -eq "0" ]; then
    echoinfo "I:$systest:No tests were found and run"
    status=255
fi


if $stopservers
then
    :
else
    exit $status
fi

if [ $status != 0 ]; then
    echofail "R:$systest:FAIL"
    # Do not clean up - we need the evidence.
else
    core_dumps="$(find $systest/ -name 'core*' -or -name '*.core' | sort | tr '\n' ' ')"
    assertion_failures=$(find $systest/ -name named.run | xargs grep "assertion failure" | wc -l)
    sanitizer_summaries=$(find $systest/ -name 'tsan.*' | wc -l)
    if [ -n "$core_dumps" ]; then
        echoinfo "I:$systest:Test claims success despite crashes: $core_dumps"
        echofail "R:$systest:FAIL"
        # Do not clean up - we need the evidence.
	find "$systest/" -name 'core*' -or -name '*.core' | while read -r coredump; do
		SYSTESTDIR="$systest"
		echoinfo "D:$systest:backtrace from $coredump start"
		binary=$(gdb --batch --core="$coredump" | sed -ne "s/Core was generated by \`//;s/ .*'.$//p;")
		"$TOP/libtool" --mode=execute gdb \
			       --batch \
			       --command=run.gdb \
			       --core="$coredump" \
			       -- \
			       "$binary"
		echoinfo "D:$systest:backtrace from $coredump end"
	done
    elif [ $assertion_failures -ne 0 ]; then
	SYSTESTDIR="$systest"
        echoinfo "I:$systest:Test claims success despite $assertion_failures assertion failure(s)"
	grep "SUMMARY: " $(find $systest/ -name 'tsan.*') | sort -u | cat_d
        echofail "R:$systest:FAIL"
        # Do not clean up - we need the evidence.
    elif [ $sanitizer_summaries -ne 0 ]; then
        echoinfo "I:$systest:Test claims success despite $sanitizer_summaries sanitizer reports(s)"
        echofail "R:$systest:FAIL"
    else
        echopass "R:$systest:PASS"
        if $clean
        then
            ( cd $systest && $SHELL clean.sh "$@" )
            if test -d ../../../.git
            then
                git status -su --ignored $systest 2>/dev/null | \
                sed -n -e 's|^?? \(.*\)|I:file \1 not removed|p' \
                -e 's|^!! \(.*/named.run\)$|I:file \1 not removed|p' \
                -e 's|^!! \(.*/named.memstats\)$|I:file \1 not removed|p'
            fi
        fi
    fi
fi

echoend "E:$systest:$(date_with_args)"

exit $status
