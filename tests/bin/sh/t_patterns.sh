# $NetBSD: t_patterns.sh,v 1.2.2.2 2018/07/28 04:38:12 pgoyette Exp $
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
: ${TEST_SH:=/bin/sh}

#
# This file tests pattern matching (glob)
#
# Three forms:
#	standard filename expansion (echo *.c)
#	case statements (case word in (*.c) ...;;)
#	var expansions with substring matching ${var%*.c}
#
# Note: the emphasis here is on testing the various possible patterns,
# not that case statements, or var expansions (etc) work in general.

### Helper functions

nl='
'
reset()
{
	TEST_NUM=0
	TEST_FAILURES=''
	TEST_FAIL_COUNT=0
	TEST_ID="$1"
}

# Test run & validate.
#
#	$1 is the command to run (via sh -c)
#	$2 is the expected output (with any \n's in output replaced by spaces)
#	$3 is the expected exit status from sh
#
# Stderr is exxpected to be empty, unless the expected exit code ($3) is != 0
# in which case some message there is expected (and nothing is a failure).
# When non-zero exit is expected, we note a different (non-zero) value
# observed, but do not fail the test because of that.

check()
{
	fail=false
	# Note TEMP_FILE must not be in the current directory (or nearby).
	TEMP_FILE=$( mktemp /tmp/OUT.XXXXXX )
	TEST_NUM=$(( $TEST_NUM + 1 ))
	MSG=

	# our local shell (ATF_SHELL) better do quoting correctly...
	# some of the tests expect us to expand $nl internally...
	CMD="$1"

	result="$( ${TEST_SH} -c "${CMD}" 2>"${TEMP_FILE}" )"
	STATUS=$?

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

	case "${result}" in
	(*[!0-9" 	$nl"]*)
		# A word of some kind: at least 1 char that is not digit or wsp
		# Remove newlines (use local shell for this)
		result="$(
			set -f
			IFS="$nl"
			set -- $result
			IFS=' '
			printf '%s' "$*"
		)"
		;;
	(*[0-9]*)
		# a numeric result, return just the number, trim whitespace
		result=$(( ${result} ))
		;;
	(*)
		# whitespace only, or empty string: just leave it as is
		;;
	esac

	if [ "$2" != "${result}" ]
	then
		MSG="${MSG}${MSG:+${nl}}[$TEST_NUM]"
		MSG="${MSG} Expected output '$2', received '$result'"
		fail=true
	fi

	if $fail
	then
		MSG="${MSG}${MSG:+${nl}}[$TEST_NUM]"
		MSG="${MSG} Full command: <<${CMD}>>"
	fi

	$fail && test -n "$TEST_ID" && {
		TEST_FAILURES="${TEST_FAILURES}${TEST_FAILURES:+${nl}}"
		TEST_FAILURES="${TEST_FAILURES}${TEST_ID}[$TEST_NUM]:"
		TEST_FAILURES="${TEST_FAILURES} Test of '$1' failed.";
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

atf_test_case filename_expansion
filename_expansion() {
	atf_set descr "Test correct operation of filename expansion"
}
filename_expansion_body() {
	atf_require_prog mktemp
	atf_require_prog wc
	atf_require_prog mv
	atf_require_prog rm
	atf_require_prog mkdir

	reset filename_expansion

	# First create a known set of filenames to match against

	# Note: This creates almost 17000 files/directories, so
	# needs at least that many free inodes (only space consumed
	# is for the directory contents, with a 1K frag size, it
	# should be about 1.2MiB).  Switching to making links would
	# save inodes, but would require running "ln" many times, so
	# would be a lot slower.

	# This should work on a case insensitive, but preserving,
	# filesystem - but case sensitive filesystems are preferred.

	D=$(mktemp -d "DIR.$$.XXXXXX") || atf_fail "mktemp -d failed"
	cd "${D}" || atf_fail "cd to temp dir '$D' failed"

	# we need another level of directory, so we know what
	# files to expect in ".." (ie: in $D) - only ".D".
	mkdir .D && cd .D || atf_fail "failed to make or enter .D in $D"

	> Xx || atf_fail "Unable to make files in temporary directory"
	case "$( printf '%s\n' *)" in
		(Xx) rm Xx || atf_fail "Unable to delete file";;
		(\*) atf_fail "Created file vanished";;
		(xx|XX|xX) atf_skip "Case preserving filesystem required";;
		(*) atf_fail "Unexpected file expansion for '*'";;
	esac

	# from here on we make files/directories that we will be
	# using in the tests.

	# CAUTION: Change *any* of this and the expected results from the
	# tests will all need verifying and updating as well.

	mkdir D || atf_fail "mkdir D failed"

	for F in a b c e V W X Y 1 2 3 4 5 \\ \] \[ \* \? \- \! \^ \| \# \' \"
	do
		> "${F}"
		> ".${F}"
		> "${F}.${F}"
		> "${F}-${F}"
		> "${F}${F}${F}${F}"
		> "x${F}z"
		> ab"${F}"yz
		> "m${F}n${F}p${F}q"

		> "D/${F}"
		> "D/.${F}"
		> "D/${F}${F}${F}${F}"

		mkdir "D${F}" || atf_fail "mkdir 'D${F}' failed"
		mkdir ".D${F}" || atf_fail "mkdir '.D${F}' failed"

		for G in a b c e W X Y 0 2 4 6 \\ \] \[ \* \? \- \! \^ \| \#
		do
			> "${F}${G}"
			> "${F}${G}${G}"
			> "${F}${G}${F}"
			> "${G}${F}${G}"
			> "${F}${G}${G}${F}"
			> "${G}${F}${F}${G}"
			> "${F}.${G}"
			> "${F}${G}.${G}${G}"
			> "${F}${G}${F}${G}.${G}"
			> "x${F}${G}y"
			> "${F}z${G}"
			> "${G}zz${F}"
			> "${G}+${G}"

			> "D${F}/${G}"
			> "D${F}/.${G}"
			> "D${F}/${G}${F}${G}"
			> "D${F}/.${G}${F}${G}"

			> ".D${F}/${G}"
			> ".D${F}/.${G}"
			> ".D${F}/${G}${F}${G}"
			> ".D${F}/.${G}${F}${G}"

			mkdir "D${F}/D${G}" "D${F}/D${F}${G}" ||
				atf_fail \
			    "subdir mkdirs failed D${F}/D${G} D${F}/D${F}${G}"

			> "D${F}/D${G}/${G}"
			> "D${F}/D${G}.${G}"
			> "D${F}/D${G}/${F}${G}"
			> "D${F}/D${G}/${G}${F}${G}"
			> "D${F}/D${G}/.${G}${F}${G}"
			> "D${F}/D${G}/.${G}${F}${G}"

			> "D${F}/D${F}${G}/${G}"
			> "D${F}/D${F}${G}.${G}"
			> "D${F}/D${F}${G}/${G}${F}"
			> "D${F}/D${F}${G}/${G}${G}${F}"
			> "D${F}/D${F}${G}/.${F}${F}${G}"
			> "D${F}/D${F}${G}/.${G}${F}${F}"

		done
	done

	# Debug hooks ... run with environment var set to filename

	case "${ATF_TEST_SAVE_FILENAMES}" in
	'')	;;
	/*)	ls -R >"${ATF_TEST_SAVE_FILENAMES}" ;;
	*)	ls -R >"${TMPDIR:-/tmp}/${ATF_TEST_SAVE_FILENAMES}" ;;
	esac
	case "${ATF_TEST_SAVE_FILES}" in
	'')	;;
	/*)	(cd ../..; tar cf "${ATF_TEST_SAVE_FILES}" D) ;;
	*)	(cd ../..; tar cf "${TMPDIR:-/tmp}/${ATF_TEST_SAVE_FILES}" D) ;;
	esac

	# Now we have lots of files, try some matching

	# First just check that "printf | wc -l" works properly...
	check 'printf "%s\n" 1 2 3 | wc -l'		'3'	0	#1

	# Next a whole bunch of elementary patterns
	check 'printf "%s\n" ab* | wc -l'		'31'	0
	check 'printf "%s\n" x*y | wc -l'		'525'	0
	check 'printf "%s\n" * | wc -l'			'5718'	0
	check 'printf "%s\n" ? | wc -l'			'26'	0	#5
	check 'printf "%s\n" ?? | wc -l'		'550'	0
	check 'printf "%s\n" ??? | wc -l'		'2297'	0
	check 'printf "%s\n" ???? | wc -l'		'1745'	0
	check 'printf "%s\n" ????? | wc -l'		'550'	0

	check 'printf "%s\n" ?????? | wc -l'		'525'	0	#10
	check 'printf "%s\n" ??????? | wc -l'		'25'	0
	check 'printf "%s\n" ???????? | wc -l'		'1'	0
	check 'printf "%s\n" ????????'			'????????'	0
	check 'printf "%s\n" m* | wc -l'		'25'	0
	check 'printf "%s\n" -* | wc -l'		'206'	0	#15
	check 'printf "%s\n" *- | wc -l'		'227'	0
	check 'printf "%s\n" -? | wc -l'		'21'	0
	check 'printf "%s\n" ?- | wc -l'		'26'	0
	check 'printf "%s\n" [ab] | wc -l'		'2'	0

	check 'printf "%s\n" [ab]* | wc -l'		'437'	0	#20
	check 'printf "%s\n" [A-Z]* | wc -l'		'815'	0
	check 'printf "%s\n" [0-4]* | wc -l'		'830'	0
	check 'printf "%s\n" [-04]* | wc -l'		'488'	0
	check 'printf "%s\n" [40-]* | wc -l'		'488'	0
	check 'printf "%s\n" *[0-9] | wc -l'		'1057'	0	#25
	check 'printf "%s\n" *[0-9]* | wc -l'		'2109'	0
	check 'printf "%s\n" ?[0-9]* | wc -l'		'855'	0
	check 'printf "%s\n" ?[0-9]? | wc -l'		'270'	0
	check 'printf "%s\n" *[0-9]? | wc -l'		'750'	0

	check 'printf "%s\n" [a-c][0-9]? | wc -l'	'33'	0	#30
	check 'printf "%s\n" [[:alpha:]] | wc -l'	'9'	0
	check 'printf "%s\n" [[:alpha:][:digit:]] | wc -l' '14'	0
	check 'printf "%s\n" [[:alpha:]][[:digit:]] | wc -l' '37' 0
	check								    \
	   'printf "%s\n" [[:alpha:][:digit:]][[:alpha:][:digit:]] | wc -l' \
							'156'	0
	check 'printf "%s\n" D*/*a | wc -l'		'152'	0	#35
	check 'printf "%s\n" D?/*a | wc -l'		'150'	0
	check 'printf "%s\n" D*/?a | wc -l'		'25'	0
	check 'printf "%s\n" D?/?a | wc -l'		'25'	0
	check 'printf "%s\n" */*a | wc -l'		'152'	0

	check 'printf "%s\n" [A-Z]*/*a | wc -l'		'152'	0	#40
	check 'printf "%s\n" ??/*a | wc -l'		'150'	0
	check 'printf "%s\n" .*/*a | wc -l'		'277'	0
	check 'printf "%s\n" .?*/*a | wc -l'		'50'	0
	check 'printf "%s\n" *-/-* | wc -l'		'2'	0
	check 'printf "%s\n" *-/-*'		'D-/- D-/---'	0	#45

	# now some literal magic chars
	check 'printf "%s\n" \?* | wc -l'		'206'	0
	check 'printf "%s\n" *\?* | wc -l'		'471'	0
	check 'printf "%s\n" \*? | wc -l'		'21'	0
	check 'printf "%s\n" \** | wc -l'		'206'	0

	check 'printf "%s\n" *\?* | wc -l'		'471'	0	#50
	check 'printf "%s\n" \[?] | wc -l'		'3'	0
	check 'printf "%s\n" \[?]'		'[.] []] [z]'	0
	check 'printf "%s\n" *\[* | wc -l'		'471'	0
	check 'printf "%s\n" \?\?* | wc -l'		'5'	0
	check 'printf "%s\n" \?\?*'	'?? ??.?? ??? ???? ????.?' 0	#55
	check 'printf "%s\n" [A\-C]* | wc -l'		'206'	0
	check 'printf "%s\n" [-AC]* | wc -l'		'206'	0
	check 'printf "%s\n" [CA-]* | wc -l'		'206'	0
	check 'printf "%s\n" [A\]-]? | wc -l'		'42'	0

	check 'printf "%s\n" []A\-]? | wc -l'		'42'	0	#60
	check 'printf "%s\n" []A-]? | wc -l'		'42'	0
	check 'printf "%s\n" \\* | wc -l'		'206'	0
	check 'printf "%s\n" [[-\]]?\?* | wc -l'	'12'	0
	check 'printf "%s\n" []\\[]?\? | wc -l'		'9'	0
	check 'printf "%s\n" *\\\\ | wc -l'		'52'	0	#65
	check 'printf "%s\n" [*][?]* | wc -l'		'6'	0
	check 'printf "%s\n" "*?"* | wc -l'		'6'	0
	check "printf '%s\\n' '\\'*\\\\ | wc -l"	'61'	0
	check 'printf "%s\n" ["a-b"]* | wc -l'		'643'	0

	check 'printf "%s\n" ["A-C"]z[[] | wc -l'	'1'	0	#70
	check 'printf "%s\n" ["A-C"]z[[]'		'-z['	0
	check 'printf "%s\n" ?"??"* | wc -l'		'54'	0
	check 'printf "%s\n" \??\?* | wc -l'		'52'	0
	check 'printf "%s\n" [?][\?]* | wc -l'		'5'	0
	check 'printf "%s\n" [?][\?]*'	'?? ??.?? ??? ???? ????.?' 0	#75
	check 'printf "%s\n" [!ab] | wc -l'		'24'	0
	check 'printf "%s\n" [!ab]* | wc -l'		'5281'	0
	check 'printf "%s\n" [!A-D]* | wc -l'		'5692'	0
	check 'printf "%s\n" [!0-3]* | wc -l'		'5094'	0

	check 'printf "%s\n" [!-03]* | wc -l'		'5265'	0	#80
	check 'printf "%s\n" [!30-]* | wc -l'		'5265'	0
	check 'printf "%s\n" [!0\-3]* | wc -l'		'5265'	0
	check 'printf "%s\n" [\!0-3]* | wc -l'		'830'	0
	check 'printf "%s\n" [0-3!]* | wc -l'		'830'	0
	check 'printf "%s\n" [0!-3]* | wc -l'		'1790'	0	#85
	check 'printf "%s\n" *[!0-3] | wc -l'		'5156'	0
	check 'printf "%s\n" *[!0-3]* | wc -l'		'5680'	0
	check 'printf "%s\n" ?[!0-3]* | wc -l'		'5231'	0
	check 'printf "%s\n" ?[!0-3]? | wc -l'		'2151'	0

	check 'printf "%s\n" *[!0-3]? | wc -l'		'5284'	0	#90
	check 'printf "%s\n" [!a-c][!0-3]? | wc -l'	'1899'	0
	check 'printf "%s\n" [![:alpha:]] | wc -l'	'17'	0
	check 'printf "%s\n" [![:alpha:][:digit:]] | wc -l' '12' 0
	check 'printf "%s\n" [![:alpha:]][[:digit:]] | wc -l'	'68' 0
	check 'printf "%s\n" [[:alpha:]][![:digit:]] | wc -l'	'156' 0	#95
	check 'printf "%s\n" [![:alpha:]][![:digit:]] | wc -l'	'289' 0
	check 'printf "%s\n" [!A-Z]*/*a | wc -l'	'1'	0
	check 'printf "%s\n" [!A-Z]*/*a'	'[!A-Z]*/*a'	0
	check 'printf "%s\n" [!A\-D]* | wc -l'		'5486'	0

	check 'printf "%s\n" [!-AD]* | wc -l'		'5486'	0	#100
	check 'printf "%s\n" [!DA-]* | wc -l'		'5486'	0
	check 'printf "%s\n" [!A\]-]? | wc -l'		'508'	0
	check 'printf "%s\n" [!]A\-]? | wc -l'		'508'	0
	check 'printf "%s\n" [!]A-]? | wc -l'		'508'	0
	check 'printf "%s\n" [![-\]]?\?* | wc -l'	'164'	0	#105
	check 'printf "%s\n" [!]\\[]?\? | wc -l'	'93'	0
	check 'printf "%s\n" [!*][?]* | wc -l'		'171'	0
	check 'printf "%s\n" [*][!?]* | wc -l'		'199'	0
	check 'printf "%s\n" [!*][!?]* | wc -l'		'5316'	0

	check 'printf "%s\n" [!"a-b"]* | wc -l'		'5075'	0	#110
	check 'printf "%s\n" ["!a-b"]* | wc -l'		'849'	0
	check 'printf "%s\n" [!"A-D"]z[[] | wc -l'	'24'	0
	check 'printf "%s\n" ["!A-D"]z[[] | wc -l'	'2'	0
	check 'printf "%s\n" ["!A-D"]z[[]'	'!z[ -z['	0
	check 'printf "%s\n" ["A-D"]z[![] | wc -l'	'20'	0	#115
	check 'printf "%s\n" [!"A-D"]z[![] | wc -l'	'480'	0
	check 'printf "%s\n" ["!A-D"]z[![] | wc -l'	'40'	0
	check 'printf "%s\n" [!?][\?]* | wc -l'		'172'	0
	check 'printf "%s\n" [?][!\?]* | wc -l'		'200'	0

	check 'printf "%s\n" [!?][!\?]* | wc -l'	'5315'	0	#120
	check 'printf "%s\n" [!?][?!]* | wc -l'		'343'	0
	check 'printf "%s\n" [?][\?!]* | wc -l'		'11'	0
	check "printf '%s\\n' [\']*[!#] | wc -l"	'164'	0
	check 'printf "%s\n" [\"]*[\|] | wc -l'		'6'	0
	check 'printf "%s\n" [\"]*[\|]' '".| "z| "| "|"|.| "|.|| "||' 0	#125
	check "printf '%s\\n' '\"['* | wc -l"		'6'	0
	check "printf '%s\\n' '\"['*" '"[ "[" "["[.[ "[.[[ "[[ "[["' 0

	# Now test cases where the pattern is the result of a
	# variable expansion (will assume, for now, that cmdsub & arith
	# work the same way, so omit tests using those)
	# we need to check both unquoted & quoted var expansions,
	# expansions that result from ${xxx-xxx} and ${xxx%yyy}
	# and expansions that form just part of the eventual pattern.

	check 'var="x*y";printf "%s\n" ${var} | wc -l'	'525'	0
	check 'var="[a-e]?[0-9]";printf "%s\n" ${var} | wc -l' '48' 0

	check 'var="[a-e]?.*";printf "%s\n" ${var} | wc -l' '84' 0	#130
	check 'var="[a-e]\?.*";printf "%s\n" ${var} | wc -l' '4' 0
	check 'var="[a-e]\?.*";printf "%s\n" ${var}' 'a?.?? b?.?? c?.?? e?.??' 0

	# and if you're looking for truly weird...

	check 'set -- a b; IFS=\?; printf "%s\n" "$*" | wc -l' '1' 0
	check 'set -- a b; IFS=\?; printf "%s\n" "$*"'	'a?b'	0
	check 'set -- a b; IFS=\?; printf "%s\n" $* | wc -l' '2' 0 #boring #135
	check 'set -- a b; IFS=\?; var=$*; unset IFS; printf "%s\n" ${var}' \
						'a.b abb azb'	0
	check 'set -- a b; IFS=\?; var=$*; unset IFS; printf "%s\n" "${var}"' \
						'a?b'	0
	check 'set -- a \?; IFS=\\; printf "%s\n" "$*"'	'a\?'	0
	check 'set -- a \?; IFS=\\; var=$*; unset IFS; printf "%s\n" "${var}"' \
						'a\?'	0

	check 'set -- a \?; IFS=\\; var=$*; unset IFS; printf "%s\n" ${var}' \
						'a?'	0		#140
	mv 'a?' 'a@'
	check 'set -- a \?; IFS=\\; var=$*; unset IFS; printf "%s\n" ${var}' \
						'a\?'	0
	mv 'a@' 'a?'

	# This is unspecified by POSIX, but everyone (sane) does it this way
	check 'printf "%s\n"  D*[/*] | wc -l'		'6'	0
	check 'printf "%s\n"  D*[\/*] | wc -l'		'6'	0
	check 'printf "%s\n"  D*\[/*] | wc -l'		'6'	0
	check 'printf "%s\n"  D*\[\/*] | wc -l'		'6'	0	#145
	check 'printf "%s\n"  D*[/*]' \
		'D[/D[] D[/D[].] D[/D] D[/D].] D[/] D[/][]'	0

	# '^' as the first char in a bracket expr is unspecified by POSIX,
	# but for compat with REs everyone (sane) makes it the same as !

	# But just in case we are testing an insane shell ...
    ${TEST_SH} -c 'case "^" in ([^V^]) exit 1;; (*) exit 0;; esac' && {

	check 'printf "%s\n" [^ab] | wc -l'		'24'	0
	check 'printf "%s\n" [^ab]* | wc -l'		'5281'	0
	check 'printf "%s\n" [^A-D]* | wc -l'		'5692'	0

	check 'printf "%s\n" [^0-3]* | wc -l'		'5094'	0	#150
	check 'printf "%s\n" [^-03]* | wc -l'		'5265'	0
	check 'printf "%s\n" [^0\-3]* | wc -l'		'5265'	0
	check 'printf "%s\n" [^-a3]* | wc -l'		'5110'	0
	check 'printf "%s\n" [\^-a3]* | wc -l'		'608'	0
	check 'printf "%s\n" [\^0-3]* | wc -l'		'830'	0	#155
	check 'printf "%s\n" [0-3^]* | wc -l'		'830'	0
	check 'printf "%s\n" [0^-a]* | wc -l'		'513'	0
	check 'printf "%s\n" *[^0-3] | wc -l'		'5156'	0
	check 'printf "%s\n" [!^]? | wc -l'		'529'	0

	check 'printf "%s\n" [^!]? | wc -l'		'529'	0	#160
	check 'printf "%s\n" [!!^]? | wc -l'		'508'	0
	check 'printf "%s\n" [!^!]? | wc -l'		'508'	0
	check 'printf "%s\n" [^!]? | wc -l'		'529'	0
	check 'printf "%s\n" [^!^]? | wc -l'		'508'	0
	check 'printf "%s\n" [^^!]? | wc -l'		'508'	0	#165
	check 'printf "%s\n" [!^-b]? | wc -l'		'487'	0
	check 'printf "%s\n" [^!-b]? | wc -l'		'63'	0

    }

	# No need to clean up the directory, we're in the ATF working
	# directory, and ATF cleans up for us.

	results
}

