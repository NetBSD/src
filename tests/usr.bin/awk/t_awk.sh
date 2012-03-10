# $NetBSD: t_awk.sh,v 1.2 2012/03/10 19:19:24 christos Exp $
#
# Copyright (c) 2012 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Christos Zoulas
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

awk=awk

atf_test_case single_char_rs

single_char_rs_head() {
	atf_set "descr" "Test awk(1) with single character RS"
}

single_char_rs_body() {
	atf_check \
		-o "inline:1\n2\n\n3\n\n\n4\n\n" \
		-x "echo 1a2aa3aaa4 | $awk 1 RS=a"
}

atf_test_case two_char_rs

two_char_rs_head() {
	atf_set "descr" "Test awk(1) with two characters RS"
}

two_char_rs_body() {
	atf_check \
		-o "inline:1\n2\n3\n4\n\n" \
		-x "echo 1ab2ab3ab4 | $awk 1 RS=ab"
}

atf_test_case single_char_regex_group_rs

single_char_regex_group_rs_head() {
	atf_set "descr" "Test awk(1) with single character regex group RS"
}

single_char_regex_group_rs_body() {
	atf_check \
		-o "inline:1\n2\n\n3\n\n\n4\n\n" \
		-x "echo 1a2aa3aaa4 | $awk 1 RS='[a]'"
}

atf_test_case two_char_regex_group_rs

two_char_regex_group_rs_head() {
	atf_set "descr" "Test awk(1) with two characters regex group RS"
}

two_char_regex_group_rs_body() {
	atf_check \
		-o "inline:1\n2\n\n3\n\n\n4\n\n" \
		-x "echo 1a2ab3aba4 | $awk 1 RS='[ab]'"
}

atf_test_case single_char_regex_star_rs

single_char_regex_star_rs_head() {
	atf_set "descr" "Test awk(1) with single character regex star RS"
}

single_char_regex_star_rs_body() {
	atf_check \
		-o "inline:1\n2\n3\n4\n\n" \
		-x "echo 1a2aa3aaa4 | $awk 1 RS='a*'"
}

atf_test_case two_char_regex_star_rs

two_char_regex_star_rs_head() {
	atf_set "descr" "Test awk(1) with two characters regex star RS"
}

two_char_regex_star_rs_body() {
	atf_check \
		-o "inline:1\n2\n3\n4\n\n" \
		-x "echo 1a2aa3aaa4 | $awk 1 RS='aa*'"
}

atf_test_case regex_two_star_rs

regex_two_star_rs_head() {
	atf_set "descr" "Test awk(1) with regex two star RS"
}

regex_two_star_rs_body() {
	atf_check \
		-o "inline:1\n2\n3\n4\n\n" \
		-x "echo 1a2ab3aab4 | $awk 1 RS='aa*b*'"
}

atf_test_case regex_or_1_rs

regex_or_1_rs_head() {
	atf_set "descr" "Test awk(1) with regex | case 1 RS"
}

regex_or_1_rs_body() {
	atf_check \
		-o "inline:1a\nc\n\n" \
		-x "echo 1abc | $awk 1 RS='abcde|b'"
}

atf_test_case regex_or_2_rs

regex_or_2_rs_head() {
	atf_set "descr" "Test awk(1) with regex | case 2 RS"
}

regex_or_2_rs_body() {
	atf_check \
		-o "inline:1a\ncdf2\n\n" \
		-x "echo 1abcdf2 | $awk 1 RS='abcde|b'"
}

atf_test_case regex_or_3_rs

regex_or_3_rs_head() {
	atf_set "descr" "Test awk(1) with regex | case 3 RS"
}

regex_or_3_rs_body() {
	atf_check \
		-o "inline:1\n\nf2\n\n" \
		-x "echo 1abcdebf2 | $awk 1 RS='abcde|b'"
}

atf_test_case regex_or_4_rs

regex_or_4_rs_head() {
	atf_set "descr" "Test awk(1) with regex | case 4 RS"
}

regex_or_4_rs_body() {
	atf_check \
		-o "inline:1\nbcdf2\n\n" \
		-x "echo 1abcdf2 | $awk 1 RS='abcde|a'"

}

atf_test_case regex_caret_1_rs

regex_caret_1_rs_head() {
	atf_set "descr" "Test awk(1) with regex ^ case 1 RS"
}

regex_caret_1_rs_body() {
	atf_check \
		-o "inline:\n1a2a3a\n\n" \
		-x "echo a1a2a3a | $awk 1 RS='^a'"

}

atf_test_case regex_caret_2_rs

regex_caret_2_rs_head() {
	atf_set "descr" "Test awk(1) with regex ^ case 2 RS"
}

regex_caret_2_rs_body() {
	atf_check \
		-o "inline:\naa1a2a\n\n" \
		-x "echo aaa1a2a | $awk 1 RS='^a'"

}

atf_test_case regex_dollar_1_rs

regex_dollar_1_rs_head() {
	atf_set "descr" "Test awk(1) with regex $ case 1 RS"
}

regex_dollar_1_rs_body() {
	atf_check \
		-o "inline:a1a2a3a\n\n" \
		-x "echo a1a2a3a | $awk 1 RS='a$'"

}

atf_test_case regex_dollar_2_rs

regex_dollar_2_rs_head() {
	atf_set "descr" "Test awk(1) with regex $ case 2 RS"
}

regex_dollar_2_rs_body() {
	atf_check \
		-o "inline:a1a2aaa\n\n" \
		-x "echo a1a2aaa | $awk 1 RS='a$'"

}

atf_test_case regex_reallocation_rs

regex_reallocation_rs_head() {
	atf_set "descr" "Test awk(1) with regex reallocation RS"
}

regex_reallocation_rs_body() {
	atf_check \
		-o "inline:a\na\na\na\na\na\na\na\na\na10000\n\n" \
		-x "jot -s a 10000 | $awk 'NR>1' RS='999[0-9]'"

}

atf_test_case empty_rs

empty_rs_head() {
	atf_set "descr" "Test awk(1) with empty RS"
}

empty_rs_body() {
	atf_check \
		-o "inline:foo\n" \
		-x "echo foo | $awk 1 RS=''"

}

atf_test_case newline_rs

newline_rs_head() {
	atf_set "descr" "Test awk(1) with newline RS"
}

newline_rs_body() {
	atf_check \
		-o "inline:r1f1:r1f2\nr2f1:r2f2\n" \
		-x "printf '\n\n\nr1f1\nr1f2\n\nr2f1\nr2f2\n\n\n' | $awk '{\$1=\$1}1' RS= OFS=:"
}

atf_init_test_cases() {
	atf_add_test_case single_char_rs
	atf_add_test_case two_char_rs
	atf_add_test_case single_char_regex_group_rs
	atf_add_test_case two_char_regex_group_rs
	atf_add_test_case two_char_regex_star_rs
	atf_add_test_case single_char_regex_star_rs
	atf_add_test_case regex_two_star_rs
	atf_add_test_case regex_or_1_rs
	atf_add_test_case regex_or_2_rs
	atf_add_test_case regex_or_3_rs
	atf_add_test_case regex_caret_1_rs
	atf_add_test_case regex_caret_2_rs
	atf_add_test_case regex_dollar_1_rs
	atf_add_test_case regex_dollar_2_rs
	atf_add_test_case regex_reallocation_rs
	atf_add_test_case empty_rs
	atf_add_test_case newline_rs
}
