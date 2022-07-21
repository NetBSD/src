# $NetBSD: t_realpath.sh,v 1.1 2022/07/21 09:52:49 kre Exp $
#
# Copyright (c) 2022 The NetBSD Foundation, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# ===========================================================
#
# Test data and expected results

# Note that the empty line calls realpath with no file arg
existing='.

../Dir/StdOut
./S1
./S1/../S4
./Snr/../S4
S1/S2/File
S1/S3/Link
Snr/HoHo
Snr/Link
L
/
/bin
Self
Self/
S4/S1/'

exist_results='Dir
Dir
Dir/StdOut
Dir/S1
Dir/S4
Dir/S4
Dir/S1/S2/File
Dir/S1/S2/File
Dir/Snr/HoHo
Dir/S1
Dir/StdOut
/
/bin
Dir
Dir
Dir/S1'

exist_root_only='Snx/HaHa
Snx/Link'

exist_root_results='Dir/Snx/HaHa
Dir/S1/S2/File'

nofile='-
trash
Snr/Haha
T1
T2
T3
T4
T5
../Dir/T2
../Dir/T3
/nonsense
/bin/../nonsense
./Self/Self/Self/Self/S1/../Self/../Dir/Self/T1
Self/nonsense'

nofile_results='Dir/-
Dir/trash
Dir/Snr/Haha
Dir/NoSuchFile
Dir/S1/NoSuchFile
Dir/S1/NoSuchFile
Dir/S1/S2/NoSuchFile
Dir/S1/S2/NoSuchFile
Dir/S1/NoSuchFile
Dir/S1/NoSuchFile
/nonsense
/nonsense
Dir/NoSuchFile
Dir/nonsense'

always_fail='StdOut/
StdOut/../StdErr
Loop
S1/S5/Link
Loop/../StdOut
BigLoop
U1
U2
U3
U4
U5
U6
U7
U8
U9
T1/NoSuchFile
T1/../NoSuchFile
U9/../NoSuchFile
U9/../StdOut'


# ===========================================================
# Helper functions
#

