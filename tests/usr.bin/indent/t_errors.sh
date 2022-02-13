#! /bin/sh
# $NetBSD: t_errors.sh,v 1.23 2022/02/13 11:07:48 rillig Exp $
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
# $FreeBSD$

# Tests for error handling in indent.

indent=$(atf_config_get usr.bin.indent.test_indent /usr/bin/indent)

expect_error()
{
	local msg

	msg="$1"
	shift

	atf_check -s 'exit:1' \
	    -e "inline:$msg\n" \
	    "$indent" "$@"
}

atf_test_case 'option_unknown'
option_unknown_body()
{
	expect_error \
	    'indent: Command line: unknown option "-Z-unknown"' \
	    -Z-unknown
}

atf_test_case 'option_bool_trailing_garbage'
option_bool_trailing_garbage_body()
{
	expect_error \
	    'indent: Command line: unknown option "-bacchus"' \
	    -bacchus
}

atf_test_case 'option_int_missing_argument'
option_int_missing_argument_body()
{
	expect_error \
	    'indent: Command line: argument "x" to option "-ts" must be an integer' \
	    -tsx
}

atf_test_case 'option_profile_not_found'
option_profile_not_found_body()
{
	expect_error \
	    'indent: profile ./nonexistent: No such file or directory' \
	    -P./nonexistent
}

atf_test_case 'option_typedefs_not_found'
option_typedefs_not_found_body()
{
	expect_error \
	    'indent: cannot open file ./nonexistent' \
	    -U./nonexistent
}

atf_test_case 'option_tabsize_negative'
option_tabsize_negative_body()
{
	expect_error \
	    'indent: Command line: argument "-1" to option "-ts" must be between 1 and 80' \
	    -ts-1
}

atf_test_case 'option_tabsize_zero'
option_tabsize_zero_body()
{
	expect_error \
	    'indent: Command line: argument "0" to option "-ts" must be between 1 and 80' \
	    -ts0
}

atf_test_case 'option_tabsize_large'
option_tabsize_large_body()
{
	# Integer overflow, on both ILP32 and LP64 platforms.
	expect_error \
	    'indent: Command line: argument "81" to option "-ts" must be between 1 and 80' \
	    -ts81
}

atf_test_case 'option_tabsize_very_large'
option_tabsize_very_large_body()
{
	# Integer overflow, on both ILP32 and LP64 platforms.
	expect_error \
	    'indent: Command line: argument "3000000000" to option "-ts" must be between 1 and 80' \
	    -ts3000000000
}

atf_test_case 'option_indent_size_zero'
option_indent_size_zero_body()
{
	expect_error \
	    'indent: Command line: argument "0" to option "-i" must be between 1 and 80' \
	    -i0
}

atf_test_case 'option_int_trailing_garbage'
option_int_trailing_garbage_body()
{
	expect_error \
	    'indent: Command line: argument "3garbage" to option "-i" must be an integer' \
	    -i3garbage
}

atf_test_case 'option_cli_trailing_garbage'
option_cli_trailing_garbage_body()
{
	expect_error \
	    'indent: Command line: argument "3garbage" to option "-cli" must be numeric' \
	    -cli3garbage
}

atf_test_case 'option_npro_trailing_garbage'
option_npro_trailing_garbage_body()
{
	atf_check -s 'exit:1' \
	    -e 'inline:indent: Command line: unknown option "-npro-garbage"\n' \
	    "$indent" -npro-garbage
}

atf_test_case 'option_st_trailing_garbage'
option_st_trailing_garbage_body()
{
	atf_check -s 'exit:1' \
	    -e 'inline:indent: Command line: unknown option "-stdio"\n' \
	    "$indent" -stdio
}

atf_test_case 'option_version_trailing_garbage'
option_version_trailing_garbage_body()
{
	atf_check -s 'exit:1' \
	    -e 'inline:indent: Command line: unknown option "--version-dump"\n' \
	    "$indent" --version-dump
}

