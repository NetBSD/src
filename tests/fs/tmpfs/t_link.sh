# $NetBSD: t_link.sh,v 1.1.4.2 2008/01/09 01:59:15 matt Exp $
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
# Verifies that the link operation works.
#

atf_test_case basic
basic_head() {
	atf_set "descr" "Verifies that the link operation works on files" \
	                "at the top directory"
	atf_set "require.user" "root"
}
basic_body() {
	test_mount

	atf_check 'touch a' 0 null null
	atf_check 'touch z' 0 null null
	eval $(stat -s a | sed -e 's|st_|sta_|g')
	eval $(stat -s z | sed -e 's|st_|stz_|g')
	test ${sta_ino} != ${stz_ino} || \
	    atf_fail "Node numbers are not different"
	test ${sta_nlink} -eq 1 || atf_fail "Number of links is incorrect"
	atf_check 'ln a b' 0 null null

	echo "Checking if link count is correct after links are created"
	eval $(stat -s a | sed -e 's|st_|sta_|g')
	eval $(stat -s b | sed -e 's|st_|stb_|g')
	test ${sta_ino} = ${stb_ino} || atf_fail "Node number changed"
	test ${sta_nlink} -eq 2 || atf_fail "Link count is incorrect"
	test ${stb_nlink} -eq 2 || atf_fail "Link count is incorrect"

	echo "Checking if link count is correct after links are deleted"
	atf_check 'rm a' 0 null null
	eval $(stat -s b | sed -e 's|st_|stb_|g')
	test ${stb_nlink} -eq 1 || atf_fail "Link count is incorrect"
	atf_check 'rm b' 0 null null

	test_unmount
}

atf_test_case subdirs
subdirs_head() {
	atf_set "descr" "Verifies that the link operation works if used" \
	                "in subdirectories"
	atf_set "require.user" "root"
}
subdirs_body() {
	test_mount

	atf_check 'touch a' 0 null null
	atf_check 'mkdir c' 0 null null
	atf_check 'ln a c/b' 0 null null

	echo "Checking if link count is correct after links are created"
	eval $(stat -s a | sed -e 's|st_|sta_|g')
	eval $(stat -s c/b | sed -e 's|st_|stb_|g')
	test ${sta_ino} = ${stb_ino} || atf_fail "Node number changed"
	test ${sta_nlink} -eq 2 || atf_fail "Link count is incorrect"
	test ${stb_nlink} -eq 2 || atf_fail "Link count is incorrect"

	echo "Checking if link count is correct after links are deleted"
	atf_check 'rm a' 0 null null
	eval $(stat -s c/b | sed -e 's|st_|stb_|g')
	test ${stb_nlink} -eq 1 || atf_fail "Link count is incorrect"
	atf_check 'rm c/b' 0 null null
	atf_check 'rmdir c' 0 null null

	test_unmount
}

atf_test_case kqueue
kqueue_head() {
	atf_set "descr" "Verifies that creating a link raises the correct" \
	                "kqueue events"
	atf_set "require.user" "root"
}
kqueue_body() {
	test_mount

	atf_check 'mkdir dir' 0 null null
	atf_check 'touch dir/a' 0 null null
	echo 'ln dir/a dir/b' | kqueue_monitor 2 dir dir/a
	kqueue_check dir/a NOTE_LINK
	kqueue_check dir NOTE_WRITE

	echo 'rm dir/a' | kqueue_monitor 2 dir dir/b
	# XXX According to the (short) kqueue(2) documentation, the following
	# should raise a NOTE_LINK but FFS raises a NOTE_DELETE...
	kqueue_check dir/b NOTE_LINK
	kqueue_check dir NOTE_WRITE
	atf_check 'rm dir/b' 0 null null
	atf_check 'rmdir dir' 0 null null

	test_unmount
}

atf_init_test_cases() {
	. $(atf_get_srcdir)/../h_funcs.subr
	. $(atf_get_srcdir)/h_funcs.subr

	atf_add_test_case basic
	atf_add_test_case subdirs
	atf_add_test_case kqueue
}
