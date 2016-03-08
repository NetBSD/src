# $NetBSD: t_expand.sh,v 1.6 2016/03/08 14:26:54 christos Exp $
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

atf_init_test_cases() {
	atf_add_test_case dollar_at
	atf_add_test_case dollar_at_with_text
	atf_add_test_case strip
	atf_add_test_case varpattern_backslashes
	atf_add_test_case arithmetic
	atf_add_test_case iteration_on_null_parameter
	atf_add_test_case iteration_on_quoted_null_parameter
	atf_add_test_case iteration_on_null_or_null_parameter
	atf_add_test_case iteration_on_null_or_missing_parameter
}