atf_test_case 'option_buffer_overflow'
option_buffer_overflow_body()
{
	opt='12345678123456781234567812345678'	# 32
	opt="$opt$opt$opt$opt$opt$opt$opt$opt"	# 256
	opt="$opt$opt$opt$opt$opt$opt$opt$opt"	# 2048
	opt="$opt$opt$opt$opt$opt$opt$opt$opt"	# 16384
	printf '%s\n' "-$opt" > indent.pro

	expect_error \
	    'indent: buffer overflow in indent.pro, starting with '\''-123456781'\''' \
	    -Pindent.pro
}

atf_test_case 'option_special_missing_param'
option_special_missing_param_body()
{
	expect_error \
	    'indent: Command line: ``-cli'\'\'' requires an argument' \
	    -cli

	expect_error \
	    'indent: Command line: ``-T'\'\'' requires an argument' \
	    -T

	expect_error \
	    'indent: Command line: ``-U'\'\'' requires an argument' \
	    -U
}

atf_test_case 'unterminated_comment_wrap'
unterminated_comment_wrap_body()
{
	echo '/*' > comment.c

	atf_check -s 'exit:1' \
	    -o 'inline:/*\n *\n' \
	    -e 'inline:error: Standard Input:2: Unterminated comment\n' \
	    "$indent" -st < comment.c
}

atf_test_case 'unterminated_comment_nowrap'
unterminated_comment_nowrap_body()
{
	echo '/*-' > comment.c

	atf_check -s 'exit:1' \
	    -o 'inline:/*-\n\n' \
	    -e 'inline:error: Standard Input:2: Unterminated comment\n' \
	    "$indent" -st < comment.c
}

atf_test_case 'in_place_wrong_backup'
in_place_wrong_backup_body()
{
	cat <<-\EOF > code.c
		int decl;
	EOF
	cp code.c code.c.orig

	# Due to the strange backup suffix '/subdir', indent tries to create
	# a file named 'code.c/subdir', but 'code.c' is already a regular
	# file, not a directory.
	atf_check -s 'exit:1' \
	    -e 'inline:indent: code.c/subdir: Not a directory\n' \
	    env SIMPLE_BACKUP_SUFFIX="/subdir" "$indent" code.c

	# Since there was an early error, the original file is kept as is.
	atf_check -o 'file:code.c.orig' \
	    cat code.c
}

atf_test_case 'argument_input_enoent'
argument_input_enoent_body()
{
	atf_check -s 'exit:1' \
	    -e 'inline:indent: ./nonexistent.c: No such file or directory\n' \
	    "$indent" ./nonexistent.c
}

atf_test_case 'argument_output_equals_input_name'
argument_output_equals_input_name_body()
{
	echo '/* comment */' > code.c

	atf_check -s 'exit:1' \
	    -e 'inline:indent: input and output files must be different\n' \
	    "$indent" code.c code.c
}

atf_test_case 'argument_output_equals_input_file'
argument_output_equals_input_file_body()
{
	echo '/* comment */' > code.c

	atf_check \
	    "$indent" code.c ./code.c

	# Oops, the file has become empty since the output is first emptied,
	# before reading any of the input.
	atf_check \
	    cat code.c
}

atf_test_case 'argument_output_enoent'
argument_output_enoent_body()
{
	expect_error \
	    'indent: subdir/nonexistent.c: No such file or directory' \
	    /dev/null subdir/nonexistent.c
}

atf_test_case 'argument_too_many'
argument_too_many_body()
{
	echo '/* comment */' > arg1.c

	expect_error \
	    'indent: too many arguments: arg3.c' \
	    arg1.c arg2.c arg3.c arg4.c
}

atf_test_case 'unexpected_end_of_file'
unexpected_end_of_file_body()
{
	echo 'struct{' > code.c

	expect_error \
	    'error: code.c:1: Stuff missing from end of file' \
	    code.c

	atf_check \
	    -o 'inline:struct {\n' \
	    cat code.c
}

