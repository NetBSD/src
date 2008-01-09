# $NetBSD: t_remove.sh,v 1.1.4.2 2008/01/09 01:59:20 matt Exp $
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
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
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

#
# Verifies that the remove operation works.
#

atf_test_case single
single_head() {
	atf_set "descr" "Checks that file removal works"
	atf_set "require.user" "root"
}
single_body() {
	test_mount

	atf_check 'test -f a' 1 null null
	atf_check 'touch a' 0 null null
	atf_check 'test -f a' 0 null null
	atf_check 'rm a' 0 null null
	atf_check 'test -f a' 1 null null

	test_unmount
}

atf_test_case uchg
uchg_head() {
	atf_set "descr" "Checks that files with the uchg flag set cannot" \
	                "be removed"
	atf_set "require.user" "root"
}
uchg_body() {
	test_mount

	atf_check 'touch a' 0 null null
	atf_check 'chflags uchg a' 0 null null
	atf_check 'rm -f a' 1 null ignore
	atf_check 'chflags nouchg a' 0 null null
	atf_check 'rm a' 0 null null
	atf_check 'test -f a' 1 null null

	test_unmount
}

atf_test_case dot
dot_head() {
	atf_set "descr" "Checks that '.' cannot be removed"
	atf_set "require.user" "root"
}
dot_body() {
	test_mount

	atf_check 'mkdir a' 0 null null
	atf_check 'unlink a/.' 1 null ignore
	atf_check 'rmdir a' 0 null null

	test_unmount
}

atf_test_case kqueue
kqueue_head() {
	atf_set "descr" "Removes a file and checks the kqueue events" \
	                "raised"
	atf_set "require.user" "root"
}
kqueue_body() {
	test_mount

	atf_check 'mkdir dir' 0 null null
	atf_check 'touch dir/a' 0 null null
	echo 'rm dir/a' | kqueue_monitor 2 dir dir/a
	kqueue_check dir/a NOTE_DELETE
	kqueue_check dir NOTE_WRITE
	atf_check 'rmdir dir' 0 null null

	test_unmount
}

atf_init_test_cases() {
	. $(atf_get_srcdir)/../h_funcs.subr
	. $(atf_get_srcdir)/h_funcs.subr

	atf_add_test_case single
	atf_add_test_case uchg
	atf_add_test_case dot
	atf_add_test_case kqueue
}
