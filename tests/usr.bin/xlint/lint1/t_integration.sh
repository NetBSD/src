# $NetBSD: t_integration.sh,v 1.57 2021/06/20 18:09:48 rillig Exp $
#
# Copyright (c) 2008, 2010 The NetBSD Foundation, Inc.
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

lint1=/usr/libexec/lint1

test_case_names=


extract_flags()
{
	local extract_flags_awk

	# shellcheck disable=SC2016
	extract_flags_awk='
		BEGIN {
			flags = "-g -S -w"
		}
		/^\/\* (lint1-flags|lint1-extra-flags): .*\*\/$/ {
			if ($2 == "lint1-flags:")
				flags = ""
			for (i = 3; i < NF; i++)
				flags = flags " " $i
		}
		END {
			print flags
		}
	'

	awk "$extract_flags_awk" "$@"
}

# shellcheck disable=SC2155
check_lint1()
{
	local src="$(atf_get_srcdir)/$1"
	local exp="${src%.c}.exp"
	local src_ln="${src%.c}.ln"
	local wrk_ln="${1%.c}.ln"
	local flags="$(extract_flags "$src")"

	if [ ! -f "$src_ln" ]; then
		src_ln='/dev/null'
		wrk_ln='/dev/null'
	fi

	if [ -f "$exp" ]; then
		# shellcheck disable=SC2086
		atf_check -s not-exit:0 -o "file:$exp" -e empty \
		    "$lint1" $flags "$src" "$wrk_ln"
	else
		# shellcheck disable=SC2086
		atf_check -s exit:0 \
		    "$lint1" $flags "$src" "$wrk_ln"
	fi

	if [ "$src_ln" != '/dev/null' ]; then
		atf_check -o "file:$src_ln" cat "$wrk_ln"
	fi
}

test_case()
{
	local name="$1"

	atf_test_case "$name"
	eval "${name}_head() {
		atf_set 'require.progs' '$lint1'
	}"
	eval "${name}_body() {
		check_lint1 '$name.c'
	}"

	test_case_names="$test_case_names $name"
}


test_case all_messages
test_case c99_init_designator
test_case d_alignof
test_case d_bltinoffsetof
test_case d_c99_anon_struct
test_case d_c99_anon_union
test_case d_c99_bool
test_case d_c99_bool_strict
test_case d_c99_bool_strict_syshdr
test_case d_c99_complex_num
test_case d_c99_complex_split
test_case d_c99_compound_literal_comma
test_case d_c99_decls_after_stmt
test_case d_c99_decls_after_stmt2
test_case d_c99_decls_after_stmt3
test_case d_c99_flex_array_packed
test_case d_c99_for_loops
test_case d_c99_func
test_case d_c99_init
test_case d_c99_nested_struct
test_case d_c99_recursive_init
test_case d_c99_struct_init
test_case d_c99_union_cast
test_case d_c99_union_init1
test_case d_c99_union_init2
test_case d_c99_union_init3
test_case d_c99_union_init4
test_case d_c99_union_init5
test_case d_c9x_array_init
test_case d_c9x_recursive_init
test_case d_cast_fun_array_param
test_case d_cast_init
test_case d_cast_init2
test_case d_cast_lhs
test_case d_cast_typeof
test_case d_compound_literals1
test_case d_compound_literals2
test_case d_constant_conv1
test_case d_constant_conv2
test_case d_cvt_constant
test_case d_cvt_in_ternary
test_case d_decl_old_style_arguments
test_case d_ellipsis_in_switch
test_case d_fold_test
test_case d_gcc_compound_statements1
test_case d_gcc_compound_statements2
test_case d_gcc_compound_statements3
test_case d_gcc_extension
test_case d_gcc_func
test_case d_gcc_variable_array_init
test_case d_incorrect_array_size
test_case d_init_array_using_string
test_case d_init_pop_member
test_case d_lint_assert
test_case d_long_double_int
test_case d_nested_structs
test_case d_nolimit_init
test_case d_packed_structs
test_case d_pr_22119
test_case d_return_type
test_case d_shift_to_narrower_type
test_case d_struct_init_nested
test_case d_type_conv1
test_case d_type_conv2
test_case d_type_conv3
test_case d_type_question_colon
test_case d_typefun
test_case d_typename_as_var
test_case d_zero_sized_arrays
test_case decl_struct_member
test_case emit
test_case expr_range
test_case feat_stacktrace
test_case gcc_attribute
test_case gcc_attribute_aligned
test_case gcc_bit_field_types
test_case gcc_init_compound_literal
test_case gcc_typeof_after_statement
test_case lex_char
test_case lex_comment
test_case lex_floating
test_case lex_integer
test_case lex_string
test_case lex_wide_char
test_case lex_wide_string
test_case op_colon
test_case stmt_for

all_messages_body()
{
	local failed msg

	failed=""

	for msg in $(seq 0 344); do
		name="$(printf 'msg_%03d.c' "$msg")"
		check_lint1 "$name" \
		|| failed="$failed${failed:+ }$name"
	done

	if [ "$failed" != "" ]; then
		atf_check "false" "$failed"
	fi
}


atf_init_test_cases()
{
	for name in $test_case_names; do
		atf_add_test_case "$name"
	done
}
