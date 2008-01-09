# $NetBSD: t_rename.sh,v 1.1.4.2 2008/01/09 01:59:20 matt Exp $
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
# Verifies that the rename operation works (either by renaming entries or
# by moving them).
#

atf_test_case dots
dots_head() {
	atf_set "descr" "Tests that '.' and '..' cannot be renamed"
	atf_set "require.user" "root"
}
dots_body() {
	test_mount

	atf_check 'mkdir a' 0 null null
	atf_check 'mv a/. c' 1 null ignore
	atf_check 'mv a/.. c' 1 null ignore
	atf_check 'rmdir a' 0 null null

	test_unmount
}

atf_test_case crossdev
crossdev_head() {
	atf_set "descr" "Tests that cross-device renames do not work"
	atf_set "require.user" "root"
}
crossdev_body() {
	test_mount

	atf_check 'mkdir a' 0 null null
	atf_check '$(atf_get_srcdir)/h_tools rename a /var/tmp/a' \
	    1 null stderr
	atf_check 'grep "Cross-device link" ../stderr' 0 ignore null
	atf_check 'test -d a' 0 null null
	atf_check 'rmdir a' 0 null null

	test_unmount
}

atf_test_case basic
basic_head() {
	atf_set "descr" "Tests that basic renames work"
	atf_set "require.user" "root"
}
basic_body() {
	test_mount

	atf_check 'mkdir a' 0 null null
	atf_check 'mv a c' 0 null null
	atf_check 'test -d a' 1 null null
	atf_check 'test -d c' 0 null null
	atf_check 'rmdir c' 0 null null

	test_unmount
}

atf_test_case dotdot
dotdot_head() {
	atf_set "descr" "Tests that the '..' entry is properly updated" \
	                "during moves"
	atf_set "require.user" "root"
}
dotdot_body() {
	test_mount

	echo "Checking if the '..' entry is updated after moves"
	atf_check 'mkdir a' 0 null null
	atf_check 'mkdir b' 0 null null
	atf_check 'mv b a' 0 null null
	atf_check 'test -d a/b/../b' 0 null null
	atf_check 'test -d a/b/../../a' 0 null null
	eval $(stat -s a/b)
	[ ${st_nlink} = 2 ] || atf_fail "Incorrect number of links"
	eval $(stat -s a)
	[ ${st_nlink} = 3 ] || atf_fail "Incorrect number of links"
	atf_check 'rmdir a/b' 0 null null
	atf_check 'rmdir a' 0 null null

	echo "Checking if the '..' entry is correct after renames"
	atf_check 'mkdir a' 0 null null
	atf_check 'mkdir b' 0 null null
	atf_check 'mv b a' 0 null null
	atf_check 'mv a c' 0 null null
	atf_check 'test -d c/b/../b' 0 null null
	atf_check 'test -d c/b/../../c' 0 null null
	atf_check 'rmdir c/b' 0 null null
	atf_check 'rmdir c' 0 null null

	echo "Checking if the '..' entry is correct after multiple moves"
	atf_check 'mkdir a' 0 null null
	atf_check 'mkdir b' 0 null null
	atf_check 'mv b a' 0 null null
	atf_check 'mv a c' 0 null null
	atf_check 'mv c/b d' 0 null null
	atf_check 'test -d d/../c' 0 null null
	atf_check 'rmdir d' 0 null null
	atf_check 'rmdir c' 0 null null

	test_unmount
}

atf_test_case dir_to_emptydir
dir_to_emptydir_head() {
	atf_set "descr" "Tests that renaming a directory to override an" \
	                "empty directory works"
	atf_set "require.user" "root"
}
dir_to_emptydir_body() {
	test_mount

	atf_check 'mkdir a' 0 null null
	atf_check 'touch a/c' 0 null null
	atf_check 'mkdir b' 0 null null
	atf_check '$(atf_get_srcdir)/h_tools rename a b' 0 null null
	atf_check 'test -e a' 1 null null
	atf_check 'test -d b' 0 null null
	atf_check 'test -f b/c' 0 null null
	rm b/c
	rmdir b

	test_unmount
}

