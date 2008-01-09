# $NetBSD: t_id_gen.sh,v 1.1.4.2 2008/01/09 01:59:15 matt Exp $
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

atf_test_case main
main_head() {
	atf_set "descr" "Verifies that node identifiers and generation" \
	                "numbers are assigned correctly"
	atf_set "require.user" "root"
}
main_body() {
	test_mount

	echo "Creating directory with node number = 3"
	atf_check 'mkdir a' 0 null null
	eval $(stat -s a | sed -e 's|st_|ost_|g') || atf_fail "stat failed"
	ofhsum=$($(atf_get_srcdir)/h_tools getfh a | md5) || \
	    atf_fail "Failed to calculate file handle"
	[ ${ost_ino} -eq 3 ] || atf_fail "Node number is not 3"

	echo "Deleting the directory"
	atf_check 'rmdir a' 0 null null

	echo "Creating another directory to reuse node number 3"
	atf_check 'mkdir b' 0 null null
	eval $(stat -s b) || atf_fail "stat failed"
	fhsum=$($(atf_get_srcdir)/h_tools getfh b | md5) || \
	    atf_fail "Failed to calculate file handle"
	[ ${st_ino} -eq 3 ] || atf_fail "Node number is not 3"
	echo "Checking if file handle is different"
	[ ${ofhsum} != ${fhsum} ] || atf_fail "Handles are not different"

	test_unmount
}

atf_init_test_cases() {
	. $(atf_get_srcdir)/../h_funcs.subr
	. $(atf_get_srcdir)/h_funcs.subr

	atf_add_test_case main
}
