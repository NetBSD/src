#	$NetBSD: t_setjmp.sh,v 1.2 2025/04/28 15:41:25 martin Exp $
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

module_loaded=no
atf_test_case setjmp cleanup
setjmp_head()
{
	atf_set "descr" "Test setjmp(9)/longjmp(9)"
}
setjmp_body()
{
	case `uname -p` in
	powerpc*)
		atf_expect_fail "PR port-powerpc/59306:" \
		    " kernel longjmp(9) fails to make setjmp(9) return 1"
		;;
	sparc*)
		atf_expect_fail "PR port-sparc/59307:" \
		    " kernel longjmp(9) fails to make setjmp(9) return 1"
		;;
	vax)
		atf_expect_fail "PR port-vax/59308:" \
		    " kernel longjmp(9) fails to make setjmp(9) return 1"
		;;
	esac

	err=$( modstat -e 2>&1 )
	if [ $? -gt 0 ]; then
		atf_skip "${err##modstat:}"
	fi

	module_loaded="yes"
	modload "$(atf_get_srcdir)/setjmp_tester/setjmp_tester.kmod"
	atf_check -s exit:0 -o inline:'1\n' \
	    sysctl -n -w kern.setjmp_tester.test=1
}
setjmp_cleanup()
{
	if [ "${module_loaded}" != "no" ]; then
		modunload setjmp_tester
	fi
}

atf_init_test_cases()
{
	atf_add_test_case setjmp
}
