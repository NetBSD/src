# $NetBSD: t_redir.sh,v 1.2 2016/02/23 14:21:37 christos Exp $
#
# Copyright (c) 2016 The NetBSD Foundation, Inc.
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

atf_test_case redir_in_case
redir_in_case_head() {
	atf_set "descr" "Tests that sh(1) allows plain redirections " \
	                "in case statements. (PR bin/48631)"
}
redir_in_case_body() {
	atf_check -s exit:0 -o empty -e empty \
	    ${TEST_SH} -c 'case x in (whatever) >foo;; esac'
}

atf_test_case redir_here_doc
redir_here_doc_head() {
	atf_set "descr" "Tests that sh(1) correctly processes 'here' doc " \
	                "input redirections"
}
redir_here_doc_body() {
	atf_check -s exit:0 -o match:'hello\\n' -e empty \
		"${TEST_SH}" -ec '{
					echo "cat <<EOF"
					echo '"'"'"hello\n"'"'"'
					echo "EOF"
				} | '"'${TEST_SH}'"' -e'
}

atf_init_test_cases() {
	atf_add_test_case redir_in_case
	atf_add_test_case redir_here_doc
}
