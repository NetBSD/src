# $NetBSD: t_lint2.sh,v 1.13 2021/12/16 09:38:54 rillig Exp $
#
# Copyright (c) 2021 The NetBSD Foundation, Inc.
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

lint2=/usr/libexec/lint2

std_head()
{
	atf_set 'require.progs' "$lint2"
}

std_body()
{
	# shellcheck disable=SC2155
	local srcdir="$(atf_get_srcdir)"

	# remove comments and whitespace from the .ln file
	sed -e '/^#/d' -e '/^$/d' -e 's,#.*,,' -e 's,[[:space:]],,g' \
	    < "$srcdir/$1.ln" \
	    > "$1.ln"

	atf_check -o "file:$srcdir/$1.exp" \
	    "$lint2" -h -x "$1.ln"
}

std_emit_body()
{
	# shellcheck disable=SC2155
	local srcdir="$(atf_get_srcdir)"

	# remove comments and whitespace from the .ln files
	sed -e '/^#/d' -e '/^$/d' -e 's,#.*,,' -e 's,[[:space:]],,g' \
	    < "$srcdir/$1.ln" \
	    > "$1.ln"
	sed -e '/^#/d' -e '/^$/d' -e 's,#.*,,' -e 's,[[:space:]],,g' \
	    < "$srcdir/$1.exp-ln" \
	    > "$1.exp-ln"

	atf_check \
	    "$lint2" -h -x -C "$1" "$1.ln"

	atf_check -o "file:$1.exp-ln" \
	    cat "llib-l$1.ln"
}

emit_body()
{
	std_emit_body 'emit'
}

emit_lp64_body()
{
	std_emit_body 'emit_lp64'
}

# usage: test_error input message-regex [input-regex]
test_error()
{
	printf '%s\n' \
	    "$1"
	printf '%s\n' \
	    '0sinput.ln' \
	    'Sinput.ln' \
	    "$1" \
	    > 'input.ln'

	atf_check -s 'exit:1' \
	    -e "match:error: input\\.ln:3: $2 \\(for '${3-$1}'\\)\$" \
	    "$lint2" 'input.ln'
}

test_error_ignored()
{
	printf '%s\n' \
	    "$1"
	printf '%s\n' \
	    '0sinput.ln' \
	    'Sinput.ln' \
	    "$1" \
	    > 'input.ln'

	atf_check -o 'ignore' \
	    "$lint2" 'input.ln'
}

error_cases_head()
{
	std_head
}
error_cases_body()
{
	test_error ''			'missing record type'
	test_error '123'		'missing record type'
	test_error '0X'			'not a number: '
	test_error '0d'			'not a number: '
	test_error '0dXYZ'		'not a number: XYZ'
	test_error '0d123'		'bad line number'
	test_error '0d123.XYZ'		'not a number: XYZ'
	test_error '0X0.0'		'bad record type X'

	# function calls
	test_error '0c0.0'		'not a number: '
	test_error '0c0.0uu'		'used or discovered: u'
	test_error '0c0.0du'		'used or discovered: u'
	test_error '0c0.0ui'		'used or discovered: i'
	test_error '0c0.0di'		'used or discovered: i'
	test_error '0c0.0ud'		'used or discovered: d'
	test_error '0c0.0dd'		'used or discovered: d'
	# Unlike 'd' and 'u', the 'i' may be repeated.
	test_error '0c0.0iiiiiii1n_'	'bad type: _ '
	# Negative argument numbers like in 'z-1' are accepted but ignored.
	test_error '0c0.0z-1d_'		'not a number: _'
	# Argument 1 is both positive '1p' and negative '1n', which is
	# impossible in practice.  It is not worth handling this though since
	# only lint1 generates these .ln files.
	test_error '0c0.0p1n1d_'	'not a number: _'
	test_error '0c0.0s'		'not a number: '
	test_error '0c0.0s2'		'not quote: '
	test_error '0c0.0s2|'		'not quote: |'
	test_error '0c0.0s2"'		'trailing data: '
	test_error '0c0.0s2"%'		'missing closing quote'
	# shellcheck disable=SC1003
	test_error '0c0.0s2"\'		'missing after \\'	'0c0\.0s2"\\'
	# shellcheck disable=SC1003
	test_error '0c0.0s2"%\'		'missing after \\'	'0c0\.0s2"%\\'

	# declarations and definitions
	test_error '0d0'		'bad line number'
	test_error '0d0.0'		'not a number: '
	test_error '0d0.0dd'		'def'
	test_error '0d0.0de'		'decl'
	test_error '0d0.0ee'		'decl'
	test_error '0d0.0ii'		'inline'
	test_error '0d0.0oo'		'osdef'
	test_error '0d0.0rr'		'r'
	test_error '0d0.0ss'		'static'
	test_error '0d0.0tt'		'tdef'
	test_error '0d0.0uu'		'used'
	test_error '0d0.0v1v1'		'v'
	test_error '0d0.0P1P1'		'P'
	test_error '0d0.0S1S1'		'S'
	test_error '0d0.0v1P1S_'	'not a number: _'
	test_error '0d0.0d3var_'	'bad type: _ '
	test_error '0d0.0d3varPV_'	'trailing line: _'

	# usage of a variable or a function
	test_error '0u0.0'		'bad delim '
	test_error '0u0.0_'		'bad delim _'
	test_error '0u0.0x'		'not a number: '

	# trailing garbage is not detected
	test_error_ignored '0u0.0x3var_'
}

missing_newline_head()
{
	std_head
}

missing_newline_body()
{
	# Before read.c 1.72 from 2021-12-16, the error message was just 'c'
	# without any textual description or context, and the line number was
	# off by one, it was reported as line 0.

	printf '1d1.1e5func' > 'input.ln'

	atf_check -s 'exit:1' \
	    -e 'match:^.*: error: input\.ln:1: missing newline after .c. \(for .1d1\.1e5func.\)$' \
	    "$lint2" 'input.ln'
}

atf_init_test_cases()
{
	local i

	# shellcheck disable=SC2013
	# shellcheck disable=SC2035
	for i in $(cd "$(atf_get_srcdir)" && echo *.ln); do
		i=${i%.ln}

		case "$i" in
		*lp64*)
			case "$(uname -p)" in
			*64) ;;
			*) continue
			esac
		esac

		type "${i}_head" 1>/dev/null 2>&1 \
		|| eval "${i}_head() { std_head; }"
		type "${i}_body" 1>/dev/null 2>&1 \
		|| eval "${i}_body() { std_body '$i'; }"
		atf_add_test_case "$i"
	done

	atf_add_test_case 'error_cases'
	atf_add_test_case 'missing_newline'
}
