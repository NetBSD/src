# $NetBSD: t_mount.sh,v 1.1.4.2 2008/01/09 01:59:17 matt Exp $
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
# Verifies that an execution of mount and umount works correctly without
# causing errors and that the root node gets correct attributes.
# Also verifies command line parsing from mount_tmpfs.
#

atf_test_case plain
plain_head() {
	atf_set "descr" "Tests a mount and unmount without any options"
	atf_set "require.user" "root"
}
plain_body() {
	test_mount
	test_unmount
}

atf_test_case links
links_head() {
	atf_set "descr" "Tests that the mount point has two hard links"
	atf_set "require.user" "root"
}
links_body() {
	test_mount
	eval $(stat -s ${Mount_Point})
	[ ${st_nlink} = 2 ] || \
	    atf_fail "Root directory does not have two hard links"
	test_unmount
}

atf_test_case options
options_head() {
	atf_set "descr" "Tests the read-only mount option"
	atf_set "require.user" "root"
}
options_body() {
	test_mount -o ro
	mount | grep ${Mount_Point} | grep -q read-only || \
	    atf_fail "read-only option (ro) does not work"
	test_unmount
}

atf_test_case attrs
attrs_head() {
	atf_set "descr" "Tests that root directory attributes are set" \
	                "correctly"
	atf_set "require.user" "root"
}
attrs_body() {
	test_mount -o -u1000 -o -g100 -o -m755
	eval $(stat -s ${Mount_Point})
	[ ${st_uid} = 1000 ] || atf_fail "uid is incorrect"
	[ ${st_gid} = 100 ] || atf_fail "gid is incorrect"
	[ ${st_mode} = 040755 ] || atf_fail "mode is incorrect"
	test_unmount
}

atf_test_case negative
negative_head() {
	atf_set "descr" "Tests that negative values passed to to -s are" \
	                "handled correctly"
	atf_set "require.user" "root"
}
negative_body() {
	mkdir tmp
	test_mount -o -s-10
	test_unmount
}

atf_test_case large
large_head() {
	atf_set "descr" "Tests that extremely long values passed to -s" \
	                "are handled correctly"
	atf_set "require.user" "root"
}
large_body() {
	test_mount -o -s9223372036854775807
	test_unmount

	mkdir tmp
	atf_check "mount -t tmpfs -o -s9223372036854775808 tmpfs \
	    tmp" 1 null ignore
	atf_check "mount -t tmpfs -o -s9223372036854775808g tmpfs \
	    tmp" 1 null ignore
	rmdir tmp
}

atf_init_test_cases() {
	. $(atf_get_srcdir)/../h_funcs.subr
	. $(atf_get_srcdir)/h_funcs.subr

	atf_add_test_case plain
	atf_add_test_case options
	atf_add_test_case attrs
	atf_add_test_case negative
	atf_add_test_case large
}
