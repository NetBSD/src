# $NetBSD: t_redircloexec.sh,v 1.1 2016/03/13 18:55:12 christos Exp $
#
# Copyright (c) 2016 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Christos Zoulas.
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

mkhelper() {
	local name="$1"
	local fd="$2"
	shift 2
	(echo "#!${TEST_SH}"; echo "$@") > ./"$name"1
	(echo "#!${TEST_SH}"; echo "echo $name"2 ">&$fd") > ./"$name"2
	chmod +x ./"$name"1 ./"$name"2
}

runhelper() {
	local name="$1"
	shift

	"./$name"1
}

cleanhelper() {
	local name="$1"
	shift
	rm -f ./"$name"1 ./"$name"2
}

atf_test_case exec_redir_closed
exec_redir_closed_head() {
	atf_set "descr" "Tests that redirections created by exec are closed on exec"
}
exec_redir_closed_body() {
	mkhelper exec 6 "exec 6> out; echo exec1 >&6; ./exec2; exec 6>&-"
	atf_check -s exit:0 \
		-o empty \
		-e match:"./exec2: 6: Bad file descriptor" \
		./exec1
	cleanhelper exec
}

atf_test_case loop_redir_open
loop_redir_open_head() {
	atf_set "descr" "Tests that redirections in loops don't close on exec"
}
loop_redir_open_body() {
	mkhelper for 3 "for x in x; do ./for2; done 3>out"
	atf_check -s exit:0 \
		-o empty \
		-e empty \
		./for1
	cleanhelper for
}

atf_test_case compound_redir_open
compound_redir_open_head() {
	atf_set "descr" "Tests that redirections in compound statements don't close on exec"
}
compound_redir_open_body() {
	mkhelper comp 3 "{ ./comp2; } 3>out"
	atf_check -s exit:0 \
		-o empty \
		-e empty \
		./comp1
	cleanhelper comp
}

atf_test_case simple_redir_open
simple_redir_open_head() {
	atf_set "descr" "Tests that redirections in simple commands don't close on exec"
}
simple_redir_open_body() {
	mkhelper simp 6 "./simp2 6>out"
	atf_check -s exit:0 \
		-o empty \
		-e empty \
		./simp1
	cleanhelper simp
}

atf_init_test_cases() {
	atf_add_test_case exec_redir_closed
	atf_add_test_case loop_redir_open
	atf_add_test_case compound_redir_open
	atf_add_test_case simple_redir_open
}