atf_test_case case_matching
case_matching_head() {
	atf_set descr "Test expansion of vars with embedded cmdsub"
}

# helper functions for case matching
#
# usage: cm word [ pattern ] [ preamble ]	(expect word to match pattern)
#        cf word [ pattern ] [ preamble ]	(expect word to fail to match)
#
# The last used (non-null) pattern, and the last used preamble, are
# remembered and used again if only the word is given.  To give a
# new preamble while using the last pattern, give '' as the pattern.
#
# nb: a null (empty) pattern is a syntax error, to get '' use "''"
#
cm() {
	case "$2" in
	'')	set -- "$1" "${LAST_PATTERN}" "${3:-${LAST_PFX}}";;
	*)	LAST_PATTERN="$2";;
	esac
	LAST_PFX="$3"

	check \
	    "${3:+${3}; }case $1 in ($2) printf M;; (*) printf X;; esac" M 0
}
cf() {
	case "$2" in
	'')	set -- "$1" "${LAST_PATTERN}" "${3:-${LAST_PFX}}";;
	*)	LAST_PATTERN="$2";;
	esac
	LAST_PFX="$3"

	check \
	    "${3:+${3}; }case $1 in ($2) printf M;; (*) printf X;; esac" X 0
}

case_matching_body() {

	# nb: we are not testing execution of case, so no ;& or alternate
	# patterns (etc) are needed here, we just want to validate the
	# case variant of pattern matching, so simple one word, one pattern
	# match or not match.

	reset case_matching

	cm abcd 'ab*'; cf bcda; cf aabce; cm ab				#  4
	cm abcd '$var' 'var="ab*"'; cf abcd '"$var"' 'var="ab*"'	#  6

	cm xy 'x*y'; cm xyxy; cm '"x*y"'; cf xxyz 			# 10

	cm '""' '*'; cm '\*'; cm '\?'; cm -; cm 12345			# 15
	cm abcd '$var' 'var="*"'; cf abcd '"$var"' 'var="*"'		# 17
	cm '"*"' '\*'; cm '"*"' '"*"'; cm '"*"' '"$var"' 'var="*"'	# 20

	cm X '?'; cf XX '?'; cf X '"?"'; cm Y '$var' 'var="?"'		# 24
	cf Z '"$var"' 'var="?"'; cm '"?"' '"$var"' 'var="?"'		# 26

	cm XA '??'; cf X '??'; cf XX '"??"'; cm YZ '$var' 'var="??"'	# 30
	cf ZZ '"$var"' 'var="??"'; cm '"??"' '"$var"' 'var="??"'	# 32

	cm a '[ab]'; cm b; cf c; cf aa; cf '"[ab]"'			# 37
	cm '"[ab]"' '"[ab]"'; cm '"[ab]"' '\[ab]'			# 39
	cm a '$var' 'var="[ab]"'; cf a '"$var"' 'var="[ab]"'		# 41
	cm '"[ab]"' '"$var"' 'var="[ab]"'; cm a '["$var"]' 'var=ab'	# 43

	cm b '[a-c]'; cm a '[a-c]'; cm c '[a-c]'; cf d '[a-c]'		# 47
	cf '"[a-c]"' '[a-c]'; cm '"[a-c]"' '"[a-c]"'			# 49
	cm '"[a-c]"' '\[a-c]'; cm '"[a-c]"' '[a-c\]'			# 51
	cm a '$var' 'var="[a-c]"'; cf a '"$var"' 'var="[a-c]"'		# 53
	cm '"[a-c]"' '"$var"' 'var="[a-c]"'; cf b '["$var"]' 'var=a-c'	# 55

	cm 2 '[0-4]'; cm 0 '[0-4]'; cf - '[0-4]'; cm 0 '[-04]'		# 59
	cf 2 '[-04]'; cf 2 '[40-]'; cm 0 '[40-]'; cm - '[-04]'		# 63
	cf 2 '[0\-4]'; cm - '[0\-4]'; cf 2 '["0-4"]'; cm - '["0-4"]'	# 67
	cf 2 "[0'-'4]"; cm - "[0'-'4]"; cm 4 "[0'-'4]"			# 70
	cm 0 "['0'-'4']"; cf '"\\"' '[0\-4]'; cm '"\\"' '[\\0-\\4]'	# 73

	cm a '[[:alpha:]]'; cf 0; cf '"["'; cm Z; cf aa; cf .; cf '""'	# 80
	cf a '[[:digit:]]'; cm 0; cf '"["'; cm 9; cf 10; cf .; cf '""'	# 87
	cm '"["' '[][:alpha:][]'; cf a '[\[:alpha:]]'; cf a '[[\:alpha:]]' #90
	cm a '[$var]' 'var="[:alpha:]"'; cm a '[[$var]]' 'var=":alpha:"' # 92
	cm a '[[:$var:]]' 'var=alpha'; cm B '[[:"$var":]]' 'var=alpha'	# 94
	cf B '["$var"]' 'var="[:alpha:]"'; cf B '[["$var"]]' 'var=":alpha:"' #96
	cm '"["' '["$var"]' 'var="[:alpha:]"'				# 97
	cm '"[]"' '[["$var"]]' 'var=":alpha:"'; 			# 98
	cm A3 '[[:alpha:]][[:digit:]]'; cf '"[["'			#100
	cm 3 '[[:alpha:][:digit:]]'; cf '"["'; cm A; cf '":"'		#104
	for W in AA A7 8x 77; do
		cm "$W" '[[:alpha:][:digit:]][[:alpha:][:digit:]]'	#108
	done

	cm dir/file '*/*'; cm /dir/file; cm /dir/file '*/file'		#111
	for W in aa/bcd /x/y/z .x/.abc --/--- '\\//\\//' '[]/[][]'
	do
		cm "'$W'" '??/*'; cm "'$W'" '[-a/.\\[]?/??*[]dzc/-]'
	done								#123

	cm '"?abc"' '\?*'; cf '"\\abc"'; cm '"?"'			#126

	cm '\\z' '"\\z"'; cf '\z'; cf z; cf '"\\"'			#130

	cm '"[x?abc"' '[[-\]]?\?*'; cm '"]x?abc"'; cm '"\\x?abc"'	#133
		cf '"-x?abc"'; cf '"[xyzabc"'; cm '"[]?"'		#136

	cm '"[x?"' '[]\\[]?\?'; cm '"]x?"'; cm '"\\y?"'; cm '"[]?"'	#140

	cm "'\z'" '"\z"'; cf z; cm '\\z'; cm '$var' '' 'var="\z"'	#144
	cm '${var}' '' "var='\z'"; cm '"${var}"'			#146
	cf '${var}' '${var}' "var='\z'"; cm '${var}' '"${var}"' "var='\z'" #148
	cf "'${var}'"; cm "'${var}'" "'${var}'" "var='\z'"		#150

	cf abc '"$*"' 'IFS=?; set -- a c';cf '"a c"';cm "'a?c'";cm '"$*"' #154
	cf abc '"$*"' 'IFS=*; set -- a c';cf '"a c"';cm "'a*c'";cm '"$*"' #158
	cf abc '"$*"' 'IFS=\\;set -- a c';cf '"a c"';cm "'a\c'";cm '"$*"' #162
	cf abc '"$*"' 'IFS="";set -- a c';cf '"a c"';cm "'ac'"; cm '"$*"' #166

	cm a '["$*"]' 'IFS=-; set -- a c';cf b;cm c;cm '-';   cf "']'"	#171
	cm a '["$*"]' 'IFS=?; set -- a c';cf b;cm c;cm '"?"'; cf "'['"	#176
	cm a '["$*"]' 'IFS=*; set -- a c';cf b;cm c;cm '"*"'; cf -	#181
	cm a '["$*"]' 'IFS=\\;set -- a c';cf b;cm c;cm "'\\'";cf "'$'"	#186
	cm a '["$*"]' 'IFS="";set -- a c';cf b;cm c			#189


	# Now repeat the ones using bracket expressions, adding !

	cf a '[!ab]'; cf b; cm c; cf aa; cf '"[!ab]"'; cm a '[ab!]'; cm ! #196
	cf a '$var' 'var="[!ab]"';cm x;cf a '"$var"' 'var="[!ab]"'; cf x  #200
	cm '"[!ab]"' '"$var"' 'var="[!ab]"'; cf a; cf b; cf !; cf "'['"	  #205
	cf a '[!"$var"]' 'var=ab'; cm x; cm a '["!$var"]' 'var=ab'	  #208
	cf x; cm !; cm a '["$var"]' 'var=!ab'; cf x			  #212
	cf a '[$var]' 'var=!ab'; cm !					  #214

	cf b '[!a-c]'; cf a; cf c; cm d; cm !; cm -; cm _; cm '\\'	#222
	cf a '$var' 'var="[!a-c]"'; cf b; cf c; cm d; cm !; cm -	#228

	cf 2 '[!0-4]'; cf 0; cm -; cf 4; cm !; cm "'['"; cm "']'"	#235
	cm 2 '[!-04]'; cm 2 '[!40-]'; cf 0; cf -; cm !;			#240
	cm 2 '[!0\-4]'; cf -; cm 2 '[!"0-4"]'; cf -			#244

	cf a '[![:alpha:]]'; cm 0; cm '"["'; cf aa; cm .; cf '""'	#250
	cf '"["' '[!][:alpha:][!]'; cf a; cm 0; cf !; cf "']'"; cm %	#256
	cf a '[$var]' 'var="![:alpha:]"'; cm 0; cm !; cm "']'"; cm @	#261

	results
}

