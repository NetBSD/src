#!/bin/sh
#
# Copyright (C) 2004, 2007, 2010, 2012, 2014-2017  Internet Systems Consortium, Inc. ("ISC")
# Copyright (C) 2000, 2001  Internet Software Consortium.
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

#
# Run a system test.
#

SYSTEMTESTTOP=.
. $SYSTEMTESTTOP/conf.sh

stopservers=true
clean=true

case $1 in
   --keep|-k) stopservers=false; shift ;;
   --noclean|-n) clean=false; shift ;;
esac

test $# -gt 0 || { echo "usage: $0 [--keep|--noclean] test-directory" >&2; exit 1; }

test=$1
shift

test -d $test || { echo "$0: $test: no such test" >&2; exit 1; }

echoinfo "S:$test:`date`" >&2
echoinfo "T:$test:1:A" >&2
echoinfo "A:System test $test" >&2

if [ x${PERL:+set} = x ]
then
    echowarn "I:Perl not available.  Skipping test." >&2
    echowarn "R:UNTESTED" >&2
    echoinfo "E:$test:`date`" >&2
    exit 0;
fi

$PERL testsock.pl || {
    echowarn "I:Network interface aliases not set up.  Skipping test." >&2;
    echowarn "R:UNTESTED" >&2;
    echoinfo "E:$test:`date`" >&2;
    exit 0;
}


# Check for test-specific prerequisites.
test ! -f $test/prereq.sh || ( cd $test && $SHELL prereq.sh "$@" )
result=$?

if [ $result -eq 0 ]; then
    : prereqs ok
else
    echowarn "I:Prerequisites for $test missing, skipping test." >&2
    [ $result -eq 255 ] && echowarn "R:SKIPPED" || echowarn "R:UNTESTED"
    echoinfo "E:$test:`date`" >&2
    exit 0
fi

# Check for PKCS#11 support
if
    test ! -f $test/usepkcs11 || $SHELL cleanpkcs11.sh
then
    : pkcs11 ok
else
    echowarn "I:Need PKCS#11 for $test, skipping test." >&2
    echowarn "R:PKCS11ONLY" >&2
    echoinfo "E:$test:`date`" >&2
    exit 0
fi

# Set up any dynamically generated test data
if test -f $test/setup.sh
then
   ( cd $test && $SHELL setup.sh "$@" )
fi

# Start name servers running
$PERL start.pl $test || { echofail "R:FAIL"; echoinfo "E:$test:`date`"; exit 1; }

# Run the tests
( cd $test ; $SHELL tests.sh )

status=$?

if $stopservers
then
    :
else
    exit $status
fi

# Shutdown
$PERL stop.pl $test

status=`expr $status + $?`

if [ $status != 0 ]; then
    echofail "R:FAIL"
    # Don't clean up - we need the evidence.
    find . -name core -exec chmod 0644 '{}' \;
else
    echopass "R:PASS"

    if $clean
    then
	rm -f $SYSTEMTESTTOP/random.data
	if test -f $test/clean.sh
	then
	    ( cd $test && $SHELL clean.sh "$@" )
	fi
	if test -d ../../../.git
	then
	    git status -su --ignored $test |
	    sed -n -e 's|^?? \(.*\)|I:file \1 not removed|p' \
		-e 's|^!! \(.*/named.run\)$|I:file \1 not removed|p' \
		-e 's|^!! \(.*/named.memstats\)$|I:file \1 not removed|p'
	fi
    fi
fi

echoinfo "E:$test:`date`"

exit $status