atf_test_case 'unexpected_closing_brace_top_level'
unexpected_closing_brace_top_level_body()
{
	echo '}' > code.c

	expect_error \
	    'error: code.c:1: Statement nesting error' \
	    code.c
	atf_check \
	    -o 'inline:}\n' \
	    cat code.c
}

atf_test_case 'unexpected_closing_brace_decl'
unexpected_closing_brace_decl_body()
{
	echo 'int i = 3};' > code.c

	expect_error \
	    'error: code.c:1: Statement nesting error' \
	    code.c
	# Despite the error message, the original file got overwritten with a
	# best-effort rewrite of the code.
	atf_check \
	    -o 'inline:int		i = 3};\n' \
	    cat code.c
}

atf_test_case 'preprocessing_overflow'
preprocessing_overflow_body()
{
	cat <<-\EOF > code.c
		#if 1
		#if 2
		#if 3
		#if 4
		#if 5
		#if 6
		#endif 6
		#endif 5
		#endif 4
		#endif 3
		#endif 2
		#endif 1
		#endif too much
	EOF
	cat <<-\EOF > stderr.exp
		error: code.c:6: #if stack overflow
		error: code.c:12: Unmatched #endif
		error: code.c:13: Unmatched #endif
	EOF

	atf_check -s 'exit:1' \
	    -e 'file:stderr.exp' \
	    "$indent" code.c
}

atf_test_case 'preprocessing_unrecognized'
preprocessing_unrecognized_body()
{
	cat <<-\EOF > code.c
		#unknown
		# 3 "file.c"
		#elif 3
		#else
	EOF
	cat <<-\EOF > stderr.exp
		error: code.c:1: Unrecognized cpp directive
		error: code.c:2: Unrecognized cpp directive
		error: code.c:3: Unmatched #elif
		error: code.c:4: Unmatched #else
	EOF

	atf_check -s 'exit:1' \
	    -e 'file:stderr.exp' \
	    "$indent" code.c
}

atf_test_case 'unbalanced_parentheses_1'
unbalanced_parentheses_1_body()
{
	cat <<-\EOF > code.c
		int var =
		(
		;
		)
		;
	EOF
	cat <<-\EOF > stderr.exp
		error: code.c:3: Unbalanced parentheses
		warning: code.c:4: Extra ')'
	EOF

	atf_check -s 'exit:1' -e 'file:stderr.exp' \
	    "$indent" code.c
}

atf_test_case 'unbalanced_parentheses_2'
unbalanced_parentheses_2_body()
{
	# '({...})' is the GCC extension "Statement expression".
	cat <<-\EOF > code.c
		int var =
		(
		{
		1
		}
		)
		;
	EOF
	cat <<-\EOF > stderr.exp
		error: code.c:3: Unbalanced parentheses
		warning: code.c:6: Extra ')'
	EOF

	atf_check -s 'exit:1' -e 'file:stderr.exp' \
	    "$indent" code.c
}

atf_test_case 'unbalanced_parentheses_3'
unbalanced_parentheses_3_body()
{
	# '({...})' is the GCC extension "Statement expression".
	cat <<-\EOF > code.c
		int var =
		(
		1
		}
		;
	EOF
	cat <<-\EOF > stderr.exp
		error: code.c:4: Unbalanced parentheses
		error: code.c:4: Statement nesting error
	EOF

	atf_check -s 'exit:1' -e 'file:stderr.exp' \
	    "$indent" code.c
}

atf_test_case 'search_stmt_comment_segv'
search_stmt_comment_segv_body()
{
	# As of NetBSD indent.c 1.188 from 2021-10-30, indent crashes while
	# trying to format the following artificial code.

	printf '{if(expr\n)/*c*/;}\n' > code.c

	cat <<\EOF > code.exp
{
	if (expr
		)		/* c */
		;
}
EOF

	# TODO: actually produce code.exp instead of an assertion failure.
	atf_check -s 'signal' -o 'ignore' -e 'match:assert' \
	    "$indent" code.c -st
}

