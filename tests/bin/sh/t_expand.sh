# $NetBSD: t_expand.sh,v 1.19 2018/04/21 21:28:35 kre Exp $
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

atf_test_case wrap_strip
wrap_strip_head() {
	atf_set "descr" "Checks that the %% operator works and strips" \
	                "the contents of a variable from the given point" \
			'to the end, and that \ \n sequences do not break it'
}
wrap_strip_body() {
	line='#define bindir "/usr/bin" /* comment */'
	stripped='#define bindir "/usr/bin" '

	for exp in 				\
		'${line\
%%/\**}'					\
		'${line%%"/\
*"*}'						\
		'${line%%'"'"'/*'"'"'\
*}'						\
		'"${li\
ne%%/\**}"'					\
		'"${line%%"\
/*"*}"'						\
		'"${line%\
%'"'"'/*'"'"'*}"'				\
		'${line\
%\
/\*\
*\
}'						\
		'${line%"/*\
"*\
}'						\
		'${line\
%\
'"'"'/*'"'"'*}'					\
		'"$\
{li\
ne%\
'"'"'/*'"'"'*}"'
	do
		atf_check -o inline:":$stripped:\n" -e empty ${TEST_SH} -c \
			"line='${line}'; echo :${exp}:"
	done
}

atf_test_case tilde
tilde_head() {
	atf_set "descr" "Checks that the ~ expansions work"
}
tilde_body() {
	for HOME in '' / /home/foo \
/a/very/long/home/directory/path/that/might/push/the/tilde/expansion/code/beyond/what/it/would/normally/ever/see/on/any/sane/system/and/perhaps/expose/some/bugs
	do
		export HOME

		atf_check -s exit:0 -e empty \
			-o inline:'HOME\t'"${HOME}"'
~\t'"${HOME}"'
~/foobar\t'"${HOME}"'/foobar
"$V"\t'"${HOME}"'
"$X"\t~
"$Y"\t'"${HOME}"':'"${HOME}"'
"$YY"\t'"${HOME}"'/foo:'"${HOME}"'/bar
"$Z"\t'"${HOME}"'/~
${U:-~}\t'"${HOME}"'
$V\t'"${HOME}"'
$X\t~
$Y\t'"${HOME}"':'"${HOME}"'
$YY\t'"${HOME}"'/foo:'"${HOME}"'/bar
$Z\t'"${HOME}"'/~
${U:=~}\t'"${HOME}"'
${UU:=~:~}\t'"${HOME}"':'"${HOME}"'
${UUU:=~/:~}\t'"${HOME}"'/:'"${HOME}"'
${U4:=~/:~/}\t'"${HOME}"'/:'"${HOME}"'/\n' \
			${TEST_SH} -s <<- \EOF
				unset -v U UU UUU U4
				V=~
				X="~"
				Y=~:~ YY=~/foo:~/bar
				Z=~/~
				printf '%s\t%s\n' \
					'HOME' "${HOME}" \
					'~' ~ \
					'~/foobar' ~/foobar \
					'"$V"' "$V" \
					'"$X"' "$X" \
					'"$Y"' "$Y" \
					'"$YY"' "$YY" \
					'"$Z"' "$Z" \
					'${U:-~}' ''${U:-~} \
					'$V' ''$V \
					'$X' ''$X \
					'$Y' ''$Y \
					'$YY' ''$YY \
					'$Z' ''$Z \
					'${U:=~}' ''${U:=~} \
					'${UU:=~:~}' ''${UU:=~:~} \
					'${UUU:=~/:~}' ''${UUU:=~/:~} \
					'${U4:=~/:~/}' ''${U4:=~/:~/}
			EOF
	done

	# Testing ~user is harder, so, perhaps later...
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

	check 'set -- a b c d; echo ${4294967297}' '' 0  # result 'a' => ${1}
	check 'set -- a b c; echo ${01}' 'a' 0
	check "${TEST_SH} -c 'echo 0=\${00} 1=\${01} 2=\${02}' a b c" \
			'0=a 1=b 2=c' 0

	# by special request, for PaulG...  (${0...} is not octal!)

	# Posix XCU 2.5.1 (Issue 7 TC2 pg 2349 lines 74835..6):
	#   The digits denoting the positional parameters shall always
	#   be interpreted as a decimal value, even if there is a leading zero.

	check \
	    'set -- a b c d e f g h i j k l m; echo "$#: ${08} ${010} ${011}"' \
		'13: h j k' 0

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

atf_test_case dollar_hash
dollar_hash_head() {
	atf_set "descr" 'Test expansion of various aspects of $#'
}
dollar_hash_body() {

#
#	$# looks like it should be so simple that it doesn't really
#	need a test of its own, and used in that way, it really doesn't.
#	But when we add braces ${#} we need to deal with the three
#	(almost 4) different meanings of a # inside a ${} expansion...
#
#	Note that some of these are just how we treat expansions that
#	are unspecified by posix (as noted below.)
#	
#		1.   ${#} is just $# (number of params)
#		1.a	${\#} is nothing at all (error: invalid expansion)
#		1.b	${\#...} (anything after) is the same (invalid)
#		2.   ${#VAR} is the length of the value VAR
#		2.a	Including ${##} - the length of ${#}
#		3    ${VAR#pat} is the value of VAR with leading pat removed
#		3.a	Including ${VAR#} which just removes leading nothing
#			This is relevant in case of ${VAR#${X}} with X=''
#				nb: not required by posix, see XCU 2.6.2
#		3.b	${##} is not a case of 3.a but rather 2.a 
#		3.c	Yet ${##pat} is a case of 3.a
#			Including ${##${X}} where X='' or X='#'
#				nb: not required by posix, see XCU 2.6.2
#		3.d	And ${#\#} is invalid (error)
#		3.e	But ${##\#} removes a leading # from the value of $#
#			(so is just $# as there is no leading # there)
#				nb: not required by posix, see XCU 2.6.2
#		4    ${VAR##pat} is the value of VAR with longest pat removed
#		4.a	Including ${VAR##} which removes the longest nothing
#		4.b	Which in this case includes ${###} (so is == $#)
#				nb: not required by posix, see XCU 2.6.2
#		4.c	But not ${##\#} which is $# with a leading '#' removed
#			(and so is also == $#), i.e.: like ${###} but different.
#				nb: not required by posix, see XCU 2.6.2
#		4.d	As is ${###\#} or just ${####} - remove  # (so just $#)
#				nb: not required by posix, see XCU 2.6.2
#

	reset dollar_hash

	check 'set -- ; echo $#'			'0'		0  # 1
	check 'set -- a b c; echo $#'			'3'		0  # 2
	check 'set -- a b c d e f g h i j; echo $#'	'10'		0  # 3
# rule 1
	check 'set -- ; echo ${#}'			'0'		0  # 4
	check 'set -- a b c; echo ${#}'			'3'		0  # 5
	check 'set -- a b c d e f g h i j; echo ${#}'	'10'		0  # 6
# rule 1.a
	check 'set -- a b c; echo ${\#}'		''		2  # 7
# rule 1.b
	check 'set -- a b c; echo ${\#:-foo}'		''		2  # 8
# rule 2
	check 'VAR=12345; echo ${#VAR}'			'5'		0  # 9
	check 'VAR=123456789012; echo ${#VAR}'		'12'		0  #10
# rule 2.a
	check 'set -- ; echo ${##}'			'1'		0  #11
	check 'set -- a b c; echo ${##}'		'1'		0  #12
	check 'set -- a b c d e f g h i j; echo ${##}'	'2'		0  #13
# rule 3
	check 'VAR=12345; echo ${VAR#1}'		'2345'		0  #14
	check 'VAR=12345; echo ${VAR#2}'		'12345'		0  #15
	check 'VAR=#2345; echo ${VAR#\#}'		'2345'		0  #16
	check 'X=1; VAR=12345; echo ${VAR#${X}}'	'2345'		0  #17
	check 'X=1; VAR=#2345; echo ${VAR#${X}}'	'#2345'		0  #18
# rule 3.a
	check 'VAR=12345; echo ${VAR#}'			'12345'		0  #19
	check 'X=; VAR=12345; echo ${VAR#${X}}'		'12345'		0  #20
# rule 3.b (tested above, rule 2.a)
# rule 3.c
	check 'set -- ; echo ${##0}'			''		0  #21
	check 'set -- a b c; echo ${##1}'		'3'		0  #22
	check 'set -- a b c d e f g h i j; echo ${##1}'	'0'		0  #23
	check 'X=0; set -- ; echo ${##${X}}'		''		0  #24
	check 'X=; set -- ; echo ${##${X}}'		'0'		0  #25
	check 'X=1; set -- a b c; echo ${##${X}}'	'3'		0  #26
	check 'X=1; set -- a b c d e f g h i j; echo ${##${X}}'	'0'	0  #27
	check 'X=; set -- a b c d e f g h i j; echo ${##${X}}'	'10'	0  #28
	check 'X=#; VAR=#2345; echo ${VAR#${X}}'	'2345'		0  #29
	check 'X=#; VAR=12345; echo ${VAR#${X}}'	'12345'		0  #30
# rule 3.d
	check 'set -- a b c; echo ${#\#}'		''		2  #31
# rule 3.e
	check 'set -- ; echo ${##\#}'			'0'		0  #32
	check 'set -- a b c d e f g h i j; echo ${##\#}' '10'		0  #33

# rule 4
	check 'VAR=12345; echo ${VAR##1}'		'2345'		0  #34
	check 'VAR=12345; echo ${VAR##\1}'		'2345'		0  #35
# rule 4.a
	check 'VAR=12345; echo ${VAR##}'		'12345'		0  #36
# rule 4.b
	check 'set -- ; echo ${###}'			'0'		0  #37
	check 'set -- a b c d e f g h i j; echo ${###}'	'10'		0  #38
# rule 4.c
	check 'VAR=12345; echo ${VAR#\#}'		'12345'		0  #39
	check 'VAR=12345; echo ${VAR#\#1}'		'12345'		0  #40
	check 'VAR=#2345; echo ${VAR#\#}'		'2345'		0  #41
	check 'VAR=#12345; echo ${VAR#\#1}'		'2345'		0  #42
	check 'VAR=#2345; echo ${VAR#\#1}'		'#2345'		0  #43
	check 'set -- ; echo ${####}'			'0'		0  #44
	check 'set -- ; echo ${###\#}'			'0'		0  #45
	check 'set -- a b c d e f g h i j; echo ${####}' '10'		0  #46
	check 'set -- a b c d e f g h i j; echo ${###\#}' '10'		0  #47

# now check for some more utter nonsense, not mentioned in the rules
# above (doesn't need to be)

	check 'x=hello; set -- a b c; echo ${#x:-1}'	''		2  #48
	check 'x=hello; set -- a b c; echo ${#x-1}'	''		2  #49
	check 'x=hello; set -- a b c; echo ${#x:+1}'	''		2  #50
	check 'x=hello; set -- a b c; echo ${#x+1}'	''		2  #51
	check 'x=hello; set -- a b c; echo ${#x+1}'	''		2  #52
	check 'x=hello; set -- a b c; echo ${#x:?msg}'	''		2  #53
	check 'x=hello; set -- a b c; echo ${#x?msg}'	''		2  #54
	check 'x=hello; set -- a b c; echo ${#x:=val}'	''		2  #55
	check 'x=hello; set -- a b c; echo ${#x=val}'	''		2  #56
	check 'x=hello; set -- a b c; echo ${#x#h}'	''		2  #57
	check 'x=hello; set -- a b c; echo ${#x#*l}'	''		2  #58
	check 'x=hello; set -- a b c; echo ${#x##*l}'	''		2  #59
	check 'x=hello; set -- a b c; echo ${#x%o}'	''		2  #60
	check 'x=hello; set -- a b c; echo ${#x%l*}'	''		2  #61
	check 'x=hello; set -- a b c; echo ${#x%%l*}'	''		2  #62

# but just to be complete, these ones should work

	check 'x=hello; set -- a b c; echo ${#%5}'	'3'		0  #63
	check 'x=hello; set -- a b c; echo ${#%3}'	''		0  #64
	check 'x=hello; set -- a b c; echo ${#%?}'	''		0  #65
	check 'X=#; set -- a b c; echo ${#%${X}}'	'3'		0  #66
	check 'X=3; set -- a b c; echo ${#%${X}}'	''		0  #67
	check 'set -- a b c; echo ${#%%5}'		'3'		0  #68
	check 'set -- a b c; echo ${#%%3}'		''		0  #69
	check 'set -- a b c d e f g h i j k l; echo ${#%1}' '12'	0  #70
	check 'set -- a b c d e f g h i j k l; echo ${#%2}' '1'		0  #71
	check 'set -- a b c d e f g h i j k l; echo ${#%?}' '1'		0  #72
	check 'set -- a b c d e f g h i j k l; echo ${#%[012]}' '1'	0  #73
	check 'set -- a b c d e f g h i j k l; echo ${#%[0-4]}' '1'	0  #74
	check 'set -- a b c d e f g h i j k l; echo ${#%?2}' ''		0  #75
	check 'set -- a b c d e f g h i j k l; echo ${#%1*}' ''		0  #76
	check 'set -- a b c d e f g h i j k l; echo ${#%%2}' '1'	0  #77
	check 'set -- a b c d e f g h i j k l; echo ${#%%1*}' ''	0  #78

# and this lot are stupid, as $# is never unset or null, but they do work...

	check 'set -- a b c; echo ${#:-99}'		'3'		0  #79
	check 'set -- a b c; echo ${#-99}'		'3'		0  #80
	check 'set -- a b c; echo ${#:+99}'		'99'		0  #81
	check 'set -- a b c; echo ${#+99}'		'99'		0  #82
	check 'set -- a b c; echo ${#:?bogus}'		'3'		0  #83
	check 'set -- a b c; echo ${#?bogus}'		'3'		0  #84

# even this utter nonsense is OK, as while special params cannot be
# set this way, here, as $# is not unset, or null, the assignment
# never happens (isn't even attempted)

	check 'set -- a b c; echo ${#:=bogus}'		'3'		0  #85
	check 'set -- a b c; echo ${#=bogus}'		'3'		0  #86

	for n in 0 1 10 25 100				#87 #88 #89 #90 #91
	do
		check "(exit $n)"'; echo ${#?}'		"${#n}"		0
	done

	results		# results so far anyway...

# now we have some harder to verify cases, as they (must) check unknown values
# and hence the resuls cannot just be built into the script, but we have
# to use some tricks to validate them, so for these, just do regular testing
# and don't attempt to use the check helper function, nor to include
# these tests in the result summary.   If anything has already failed, we
# do not get this far...   From here on, first failure ends this test.

	for opts in '' '-a' '-au' '-auf' '-aufe' # options safe enough to set
	do
		# Note the shell might have other (or these) opts set already

		RES=$(${TEST_SH} -c "test -n '${opts}' && set ${opts};
			printf '%s' \"\$-\";printf ' %s\\n' \"\${#-}\"") ||
			atf_fail '${#-} test exited with status '"$?"
		LEN="${RES##* }"
		DMINUS="${RES% ${LEN}}"
		if [ "${#DMINUS}" != "${LEN}" ]
		then
			atf_fail \
		   '${#-} test'" produced ${LEN} for opts ${DMINUS} (${RES})"
		fi
	done

	for seq in a b c d e f g h i j
	do
		# now we are tryin to generate different pids for $$ and $!
		# so we just run the test a number of times, and hope...
		# On NetBSD pid randomisation will usually help the tests

		eval "$(${TEST_SH} -c \
		    '(exit 0)& BG=$! LBG=${#!};
	    printf "SH=%s BG=%s LSH=%s LBG=%s" "$$" "$BG" "${#$}" "$LBG";
		      wait')"

		if [ "${#SH}" != "${LSH}" ] || [ "${#BG}" != "${LBG}" ]
		then
			atf_fail \
	'${#!] of '"${BG} was ${LBG}, expected ${#BG}"'; ${#$} of '"${SH} was ${LSH}, expected ${#SH}"
		fi
	done
}

atf_test_case dollar_star
dollar_star_head() {
	atf_set "descr" 'Test expansion of various aspects of $*'
}
dollar_star_body() {

	reset dollar_star

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

atf_test_case embedded_nl
embedded_nl_head() {
	atf_set "descr" 'Test literal \n in xxx string in ${var-xxx}'
}
embedded_nl_body() {

	atf_check -s exit:0 -o inline:'a\nb\n' -e empty ${TEST_SH} <<- 'EOF'
		unset V
		X="${V-a
		b}"
		printf '%s\n' "${X}"
		EOF

	atf_check -s exit:0 -o inline:'a\nb\n' -e empty ${TEST_SH} <<- 'EOF'
		unset V
		X=${V-"a
		b"}
		printf '%s\n' "${X}"
		EOF

	# This should not generate a syntax error, see PR bin/53201
	atf_check -s exit:0 -o inline:'abc\n' -e empty ${TEST_SH} <<- 'EOF'
		V=abc
		X=${V-a
		b}
		printf '%s\n' "${X}"
		EOF

	# Nor should any of these...
	atf_check -s exit:0 -o inline:'a\nb\n' -e empty ${TEST_SH} <<- 'EOF'
		unset V
		X=${V-a
		b}
		printf '%s\n' "${X}"
		EOF

	atf_check -s exit:0 -o inline:'a\nb\n' -e empty ${TEST_SH} <<- 'EOF'
		unset V
		X=${V:=a
		b}
		printf '%s\n' "${X}"
		EOF

	atf_check -s exit:0 -o inline:'xa\nby\na\nb\n' -e empty \
	    ${TEST_SH} <<- 'EOF'
		unset V
		X=x${V:=a
		b}y
		printf '%s\n' "${X}" "${V}"
		EOF
}

atf_init_test_cases() {
	# Listed here in the order ATF runs them, not the order from above

	atf_add_test_case arithmetic
	atf_add_test_case dollar_at
	atf_add_test_case dollar_at_with_text
	atf_add_test_case dollar_hash
	atf_add_test_case dollar_star
	atf_add_test_case dollar_star_in_quoted_word
	atf_add_test_case dollar_star_in_word
	atf_add_test_case dollar_star_in_word_empty_ifs
	atf_add_test_case dollar_star_with_empty_ifs
	atf_add_test_case embedded_nl
	atf_add_test_case iteration_on_null_parameter
	atf_add_test_case iteration_on_quoted_null_parameter
	atf_add_test_case iteration_on_null_or_null_parameter
	atf_add_test_case iteration_on_null_or_missing_parameter
	atf_add_test_case shell_params
	atf_add_test_case strip
	atf_add_test_case tilde
	atf_add_test_case wrap_strip
	atf_add_test_case var_with_embedded_cmdsub
	atf_add_test_case varpattern_backslashes
}
