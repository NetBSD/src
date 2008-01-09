# $NetBSD: t_symlink.sh,v 1.1.4.2 2008/01/09 01:59:23 matt Exp $
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
# Verifies that the symlink and readlink operations work.
#

atf_test_case file
file_head() {
	atf_set "descr" "Tests that symlinks to files work"
	atf_set "require.user" "root"
}
file_body() {
	test_mount

	atf_check 'touch a' 0 null null
	atf_check 'ln -s a b' 0 null null
	[ $(md5 b | cut -d ' ' -f 4) = d41d8cd98f00b204e9800998ecf8427e ] || \
	    atf_fail "Symlink points to an incorrect file"

	atf_check 'echo foo >a' 0 null null
	[ $(md5 b | cut -d ' ' -f 4) = d3b07384d113edec49eaa6238ad5ff00 ] || \
	    atf_fail "Symlink points to an incorrect file"

	test_unmount
}

atf_test_case exec
exec_head() {
	atf_set "descr" "Tests symlinking to a known system binary and" \
	                "executing it through the symlink"
	atf_set "require.user" "root"
}
exec_body() {
	test_mount

	atf_check 'touch b' 0 null null
	atf_check 'ln -s /bin/cp cp' 0 null null
	atf_check './cp b c' 0 null null
	atf_check 'test -f c' 0 null null

	test_unmount
}

atf_test_case dir
dir_head() {
	atf_set "descr" "Tests that symlinks to directories work"
	atf_set "require.user" "root"
}
dir_body() {
	test_mount

	atf_check 'mkdir d' 0 null null
	atf_check 'test -f d/foo' 1 null null
	atf_check 'test -f e/foo' 1 null null
	atf_check 'ln -s d e' 0 null null
	atf_check 'touch d/foo' 0 null null
	atf_check 'test -f d/foo' 0 null null
	atf_check 'test -f e/foo' 0 null null

	test_unmount
}

atf_test_case kqueue
kqueue_head() {
	atf_set "descr" "Tests that creating a symlink raises the" \
	                "appropriate kqueue events"
	atf_set "require.user" "root"
}
kqueue_body() {
	test_mount

	atf_check 'mkdir dir' 0 null null
	echo 'ln -s non-existent dir/a' | kqueue_monitor 1 dir
	kqueue_check dir NOTE_WRITE
	atf_check 'rm dir/a' 0 null null
	atf_check 'rmdir dir' 0 null null

	test_unmount
}

atf_init_test_cases() {
	. $(atf_get_srcdir)/../h_funcs.subr
	. $(atf_get_srcdir)/h_funcs.subr

	atf_add_test_case file
	atf_add_test_case exec
	atf_add_test_case dir
	atf_add_test_case kqueue
}