atf_test_case dir_to_fulldir
dir_to_fulldir_head() {
	atf_set "descr" "Tests that renaming a directory to override a" \
	                "non-empty directory fails"
	atf_set "require.user" "root"
}
dir_to_fulldir_body() {
	test_mount

	atf_check 'mkdir a' 0 null null
	atf_check 'touch a/c' 0 null null
	atf_check 'mkdir b' 0 null null
	atf_check 'touch b/d' 0 null null
	atf_check '$(atf_get_srcdir)/h_tools rename a b' 1 null stderr
	atf_check 'grep "Directory not empty" ../stderr' 0 ignore null
	atf_check 'test -d a' 0 null null
	atf_check 'test -f a/c' 0 null null
	atf_check 'test -d b' 0 null null
	atf_check 'test -f b/d' 0 null null
	rm a/c
	rm b/d
	rmdir a
	rmdir b

	test_unmount
}

atf_test_case dir_to_file
dir_to_file_head() {
	atf_set "descr" "Tests that renaming a directory to override a" \
	                "file fails"
	atf_set "require.user" "root"
}
dir_to_file_body() {
	test_mount

	atf_check 'mkdir a' 0 null null
	atf_check 'touch b' 0 null null
	atf_check '$(atf_get_srcdir)/h_tools rename a b' 1 null stderr
	atf_check 'grep "Not a directory" ../stderr' 0 ignore null
	atf_check 'test -d a' 0 null null
	atf_check 'test -f b' 0 null null
	rmdir a
	rm b

	test_unmount
}

atf_test_case file_to_dir
file_to_dir_head() {
	atf_set "descr" "Tests that renaming a file to override a" \
	                "directory fails"
	atf_set "require.user" "root"
}
file_to_dir_body() {
	test_mount

	atf_check 'touch a' 0 null null
	atf_check 'mkdir b' 0 null null
	atf_check '$(atf_get_srcdir)/h_tools rename a b' 1 null stderr
	atf_check 'grep "Is a directory" ../stderr' 0 ignore null
	atf_check 'test -f a' 0 null null
	atf_check 'test -d b' 0 null null
	rm a
	rmdir b

	test_unmount
}

atf_test_case kqueue
kqueue_head() {
	atf_set "descr" "Tests that moving and renaming files raise the" \
	                "correct kqueue events"
	atf_set "require.user" "root"
}
kqueue_body() {
	test_mount

	atf_check 'mkdir dir' 0 null null
	atf_check 'touch dir/a' 0 null null
	echo 'mv dir/a dir/b' | kqueue_monitor 2 dir dir/a
	kqueue_check dir/a NOTE_RENAME
	kqueue_check dir NOTE_WRITE
	atf_check 'rm dir/b' 0 null null
	atf_check 'rmdir dir' 0 null null

	atf_check 'mkdir dir' 0 null null
	atf_check 'touch dir/a' 0 null null
	atf_check 'touch dir/b' 0 null null
	echo 'mv dir/a dir/b' | kqueue_monitor 3 dir dir/a dir/b
	kqueue_check dir/a NOTE_RENAME
	kqueue_check dir NOTE_WRITE
	kqueue_check dir/b NOTE_DELETE
	atf_check 'rm dir/b' 0 null null
	atf_check 'rmdir dir' 0 null null

	atf_check 'mkdir dir1' 0 null null
	atf_check 'mkdir dir2' 0 null null
	atf_check 'touch dir1/a' 0 null null
	echo 'mv dir1/a dir2/a' | kqueue_monitor 3 dir1 dir1/a dir2
	kqueue_check dir1/a NOTE_RENAME
	kqueue_check dir1 NOTE_WRITE
	kqueue_check dir2 NOTE_WRITE
	atf_check 'rm dir2/a' 0 null null
	atf_check 'rmdir dir1' 0 null null
	atf_check 'rmdir dir2' 0 null null

	test_unmount
}

atf_init_test_cases() {
	. $(atf_get_srcdir)/../h_funcs.subr
	. $(atf_get_srcdir)/h_funcs.subr

	atf_add_test_case dots
	atf_add_test_case crossdev
	atf_add_test_case basic
	atf_add_test_case dotdot
	atf_add_test_case dir_to_emptydir
	atf_add_test_case dir_to_fulldir
	atf_add_test_case dir_to_file
	atf_add_test_case file_to_dir
	atf_add_test_case kqueue
}
