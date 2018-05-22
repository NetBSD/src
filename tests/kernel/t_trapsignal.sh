# $NetBSD: t_trapsignal.sh,v 1.3 2018/05/22 04:32:56 kamil Exp $
#
# Copyright (c) 2017 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Christos Zoulas.
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

HELPER=$(atf_get_srcdir)/h_segv
atf_test_case segv_simple
segv_simple()
{
	atf_set "descr" "Test unhandled SIGSEGV with the right exit code"
}
segv_simple_body()
{
	atf_check -s signal:11 -o "inline:" -e "inline:" \
		${HELPER} segv recurse
}

atf_test_case segv_handle
segv_handle()
{
	atf_set "descr" "Test handled SIGSEGV traps call the signal handler"
}
segv_handle_body()
{
	atf_check -s exit:0 -o "inline:" -e "inline:got 11\n" \
		${HELPER} segv handle
}

atf_test_case segv_mask
segv_mask()
{
	atf_set "descr" "Test that masking SIGSEGV get reset"
}
segv_mask_body()
{
	atf_check -s signal:11 -o "inline:" -e "inline:" \
		${HELPER} segv mask
}

atf_test_case segv_handle_mask
segv_handle_mask()
{
	atf_set "descr" "Test handled and masked SIGSEGV traps get reset"
}
segv_handle_mask_body()
{
	atf_check -s signal:11 -o "inline:" -e "inline:" \
		${HELPER} segv mask handle
}

atf_test_case segv_handle_recurse
segv_handle_recurse()
{
	atf_set "descr" "Test that receiving SIGSEGV in the handler resets"
}

segv_handle_recurse_body()
{
	atf_check -s signal:11 -o "inline:" -e "inline:got 11\n" \
		${HELPER} segv handle recurse
}

atf_test_case segv_ignore
segv_ignore()
{
	atf_set "descr" "Test ignored SIGSEGV trap with right exit code"
}

segv_ignore_body()
{
	atf_check -s signal:11 -o "inline:" -e "inline:" \
		${HELPER} segv ignore
}

atf_test_case trap_simple
trap_simple()
{
	atf_set "descr" "Test unhandled SIGTRAP with the right exit code"
}
trap_simple_body()
{
	atf_check -s signal:5 -o "inline:" -e "inline:" \
		${HELPER} trap recurse
}

atf_test_case trap_handle
trap_handle()
{
	atf_set "descr" "Test handled SIGTRAP traps call the signal handler"
}
trap_handle_body()
{
	atf_check -s exit:0 -o "inline:" -e "inline:got 5\n" \
		${HELPER} trap handle
}

atf_test_case trap_mask
trap_mask()
{
	atf_set "descr" "Test that masking the trapped SIGTRAP signal get reset"
}
trap_mask_body()
{
	atf_check -s signal:5 -o "inline:" -e "inline:" \
		${HELPER} trap mask
}

atf_test_case trap_handle_mask
trap_handle_mask()
{
	atf_set "descr" "Test handled and masked SIGTRAP traps get reset"
}
trap_handle_mask_body()
{
	atf_check -s signal:5 -o "inline:" -e "inline:" \
		${HELPER} trap mask handle
}

atf_test_case trap_handle_recurse
trap_handle_recurse()
{
	atf_set "descr" "Test that receiving SIGTRAP in the handler resets"
}

trap_handle_recurse_body()
{
	atf_check -s signal:5 -o "inline:" -e "inline:got 5\n" \
		${HELPER} trap handle recurse
}

atf_test_case trap_ignore
trap_ignore()
{
	atf_set "descr" "Test ignored trap with right exit code"
}

trap_ignore_body()
{
	atf_check -s signal:5 -o "inline:" -e "inline:" \
		${HELPER} trap ignore
}

atf_init_test_cases()
{
	atf_add_test_case segv_simple
	atf_add_test_case segv_handle
	atf_add_test_case segv_mask
	atf_add_test_case segv_handle_recurse
	atf_add_test_case segv_ignore

	atf_add_test_case trap_simple
	atf_add_test_case trap_handle
	atf_add_test_case trap_mask
	atf_add_test_case trap_handle_recurse
	atf_add_test_case trap_ignore
}
