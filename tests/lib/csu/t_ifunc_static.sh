# $NetBSD: t_ifunc_static.sh,v 1.1 2018/03/09 20:20:47 joerg Exp $
#
# Copyright (c) 2018 The NetBSD Foundation, Inc.
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

atf_test_case ifunc_static
ifunc_static_head()
{
	atf_set "descr" "Checks support for ifunc relocations in static binaries"
}
ifunc_static_body()
{
	case `uname -m` in
	i386|amd64|*ppc*|*sparc*|*arm*)
		;;
	*)
		atf_skip "ifunc is supposed only on ARM, i386, PowerPC, SPARC and x86-64"
		;;
	esac

	USE_IFUNC2=0 "$(atf_get_srcdir)/h_ifunc_static" 3735928559
	atf_check_equal $? 0
	USE_IFUNC2=0 "$(atf_get_srcdir)/h_ifunc_static" 3203391149
	atf_check_equal $? 1
	USE_IFUNC2=1 "$(atf_get_srcdir)/h_ifunc_static" 3735928559
	atf_check_equal $? 1
	USE_IFUNC2=1 "$(atf_get_srcdir)/h_ifunc_static" 3203391149
	atf_check_equal $? 0
}

atf_init_test_cases()
{
	atf_add_test_case ifunc_static
}
