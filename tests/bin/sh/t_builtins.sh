# $NetBSD: t_builtins.sh,v 1.6 2021/05/18 21:37:56 kre Exp $
#
# Copyright (c) 2018 The NetBSD Foundation, Inc.
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
# the implementation of "sh" to test
: ${TEST_SH:="/bin/sh"}

#
# This file tests the various sh builtin utilities.
#
# Those utilities that are really external programs, which are builtin in
# for (mostly) performance (printf, kill, test, ...), are tested elsewhere.
# We do test the builtin "echo" here as (in NetBSD) it is different than
# the external one.
#
# The (mostly special) builtins which appear to be more syntax than command
# are tested in other test programs, rather than here (break, continue...)
#
# And finally, those which are fundamental to the operation of the shell,
# like wait, set, shift, ... are also tested in other test programs where
# all their operations can be more thoroughly verified.
#
# This leaves those which need to be built in (cd, umask, ...) but whose
# purpose is mostly to alter the environment in which the shell operates
# of that of the commands it runs.   These tests act in co-operation with
# other tests exist here (where thy do) by not duplicating tests run
# elsewhere (ulimit is one example) but just adding to those.
# One day these might be unified.
#
# We do test both standard use of the builtins (where they are standard)
# and NetBSD sh extensions (when run on a shell with no support, such tests
# should be skipped.)
#

# Utility function able to test whether most of the builtins exist in
# the shell being tested.
have_builtin()
{
	${TEST_SH} -c "( $3 $1 $4 ) >/dev/null 2>&1" 	&&
	LC_ALL=C ${TEST_SH} -c \
	    'case "$( (type '"$1"') 2>&1)" in
		(*built*)	exit 0 ;;
		(*reserved*)	exit 0 ;;   # zsh!! (reserved words are builtin)
	     esac
	     exit 1'					||
	{
		test -z "$2" && atf_skip "${TEST_SH} has no '$1$5' built-in"
		return 1;
	}

	return 0
}

# And another to test if the shell being tested is the NetBSD shell,
# as we use these tests both to test standards conformance (correctness)
# which should be possible for all shells, and to test NetBSD
# extensions (which we mostly do by testing if the extension exists)
# and NetBSD sh behaviour for what is unspecified by the standard
# (so we will be notified via test failure should that unspecified
# behaviour alter) for which we have to discover if that shell is the
# one being tested.

is_netbsd_sh()
{
	unset NETBSD_SHELL 2>/dev/null
	test -n "$( ${TEST_SH} -c 'printf %s "${NETBSD_SHELL}"')"
}

### Helper functions

nl='
'
reset()
{
	TEST_NUM=0
	TEST_FAILURES=''
	TEST_FAIL_COUNT=0
	TEST_ID="$1"

	# These are used in check()
	atf_require_prog tr
	atf_require_prog printf
	atf_require_prog mktemp
}

# Test run & validate.
#
#	$1 is the command to run (via sh -c)
#	$2 is the expected output
#	$3 is the expected exit status from sh
#	$4 is optional extra data for the error msg (if there is one)
#
# Stderr is exxpected to be empty, unless the expected exit code ($3) is != 0
# in which case some message there is expected (and nothing is a failure).
# When non-zero exit is expected, we note a different (non-zero) value
# observed, but do not fail the test because of that.