atf_test_case 'search_stmt_fits_in_one_line'
search_stmt_fits_in_one_line_body()
{
	# The comment is placed after 'if (0) ...', where it is processed
	# by search_stmt_comment. That function redirects the input buffer to
	# a temporary buffer that is not guaranteed to be terminated by '\n'.
	# Before NetBSD pr_comment.c 1.91 from 2021-10-30, this produced an
	# assertion failure in fits_in_one_line.
	cat <<EOF > code.c
int f(void)
{
	if (0)
		/* 0123456789012345678901 */;
}
EOF

	# Indent tries hard to make the comment fit to the 34-character line
	# length, but it is just not possible.
	cat <<EOF > expected.out
int
f(void)
{
	if (0)
		/*
		 * 0123456789012345678901
		  */ ;
}
EOF

	atf_check -o 'file:expected.out' \
	    "$indent" -l34 code.c -st
}


atf_test_case 'compound_literal'
compound_literal_body()
{
	# Test handling of compound literals (C99 6.5.2.5), as well as casts.

	cat <<EOF > code.c
void
function(void)
{
	origin =
	((int)
	((-1)*
	(struct point){0,0}
	)
	);
}
EOF

	sed '/^#/d' <<EOF > expected.out
void
function(void)
{
	origin =
		    ((int)
		     ((-1) *
		      (struct point){
# FIXME: the '{' is part of the expression, not a separate block.
		0, 0
# FIXME: the '}' is part of the expression, not a separate block.
	}
# FIXME: the ')' must be aligned with the corresponding '('.
	)
		    );
}
EOF
	sed '/^#/d' <<EOF > expected.err
# FIXME: The parentheses _are_ balanced, the '}' does not end the block.
error: code.c:9: Unbalanced parentheses
warning: code.c:10: Extra ')'
# FIXME: There is no line 12 in the input file.
warning: code.c:12: Extra ')'
EOF

	atf_check -s 'exit:1' -o 'file:expected.out' -e 'file:expected.err' \
	    "$indent" -nfc1 -ci12 code.c -st
}

atf_init_test_cases()
{
	atf_add_test_case 'option_unknown'
	atf_add_test_case 'option_bool_trailing_garbage'
	atf_add_test_case 'option_int_missing_argument'
	atf_add_test_case 'option_profile_not_found'
	atf_add_test_case 'option_buffer_overflow'
	atf_add_test_case 'option_typedefs_not_found'
	atf_add_test_case 'option_special_missing_param'
	atf_add_test_case 'option_tabsize_negative'
	atf_add_test_case 'option_tabsize_zero'
	atf_add_test_case 'option_tabsize_large'
	atf_add_test_case 'option_tabsize_very_large'
	atf_add_test_case 'option_int_trailing_garbage'
	atf_add_test_case 'option_cli_trailing_garbage'
	atf_add_test_case 'option_npro_trailing_garbage'
	atf_add_test_case 'option_st_trailing_garbage'
	atf_add_test_case 'option_version_trailing_garbage'
	atf_add_test_case 'option_indent_size_zero'
	atf_add_test_case 'unterminated_comment_wrap'
	atf_add_test_case 'unterminated_comment_nowrap'
	atf_add_test_case 'in_place_wrong_backup'
	atf_add_test_case 'argument_input_enoent'
	atf_add_test_case 'argument_output_equals_input_name'
	atf_add_test_case 'argument_output_equals_input_file'
	atf_add_test_case 'argument_output_enoent'
	atf_add_test_case 'argument_too_many'
	atf_add_test_case 'unexpected_end_of_file'
	atf_add_test_case 'unexpected_closing_brace_top_level'
	atf_add_test_case 'unexpected_closing_brace_decl'
	atf_add_test_case 'preprocessing_overflow'
	atf_add_test_case 'preprocessing_unrecognized'
	atf_add_test_case 'unbalanced_parentheses_1'
	atf_add_test_case 'unbalanced_parentheses_2'
	atf_add_test_case 'unbalanced_parentheses_3'
	atf_add_test_case 'search_stmt_comment_segv'
	atf_add_test_case 'search_stmt_fits_in_one_line'
	atf_add_test_case 'compound_literal'
}