# Create the test environment
setup()
{
	atf_require_prog /usr/bin/mktemp
	atf_require_prog /bin/ln
	atf_require_prog /bin/cp
	atf_require_prog /bin/mkdir
	atf_require_prog /bin/chmod

	DIR=${PWD}/$(mktemp -d Dir.XXXXX) ||
		atf_fail "Did not make test directory"
	cd "${DIR}" || atf_fail "Unable to cd $DIR"

	ID=$( set -- $( type id ) && test "$1" = id && test "$2" = is &&
		test $# -eq 3 && printf %s "$3"  || printf no-id-program)

	mkdir Dir && cd Dir			|| atf_fail "enter Dir"

	>StdOut					|| atf_fail "setup StdOut"
	>StdErr					|| atf_fail "setup StdErr"
	ln -s ../Dir Dir			|| atf_fail "setup Dir"
	ln -s Loop Loop				|| atf_fail "setup Loop"
	ln -s . Self				|| atf_fail "setup Self"
	mkdir S1 S1/S2 S1/S3 S4 S4/S5		|| atf_fail "setup subdirs"
	echo S1/S2/File > S1/S2/File		|| atf_fail "setup File"
	ln -s ../S2/File S1/S3/Link		|| atf_fail "setup S3/Link"
	ln -s ../S1 S4/S1			|| atf_fail "setup S4/S1"
	ln -s StdOut L1				|| atf_fail "setup L1"
	ln -s L1 L2				|| atf_fail "setup L2"
	ln -s ../L2 S1/L3			|| atf_fail "setup L3"
	ln -s ../L3 S1/S2/L4			|| atf_fail "setup L4"
	ln -s ../S2/L4 S1/S3/L5			|| atf_fail "setup L5"
	ln -s S1/S3/L5 L			|| atf_fail "setup L"
	ln -s ${PWD}/S1 S4/PWDS1		|| atf_fail "setup PWDS1"
	ln -s ${PWD}/S9 S4/PWDS9		|| atf_fail "setup PWDS9"
	ln -s ${PWD}/S9/File S4/PWDS9F		|| atf_fail "setup PWDS9F"
	ln -s ../S4/BigLoop S1/BigLoop		|| atf_fail "setup S1/BigLoop"
	ln -s ../BigLoop S4/BigLoop		|| atf_fail "setup S4/BigLoop"
	ln -s "${DIR}"/Dir/S1/BigLoop BigLoop	|| atf_fail "setup BigLoop"
	mkdir Snx				|| atf_fail "setup Snx"
	cp /dev/null Snx/HaHa			|| atf_fail "setup Snx/HaHa"
	ln -s "${DIR}"/Dir/S1/S2/File Snx/Link	|| atf_fail "setup Snx/Link"
	mkdir Snr				|| atf_fail "setup Snr"
	cp /dev/null Snr/HoHo			|| atf_fail "setup Snr/HoHo"
	ln -s "${DIR}"/Dir/S4/PWDS1 Snr/Link	|| atf_fail "setup Snr/Link"
	ln -s ../Snx/HaHa Snr/HaHa		|| atf_fail "setup HaHa"
	ln -s "${DIR}"/Dir/NoSuchFile T1	|| atf_fail "setup T1"
	ln -s "${DIR}"/Dir/S1/NoSuchFile T2	|| atf_fail "setup T2"
	ln -s S1/NoSuchFile T3			|| atf_fail "setup T3"
	ln -s "${DIR}"/Dir/S1/S2/NoSuchFile T4	|| atf_fail "setup T4"
	ln -s S1/S2/NoSuchFile T5		|| atf_fail "setup T5"
	ln -s "${DIR}"/Dir/StdOut/CannotExist T6 || atf_fail "setup T6"
	ln -s "${DIR}"/Dir/NoDir/WhoKnows U1	|| atf_fail "setup U1"
	ln -s "${DIR}"/Dir/S1/NoDir/WhoKnows U2	|| atf_fail "setup U2"
	ln -s "${DIR}"/Dir/S1/S2/NoDir/WhoKnows U3 || atf_fail "setup U3"
	ln -s "${DIR}"/Dir/S1/../NoDir/WhoKnows U4 || atf_fail "setup U4"
	ln -s "${DIR}"/Dir/NoDir/../StdOut U5	|| atf_fail "setup U5"
	ln -s NoDir/../StdOut U6		|| atf_fail "setup U6"
	ln -s S1/NoDir/../../StdOut U7		|| atf_fail "setup U7"
	ln -s "${DIR}"/Dir/Missing/NoDir/WhoKnows U8 || atf_fail "setup U8"
	ln -s "${DIR}"/Dir/Missing/NoDir/../../StdOut U9 || atf_fail "setup U9"
	chmod a+r,a-x Snx			|| atf_fail "setup a-x "
	chmod a+x,a-r Snr			|| atf_fail "setup a-r"
}

# ATF will remove all the files we made, just ensure perms are OK
cleanup()
{
	chmod -R u+rwx .
	return 0
}

run_tests_pass()
{
	opt=$1
	tests=$2
	results=$3

	FAILS=
	FAILURES=0
	T=0

	while [ "${#tests}" -gt 0 ]
	do
		FILE=${tests%%$'\n'*}
		EXP=${results%%$'\n'*}

		tests=${tests#"${FILE}"};	tests=${tests#$'\n'}
		results=${results#"${EXP}"};	results=${results#$'\n'}

		test -z "${EXP}" && atf_fail "Too few results (test botch)"

		T=$(( $T + 1 ))

		GOT=$(realpath $opt -- ${FILE:+"${FILE}"})
		STATUS=$?

		case "${GOT}" in
		'')	;;		# nothing printed, deal with that below

		/*)			# Full Path (what we want)
			# Remove the unpredictable ATF dir prefix (if present)
			GOT=${GOT#"${DIR}/"}
			# Now it might be a relative path, that's OK
			# at least it can be compared (its prefix is known)
			;;

		*)			# a relative path was printed
			FAILURES=$(($FAILURES + 1))
			FAILS=${FAILS:+"${FAILS}"$'\n'}"Path $T:"
			FAILS="${FAILS}${opt:+ $opt} '${FILE}'"
			FAILS="${FAILS}: output relative path '${GOT}'"
			FAILS="${FAILS}, and exit($STATUS)"
			continue
			;;
		esac


		if [ $STATUS -ne 0 ] || [ "${EXP}" != "${GOT}" ]
		then
			FAILURES=$(($FAILURES + 1))
			if [ $STATUS -ne 0 ]
			then
			    FAILS=${FAILS:+"${FAILS}"$'\n'}"Path $T:"
			    FAILS="${FAILS}${opt:+ $opt} '${FILE}'"
			    FAILS="${FAILS} failed: status ${STATUS}"
			else
			    FAILS=${FAILS:+"${FAILS}"$'\n'}"Path $T:"
			    FAILS="${FAILS}${opt:+ $opt} '${FILE}'"
			    FAILS="${FAILS} expected '${EXP}' received '${GOT}'"
			fi
		fi
	done

	if test  -n "${results}"
	then
		FAILURES=$(( $FAILURES + 1 ))

		N=$(( $(printf '%s\n' "${results}" | wc -l) ))
		s=s; if [ $N -eq 1 ]; then s=; fi
		FAILS=${FAILS:+"${FAILS}"$'\n'}"After $T tests"
		FAILS="still $N more result$s (test botch)"
	fi

	if [ $FAILURES -gt 0 ]
	then
		s=s
		if [ $FAILURES -eq 1 ]; then s=; fi
		printf >&2 '%d path%s resolved incorrectly:\n%s\n' \
			"$FAILURES" "$s" "${FAILS}"
		atf_fail "$FAILURES path$s resolved incorrectly; see stderr"
	fi
	return 0
}

run_tests_fail()
{
	opt=$1
	tests=$2

	FAILS=
	FAILURES=0
	T=0

	while [ "${#tests}" -gt 0 ]
	do
		FILE=${tests%%$'\n'*}

		tests=${tests#"${FILE}"};	tests=${tests#$'\n'}

		test -z "${FILE}" && continue

		T=$(( $T + 1 ))

		GOT=$(realpath $opt -- "${FILE}" 2>StdErr)
		STATUS=$?

		ERR=$(cat StdErr)

		if [ $STATUS -eq 0 ] || [ "${GOT}" ] || ! [ "${ERR}" ]
		then
			FAILURES=$(($FAILURES + 1))
			if [ "${STATUS}" -eq 0 ]
			then
				FAILS=${FAILS:+"${FAILS}"$'\n'}"Path $T: "
				FAILS="${FAILS}${opt:+ $opt} '${FILE}' worked;"
				FAILS="${FAILS} received: '${GOT}'}"

				if [ "${ERR}" ]; then
					FAILS="${FAILS} and on stderr '${ERR}'"
				fi
			elif [ "${GOT}" ]
			then
				FAILS=${FAILS:+"${FAILS}"$'\n'}"Path $T:"
				FAILS="${FAILS}${opt:+ $opt} '${FILE}' failed,"
				FAILS="${FAILS} but with '${GOT}' on stdout"

				if [ "${ERR}" ]; then
					FAILS="${FAILS}, and on stderr '${ERR}'"
				else
					FAILS="${FAILS}, and empty stderr"
				fi
			else
				FAILS=${FAILS:+"${FAILS}"$'\n'}"Path $T:"
				FAILS="${FAILS}${opt:+ $opt} '${FILE}' failed,"
				FAILS="${FAILS} but with no error message"
			fi
		fi
	done
	if [ $FAILURES -gt 0 ]
	then
		S=s
		if [ $FAILURES -eq 1 ]; then s=; fi
		printf >&2 '%d path%s resolved incorrectly:\n%s\n' \
			"$FAILURES" "$s" "${FAILS}"
		atf_fail "$FAILURES path$s resolved incorrectly; see stderr"
	fi
	return 0
}

# ===================================================================
# Test cases setup follows (but almost all the work is earlier)

atf_test_case a__e_ok cleanup
realpath_e_ok_head()
{
	atf_set descr "Test realpath (with -e) cases which should work"
}
a__e_ok_body()
{
	setup
	run_tests_pass -e "${existing}" "${exist_results}"

	if [ -x "${ID}" ] && [ "$("$ID" -u)" = 0 ]
	then
		run_tests_pass -e "${exist_root_only}" "${exist_root_results}"
	fi
}
a__e_ok_cleanup()
{
	cleanup
}

atf_test_case b__E_ok cleanup
b__E_ok_head()
{
	atf_set descr "Test realpath (with -E) cases which should work"
}
b__E_ok_body() {
	setup
	# everything which works with -e should also work with -E
	run_tests_pass -E "${existing}" "${exist_results}"
	run_tests_pass -E "${nofile}" "${nofile_results}"

	if [ -x "${ID}" ] && [ "$("${ID}" -u)" = 0 ]
	then
		run_tests_pass -E "${exist_root_only}" "${exist_root_results}"
	fi
}
b__E_ok_cleanup()
{
	cleanup
}

atf_test_case c__ok cleanup
c__ok_head()
{
	atf_set descr "Test realpath (without -e or -E) cases which should work"
}
c__ok_body() {
	setup
	# Our default for realpath is -E, so the -E tests should work
	run_tests_pass '' "${existing}" "${exist_results}"
	# but more should work as well
	run_tests_pass '' "${nofile}" "${nofile_results}"

	if [ -x "${ID}" ] && [ "$("${ID}" -u)" = 0 ]
	then
		run_tests_pass '' "${exist_root_only}" "${exist_root_results}"
	fi
}
c__ok_cleanup()
{
	cleanup
}

atf_test_case d__E_fail
d__E_fail_head()
{
	atf_set descr "Test realpath -e cases which should not work"
}
d__E_fail_body()
{
	setup
	run_tests_fail -E "${always_fail}"
	if [ -x "${ID}" ] && [ "$("${ID}" -u)" != 0 ]
	then
		run_tests_fail -E "${exist_root_only}"
	fi
}
d__E_fail_cleanup()
{
	cleanup
}

atf_test_case e__e_fail
e__e_fail_head()
{
	atf_set descr "Test realpath -e cases which should not work"
}
e__e_fail_body()
{
	setup
	# Some -E tests that work should fail with -e
	run_tests_fail -e "${nofile}"
	run_tests_fail -e "${always_fail}"
	if [ -x "${ID}" ] && [ "$("${ID}" -u)" != 0 ]
	then
		run_tests_fail -e "${exist_root_only}"
	fi
}
e__e_fail_cleanup()
{
	cleanup
}

atf_test_case f__fail
f__fail_head()
{
	atf_set descr "Test realpath cases which should not work (w/o -[eE])"
}
f__fail_body()
{
	setup
	run_tests_fail '' "${always_fail}"
	if [ -x "${ID}" ] && [ "$("${ID}" -u)" != 0 ]
	then
		run_tests_fail '' "${exist_root_only}"
	fi
}
f__fail_cleanup()
{
	cleanup
}

atf_test_case g__q cleanup
g__q_head()
{
	atf_set descr "Test realpath's -q option; also test usage message"
}
g__q_body()
{
	setup

	# Just run these tests here, the paths have been tested
	# already, all we care about is that -q suppresses err messages
	# about the ones that fail, so just test those.  Since those
	# always fail, it is irrlevant which of -e or -E we would use,
	# so simply use neither.

	# This is adapted from run_tests_fail

	FAILURES=0
	FAILS=

	opt=-q

	T=0
	for FILE in ${always_fail}
	do

		test -z "${FILE}" && continue

		T=$(( $T + 1 ))

		GOT=$(realpath $opt -- "${FILE}" 2>StdErr)
		STATUS=$?

		ERR=$(cat StdErr)

		if [ $STATUS -eq 0 ] || [ "${GOT}" ] || [ "${ERR}" ]
		then
			FAILURES=$(($FAILURES + 1))
			if [ "${STATUS}" -eq 0 ]
			then
				FAILS=${FAILS:+"${FAILS}"$'\n'}"Path $T: "
				FAILS="${FAILS}${opt:+ $opt} '${FILE}' worked;"
				FAILS="${FAILS} received: '${GOT}'}"

				if [ "${ERR}" ]; then
					FAILS="${FAILS} and on stderr '${ERR}'"
				fi
			elif [ "${GOT}" ]
			then
				FAILS=${FAILS:+"${FAILS}"$'\n'}"Path $T:"
				FAILS="${FAILS}${opt:+ $opt} '${FILE}' failed,"
				FAILS="${FAILS} but with '${GOT}' on stdout"

				if [ "${ERR}" ]; then
					FAILS="${FAILS}, and on stderr '${ERR}'"
				else
					FAILS="${FAILS}, and empty stderr"
				fi
			else
				FAILS=${FAILS:+"${FAILS}"$'\n'}"Path $T:"
				FAILS="${FAILS}${opt:+ $opt} '${FILE}' failed,"
				FAILS="${FAILS} stderr: '${ERR}'"
			fi
		fi
	done

	# There are a couple of cases where -q does not suppress stderr

	for FILE in '' -wObBl@ --
	do

		T=$(( $T + 1 ))

		unset XTRA
		case "${FILE}" in
		'')	;;
		--)	XTRA=;;
		-*)	XTRA=/junk;;
		esac

		# Note lack of -- in the following, so $FILE can be either
		# a file name (well, kind of...), or options.

		GOT=$(realpath $opt "${FILE}" ${XTRA+"${XTRA}"} 2>StdErr)
		STATUS=$?

		ERR=$(cat StdErr)

		if [ $STATUS -eq 0 ] || [ "${GOT}" ] || ! [ "${ERR}" ]
		then
			FAILURES=$(($FAILURES + 1))
			if [ "${STATUS}" -eq 0 ]
			then
				FAILS=${FAILS:+"${FAILS}"$'\n'}"Path $T: "
				FAILS="${FAILS}${opt:+ $opt} ${FILE:-''}"
				FAILS="${FAILS}${XTRA:+ $XTRA} worked;"
				FAILS="${FAILS} received: '${GOT}'}"

				if [ "${ERR}" ]; then
					FAILS="${FAILS} and on stderr '${ERR}'"
				fi
			elif [ "${GOT}" ]
			then
				FAILS=${FAILS:+"${FAILS}"$'\n'}"Path $T:"
				FAILS="${FAILS}${opt:+ $opt} ${FILE:-''}"
				FAILS="${FAILS}${XTRA:+ ${XTRA}} failed,"
				FAILS="${FAILS} but with '${GOT}' on stdout"

				if [ "${ERR}" ]; then
					FAILS="${FAILS}, and on stderr '${ERR}'"
				else
					FAILS="${FAILS}, and empty stderr"
				fi
			else
				FAILS=${FAILS:+"${FAILS}"$'\n'}"Path $T:"
				FAILS="${FAILS}${opt:+ $opt} ${FILE:-''}"
				FAILS="${FAILS}${XTRA:+ ${XTRA}} failed,"
				FAILS="${FAILS} with stderr empty"
			fi
		fi
	done

	if [ $FAILURES -gt 0 ]
	then
		s=s
		if [ $FAILURES -eq 1 ]; then s=; fi
		printf >&2 '%d path%s resolved incorrectly:\n%s\n' \
			"$FAILURES" "$s" "${FAILS}"
		atf_fail "$FAILURES path$s resolved incorrectly; see stderr"
	fi
	return 0
}
g__q_cleanup()
{
	cleanup
}

atf_test_case h__n_args
h__n_args_head()
{
	atf_set descr "Test realpath with multiple file args"
}
h__n_args_body()
{
	setup

	# Since these paths have already (hopefully) tested and work
	# (if a__e_ok had any failures, fix those before even looking
	# at any failure here)

	# Since we are assuming that the test cases all work, simply
	# Count how many there are, and then expect the same number
	# of answers

	unset IFS
	set -- ${existing}
	# Note that any empty line (no args) case just vanished...
	# That would be meaningless here, removing it is correct.

 	GOT=$(realpath -e -- "$@" 2>StdErr)
	STATUS=$?

	ERR=$(cat StdErr; printf X)
	ERR=${ERR%X}

	NR=$(( $(printf '%s\n' "${GOT}" | wc -l) ))

	if [ $NR -ne $# ] || [ $STATUS -ne 0 ] || [ -s StdErr ]
	then
		printf >&2 'Stderr from test:\n%s\n' "${ERR}"
		if [ $STATUS -eq 0 ]; then S="OK"; else S="FAIL($STATUS)"; fi
		if [ ${#ERR} -ne 0 ]
		then
			E="${#ERR} bytes on stderr"
		else
			E="nothing on stderr"
		fi
		atf_fail 'Given %d args, got %d results; Status:%s; %s\n' \
			"$#" "${NR}" "${S}" "${E}"
	fi
	return 0
}
h__n_args_cleanup()
{
	cleanup
}

atf_init_test_cases()
{
	atf_add_test_case a__e_ok
	atf_add_test_case b__E_ok
	atf_add_test_case c__ok
	atf_add_test_case d__E_fail
	atf_add_test_case e__e_fail
	atf_add_test_case f__fail
	atf_add_test_case g__q
	atf_add_test_case h__n_args
}