check()
{
	fail=false
	TEMP_FILE=$( mktemp OUT.XXXXXX )
	TEST_NUM=$(( $TEST_NUM + 1 ))
	MSG=

	# our local shell (ATF_SHELL) better do quoting correctly...
	# some of the tests expect us to expand $nl internally...
	CMD="$1"

	# determine what the test generates, preserving trailing \n's
	result="$( ${TEST_SH} -c "${CMD}" 2>"${TEMP_FILE}" && printf X )"
	STATUS=$?
	result="${result%X}"


	if [ "${STATUS}" -ne "$3" ]; then
		MSG="${MSG}${MSG:+${nl}}[$TEST_NUM]"
		MSG="${MSG} expected exit code $3, got ${STATUS}"

		# don't actually fail just because of wrong exit code
		# unless we either expected, or received "good"
		# or something else is detected as incorrect as well.
		case "$3/${STATUS}" in
		(*/0|0/*) fail=true;;
		esac
	fi

	if [ "$3" -eq 0 ]; then
		if [ -s "${TEMP_FILE}" ]; then
			MSG="${MSG}${MSG:+${nl}}[$TEST_NUM]"
			MSG="${MSG} Messages produced on stderr unexpected..."
			MSG="${MSG}${nl}$( cat "${TEMP_FILE}" )"
			fail=true
		fi
	else
		if ! [ -s "${TEMP_FILE}" ]; then
			MSG="${MSG}${MSG:+${nl}}[$TEST_NUM]"
			MSG="${MSG} Expected messages on stderr,"
			MSG="${MSG} nothing produced"
			fail=true
		fi
	fi
	rm -f "${TEMP_FILE}"

	if [ "$2" != "${result}" ]
	then
		MSG="${MSG}${MSG:+${nl}}[$TEST_NUM]"
		MSG="${MSG} Expected: <<$2>>, received: <<$result>>"
		fail=true
	fi

	if $fail
	then
		if [ -n "$4" ]; then
			MSG="${MSG}${MSG:+${nl}}[$TEST_NUM] Note: ${4}"
		fi
		MSG="${MSG}${MSG:+${nl}}[$TEST_NUM]"
 		MSG="${MSG} Full command: <<${CMD}>>"
	fi

	$fail && test -n "$TEST_ID" && {
		TEST_FAILURES="${TEST_FAILURES}${TEST_FAILURES:+${nl}}"
		TEST_FAILURES="${TEST_FAILURES}${TEST_ID}[$TEST_NUM]:"
		TEST_FAILURES="${TEST_FAILURES} Test of <<$1>> failed.";
		TEST_FAILURES="${TEST_FAILURES}${nl}${MSG}"
		TEST_FAIL_COUNT=$(( $TEST_FAIL_COUNT + 1 ))
		return 0
	}
	$fail && atf_fail "Test[$TEST_NUM] failed: $(
	    # ATF does not like newlines in messages, so change them...
		    printf '%s' "${MSG}" | tr '\n' ';'
	    )"
	return 0
}

results()
{
	test -n "$1" && atf_expect_fail "$1"

	test -z "${TEST_ID}" && return 0
	test -z "${TEST_FAILURES}" && return 0

	echo >&2 "=========================================="
	echo >&2 "While testing '${TEST_ID}'"
	echo >&2 " - - - - - - - - - - - - - - - - -"
	echo >&2 "${TEST_FAILURES}"

	atf_fail \
 "Test ${TEST_ID}: $TEST_FAIL_COUNT (of $TEST_NUM) subtests failed - see stderr"
}

####### End helpers

atf_test_case colon
colon_head() {
	atf_set "descr" "Tests the shell special builtin ':' command"
}
colon_body() {
	have_builtin : || return 0

	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c ":"

	# ':' is a special builtin, so we should exit on redirect error
	# and variable assignments should persist (stupid, but it is the rule)

	atf_check -s not-exit:0 -e not-empty -o empty ${TEST_SH} -c \
		": >/foo/bar; printf %s No-exit-BUG"
	atf_check -s exit:0 -e empty -o inline:OK ${TEST_SH} -c \
		'X=BUG; X=OK : ; printf %s "${X}"'
}

atf_test_case echo
echo_head() {
	atf_set "descr" "Tests the shell builtin version of echo"
}
echo_body() {
	have_builtin echo || return 0

	if ! is_netbsd_sh; then
		atf_skip \
	   "${TEST_SH%% *} is not the NetBSD shell, this test is for it alone"
		return 0
	fi

	reset echo

	check 'echo "hello world"' "hello world${nl}" 0
	check 'echo hello world' "hello world${nl}" 0
	check 'echo -n "hello world"' "hello world" 0
	check 'IFS=:; echo hello world' "hello world${nl}" 0
	check 'IFS=; echo hello world' "hello world${nl}" 0

	check 'echo -e "hello world"' "hello world${nl}" 0
	check 'echo -e hello world' "hello world${nl}" 0
	check 'IFS=:; echo -e hello world' "hello world${nl}" 0

	# only one of the options is used
	check 'echo -e -n "hello world"' "-n hello world${nl}" 0
	check 'echo -n -e "hello world"' "-e hello world" 0
	# and only when it is alone
	check 'echo -en "hello world"' "-en hello world${nl}" 0
	check 'echo -ne "hello world"' "-ne hello world${nl}" 0

	# echo is specifically required to *not* support --
	check 'echo -- "hello world"' "-- hello world${nl}" 0

	# similarly any other unknown option is simply part of the output
	for OPT in a b c v E N Q V 0 1 2 @ , \? \[ \] \( \; . \* -help -version
	do
		check "echo '-${OPT}' foo" "-${OPT} foo${nl}" 0
	done

	# Now test the \\ expansions, with and without -e

	# We rely upon printf %b (tested elsewhere, not only a sh test)
	# to verify the output when the \\ is supposed to be expanded.

	for E in '' -e
	do
		for B in a b c e f n r t v \\ 04 010 012 0177
		do
			S="test string with \\${B} in it"
			if [ -z "${E}" ]; then
				R="${S}${nl}"
			else
				R="$(printf '%b\nX' "${S}")"
				R=${R%X}
			fi
			check "echo $E '${S}'" "${R}" 0
		done
	done

	check 'echo foo >&-' "" 1
	check 'echo foo >&- 2>&-; echo $?; echo $?' "1${nl}0${nl}" 0

	results
}

atf_test_case eval
eval_head() {
	atf_set "descr" "Tests the shell special builtin 'eval'"
}
eval_body() {
	have_builtin eval || return 0

	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c 'eval "exit 0"'
	atf_check -s exit:1 -e empty -o empty ${TEST_SH} -c 'eval "exit 1"'
	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c 'eval exit 0'
	atf_check -s exit:1 -e empty -o empty ${TEST_SH} -c 'eval exit 1'

	atf_check -s exit:0 -e empty -o inline:0 ${TEST_SH} -c \
		'eval true; printf %d $?'
	atf_check -s exit:0 -e empty -o inline:1 ${TEST_SH} -c \
		'eval false; printf %d $?'

	atf_check -s exit:0 -e empty -o inline:abc ${TEST_SH} -c \
		'X=a Y=b Z=c; for V in X Y Z; do eval "printf %s \$$V"; done'
	atf_check -s exit:0 -e empty -o inline:abc ${TEST_SH} -c \
		'X=a Y=b Z=c; for V in X Y Z; do eval printf %s \$$V; done'
	atf_check -s exit:0 -e empty -o inline:XYZ ${TEST_SH} -c \
		'for V in X Y Z; do eval "${V}=${V}"; done; printf %s "$X$Y$Z"'

	# make sure eval'd strings affect the shell environment

	atf_check -s exit:0 -e empty -o inline:/b/ ${TEST_SH} -c \
		'X=a; eval "X=b"; printf /%s/ "${X-unset}"'
	atf_check -s exit:0 -e empty -o inline:/b/ ${TEST_SH} -c \
		'X=a; Y=X; Z=b; eval "$Y=$Z"; printf /%s/ "${X-unset}"'
	atf_check -s exit:0 -e empty -o inline:/unset/ ${TEST_SH} -c \
		'X=a; eval "unset X"; printf /%s/ "${X-unset}"'
	atf_check -s exit:0 -e empty -o inline:// ${TEST_SH} -c \
		'unset X; eval "X="; printf /%s/ "${X-unset}"'
	atf_check -s exit:0 -e empty -o inline:'2 y Z ' ${TEST_SH} -c \
		'set -- X y Z; eval shift; printf "%s " "$#" "$@"'

	# ensure an error in an eval'd string causes the shell to exit
	# unless 'eval' is preceded by 'command' (in which case the
	# string is not eval'd but execution continues)

	atf_check -s not-exit:0 -e not-empty -o empty ${TEST_SH} -c \
		'eval "if done"; printf %s status=$?'

	atf_check -s exit:0 -e not-empty -o 'match:status=[1-9]' \
	    ${TEST_SH} -c \
		'command eval "if done"; printf %s status=$?'

	atf_check -s not-exit:0 -e not-empty \
	    -o 'match:status=[1-9]' -o 'not-match:[XY]' ${TEST_SH} -c \
		 'command eval "printf X; if done; printf Y"
		  S=$?; printf %s status=$S; exit $S'

	# whether 'X' is output here is (or should be) unspecified.
	atf_check -s not-exit:0 -e not-empty \
	    -o 'match:status=[1-9]' -o 'not-match:Y' ${TEST_SH} -c \
		 'command eval "printf X
		 		if done
				printf Y"
		  S=$?; printf %s status=$S; exit $S'

	if is_netbsd_sh
	then
		# but on NetBSD we expect that X to appear...
		atf_check -s not-exit:0 -e not-empty  -o 'match:X' \
		    -o 'match:status=[1-9]' -o 'not-match:Y' ${TEST_SH} -c \
			 'command eval "printf X
					if done
					printf Y"
			  S=$?; printf %s status=$S; exit $S'
	fi
}

atf_test_case exec
exec_head() {
	atf_set "descr" "Tests the shell special builtin 'exec'"
}
exec_body() {
	have_builtin exec || return 0

	atf_check -s exit:0 -e empty -o inline:OK ${TEST_SH} -c \
		'exec printf OK; printf BROKEN; exit 3'
	atf_check -s exit:3 -e empty -o inline:OKOK ${TEST_SH} -c \
		'(exec printf OK); printf OK; exit 3'
}

atf_test_case export
export_head() {
	atf_set "descr" "Tests the shell builtin 'export'"
}
export_body() {
	have_builtin export || return 0

	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c 'export VAR'
	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c 'export VAR=abc'
	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c 'export V A R'
	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c \
		'export V A=1 R=2'

	atf_require_prog printenv

	atf_check -s exit:1 -e empty -o empty ${TEST_SH} -c \
		'unset VAR || exit 7; export VAR; printenv VAR'
	atf_check -s exit:0 -e empty -o inline:\\n ${TEST_SH} -c \
		'unset VAR || exit 7; export VAR=; printenv VAR'
	atf_check -s exit:0 -e empty -o inline:\\n ${TEST_SH} -c \
		'unset VAR || exit 7; VAR=; export VAR; printenv VAR'
	atf_check -s exit:0 -e empty -o inline:\\n ${TEST_SH} -c \
		'unset VAR || exit 7; export VAR; VAR=; printenv VAR'
	atf_check -s exit:0 -e empty -o inline:XYZ\\n ${TEST_SH} -c \
		'unset VAR || exit 7; export VAR=XYZ; printenv VAR'
	atf_check -s exit:0 -e empty -o inline:ABC\\n ${TEST_SH} -c \
		'VAR=ABC; export VAR; printenv VAR'
	atf_check -s exit:0 -e empty -o inline:ABC\\n ${TEST_SH} -c \
		'unset VAR || exit 7; export VAR; VAR=ABC; printenv VAR'
	atf_check -s exit:0 -e empty -o inline:ABC\\nXYZ\\n ${TEST_SH} -c \
		'VAR=ABC; export VAR; printenv VAR; VAR=XYZ; printenv VAR'
	atf_check -s exit:0 -e empty -o inline:ABC\\nXYZ\\n ${TEST_SH} -c \
		'unset VAR || exit 7; export VAR;
		 VAR=ABC; printenv VAR; VAR=XYZ; printenv VAR'

	# don't check VAR=value, some shells provide meaningless quoting...
	atf_check -s exit:0 -e empty -o match:VAR= -o match:foobar \
		${TEST_SH} -c \
			'VAR=foobar ; export VAR ; export -p'
	atf_check -s exit:0 -e empty -o match:VAR= -o match:foobar \
		${TEST_SH} -c \
			'export VAR=foobar ; export -p'
	atf_check -s exit:0 -e empty -o match:VAR\$ ${TEST_SH} -c \
			'unset VAR ; export VAR ; export -p'
	atf_check -s exit:0 -e empty -o not-match:VAR ${TEST_SH} -c \
			'export VAR ; unset VAR ; export -p'
	atf_check -s exit:0 -e empty -o not-match:VAR -o not-match:foobar \
		${TEST_SH} -c \
			'VAR=foobar; export VAR ; unset VAR ; export -p'

	atf_check -s exit:0 -e empty -o match:VAR= -o match:FOUND=foobar \
		${TEST_SH} -c \
			'export VAR=foobar; V=$(export -p);
			 unset VAR; eval "$V"; export -p;
			 printf %s\\n FOUND=${VAR-unset}'
	atf_check -s exit:0 -e empty -o match:VAR -o match:FOUND=unset \
		${TEST_SH} -c \
			'export VAR; V=$(export -p);
			 unset VAR; eval "$V"; export -p;
			 printf %s\\n FOUND=${VAR-unset}'

	atf_check -s exit:1 -e empty -o inline:ABC\\nXYZ\\n ${TEST_SH} -c \
		'VAR=ABC; export VAR; printenv VAR; VAR=XYZ; printenv VAR;
		unset VAR; printenv VAR; VAR=PQR; printenv VAR'
	atf_check -s exit:0 -e empty -o inline:ABC\\nXYZ\\nVAR=unset\\nMNO\\n \
	    ${TEST_SH} -c \
		'VAR=ABC; export VAR; printenv VAR; VAR=XYZ; printenv VAR;
		 unset VAR; printf %s\\n "VAR=${VAR-unset}"; printenv VAR;
		 VAR=PQR; printenv VAR; VAR=MNO; export VAR; printenv VAR'
}

atf_test_case export_nbsd
export_nbsd_head() {
	atf_set "descr" "Tests NetBSD extensions to the shell builtin 'export'"
}
export_nbsd_body() {
	have_builtin "export" "" "" "-n foo" ' -n' || return 0

	atf_require_prog printenv

	atf_check -s exit:1 -e empty -o inline:ABC\\nXYZ\\n ${TEST_SH} -c \
		'VAR=ABC; export VAR; printenv VAR; VAR=XYZ; printenv VAR;
		export -n VAR; printenv VAR; VAR=PQR; printenv VAR'

	atf_check -s exit:0 -e empty -o inline:ABC\\nXYZ\\nVAR=XYZ\\nMNO\\n \
	    ${TEST_SH} -c \
		'VAR=ABC; export VAR; printenv VAR; VAR=XYZ; printenv VAR;
		 export -n VAR; printf %s\\n "VAR=${VAR-unset}"; printenv VAR;
		 VAR=PQR; printenv VAR; VAR=MNO; export VAR; printenv VAR'

	have_builtin "export" "" "" -x ' -x' || return 0

	atf_check -s exit:1 -e empty -o empty ${TEST_SH} -c \
		'export VAR=exported; export -x VAR; printenv VAR'
	atf_check -s exit:1 -e empty -o empty ${TEST_SH} -c \
		'export VAR=exported; export -x VAR; VAR=local; printenv VAR'
	atf_check -s exit:0 -e empty -o inline:once\\nx\\n ${TEST_SH} -c \
		'export VAR=exported
		 export -x VAR
		 VAR=once printenv VAR
		 printenv VAR || printf %s\\n x'

	atf_check -s not-exit:0 -e not-empty -o empty ${TEST_SH} -c \
		'export VAR=exported; export -x VAR; export VAR=FOO'

	have_builtin export '' 'export VAR;' '-q VAR' ' -q'  || return 0

	atf_check -s exit:1 -o empty -e empty ${TEST_SH} -c \
		'unset VAR; VAR=set; export -q VAR'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'export VAR=set; export -q VAR'
	atf_check -s exit:1 -o empty -e empty ${TEST_SH} -c \
		'VAR=set; RW=set; export -q VAR RW'
	atf_check -s exit:1 -o empty -e empty ${TEST_SH} -c \
		'VAR=set; export RO=set; export -q VAR RO'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'export VAR=set RO=set; export -q VAR RO'

	atf_check -s exit:1 -o empty -e empty ${TEST_SH} -c \
		'unset VAR; export -q VAR'
	# next one is the same as the have_builtin test, so "cannot" fail...
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'unset VAR; export VAR; export -q VAR'

	# if we have -q we should also have -p var...
	# What's more, we are testing NetBSD sh, so we know output format.

	atf_check -s exit:0 -e empty -o match:VAR=foobar \
		${TEST_SH} -c \
			'VAR=foobar ; export VAR ; export -p VAR'
	atf_check -s exit:0 -e empty -o inline:1 \
		${TEST_SH} -c \
			'VAR=foobar ; export VAR ;
			printf %d $(export -p VAR | wc -l)'
	atf_check -s exit:0 -e empty \
		-o inline:'export VAR=foobar\nexport OTHER\n' \
		${TEST_SH} -c \
			'VAR=foobar; export VAR OTHER; export -p VAR OTHER'
	atf_check -s exit:0 -e empty \
		-o inline:'export A=aaa\nexport B\nexport D='"''"'\n' \
		${TEST_SH} -c \
			'A=aaa D= C=foo; unset B; export A B D;
			 export -p A B C D'
}

atf_test_case getopts
getopts_head() {
	atf_set "descr" "Tests the shell builtin 'getopts'"
}
getopts_body() {
	have_builtin getopts "" "f() {" "a x; }; f -a" || return 0
}

atf_test_case jobs
jobs_head() {
	atf_set "descr" "Tests the shell builting 'jobs' command"
}
jobs_body() {
	have_builtin jobs || return 0

	atf_require_prog sleep

	# note that POSIX requires that we reference $! otherwise
	# the shell is not required to remember the process...

	atf_check -s exit:0 -e empty -o match:sleep -o match:Running \
		${TEST_SH} -c 'sleep 1 & P=$!; jobs; wait'
	atf_check -s exit:0 -e empty -o match:sleep -o match:Done \
		${TEST_SH} -c 'sleep 1 & P=$!; sleep 2; jobs; wait'
}

atf_test_case read
read_head() {
	atf_set "descr" "Tests the shell builtin read command"
}
read_body() {
	have_builtin read "" "echo x|" "var" || return 0
}

atf_test_case readonly
readonly_head() {
	atf_set "descr" "Tests the shell builtin 'readonly'"
}
readonly_body() {
	have_builtin readonly || return 0

	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c 'readonly VAR'
	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c 'readonly VAR=abc'
	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c 'readonly V A R'
	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c 'readonly V A=1 R=2'

	atf_check -s exit:0 -e empty -o inline:unset ${TEST_SH} -c \
		'unset VAR; readonly VAR; printf %s ${VAR-unset}'
	atf_check -s exit:0 -e empty -o inline:set ${TEST_SH} -c \
		'unset VAR; readonly VAR=set; printf %s ${VAR-unset}'
	atf_check -s exit:0 -e empty -o inline:set ${TEST_SH} -c \
		'VAR=initial; readonly VAR=set; printf %s ${VAR-unset}'
	atf_check -s not-exit:0 -e not-empty -o empty ${TEST_SH} -c \
		'readonly VAR=initial; VAR=new; printf %s "${VAR}"'

	# don't check VAR=value, some shells provide meaningless quoting...
	atf_check -s exit:0 -e empty -o match:VAR= -o match:foobar \
		${TEST_SH} -c \
			'VAR=foobar ; readonly VAR ; readonly -p'
	atf_check -s exit:0 -e empty -o match:VAR= -o match:foobar \
		${TEST_SH} -c \
			'readonly VAR=foobar ; readonly -p'
	atf_check -s exit:0 -e empty -o match:VAR= -o match:foobar \
		-o not-match:badvalue ${TEST_SH} -c \
			'VAR=badvalue; readonly VAR=foobar ; readonly -p'
	atf_check -s exit:0 -e empty -o match:VAR\$ ${TEST_SH} -c \
			'unset VAR ; readonly VAR ; readonly -p'

	# checking that readonly -p works (to reset stuff) is a pain...
	# particularly since not all shells say "readonly..." by default
	atf_check -s exit:0 -e empty -o match:MYVAR= -o match:FOUND=foobar \
		${TEST_SH} -c \
			'V=$(readonly MYVAR=foobar; readonly -p | grep " MYVAR")
			 unset MYVAR; eval "$V"; readonly -p;
			 printf %s\\n FOUND=${MYVAR-unset}'
	atf_check -s exit:0 -e empty -o match:MYVAR\$ -o match:FOUND=unset \
		${TEST_SH} -c \
			'V=$(readonly MYVAR; readonly -p | grep " MYVAR")
			 unset MYVAR; eval "$V"; readonly -p;
			 printf %s\\n "FOUND=${MYVAR-unset}"'
	atf_check -s exit:0 -e empty -o match:MYVAR= -o match:FOUND=empty \
		${TEST_SH} -c \
			'V=$(readonly MYVAR=; readonly -p | grep " MYVAR")
			 unset VAR; eval "$V"; readonly -p;
			 printf %s\\n "FOUND=${MYVAR-unset&}${MYVAR:-empty}"'

	# don't test stderr, some shells inist on generating a message for an
	# unset of a readonly var (rather than simply having unset make $?=1)

	atf_check -s not-exit:0 -e empty -o empty ${TEST_SH} -c \
		'unset VAR; readonly VAR=set;
		 unset VAR 2>/dev/null && printf %s ${VAR:-XX}'
	atf_check -s not-exit:0 -e ignore -o empty ${TEST_SH} -c \
		'unset VAR; readonly VAR=set; unset VAR && printf %s ${VAR:-XX}'
	atf_check -s exit:0 -e ignore -o inline:set ${TEST_SH} -c \
		'unset VAR; readonly VAR=set; unset VAR; printf %s ${VAR-unset}'
}

atf_test_case readonly_nbsd
readonly_nbsd_head() {
	atf_set "descr" "Tests NetBSD extensions to 'readonly'"
}
readonly_nbsd_body() {
	have_builtin readonly '' 'readonly VAR;' '-q VAR' ' -q'  || return 0

	atf_check -s exit:1 -o empty -e empty ${TEST_SH} -c \
		'VAR=set; readonly -q VAR'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'readonly VAR=set; readonly -q VAR'
	atf_check -s exit:1 -o empty -e empty ${TEST_SH} -c \
		'VAR=set; RW=set; readonly -q VAR RW'
	atf_check -s exit:1 -o empty -e empty ${TEST_SH} -c \
		'VAR=set; readonly RO=set; readonly -q VAR RO'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'readonly VAR=set RO=set; readonly -q VAR RO'

	atf_check -s exit:1 -o empty -e empty ${TEST_SH} -c \
		'unset VAR; readonly -q VAR'
	# next one is the same as the have_builtin test, so "cannot" fail...
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'unset VAR; readonly VAR; readonly -q VAR'

	# if we have -q we should also have -p var...
	# What's more, we are testing NetBSD sh, so we know output format.

	atf_check -s exit:0 -e empty -o match:VAR=foobar \
		${TEST_SH} -c \
			'VAR=foobar ; readonly VAR ; readonly -p VAR'
	atf_check -s exit:0 -e empty -o inline:1 \
		${TEST_SH} -c \
			'VAR=foobar ; readonly VAR ;
			printf %d $(readonly -p VAR | wc -l)'
	atf_check -s exit:0 -e empty \
		-o inline:'readonly VAR=foobar\nreadonly OTHER\n' \
		${TEST_SH} -c \
			'VAR=foobar; readonly VAR OTHER; readonly -p VAR OTHER'
	atf_check -s exit:0 -e empty \
		-o inline:'readonly A=aaa\nreadonly B\nreadonly D='"''"'\n' \
		${TEST_SH} -c \
			'A=aaa D= C=foo; unset B; readonly A B D;
			 readonly -p A B C D'
}

atf_test_case cd_pwd
cd_pwd_head() {
	atf_set "descr" "Tests the shell builtins 'cd' & 'pwd'"
}
cd_pwd_body() {
	have_builtin cd "" "HOME=/;" || return 0
	have_builtin pwd || return 0
}

atf_test_case true_false
true_false_head() {
	atf_set "descr" "Tests the 'true' and 'false' shell builtin commands"
}
true_false_body() {
	have_builtin true || return 0

	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c true

	# true is not a special builtin, so errors do not cause exit
	# but we should still get an error from the broken redirect
	# and the exit status of true should be false...

	atf_check -s exit:0 -e not-empty -o inline:OK ${TEST_SH} -c \
		"true >/foo/bar && printf %s NOT-; printf %s OK"

	# and var-assigns should not affect the current sh env

	atf_check -s exit:0 -e empty -o inline:IS-OK ${TEST_SH} -c \
		'X=OK; X=BROKEN true && printf %s IS-; printf %s "${X}"'

	have_builtin false "" ! || return 0

	atf_check -s exit:1 -e empty -o empty ${TEST_SH} -c false
}

atf_test_case type
type_head() {
	atf_set "descr" "Tests the sh builtin 'type' command"
}
type_body() {
	have_builtin type "" "" type || return 0
}

# This currently has its own t_ulimit - either merge that here,
# or delete this one and keep that...  ulimit -n is also tested in
# the t_redir tests, as that affects the shell's use of file descriptors
atf_test_case ulimit
ulimit_head() {
	atf_set "descr" "Tests the sh builtin 'ulimit'"
}
ulimit_body() {
	have_builtin ulimit || return 0
}

atf_test_case umask
umask_head() {
	atf_set "descr" "Tests the sh builtin 'umask'"
}
umask_body() {
	have_builtin umask || return 0

	atf_require_prog touch
	atf_require_prog stat
	atf_require_prog rm
	atf_require_prog chmod

	reset umask

	# 8 octal digits
	for M in 0 1 2 3 4 5 6 7
	do
	    # Test numbers start: 1 25 49 73 97 121 145 169

	    # 8 combinations of each to test (64 inner loops)
	    # 3 tests in each loop, hence 192 subtests in all

		# Test numbers from loop above, plus (below) and the next 2
		#+     1        4        7         10	     13
	    for T in "0${M}" "00${M}" "0${M}0" "0${M}00" "0${M}${M}" \
		"0${M}${M}0" "0${M}${M}${M}"  "0${M}0${M}"
		#+    16          19		  22
	    do
		# umask turns bits off, calculate which bits will be on...

		D=$(( 0777 & ~ T ))		# for directories
		F=$(( $D & ~ 0111 ))		# and files with no 'x' bits

		# Note: $(( )) always produces decimal, so we test that format
		# (see '%d' in printf of stat result)

		# need chmod or we might have no perm to rmdir TD
		{ chmod +rwx TF TFT TD; rm -fr TF TFT TD; } 2>/dev/null || :

		# check that the umask applies to files created by the shell
		check \
		  "umask $T; > TF; printf %d \$(stat -nf %#Lp TF)" \
				  "$F" 0 "$F is $(printf %#o $F)" # 1 4 7 10 ...

		# and to files created by commands that the shell runs
		check \
		  "umask $T; touch TFT; printf %d \$(stat -nf %#Lp TFT)" \
				  "$F" 0 "$F is $(printf %#o $F)" # 2 5 8 11 ...

		# and to directories created b ... (directories keep 'x')
		check \
		  "umask $T; mkdir TD; printf %d \$(stat -nf %#Lp TD)" \
				  "$D" 0 "$D is $(printf %#o $D)" # 3 6 9 12 ...
	    done
	done

	# Now add a few more tests with less regular u/g/m masks
	# In here, include tests where umask value has no leading '0'

	# 10 loops, the same 3 tests in each loop, 30 more subtests
	# from 193 .. 222

	#        193 196 199  202 205 208 211  214  217 220
	for T in 013 047 722 0772 027 123 421 0124 0513 067
	do
		D=$(( 0777 & ~ 0$T ))
		F=$(( $D & ~ 0111 ))

		{ chmod +rwx TF TFT TD; rm -fr TF TFT TD; } 2>/dev/null || :

		check \
		  "umask $T; > TF; printf %d \$(stat -nf %#Lp TF)" \
				  "$F" 0 "$F is $(printf %#o $F)"	# +0

		check \
		  "umask $T; touch TFT; printf %d \$(stat -nf %#Lp TFT)" \
				  "$F" 0 "$F is $(printf %#o $F)"	# +1

		check \
		  "umask $T; mkdir TD; printf %d \$(stat -nf %#Lp TD)" \
				  "$D" 0 "$D is $(printf %#o $D)"	# +2
	done

	results
}

atf_test_case unset
unset_head() {
	atf_set "descr" "Tests the sh builtin 'unset'"
}
unset_body() {
	have_builtin unset || return 0
}

atf_test_case hash
hash_head() {
	atf_set "descr" "Tests the sh builtin 'hash' (ash extension)"
}
hash_body() {
	have_builtin hash || return 0
}

atf_test_case jobid
jobid_head() {
	atf_set "descr" "Tests sh builtin 'jobid' (NetBSD extension)"
}
jobid_body() {

	# have_builtin jobid || return 0	No simple jobid command test
	$TEST_SH -c '(exit 0)& jobid $!' >/dev/null 2>&1  || {
		atf_skip "${TEST_SH} has no 'jobid' built-in"
		return 0
	}
}

atf_test_case let
let_head() {
	atf_set "descr" "Tests the sh builtin 'let' (common extension from ksh)"
}
let_body() {
	have_builtin let "" "" 1 || return 0
}

atf_test_case local
local_head() {
	atf_set "descr" "Tests the shell builtin 'local' (common extension)"
}
local_body() {
	have_builtin local "" "f () {" "X; }; f" || return 0
}

atf_test_case setvar
setvar_head() {
	atf_set "descr" "Tests the shell builtin 'setvar' (BSD extension)"
}
setvar_body() {
	have_builtin setvar || return 0

	atf_check -s exit:0 -e empty -o inline:foo ${TEST_SH} -c \
		'unset PQ && setvar PQ foo; printf %s "${PQ-not set}"'
	atf_check -s exit:0 -e empty -o inline:abcd ${TEST_SH} -c \
		'for x in a b c d; do setvar "$x" "$x"; done;
		 printf %s "$a$b$c$d"'
	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c \
		'a=1; b=2; c=3; d=4
		 for x in a b c d; do setvar "$x" ""; done;
		 printf %s "$a$b$c$d"'
}

atf_test_case fdflags
fdflags_head() {
	atf_set "descr" \
	   "Tests basic operation of sh builtin 'fdflags' (NetBSD extension)"
}
fdflags_body() {
	have_builtin fdflags || return 0
}

atf_test_case fdflags__s
fdflags__s_head() {
	atf_set "descr" "Checks setting/clearing flags on file descriptors"
}
fdflags__s_body() {
	have_builtin fdflags || return 0
}

atf_test_case fdflags__v
fdflags__v_head() {
	atf_set "descr" "Checks verbose operation of fdflags"
}
fdflags__v_body() {
	have_builtin fdflags || return 0
}

atf_test_case fdflags__v_s
fdflags__v_s_head() {
	atf_set "descr" "tests verbose operation of fdflags -s"
}
fdflags__v_s_body() {
	have_builtin fdflags || return 0
}

atf_test_case fdflags_multiple_fd
fdflags_multiple_fd_head() {
	atf_set "descr" "Checks operation of fdflags with more than one fd"
}
fdflags_multiple_fd_body() {
	have_builtin fdflags || return 0
}

atf_test_case fdflags_one_flag_at_a_time
fdflags_one_flag_at_a_time_head() {
	atf_set "descr" "Tests all possible fdflags flags, and combinations"
}
fdflags_one_flag_at_a_time_body() {
	have_builtin fdflags || return 0
}

atf_test_case fdflags_save_restore
fdflags_save_restore_head() {
	atf_set "descr" 'Verify that fd flags can be saved and restored'
}
fdflags_save_restore_body() {
	have_builtin fdflags || return 0
}

atf_test_case fdflags_names_abbreviated
fdflags_names_abbreviated_head() {
	atf_set "descr" 'Tests using abbreviated names for fdflags'
}
fdflags_names_abbreviated_body() {
	have_builtin fdflags || return 0
}

atf_test_case fdflags_xx_errors
fdflags_xx_errors_head() {
	atf_set "descr" 'Check various erroneous fdflags uses'
}
fdflags_xx_errors_body() {
	have_builtin fdflags || return 0
}


atf_init_test_cases() {

	# "standard" builtin commands in sh

	# no tests of the "very special" (almost syntax) builtins
	#  (break/continue/return) - they're tested enough elsewhere

	atf_add_test_case cd_pwd
	atf_add_test_case colon
	atf_add_test_case echo
	atf_add_test_case eval
	atf_add_test_case exec
	atf_add_test_case export
	atf_add_test_case getopts
	atf_add_test_case jobs
	atf_add_test_case read
	atf_add_test_case readonly
	atf_add_test_case true_false
	atf_add_test_case type
	atf_add_test_case ulimit
	atf_add_test_case umask
	atf_add_test_case unset

	# exit/wait/set/shift/trap/alias/unalias/. should have their own tests
	# fc/times/fg/bg/%    are too messy to contemplate for now
	# command ??  (probably should have some tests)

	# Note that builtin versions of, printf, kill, ... are tested separately
	# (these are all "optional" builtins)
	# (echo is tested here because NetBSD sh builtin echo and /bin/echo
	#  are different)

	atf_add_test_case export_nbsd
	atf_add_test_case hash
	atf_add_test_case jobid
	atf_add_test_case let
	atf_add_test_case local
	atf_add_test_case readonly_nbsd
	atf_add_test_case setvar
	# inputrc should probably be tested in libedit tests (somehow)

	# fdflags has a bunch of test cases

	# Always run one test, so we get at least "skipped" result
	atf_add_test_case fdflags

	# but no need to say "skipped" lots more times...
	have_builtin fdflags available && {
		atf_add_test_case fdflags__s
		atf_add_test_case fdflags__v
		atf_add_test_case fdflags__v_s
		atf_add_test_case fdflags_multiple_fd
		atf_add_test_case fdflags_names_abbreviated
		atf_add_test_case fdflags_one_flag_at_a_time
		atf_add_test_case fdflags_save_restore
		atf_add_test_case fdflags_xx_errors
	}
	return 0
}
