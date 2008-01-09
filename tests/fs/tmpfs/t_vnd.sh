# $NetBSD: t_vnd.sh,v 1.1.4.2 2008/01/09 01:59:25 matt Exp $
#
# Copyright (c) 2006, 2007 The NetBSD Foundation, Inc.
# All rights reserved.
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
# Verifies that vnd works with files stored in tmpfs.
#

atf_test_case basic
basic_head() {
	atf_set "descr" "Verifies that vnd works with files stored in tmpfs"
	atf_set "require.user" "root"
}
basic_body() {
	test_mount

	atf_check 'dd if=/dev/zero of=disk.img bs=1m count=10' 0 ignore ignore
	atf_check 'vnconfig /dev/vnd3 disk.img' 0 null null

	atf_check 'newfs /dev/rvnd3a' 0 ignore ignore

	atf_check 'mkdir mnt' 0 null null
	atf_check 'mount /dev/vnd3a mnt' 0 null null

	echo "Creating test files"
	for f in $(jot 100); do
		jot 1000 >mnt/${f} || atf_fail "Failed to create file ${f}"
	done

	echo "Verifying created files"
	for f in $(jot 100); do
		[ $(md5 mnt/${f} | cut -d ' ' -f 4) = \
		    53d025127ae99ab79e8502aae2d9bea6 ] || \
		    atf_fail "Invalid checksum for file ${f}"
	done

	atf_check 'umount mnt' 0 null null
	atf_check 'vnconfig -u /dev/vnd3' 0 null null

	test_unmount
}
basic_cleanup() {
	umount mnt 2>/dev/null 1>&2
	vnconfig -u /dev/vnd3 2>/dev/null 1>&2
}

atf_init_test_cases() {
	. $(atf_get_srcdir)/../h_funcs.subr
	. $(atf_get_srcdir)/h_funcs.subr

	atf_add_test_case basic
}
