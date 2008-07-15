#!/bin/sh
# Derived from git's t/test-lib.sh, which is Copyright (c) 2005 Junio C Hamano
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# For repeatability, reset the environment to known value.
LANG=C
LC_ALL=C
TZ=UTC
export LANG LC_ALL TZ

. ./init.sh || { echo >&2 you must run make first; exit 1; }

# Protect ourselves from common misconfiguration to export
# CDPATH into the environment
unset CDPATH

# Each test should start with something like this, after copyright notices:
#
# test_description='Description of this test...
# This test checks if command xyzzy does the right thing...
# '
# . ./test-lib.sh

error () {
	echo "* error: $*"
	exit 1
}

say () {
	echo "* $*"
}

this_test_() { expr "./$0" : '.*/t-\([^/]*\)\.sh$'; }

test "${test_description}" != "" ||
error "Test script did not set test_description."

while test "$#" -ne 0
do
	case "$1" in
	-d|--d|--de|--deb|--debu|--debug)
		debug=t; shift ;;
	-i|--i|--im|--imm|--imme|--immed|--immedi|--immedia|--immediat|--immediate)
		immediate=t; shift ;;
	-h|--h|--he|--hel|--help)
		echo "$test_description"
		exit 0 ;;
	-v|--v|--ve|--ver|--verb|--verbo|--verbos|--verbose)
		verbose=t; shift ;;
	esac
done

exec 5>&1
if test "$verbose" = "t"
then
	exec 4>&2 3>&1
else
	exec 4>/dev/null 3>/dev/null
fi

test_failure=0
test_count=0

trap 'echo >&5 "FATAL: Unexpected exit with code $?"; exit 1' exit

# You are not expected to call test_ok_ and test_failure_ directly, use
# the text_expect_* functions instead.

test_ok_ () {
	test_count=$(expr "$test_count" + 1)
	say "  ok $test_count: $@"
}

test_failure_ () {
	test_count=$(expr "$test_count" + 1)
	test_failure=$(expr "$test_failure" + 1);
	say "FAIL $test_count: $1"
	shift
	echo "$@" | sed -e 's/^/	/'
	test "$immediate" = "" || exit 1
}

test_debug () {
	test "$debug" = "" || eval "$1"
}

test_run_ () {
	eval >&3 2>&4 "$1"
	eval_ret="$?"
	return 0
}

test_skip () {
	this_test=$(this_test_)
	this_test="$this_test.$(expr "$test_count" + 1)"
	to_skip=
	for skp in $SKIP_TESTS
	do
		case "$this_test" in
		$skp)
			to_skip=t
		esac
	done
	case "$to_skip" in
	t)
		say >&3 "skipping test: $@"
		test_count=$(expr "$test_count" + 1)
		say "skip $test_count: $1"
		: true
		;;
	*)
		false
		;;
	esac
}

test_expect_failure () {
	test "$#" = 2 ||
	error "bug in the test script: not 2 parameters to test-expect-failure"
	if ! test_skip "$@"
	then
		say >&3 "expecting failure: $2"
		test_run_ "$2"
		if [ "$?" = 0 -a "$eval_ret" != 0 -a "$eval_ret" -lt 129 ]
		then
			test_ok_ "$1"
		else
			test_failure_ "$@"
		fi
	fi
	echo >&3 ""
}

test_expect_success () {
	test "$#" = 2 ||
	error "bug in the test script: not 2 parameters to test-expect-success"
	if ! test_skip "$@"
	then
		say >&3 "expecting success: $2"
		test_run_ "$2"
		if [ "$?" = 0 -a "$eval_ret" = 0 ]
		then
			test_ok_ "$1"
		else
			test_failure_ "$@"
		fi
	fi
	echo >&3 ""
}

test_expect_code () {
	test "$#" = 3 ||
	error "bug in the test script: not 3 parameters to test-expect-code"
	if ! test_skip "$@"
	then
		say >&3 "expecting exit code $1: $3"
		test_run_ "$3"
		if [ "$?" = 0 -a "$eval_ret" = "$1" ]
		then
			test_ok_ "$2"
		else
			test_failure_ "$@"
		fi
	fi
	echo >&3 ""
}

test_done () {
	case "$test_failure" in
	0)
		# We could:
		# cd .. && rm -fr trash
		# but that means we forbid any tests that use their own
		# subdirectory from calling test_done without coming back
		# to where they started from.
		# The Makefile provided will clean this test area so
		# we will leave things as they are.

		say "passed all $test_count test(s)"
		exit 0 ;;

	*)
		say "failed $test_failure among $test_count test(s)"
		exit 1 ;;

	esac
}

this_test=$(this_test_)

skip_=0
# If $privileges_required_ is nonempty, non-root skips this test.
if test "$privileges_required_" != ''; then
    uid=`id -u` || error 'failed to run "id -u"'
    if test "$uid" != 0; then
	SKIP_TESTS="$SKIP_TESTS $this_test"
	say "you have insufficient privileges for test $this_test"
	skip_=1
    fi
fi

pwd_=`pwd`

test_dir_=${LVM_TEST_DIR-.}
test "$test_dir_" = . && test_dir_=$pwd_

# This is a stub function that is run upon trap (upon regular exit and
# interrupt).  Override it with a per-test function, e.g., to unmount
# a partition, or to undo any other global state changes.
cleanup_() { :; }

for skp in $SKIP_TESTS
do
	to_skip=
	for skp in $SKIP_TESTS
	do
		case "$this_test" in
		$skp)
			to_skip=t
		esac
	done
	case "$to_skip" in
	t)
		say >&3 "skipping test $this_test altogether"
		say "skip all tests in $this_test"
		trap - exit
		test_done
	esac
done

test_dir_rand_=$($abs_srcdir/mkdtemp $test_dir_ lvm-$this_test.XXXXXXXXXX) \
    || error "failed to create temporary directory in $test_dir_"

# Run each test from within a temporary sub-directory named after the
# test itself, and arrange to remove it upon exception or normal exit.
trap 'st=$?; cleanup_; d='"$test_dir_rand_"';
    cd '"$test_dir_"' && chmod -R u+rwx "$d" && rm -rf "$d" && exit $st' 0
trap '(exit $?); exit $?' 1 2 13 15

cd $test_dir_rand_ || error "failed to cd to $test_dir_rand_"

if test $skip_ = 0; then
  . $abs_srcdir/lvm-utils.sh || exit 1
fi

if ( diff --version < /dev/null 2>&1 | grep GNU ) 2>&1 > /dev/null; then
  compare='diff -u'
elif ( cmp --version < /dev/null 2>&1 | grep GNU ) 2>&1 > /dev/null; then
  compare='cmp -s'
else
  compare=cmp
fi
