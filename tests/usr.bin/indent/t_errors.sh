#! /bin/sh
# $NetBSD: t_errors.sh,v 1.4 2021/10/17 18:13:00 rillig Exp $
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
nl='
'

expect_error()
{
	local msg

	msg="$1"
	shift

	atf_check -s 'exit:1' \
	    -e "inline:$msg$nl" \
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

atf_test_case 'option_int_missing_parameter'
option_int_missing_parameter_body()
{
	expect_error \
	    'indent: Command line: option "-ts" requires an integer parameter' \
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
	    'indent: Command line: option "-ts" requires an integer parameter' \
	    -ts-1
}

atf_test_case 'option_tabsize_zero'
option_tabsize_zero_body()
{
	expect_error \
	    'indent: Command line: invalid argument "0" for option "-ts"' \
	    -ts0
}

atf_test_case 'option_tabsize_large'
option_tabsize_large_body()
{
	# Integer overflow, on both ILP32 and LP64 platforms.
	expect_error \
	    'indent: Command line: invalid argument "81" for option "-ts"' \
	    -ts81
}

atf_test_case 'option_tabsize_very_large'
option_tabsize_very_large_body()
{
	# Integer overflow, on both ILP32 and LP64 platforms.
	expect_error \
	    'indent: Command line: invalid argument "3000000000" for option "-ts"' \
	    -ts3000000000
}

atf_test_case 'option_indent_size_zero'
option_indent_size_zero_body()
{
	expect_error \
	    'indent: Command line: invalid argument "0" for option "-i"' \
	    -i0
}

atf_test_case 'option_int_trailing_garbage'
option_int_trailing_garbage_body()
{
	expect_error \
	    'indent: Command line: invalid argument "3garbage" for option "-i"' \
	    -i3garbage
}

atf_test_case 'option_buffer_overflow'
option_buffer_overflow_body()
{
	opt='12345678123456781234567812345678'	# 32
	opt="$opt$opt$opt$opt$opt$opt$opt$opt"	# 256
	opt="$opt$opt$opt$opt$opt$opt$opt$opt"	# 2048
	opt="$opt$opt$opt$opt$opt$opt$opt$opt"	# 16384
	printf '%s\n' "-$opt" > indent.pro

	# TODO: The call to 'diag' should be replaced with 'errx'.
	expect_error \
	    'Error@1: buffer overflow in indent.pro, starting with '\''-123456781'\''' \
	    -Pindent.pro
}

atf_test_case 'option_special_missing_param'
option_special_missing_param_body()
{
	# TODO: Write '-cli' instead of only 'cli'.
	expect_error \
	    'indent: Command line: ``cli'\'\'' requires a parameter' \
	    -cli

	expect_error \
	    'indent: Command line: ``T'\'\'' requires a parameter' \
	    -T

	expect_error \
	    'indent: Command line: ``U'\'\'' requires a parameter' \
	    -U
}

atf_test_case 'unterminated_comment'
unterminated_comment_body()
{
	echo '/*' > comment.c

	atf_check -s 'exit:1' \
	    -o 'inline:/*'"$nl"' *'"$nl" \
	    -e 'inline:/**INDENT** Error@2: Unterminated comment */'"$nl" \
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
	    -e 'inline:indent: code.c/subdir: Not a directory'"$nl" \
	    env SIMPLE_BACKUP_SUFFIX="/subdir" "$indent" code.c

	# Since there was an early error, the original file is kept as is.
	atf_check -o 'file:code.c.orig' \
	    cat code.c
}

atf_test_case 'argument_input_enoent'
argument_input_enoent_body()
{
	atf_check -s 'exit:1' \
	    -e 'inline:indent: ./nonexistent.c: No such file or directory'"$nl" \
	    "$indent" ./nonexistent.c
}

atf_test_case 'argument_output_equals_input_name'
argument_output_equals_input_name_body()
{
	echo '/* comment */' > code.c

	atf_check -s 'exit:1' \
	    -e 'inline:indent: input and output files must be different'"$nl" \
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
	    'indent: unknown parameter: arg3.c' \
	    arg1.c arg2.c arg3.c arg4.c
}

atf_test_case 'unexpected_end_of_file'
unexpected_end_of_file_body()
{
	echo 'struct{' > code.c

	expect_error \
	    'Error@1: Stuff missing from end of file' \
	    code.c

	atf_check \
	    -o 'inline:struct {'"$nl" \
	    cat code.c
}

atf_test_case 'unexpected_closing_brace_top_level'
unexpected_closing_brace_top_level_body()
{
	echo '}' > code.c

	expect_error \
	    'Error@1: Statement nesting error' \
	    code.c
	atf_check \
	    -o 'inline:}'"$nl" \
	    cat code.c
}

atf_test_case 'unexpected_closing_brace_decl'
unexpected_closing_brace_decl_body()
{
	echo 'int i = 3};' > code.c

	expect_error \
	    'Error@1: Statement nesting error' \
	    code.c
	# Despite the error message, the original file got overwritten with a
	# best-effort rewrite of the code.
	atf_check \
	    -o 'inline:int		i = 3};'"$nl" \
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
		Error@6: #if stack overflow
		Error@12: Unmatched #endif
		Error@13: Unmatched #endif
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
		Error@1: Unrecognized cpp directive
		Error@2: Unrecognized cpp directive
		Error@3: Unmatched #elif
		Error@4: Unmatched #else
	EOF

	atf_check -s 'exit:1' \
	    -e 'file:stderr.exp' \
	    "$indent" code.c
}

atf_init_test_cases()
{
	atf_add_test_case 'option_unknown'
	atf_add_test_case 'option_bool_trailing_garbage'
	atf_add_test_case 'option_int_missing_parameter'
	atf_add_test_case 'option_profile_not_found'
	atf_add_test_case 'option_buffer_overflow'
	atf_add_test_case 'option_typedefs_not_found'
	atf_add_test_case 'option_special_missing_param'
	atf_add_test_case 'option_tabsize_negative'
	atf_add_test_case 'option_tabsize_zero'
	atf_add_test_case 'option_tabsize_large'
	atf_add_test_case 'option_tabsize_very_large'
	atf_add_test_case 'option_int_trailing_garbage'
	atf_add_test_case 'option_indent_size_zero'
	atf_add_test_case 'unterminated_comment'
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
}
