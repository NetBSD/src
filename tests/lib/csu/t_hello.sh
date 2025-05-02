#	$NetBSD: t_hello.sh,v 1.2 2025/05/02 22:30:29 riastradh Exp $
#
# Copyright (c) 2025 The NetBSD Foundation, Inc.
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

checksupport()
{
	local prog

	prog=$1
	test -f "$(atf_get_srcdir)/${prog}" || atf_skip "not supported"
}

checkrun()
{
	local prog

	prog=$1
	atf_check -o inline:"${prog}: Hello, world! 42\n" \
	    "$(atf_get_srcdir)/${prog}"
}

cleanup()
{
	local prog

	prog=$1
	test -f "${prog}.core" || return 0
	readelf -rs "$(atf_get_srcdir)/${prog}"
	gdb -batch -ex bt -ex 'info registers' -ex disas \
	    "$(atf_get_srcdir)/${prog}" "${prog}.core"
}

atf_test_case dynamic cleanup
dynamic_head()
{
	atf_set "descr" "Test a dynamic executable"
}
dynamic_body()
{
	checksupport h_hello_dyn
	checkrun h_hello_dyn
}
dynamic_cleanup()
{
	cleanup h_hello_dyn
}

atf_test_case dynamicpie cleanup
dynamicpie_head()
{
	atf_set "descr" "Test a dynamic position-independent executable"
}
dynamicpie_body()
{
	checksupport h_hello_dynpie
	checkrun h_hello_dynpie
}
dynamicpie_cleanup()
{
	cleanup h_hello_dynpie
}

atf_test_case relr cleanup
relr_head()
{
	atf_set "descr" "Test a static PIE executable with RELR relocations"
}
relr_body()
{
	checksupport h_hello_relr
	case `uname -p` in
	i386|x86_64)
		# Actually csu missing RELR support for this, not ld.elf_so.
		atf_expect_fail "PR bin/59360: ld.elf_so(8):" \
		    " missing RELR support"
		;;
	*)	atf_expect_fail "PR lib/59359: static pies are broken"
		;;
	esac
	checkrun h_hello_relr
}
relr_cleanup()
{
	cleanup h_hello_relr
}

atf_test_case static cleanup
static_head()
{
	atf_set "descr" "Test a static executable"
}
static_body()
{
	checksupport h_hello_sta
	checkrun h_hello_sta
}
static_cleanup()
{
	cleanup h_hello_sta
}

atf_test_case staticpie cleanup
staticpie_head()
{
	atf_set "descr" "Test a static position-independent executable"
}
staticpie_body()
{
	checksupport h_hello_stapie
	case `uname -p` in
	i386|x86_64)
		;;
	*)	atf_expect_fail "PR lib/59359: static pies are broken"
		;;
	esac
	checkrun h_hello_stapie
}
staticpie_cleanup()
{
	cleanup h_hello_stapie
}

atf_init_test_cases()
{
	atf_add_test_case dynamic
	atf_add_test_case dynamicpie
	atf_add_test_case relr
	atf_add_test_case static
	atf_add_test_case staticpie
}
