# $NetBSD: t_expr.sh,v 1.13 2025/03/15 15:33:00 rillig Exp $
#
# Copyright (c) 2007 The NetBSD Foundation, Inc.
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

: "${expr_prog:=expr}"

# usage: test_expr operand ... result|error
test_expr() {
	i=1
	while [ $((i++)) -lt $# ]; do
		set -- "$@" "$1"
		shift
	done
	expected="$1"
	shift

	# shellcheck disable=SC2003
	actual=$("$expr_prog" "$@" 2>&1 || :)

	printf "%s => '%s'\n" "$*" "$expected" >> expected
	printf "%s => '%s'\n" "$*" "$actual" >> actual
}

test_finish() {
	atf_check -o file:expected cat actual
}

atf_test_case lang
lang_head() {
	atf_set "descr" "Test that expr(1) works with non-C LANG (PR bin/2486)"
}
lang_body() {
	# When setlocale fails, ensure that no error message is printed,
	# like for most other utilities.

	atf_check -o inline:"21\n" \
	    env LANG=nonexistent "$expr_prog" 10 + 11
	atf_check -o inline:"21\n" \
	    env LANG=ru_RU.KOI8-R "$expr_prog" 10 + 11
}

atf_test_case overflow
overflow_head() {
	atf_set "descr" "Test overflow cases"
}
overflow_body() {
	test_expr 4611686018427387904 + 4611686018427387903 \
	          '9223372036854775807'
	test_expr 4611686018427387904 + 4611686018427387904 \
	          "expr: integer overflow or underflow occurred for operation '4611686018427387904 + 4611686018427387904'"
	test_expr 4611686018427387904 - -4611686018427387904 \
	          "expr: integer overflow or underflow occurred for operation '4611686018427387904 - -4611686018427387904'"
	test_expr -4611686018427387904 - 4611686018427387903 \
	          '-9223372036854775807'
	test_expr -4611686018427387904 - 4611686018427387905 \
	          "expr: integer overflow or underflow occurred for operation '-4611686018427387904 - 4611686018427387905'"
	test_expr -4611686018427387904 \* 1 '-4611686018427387904'
	test_expr -4611686018427387904 \* -1 '4611686018427387904'
	test_expr -4611686018427387904 \* 2 '-9223372036854775808'
	test_expr -4611686018427387904 \* 3 \
	          "expr: integer overflow or underflow occurred for operation '-4611686018427387904 * 3'"
	test_expr -4611686018427387904 \* -2 \
	          "expr: integer overflow or underflow occurred for operation '-4611686018427387904 * -2'"
	test_expr 4611686018427387904 \* 1 '4611686018427387904'
	test_expr 4611686018427387904 \* 2 \
	          "expr: integer overflow or underflow occurred for operation '4611686018427387904 * 2'"
	test_expr 4611686018427387904 \* 3 \
	          "expr: integer overflow or underflow occurred for operation '4611686018427387904 * 3'"
	test_expr -9223372036854775808 % -1 \
	          "expr: integer overflow or underflow occurred for operation '-9223372036854775808 % -1'"
	test_expr -9223372036854775808 / -1 \
	          "expr: integer overflow or underflow occurred for operation '-9223372036854775808 / -1'"
	test_expr 0 + -9223372036854775808 '-9223372036854775808'
	test_expr 0 + -1 '-1'
	test_expr 0 + 0 '0'
	test_expr 0 + 1 '1'
	test_expr 0 + 9223372036854775807 '9223372036854775807'
	test_expr -9223372036854775808 + 0 '-9223372036854775808'
	test_expr 9223372036854775807 + 0 '9223372036854775807'
	test_expr 4611686018427387904 \* -1 '-4611686018427387904'
	test_expr 4611686018427387904 \* -2 '-9223372036854775808'
	test_expr 4611686018427387904 \* -3 \
	          "expr: integer overflow or underflow occurred for operation '4611686018427387904 * -3'"
	test_expr -4611686018427387904 \* -1 '4611686018427387904'
	test_expr -4611686018427387904 \* -2 \
	          "expr: integer overflow or underflow occurred for operation '-4611686018427387904 * -2'"
	test_expr -4611686018427387904 \* -3 \
	          "expr: integer overflow or underflow occurred for operation '-4611686018427387904 * -3'"
	test_expr 0 \* -1 '0'
	test_expr 0 \* 0 '0'
	test_expr 0 \* 1 '0'

	test_finish
}

atf_test_case gtkmm
gtkmm_head() {
	atf_set "descr" "Tests from gtk-- configure that cause problems on old expr"
}
gtkmm_body() {
	test_expr 3 \> 3 \| 3 = 3 \& 4 \> 4 \| 3 = 3 \& 4 = 4 \& 5 \>= 5 '1'
	test_expr 3 \> 3 \| 3 = 3 \& 4 \> 4 \| 3 = 3 \& 4 = 4 \& 5 \>= 6 '0'
	test_expr 3 \> 3 \| 3 = 3 \& 4 \> 4 \| 3 = 3 \& 4 = 3 \& 5 \>= 5 '0'
	test_expr 3 \> 3 \| 3 = 3 \& 4 \> 4 \| 3 = 2 \& 4 = 4 \& 5 \>= 5 '0'
	test_expr 3 \> 2 \| 3 = 3 \& 4 \> 4 \| 3 = 3 \& 4 = 4 \& 5 \>= 6 '1'
	test_expr 3 \> 3 \| 3 = 3 \& 4 \> 3 \| 3 = 3 \& 4 = 4 \& 5 \>= 5 '1'

	test_finish
}

atf_test_case arithmetic_ops
arithmetic_ops_head() {
	atf_set "descr" "Dangling arithmetic operator"
}
arithmetic_ops_body() {
	test_expr .java_wrapper : / '0'
	test_expr 4 : \* '0'
	test_expr 4 : + '0'
	test_expr 4 : - '0'
	test_expr 4 : / '0'
	test_expr 4 : % '0'

	test_finish
}

atf_test_case basic_functional
basic_functional_head() {
	atf_set "descr" "Basic functional tests"
}
basic_functional_body() {
	test_expr 2			'2'
	test_expr -4			'-4'
	test_expr hello			'hello'
	test_expr -- double-dash	'double-dash'
	test_expr -- -- -- six-dashes	'expr: syntax error'
	test_expr 3 -- + 4		'expr: syntax error'
	test_expr 0000005		'0000005'
	test_expr 0 + 0000005		'5'

	test_expr 111 \& 222 \& 333	'111'
	test_expr 111 \& 222 \& 0	'0'

	test_expr 1111 \| 2222		'1111'
	test_expr 1111 \| 00		'1111'
	test_expr 0000 \| 2222		'2222'
	test_expr 0000 \| 00		'00'
	# FIXME: POSIX says the result must be zero.
	test_expr 0000 \| ''		''

	test_finish
}

atf_test_case compare_ops
compare_ops_head() {
	atf_set "descr" "Compare operator tests"
}
compare_ops_body() {
	test_expr 2 \!= 5 '1'
	test_expr 2 \!= 2 '0'
	test_expr 2 \<= 3 '1'
	test_expr 2 \<= 2 '1'
	test_expr 2 \<= 1 '0'
	test_expr 2 \< 3 '1'
	test_expr 2 \< 2 '0'
	test_expr 2 = 2 '1'
	test_expr 2 = 4 '0'
	test_expr 2 \>= 1 '1'
	test_expr 2 \>= 2 '1'
	test_expr 2 \>= 3 '0'
	test_expr 2 \> 1 '1'
	test_expr 2 \> 2 '0'

	test_finish
}

atf_test_case multiply
multiply_head() {
	atf_set "descr" "Test the multiply operator (PR bin/12838)"
}
multiply_body() {
	test_expr 1 \* -1 '-1'
	test_expr 2 \> 1 \* 17 '0'

	test_finish
}

atf_test_case negative
negative_head() {
	atf_set "descr" "Test the additive inverse"
}
negative_body() {
	test_expr -1 + 5 '4'
	test_expr - 1 + 5 'expr: syntax error'

	test_expr 5 + -1 '4'
	test_expr 5 + - 1 'expr: syntax error'

	test_expr 1 - -5 '6'

	test_finish
}

atf_test_case precedence
precedence_head() {
	atf_set "descr" "Tests for operator precedence"
}
precedence_body() {
	test_expr or \| '' \& and	'or'
	test_expr '' \& and \| or	'or'
	test_expr X1/2/3 : 'X\(.*[^/]\)//*[^/][^/]*/*$' \| . : '\(.\)' '1/2'
	test_expr and \& 001 = 00001	'and'
	test_expr 001 = 00001 \& and	'1'
	test_expr 1 = 2 = 3 = 4 = 5	'0'
	test_expr 1 = 2 = 3 = 4 = 0	'1'
	test_expr 2 \> 1 \* 17		'0'
	test_expr 900 + 101 = 1000 + 1	'1'
	test_expr 1000 - 101 = 900 - 1	'1'
	test_expr 1 + 100 - 10 + 1000	'1091'
	test_expr 50 + 3 \* 4 + 80	'142'
	test_expr 12345 / 1000 \* 1000	'12000'
	test_expr 12345 % 1000 / 10	'34'
	test_expr 2 : 4 / 2		'0'
	test_expr 4 : 4 % 3		'1'
	test_expr 6 \* 1111100 : 1\*	'30'
	test_expr -3 + -1 \* 4 + 3 / -6	'-7'
	test_expr 10 \* \( 3 + 5 \)	'80'
	test_expr length 123456 : '\([1236]*\)' '6'
	test_expr length \( 123456 : '\([1236]*\)' \) '3'

	test_finish
}

atf_test_case regex
regex_head() {
	atf_set "descr" "Test proper () returning \1 from a regex"
}
regex_body() {
	test_expr 1/2 : '.*/\(.*\)' '2'

	test_finish
}

atf_test_case short_circuit
short_circuit_head() {
	atf_set "descr" "Test short-circuit evaluation of '|' and '&'"
}
short_circuit_body() {
	test_expr 0 \| 1 / 0 "expr: second argument to '/' must not be zero"
	test_expr 123 \| 1 / 0 '123'
	test_expr 123 \| a : '***' '123'

	test_expr 0 \& 1 / 0 '0'
	test_expr 0 \& a : '***' '0'
	test_expr 123 \& 1 / 0 "expr: second argument to '/' must not be zero"

	test_finish
}

atf_test_case string_length
string_length_head() {
	atf_set "descr" "Test the string length operator"
}
string_length_body() {
	# The 'length' operator is an extension to POSIX 2024.
	test_expr length "" '0'
	test_expr length + 'expr: syntax error'
	test_expr length \! '1'
	test_expr length ++ '2'
	test_expr length length '6'

	test_finish
}

atf_init_test_cases()
{
	atf_add_test_case lang
	atf_add_test_case overflow
	atf_add_test_case gtkmm
	atf_add_test_case arithmetic_ops
	atf_add_test_case basic_functional
	atf_add_test_case compare_ops
	atf_add_test_case multiply
	atf_add_test_case negative
	atf_add_test_case precedence
	atf_add_test_case regex
	atf_add_test_case short_circuit
	atf_add_test_case string_length
}
