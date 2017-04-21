# $NetBSD: t_expand.sh,v 1.8.4.1 2017/04/21 16:54:09 bouyer Exp $
#
# Copyright (c) 2007, 2009 The NetBSD Foundation, Inc.
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
# This file tests the functions in expand.c.
#

delim_argv() {
	str=
	while [ $# -gt 0 ]; do
		if [ -z "${str}" ]; then
			str=">$1<"
		else
			str="${str} >$1<"
		fi
		shift
	done
	echo ${str}
}

atf_test_case dollar_at
dollar_at_head() {
	atf_set "descr" "Somewhere between 2.0.2 and 3.0 the expansion" \
	                "of the \$@ variable had been broken.  Check for" \
			"this behavior."
}
dollar_at_body() {
	# This one should work everywhere.
	atf_check -s exit:0 -o inline:' EOL\n' -e empty \
		${TEST_SH} -c 'echo "" "" | '" sed 's,\$,EOL,'"

	# This code triggered the bug.
	atf_check -s exit:0 -o inline:' EOL\n' -e empty \
		${TEST_SH} -c 'set -- "" ""; echo "$@" | '" sed 's,\$,EOL,'"

	atf_check -s exit:0 -o inline:'0\n' -e empty ${TEST_SH} -c \
		'set -- -; shift; n_arg() { echo $#; }; n_arg "$@"'
}

atf_test_case dollar_at_with_text
dollar_at_with_text_head() {
	atf_set "descr" "Test \$@ expansion when it is surrounded by text" \
	                "within the quotes.  PR bin/33956."
}
dollar_at_with_text_body() {

	cat <<'EOF' > h-f1

delim_argv() {
	str=
	while [ $# -gt 0 ]; do
		if [ -z "${str}" ]; then
			str=">$1<"
		else
			str="${str} >$1<"
		fi
		shift
	done
	echo "${str}"
}

EOF
	cat <<'EOF' > h-f2

delim_argv() {
	str=
	while [ $# -gt 0 ]; do

		str="${str}${str:+ }>$1<"
		shift

	done
	echo "${str}"
}

EOF

	chmod +x h-f1 h-f2

	for f in 1 2
	do
		atf_check -s exit:0 -o inline:'\n' -e empty ${TEST_SH} -c \
			". ./h-f${f}; "'set -- ; delim_argv "$@"'
		atf_check -s exit:0 -o inline:'>foobar<\n' -e empty \
			${TEST_SH} -c \
			". ./h-f${f}; "'set -- ; delim_argv "foo$@bar"'
		atf_check -s exit:0 -o inline:'>foo  bar<\n' -e empty \
			${TEST_SH} -c \
			". ./h-f${f}; "'set -- ; delim_argv "foo $@ bar"'

		atf_check -s exit:0 -o inline:'>a< >b< >c<\n' -e empty \
			${TEST_SH} -c \
			". ./h-f${f}; "'set -- a b c; delim_argv "$@"'
		atf_check -s exit:0 -o inline:'>fooa< >b< >cbar<\n' -e empty \
			${TEST_SH} -c \
			". ./h-f${f}; "'set -- a b c; delim_argv "foo$@bar"'
		atf_check -s exit:0 -o inline:'>foo a< >b< >c bar<\n' -e empty \
			${TEST_SH} -c \
			". ./h-f${f}; "'set -- a b c; delim_argv "foo $@ bar"'
	done
}

atf_test_case strip
strip_head() {
	atf_set "descr" "Checks that the %% operator works and strips" \
	                "the contents of a variable from the given point" \
			"to the end"
}
strip_body() {
	line='#define bindir "/usr/bin" /* comment */'
	stripped='#define bindir "/usr/bin" '

	# atf_expect_fail "PR bin/43469" -- now fixed
	for exp in 				\
		'${line%%/\**}'			\
		'${line%%"/*"*}'		\
		'${line%%'"'"'/*'"'"'*}'	\
		'"${line%%/\**}"'		\
		'"${line%%"/*"*}"'		\
		'"${line%%'"'"'/*'"'"'*}"'	\
		'${line%/\**}'			\
		'${line%"/*"*}'			\
		'${line%'"'"'/*'"'"'*}'		\
		'"${line%/\**}"'		\
		'"${line%"/*"*}"'		\
		'"${line%'"'"'/*'"'"'*}"'
	do
		atf_check -o inline:":$stripped:\n" -e empty ${TEST_SH} -c \
			"line='${line}'; echo :${exp}:"
	done
}

atf_test_case varpattern_backslashes
varpattern_backslashes_head() {
	atf_set "descr" "Tests that protecting wildcards with backslashes" \
	                "works in variable patterns."
}
varpattern_backslashes_body() {
	line='/foo/bar/*/baz'
	stripped='/foo/bar/'
	atf_check -o inline:'/foo/bar/\n' -e empty ${TEST_SH} -c \
		'line="/foo/bar/*/baz"; echo ${line%%\**}'
}

atf_test_case arithmetic
arithmetic_head() {
	atf_set "descr" "POSIX requires shell arithmetic to use signed" \
	                "long or a wider type.  We use intmax_t, so at" \
			"least 64 bits should be available.  Make sure" \
			"this is true."
}
arithmetic_body() {

	atf_check -o inline:'3' -e empty ${TEST_SH} -c \
		'printf %s $((1 + 2))'
	atf_check -o inline:'2147483647' -e empty ${TEST_SH} -c \
		'printf %s $((0x7fffffff))'
	atf_check -o inline:'9223372036854775807' -e empty ${TEST_SH} -c \
		'printf %s $(((1 << 63) - 1))'
}

atf_test_case iteration_on_null_parameter
iteration_on_null_parameter_head() {
	atf_set "descr" "Check iteration of \$@ in for loop when set to null;" \
	                "the error \"sh: @: parameter not set\" is incorrect." \
	                "PR bin/48202."
}
iteration_on_null_parameter_body() {
	atf_check -o empty -e empty ${TEST_SH} -c \
		'N=; set -- ${N};   for X; do echo "[$X]"; done'
}

atf_test_case iteration_on_quoted_null_parameter
iteration_on_quoted_null_parameter_head() {
	atf_set "descr" \
		'Check iteration of "$@" in for loop when set to null;'
}
iteration_on_quoted_null_parameter_body() {
	atf_check -o inline:'[]\n' -e empty ${TEST_SH} -c \
		'N=; set -- "${N}"; for X; do echo "[$X]"; done'
}

atf_test_case iteration_on_null_or_null_parameter
iteration_on_null_or_null_parameter_head() {
	atf_set "descr" \
		'Check expansion of null parameter as default for another null'
}
iteration_on_null_or_null_parameter_body() {
	atf_check -o empty -e empty ${TEST_SH} -c \
		'N=; E=; set -- ${N:-${E}}; for X; do echo "[$X]"; done'
}

atf_test_case iteration_on_null_or_missing_parameter
iteration_on_null_or_missing_parameter_head() {
	atf_set "descr" \
	    'Check expansion of missing parameter as default for another null'
}
iteration_on_null_or_missing_parameter_body() {
	# atf_expect_fail 'PR bin/50834'
	atf_check -o empty -e empty ${TEST_SH} -c \
		'N=; set -- ${N:-}; for X; do echo "[$X]"; done'
}

####### The remaining tests use the following helper functions ...

nl='
'
reset()
{
	TEST_NUM=0
	TEST_FAILURES=''
	TEST_FAIL_COUNT=0
	TEST_ID="$1"
}

check()
{
	fail=false
	TEMP_FILE=$( mktemp OUT.XXXXXX )
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

	# Remove newlines (use local shell for this)
	oifs="$IFS"
	IFS="$nl"
	result="$(echo $result)"
	IFS="$oifs"
	if [ "$2" != "$result" ]
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
	$fail && atf_fail "Test[$TEST_NUM] of '$1' failed${nl}${MSG}"
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

atf_test_case shell_params
shell_params_head() {
	atf_set "descr" "Test correct operation of the numeric parameters"
}
shell_params_body() {
	atf_require_prog mktemp

	reset shell_params

	check 'set -- a b c; echo "$#: $1 $2 $3"' '3: a b c' 0
	check 'set -- a b c d e f g h i j k l m; echo "$#: ${1}0 ${10} $10"' \
		'13: a0 j a0' 0
	check 'x="$0"; set -- a b; y="$0";
	      [ "x${x}y" = "x${y}y" ] && echo OK || echo x="$x" y="$y"' \
		'OK' 0
	check "${TEST_SH} -c 'echo 0=\$0 1=\$1 2=\$2' a b c" '0=a 1=b 2=c' 0

	echo 'echo 0="$0" 1="$1" 2="$2"' > helper.sh
	check "${TEST_SH} helper.sh a b c" '0=helper.sh 1=a 2=b' 0

	check 'set -- a bb ccc dddd eeeee ffffff ggggggg hhhhhhhh \
		iiiiiiiii jjjjjjjjjj kkkkkkkkkkk
	       echo "${#}: ${#1} ${#2} ${#3} ${#4} ... ${#9} ${#10} ${#11}"' \
		 '11: 1 2 3 4 ... 9 10 11' 0

	check 'set -- a b c; echo "$#: ${1-A} ${2-B} ${3-C} ${4-D} ${5-E}"' \
		'3: a b c D E' 0
	check 'set -- a "" c "" e
	       echo "$#: ${1:-A} ${2:-B} ${3:-C} ${4:-D} ${5:-E}"' \
		'5: a B c D e' 0
	check 'set -- a "" c "" e
	       echo "$#: ${1:+A} ${2:+B} ${3:+C} ${4:+D} ${5:+E}"' \
		'5: A  C  E' 0
	check 'set -- "abab*cbb"
	       echo "${1} ${1#a} ${1%b} ${1##ab} ${1%%b} ${1#*\*} ${1%\**}"' \
	       'abab*cbb bab*cbb abab*cb ab*cbb abab*cb cbb abab' 0
	check 'set -- "abab?cbb"
    echo "${1}:${1#*a}+${1%b*}-${1##*a}_${1%%b*}%${1#[ab]}=${1%?*}/${1%\?*}"' \
	       'abab?cbb:bab?cbb+abab?cb-b?cbb_a%bab?cbb=abab?cb/abab' 0
	check 'set -- a "" c "" e; echo "${2:=b}"' '' 1

	results
}

atf_test_case var_with_embedded_cmdsub
var_with_embedded_cmdsub_head() {
	atf_set "descr" "Test expansion of vars with embedded cmdsub"
}
var_with_embedded_cmdsub_body() {

	reset var_with_embedded_cmdsub

	check 'unset x; echo ${x-$(echo a)}$(echo b)'  'ab' 0	#1
	check 'unset x; echo ${x:-$(echo a)}$(echo b)' 'ab' 0	#2
	check 'x=""; echo ${x-$(echo a)}$(echo b)'     'b'  0	#3
	check 'x=""; echo ${x:-$(echo a)}$(echo b)'    'ab' 0	#4
	check 'x=c; echo ${x-$(echo a)}$(echo b)'      'cb' 0	#5
	check 'x=c; echo ${x:-$(echo a)}$(echo b)'     'cb' 0	#6

	check 'unset x; echo ${x+$(echo a)}$(echo b)'  'b'  0	#7
	check 'unset x; echo ${x:+$(echo a)}$(echo b)' 'b'  0	#8
	check 'x=""; echo ${x+$(echo a)}$(echo b)'     'ab' 0	#9
	check 'x=""; echo ${x:+$(echo a)}$(echo b)'    'b'  0	#10
	check 'x=c; echo ${x+$(echo a)}$(echo b)'      'ab' 0	#11
	check 'x=c; echo ${x:+$(echo a)}$(echo b)'     'ab' 0	#12

	check 'unset x; echo ${x=$(echo a)}$(echo b)'  'ab' 0	#13
	check 'unset x; echo ${x:=$(echo a)}$(echo b)' 'ab' 0	#14
	check 'x=""; echo ${x=$(echo a)}$(echo b)'     'b'  0	#15
	check 'x=""; echo ${x:=$(echo a)}$(echo b)'    'ab' 0	#16
	check 'x=c; echo ${x=$(echo a)}$(echo b)'      'cb' 0	#17
	check 'x=c; echo ${x:=$(echo a)}$(echo b)'     'cb' 0	#18

	check 'unset x; echo ${x?$(echo a)}$(echo b)'  ''   2	#19
	check 'unset x; echo ${x:?$(echo a)}$(echo b)' ''   2	#20
	check 'x=""; echo ${x?$(echo a)}$(echo b)'     'b'  0	#21
	check 'x=""; echo ${x:?$(echo a)}$(echo b)'    ''   2	#22
	check 'x=c; echo ${x?$(echo a)}$(echo b)'      'cb' 0	#23
	check 'x=c; echo ${x:?$(echo a)}$(echo b)'     'cb' 0	#24

	check 'unset x; echo ${x%$(echo a)}$(echo b)'  'b'  0	#25
	check 'unset x; echo ${x%%$(echo a)}$(echo b)' 'b'  0	#26
	check 'x=""; echo ${x%$(echo a)}$(echo b)'     'b'  0	#27
	check 'x=""; echo ${x%%$(echo a)}$(echo b)'    'b'  0	#28
	check 'x=c; echo ${x%$(echo a)}$(echo b)'      'cb' 0	#29
	check 'x=c; echo ${x%%$(echo a)}$(echo b)'     'cb' 0	#30
	check 'x=aa; echo ${x%$(echo "*a")}$(echo b)'  'ab' 0	#31
	check 'x=aa; echo ${x%%$(echo "*a")}$(echo b)' 'b'  0	#32

	check 'unset x; echo ${x#$(echo a)}$(echo b)'  'b'  0	#33
	check 'unset x; echo ${x##$(echo a)}$(echo b)' 'b'  0	#34
	check 'x=""; echo ${x#$(echo a)}$(echo b)'     'b'  0	#35
	check 'x=""; echo ${x##$(echo a)}$(echo b)'    'b'  0	#36
	check 'x=c; echo ${x#$(echo a)}$(echo b)'      'cb' 0	#37
	check 'x=c; echo ${x##$(echo a)}$(echo b)'     'cb' 0	#38
	check 'x=aa; echo ${x#$(echo "*a")}$(echo b)'  'ab' 0	#39
	check 'x=aa; echo ${x##$(echo "*a")}$(echo b)' 'b'  0	#40

	results
}

atf_test_case dollar_star
dollar_star_head() {
	atf_set "descr" 'Test expansion of various aspects of $*'
}
dollar_star_body() {

	reset # dollar_star

	check 'set -- a b c; echo $# $*'		'3 a b c'	0  # 1
	check 'set -- a b c; echo $# "$*"'		'3 a b c'	0  # 2
	check 'set -- a "b c"; echo $# $*'		'2 a b c'	0  # 3
	check 'set -- a "b c"; echo $# "$*"'		'2 a b c'	0  # 4
	check 'set -- a b c; set -- $* ; echo $# $*'	'3 a b c'	0  # 5
	check 'set -- a b c; set -- "$*" ; echo $# $*'	'1 a b c'	0  # 6
	check 'set -- a "b c"; set -- $* ; echo $# $*'	'3 a b c'	0  # 7
	check 'set -- a "b c"; set -- "$*" ; echo $# $*' \
							'1 a b c'	0  # 8

	check 'IFS=". "; set -- a b c; echo $# $*'	'3 a b c'	0  # 9
	check 'IFS=". "; set -- a b c; echo $# "$*"'	'3 a.b.c'	0  #10
	check 'IFS=". "; set -- a "b c"; echo $# $*'	'2 a b c'	0  #11
	check 'IFS=". "; set -- a "b c"; echo $# "$*"'	'2 a.b c'	0  #12
	check 'IFS=". "; set -- a "b.c"; echo $# $*'	'2 a b c'	0  #13
	check 'IFS=". "; set -- a "b.c"; echo $# "$*"'	'2 a.b.c'	0  #14
	check 'IFS=". "; set -- a b c; set -- $* ; echo $# $*' \
							'3 a b c'	0  #15
	check 'IFS=". "; set -- a b c; set -- "$*" ; echo $# $*' \
							'1 a b c'	0  #16
	check 'IFS=". "; set -- a "b c"; set -- $* ; echo $# $*' \
							'3 a b c'	0  #17
	check 'IFS=". "; set -- a "b c"; set -- "$*" ; echo $# $*' \
							'1 a b c'	0  #18
	check 'IFS=". "; set -- a b c; set -- $* ; echo $# "$*"' \
							'3 a.b.c'	0  #19
	check 'IFS=". "; set -- a b c; set -- "$*" ; echo $# "$*"' \
							'1 a.b.c'	0  #20
	check 'IFS=". "; set -- a "b c"; set -- $* ; echo $# "$*"' \
							'3 a.b.c'	0  #21
	check 'IFS=". "; set -- a "b c"; set -- "$*" ; echo $# "$*"' \
							'1 a.b c'	0  #22

	results
}

atf_test_case dollar_star_in_word
dollar_star_in_word_head() {
	atf_set "descr" 'Test expansion $* occurring in word of ${var:-word}'
}
dollar_star_in_word_body() {

	reset dollar_star_in_word

	unset xXx			; # just in case!

	# Note that the expected results for these tests are identical
	# to those from the dollar_star test.   It should never make
	# a difference whether we expand $* or ${unset:-$*}

	# (note expanding ${unset:-"$*"} is different, that is not tested here)

	check 'set -- a b c; echo $# ${xXx:-$*}'		'3 a b c' 0  # 1
	check 'set -- a b c; echo $# "${xXx:-$*}"'		'3 a b c' 0  # 2
	check 'set -- a "b c"; echo $# ${xXx:-$*}'		'2 a b c' 0  # 3
	check 'set -- a "b c"; echo $# "${xXx:-$*}"'		'2 a b c' 0  # 4
	check 'set -- a b c; set -- ${xXx:-$*} ; echo $# $*'	'3 a b c' 0  # 5
	check 'set -- a b c; set -- "${xXx:-$*}" ; echo $# $*'	'1 a b c' 0  # 6
	check 'set -- a "b c"; set -- ${xXx:-$*} ; echo $# $*'	'3 a b c' 0  # 7
	check 'set -- a "b c"; set -- "${xXx:-$*}" ; echo $# $*' \
								'1 a b c' 0  # 8

	check 'IFS=". "; set -- a b c; echo $# ${xXx:-$*}'	'3 a b c' 0  # 9
	check 'IFS=". "; set -- a b c; echo $# "${xXx:-$*}"'	'3 a.b.c' 0  #10
	check 'IFS=". "; set -- a "b c"; echo $# ${xXx:-$*}'	'2 a b c' 0  #11
	check 'IFS=". "; set -- a "b c"; echo $# "${xXx:-$*}"'	'2 a.b c' 0  #12
	check 'IFS=". "; set -- a "b.c"; echo $# ${xXx:-$*}'	'2 a b c' 0  #13
	check 'IFS=". "; set -- a "b.c"; echo $# "${xXx:-$*}"'	'2 a.b.c' 0  #14
	check 'IFS=". ";set -- a b c;set -- ${xXx:-$*};echo $# ${xXx:-$*}' \
								'3 a b c' 0  #15
	check 'IFS=". ";set -- a b c;set -- "${xXx:-$*}";echo $# ${xXx:-$*}' \
								'1 a b c' 0  #16
	check 'IFS=". ";set -- a "b c";set -- ${xXx:-$*};echo $# ${xXx:-$*}' \
								'3 a b c' 0  #17
	check 'IFS=". ";set -- a "b c";set -- "${xXx:-$*}";echo $# ${xXx:-$*}' \
								'1 a b c' 0  #18
	check 'IFS=". ";set -- a b c;set -- ${xXx:-$*};echo $# "${xXx:-$*}"' \
								'3 a.b.c' 0  #19
	check 'IFS=". ";set -- a b c;set -- "$*";echo $# "$*"' \
								'1 a.b.c' 0  #20
	check 'IFS=". ";set -- a "b c";set -- $*;echo $# "$*"' \
								'3 a.b.c' 0  #21
	check 'IFS=". ";set -- a "b c";set -- "$*";echo $# "$*"' \
								'1 a.b c' 0  #22

	results
}

atf_test_case dollar_star_with_empty_ifs
dollar_star_with_empty_ifs_head() {
	atf_set "descr" 'Test expansion of $* with IFS=""'
}
dollar_star_with_empty_ifs_body() {

	reset dollar_star_with_empty_ifs

	check 'IFS=""; set -- a b c; echo $# $*'	'3 a b c'	0  # 1
	check 'IFS=""; set -- a b c; echo $# "$*"'	'3 abc'		0  # 2
	check 'IFS=""; set -- a "b c"; echo $# $*'	'2 a b c'	0  # 3
	check 'IFS=""; set -- a "b c"; echo $# "$*"'	'2 ab c'	0  # 4
	check 'IFS=""; set -- a "b.c"; echo $# $*'	'2 a b.c'	0  # 5
	check 'IFS=""; set -- a "b.c"; echo $# "$*"'	'2 ab.c'	0  # 6
	check 'IFS=""; set -- a b c; set -- $* ; echo $# $*' \
							'3 a b c'	0  # 7
	check 'IFS=""; set -- a b c; set -- "$*" ; echo $# $*' \
							'1 abc'		0  # 8
	check 'IFS=""; set -- a "b c"; set -- $* ; echo $# $*' \
							'2 a b c'	0  # 9
	check 'IFS=""; set -- a "b c"; set -- "$*" ; echo $# $*' \
							'1 ab c'	0  #10
	check 'IFS=""; set -- a b c; set -- $* ; echo $# "$*"' \
							'3 abc'		0  #11
	check 'IFS=""; set -- a b c; set -- "$*" ; echo $# "$*"' \
							'1 abc'		0  #12
	check 'IFS=""; set -- a "b c"; set -- $* ; echo $# "$*"' \
							'2 ab c'	0  #13
	check 'IFS=""; set -- a "b c"; set -- "$*" ; echo $# "$*"' \
							'1 ab c'	0  #14

	results	  # FIXED: 'PR bin/52090 expect 7 of 14 subtests to fail'
}

atf_test_case dollar_star_in_word_empty_ifs
dollar_star_in_word_empty_ifs_head() {
	atf_set "descr" 'Test expansion of ${unset:-$*} with IFS=""'
}
dollar_star_in_word_empty_ifs_body() {

	reset dollar_star_in_word_empty_ifs

	unset xXx			; # just in case

	# Note that the expected results for these tests are identical
	# to those from the dollar_star_with_empty_ifs test.   It should
	# never make a difference whether we expand $* or ${unset:-$*}

	# (note expanding ${unset:-"$*"} is different, that is not tested here)

	check 'IFS="";set -- a b c;echo $# ${xXx:-$*}'		'3 a b c' 0  # 1
	check 'IFS="";set -- a b c;echo $# "${xXx:-$*}"'	'3 abc'	  0  # 2
	check 'IFS="";set -- a "b c";echo $# ${xXx:-$*}'	'2 a b c' 0  # 3
	check 'IFS="";set -- a "b c";echo $# "${xXx:-$*}"'	'2 ab c'  0  # 4
	check 'IFS="";set -- a "b.c";echo $# ${xXx:-$*}'	'2 a b.c' 0  # 5
	check 'IFS="";set -- a "b.c";echo $# "${xXx:-$*}"'	'2 ab.c'  0  # 6
	check 'IFS="";set -- a b c;set -- ${xXx:-$*};echo $# ${xXx:-$*}' \
								'3 a b c' 0  # 7
	check 'IFS="";set -- a b c;set -- "${xXx:-$*}";echo $# ${xXx:-$*}' \
								'1 abc'   0  # 8
	check 'IFS="";set -- a "b c";set -- ${xXx:-$*};echo $# ${xXx:-$*}' \
								'2 a b c' 0  # 9
	check 'IFS="";set -- a "b c";set -- "${xXx:-$*}";echo $# ${xXx:-$*}' \
								'1 ab c'  0  #10
	check 'IFS="";set -- a b c;set -- ${xXx:-$*};echo $# "${xXx:-$*}"' \
								'3 abc'	  0  #11
	check 'IFS="";set -- a b c;set -- "${xXx:-$*}";echo $# "${xXx:-$*}"' \
								'1 abc'	  0  #12
	check 'IFS="";set -- a "b c";set -- ${xXx:-$*};echo $# "${xXx:-$*}"' \
								'2 ab c'  0  #13
	check 'IFS="";set -- a "b c";set -- "${xXx:-$*}";echo $# "${xXx:-$*}"' \
								'1 ab c'  0  #14

	results	  # FIXED: 'PR bin/52090 expect 7 of 14 subtests to fail'
}

atf_test_case dollar_star_in_quoted_word
dollar_star_in_quoted_word_head() {
	atf_set "descr" 'Test expansion $* occurring in word of ${var:-"word"}'
}
dollar_star_in_quoted_word_body() {

	reset dollar_star_in_quoted_word

	unset xXx			; # just in case!

	check 'set -- a b c; echo $# ${xXx:-"$*"}'		'3 a b c' 0  # 1
	check 'set -- a "b c"; echo $# ${xXx:-"$*"}'		'2 a b c' 0  # 2
	check 'set -- a b c; set -- ${xXx:-"$*"} ; echo $# ${xXx-"$*"}' \
								'1 a b c' 0  # 3
	check 'set -- a "b c"; set -- ${xXx:-"$*"} ; echo $# ${xXx-"$*"}' \
								'1 a b c' 0  # 4
	check 'set -- a b c; set -- ${xXx:-"$*"} ; echo $# ${xXx-"$*"}' \
								'1 a b c' 0  # 5
	check 'set -- a "b c"; set -- ${xXx:-"$*"} ; echo $# ${xXx-$*}' \
								'1 a b c' 0  # 6
	check 'set -- a b c; set -- ${xXx:-$*} ; echo $# ${xXx-"$*"}' \
								'3 a b c' 0  # 7
	check 'set -- a "b c"; set -- ${xXx:-$*} ; echo $# ${xXx-"$*"}' \
								'3 a b c' 0  # 8

	check 'IFS=". "; set -- a b c; echo $# ${xXx:-"$*"}'	'3 a.b.c' 0  # 9
	check 'IFS=". "; set -- a "b c"; echo $# ${xXx:-"$*"}'	'2 a.b c' 0  #10
	check 'IFS=". "; set -- a "b.c"; echo $# ${xXx:-"$*"}'	'2 a.b.c' 0  #11
	check 'IFS=". ";set -- a b c;set -- ${xXx:-"$*"};echo $# ${xXx:-"$*"}' \
								'1 a.b.c' 0  #12
      check 'IFS=". ";set -- a "b c";set -- ${xXx:-"$*"};echo $# ${xXx:-"$*"}' \
								'1 a.b c' 0  #13
	check 'IFS=". ";set -- a b c;set -- ${xXx:-$*};echo $# ${xXx:-"$*"}' \
								'3 a.b.c' 0  #14
	check 'IFS=". ";set -- a "b c";set -- ${xXx:-$*};echo $# ${xXx:-"$*"}' \
								'3 a.b.c' 0  #15
	check 'IFS=". ";set -- a b c;set -- ${xXx:-"$*"};echo $# ${xXx:-$*}' \
								'1 a b c' 0  #16
	check 'IFS=". ";set -- a "b c";set -- ${xXx:-"$*"};echo $# ${xXx:-$*}' \
								'1 a b c' 0  #17

	check 'IFS="";set -- a b c;echo $# ${xXx:-"$*"}'	'3 abc'   0  #18
	check 'IFS="";set -- a "b c";echo $# ${xXx:-"$*"}'	'2 ab c'  0  #19
	check 'IFS="";set -- a "b.c";echo $# ${xXx:-"$*"}'	'2 ab.c'  0  #20
	check 'IFS="";set -- a b c;set -- ${xXx:-"$*"};echo $# ${xXx:-"$*"}' \
								'1 abc'   0  #21
	check 'IFS="";set -- a "b c";set -- ${xXx:-"$*"};echo $# ${xXx:-"$*"}' \
								'1 ab c'  0  #22
	check 'IFS="";set -- a b c;set -- ${xXx:-$*};echo $# ${xXx:-"$*"}' \
								'3 abc'   0  #23
	check 'IFS="";set -- a "b c";set -- ${xXx:-$*};echo $# ${xXx:-"$*"}' \
								'2 ab c'  0  #24
	check 'IFS="";set -- a b c;set -- ${xXx:-"$*"};echo $# ${xXx:-$*}' \
								'1 abc'   0  #25
	check 'IFS="";set -- a "b c";set -- ${xXx:-"$*"};echo $# ${xXx:-$*}' \
								'1 ab c'  0  #26

	results	  # FIXED: 'PR bin/52090 - 2 of 26 subtests expected to fail'
}

atf_init_test_cases() {
	# Listed here in the order ATF runs them, not the order from above

	atf_add_test_case arithmetic
	atf_add_test_case dollar_at
	atf_add_test_case dollar_at_with_text
	atf_add_test_case dollar_star
	atf_add_test_case dollar_star_in_quoted_word
	atf_add_test_case dollar_star_in_word
	atf_add_test_case dollar_star_in_word_empty_ifs
	atf_add_test_case dollar_star_with_empty_ifs
	atf_add_test_case iteration_on_null_parameter
	atf_add_test_case iteration_on_quoted_null_parameter
	atf_add_test_case iteration_on_null_or_null_parameter
	atf_add_test_case iteration_on_null_or_missing_parameter
	atf_add_test_case shell_params
	atf_add_test_case strip
	atf_add_test_case var_with_embedded_cmdsub
	atf_add_test_case varpattern_backslashes
}