atf_test_case var_substring_matching
var_substring_matching_head() {
	atf_set descr 'Test pattern matching in var expansions'
}

# Helper function for var substring matching
#	$1 is the input string
#	$2 the substring matching operator (# % ## or %%)
#	$3 is the pattern to match (or reference to one)
#	$4 is the expected output (result of applying op($2) with pat($3) to $1
#	$5 (if given, and not null) is a command (or commands) to run first
#	$6 (if given, and not null) cause the var expansion to be quoted
#		(ie "${var%pattern}" instead of just ${var%pattern})
#		any quotes needed in "pattern" should be in $3
# Note: a variable called "var" is used (set to $1, then expanded).
vm()
{
	check "${5:+${5}; }var='$1';printf '%s\n' ${6:+\"}\${var$2$3}${6:+\"}" \
		"$4" 0
}

var_substring_matching_body() {

	reset var_substring_matching

	vm abc \# a bc; vm aaab \# a aab; vm aaab \## 'a*a' b		#  3
	vm aaab % ab aa; vm xawab %% 'a*ab' x; vm abcd \# xyz abcd
	vm file.c % .c 'f le' IFS=i ; vm file.c % .c file IFS=i Q
	vm file.c % ?c file ; vm file.c % '"?c"' file.c			# 9 10

	vm abcabcabcded \# 'a*b' cabcabcded; vm abcabcabcded \## 'a*b' cded
	vm abcabcabcded % 'c*d' abcabcab; vm abcabcabcded %% 'c*d' ab

	vm abc.jpg % '.[a-z][!0-9]?' abc				# 15

	vm xxxyyy \# '${P}' yyy P=xxx; vm xxxyyy \# '${P}' yyy 'P=x?x'
	vm xxxyyy \# '${P}' yyy 'P=x?x' Q
	vm 'x?xyyy' \# '${P}' yyy 'P=x[?]x'
	vm xxxyyy \# '${P}' xxxyyy 'P=x[?]x'				# 20
	vm 'x?xyyy' \# '${P}' yyy 'P=x?x' Q
	vm xxxyyy \# '${P}' yyy 'P=x?x' Q
	vm 'x?xyyy' \# '${P}' yyy 'P="x\?x"'
	vm 'x?xyyy' \# '${P}' yyy 'P="x\?x"' Q
	vm 'x?xyyy' \# '${P}' yyy 'P="x?x"' 				# 25
	vm 'x?xyyy' \# '${P}' yyy 'P="x?x"' Q
	vm 'x?xyyy' \# '"${P}"' 'x?xyyy' 'P="x\?x"'
	vm 'x?xyyy' \# '"${P}"' 'x?xyyy' 'P="x\?x"' Q
	vm 'x?xyyy' \# '"${P}"' yyy 'P="x?x"'
	vm 'x?xyyy' \# '"${P}"' yyy 'P="x?x"' Q				# 30
	vm 'x%xyyy' \# '${P}' 'x%xyyy' 'P="x\?x"'
	vm 'x%xyyy' \# '${P}' 'x%xyyy' 'P="x\?x"' Q
	vm 'x%xyyy' \# '${P}' yyy 'P="x?x"'
	vm 'x%xyyy' \# '${P}' yyy 'P="x?x"' Q
	vm 'x%xyyy' \# '"${P}"' 'x%xyyy' 'P="x\?x"'			# 35
	vm 'x%xyyy' \# '"${P}"' 'x%xyyy' 'P="x\?x"' Q
	vm 'x%xyyy' \# '"${P}"' 'x%xyyy' 'P="x?x"'
	vm 'x%xyyy' \# '"${P}"' 'x%xyyy' 'P="x?x"' Q

	vm abc \# '*' abc; vm abc \# '*' abc '' Q			# 39 40
	vm abc \# '"*"' abc; vm abc \# '"*"' abc '' Q
	vm abc \# '"a"' bc; vm abc \# '"a"' bc '' Q
	vm abc \## '*' ''; vm abc \## '*' '' '' Q
	vm abc % '*' abc; vm abc % '*' abc '' Q
	vm abc %% '*' ''; vm abc %% '*' '' '' Q				# 49 50
	vm abc \# '$P' abc 'P="*"'; vm abc \# '$P' abc 'P="*"' Q
	vm abc \# '"$P"' abc 'P="*"'; vm abc \# '"$P"' abc 'P="*"' Q
	vm abc \# '$P' bc 'P="[a]"'; vm abc \# '$P' bc 'P="[a]"' Q
	vm abc \# '"$P"' abc 'P="[a]"'; vm abc \# '"$P"' abc 'P="[a]"' Q
	vm '[a]bc' \# '$P' '[a]bc' 'P="[a]"'
	vm '[a]bc' \# '"$P"' bc 'P="[a]"'				# 60
	vm '[a]bc' \# '"$P"' bc 'P="[a]"' Q

	# The following two (62 & 63) are actually the same test.
	# The double \\ turns into a single \ when parsed.
	vm '[a]bc' \# '$P' bc 'P="\[a]"';  vm '[a]bc' \# '$P' bc 'P="\\[a]"'
	vm '[a]bc' \# '"$P"' '[a]bc' 'P="\[a]"'
	vm '\[a]bc' \# '"$P"' bc 'P="\[a]"'				# 65

	vm ababcdabcd \#  '[ab]*[ab]' abcdabcd
	vm ababcdabcd \## '[ab]*[ab]' cd
	vm ababcdabcd \#  '$P'  abcdabcd  'P="[ab]*[ab]"'
	vm ababcdabcd \## '$P'  cd	    "P='[ab]*[ab]'"
	vm ababcdabcd \#  '$P' 'ab dab d' 'P="[ab]*[ab]"; IFS=c'	# 70
	vm ababcdabcd \#  '$P'  abcdabcd  'P="[ab]*[ab]"; IFS=c' Q

	vm ababcdabcd \#  '[ab]*[ba]'  abcdabcd
	vm ababcdabcd \#  '[ab]*[a-b]' abcdabcd
	vm ababcdabcd \## '[ba]*[ba]'  cd
	vm ababcdabcd \## '[a-b]*[ab]' cd				# 75

	vm abcde \# '?[b-d]?' de; vm abcde \## '?[b-d]?' de
	vm abcde % '?[b-d]?' ab; vm abcde %% '?[b-d]?' ab

	vm .123. \# '.[0-9][1-8]' 3.; vm .123. % '[0-9][1-8].' .1	# 80 81
	vm .123. \# '?[0-9][1-8]' 3.; vm .123. % '[0-9][1-8]?' .1
	vm .123. \# '*[0-9][1-8]' 3.; vm .123. % '[0-9][1-8]*' .1	# 85
	vm .123. \## '*[0-9][1-8]' .; vm .123. %% '[0-9][1-8]*' .
	vm .123. \# '[.][1][2]' 3.  ; vm .123. % '[2][3][.]' .1
	vm .123. \# '[?]1[2]' .123. ; vm .123. % '2[3][?]' .123.	# 90 91
	vm .123. \# '\.[0-9][1-8]' 3.;vm .123. % '[0-9][1-8]\.' .1

	vm '[a-c]d-f' \# '[a-c\]' d-f
	vm '[abcd]' \# '[[:alpha:]]' '[abcd]'				# 95
	vm '[1?234' \# '[[-\]]?\?' 234
	vm '1-2-3-\?' % '-${P}' '1-2-3-\?' 'P="\\?"'
	vm '1-2-3-\?' % '${P}' '1-2-3-\' 'P="\\?"'
	vm '1-2-3-\?' % '-"${P}"' 1-2-3 'P="\\?"'			# 99

	results
}


atf_init_test_cases() {
	# Listed here in the order ATF runs them, not the order from above

	atf_add_test_case filename_expansion
	atf_add_test_case case_matching
	atf_add_test_case var_substring_matching
}
