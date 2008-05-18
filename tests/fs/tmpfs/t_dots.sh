# $NetBSD: t_dots.sh,v 1.1.8.1 2008/05/18 12:36:01 yamt Exp $
#
# Copyright (c) 2005, 2006, 2007 The NetBSD Foundation, Inc.
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

atf_test_case topdir
topdir_head() {
	atf_set "descr" "Verifies that looking up '.' and '..' in" \
	                "top-level directories works"
	atf_set "require.user" "root"
}
topdir_body() {
	test_mount

	atf_check 'mkdir a' 0 null null
	atf_check 'test -d ./a' 0 null null
	atf_check 'test -d a/../a' 0 null null

	test_unmount
}

atf_test_case nesteddir
nesteddir_head() {
	atf_set "descr" "Verifies that looking up '.' and '..' in" \
	                "top-level directories works"
	atf_set "require.user" "root"
}
nesteddir_body() {
	test_mount

	atf_check 'mkdir a' 0 null null
	atf_check 'mkdir a/b' 0 null null
	atf_check 'test -d a/b/../b' 0 null null
	atf_check 'test -d a/b/../../a' 0 null null

	test_unmount
}

atf_init_test_cases() {
	. $(atf_get_srcdir)/../h_funcs.subr
	. $(atf_get_srcdir)/h_funcs.subr

	atf_add_test_case topdir
	atf_add_test_case nesteddir
}
