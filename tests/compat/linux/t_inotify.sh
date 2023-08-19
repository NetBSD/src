# $NetBSD: t_inotify.sh,v 1.1 2023/08/19 22:56:44 christos Exp $
#
# Copyright (c) 2023 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Theodore Preduta.
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
#/

atf_test_case init
init_head() {
	atf_set "descr" "Tests inotify_init applies its flags correctly"
}

init_body() {
	h_ensure_emul_exists
	atf_check -s exit:0 "$(atf_get_srcdir)/h_inotify_init"
}

atf_test_case single_file
single_file_head() {
	atf_set "descr" \
		"Tests correct events are generated when a single file is watched"
}

single_file_body() {
	h_ensure_emul_exists
	atf_check -s exit:0 "$(atf_get_srcdir)/h_inotify_single_file"
}

atf_test_case directory
directory_head() {
	atf_set "descr" \
		"Tests correct events are generated when a directory is watched"
}

directory_body() {
	h_ensure_emul_exists
	atf_check -s exit:0 "$(atf_get_srcdir)/h_inotify_directory"
}

atf_test_case watch_change
watch_change_head() {
	atf_set "descr" \
		"Tests the watch descriptor can be modified"
}

watch_change_body() {
	h_ensure_emul_exists
	atf_check -s exit:0 "$(atf_get_srcdir)/h_inotify_watch_change"
}

atf_init_test_cases() {
	atf_add_test_case init
	atf_add_test_case directory
	atf_add_test_case single_file
	atf_add_test_case watch_change
}
