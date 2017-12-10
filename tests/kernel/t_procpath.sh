# $NetBSD: t_procpath.sh,v 1.1 2017/12/10 15:37:54 christos Exp $
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

HELPER=$(atf_get_srcdir)/h_getprocpath
INTERP=$(atf_get_srcdir)/h_interpreter

atf_test_case absolute_proc
absolute_proc_head()
{
	atf_set "descr" "Test absolute process argv0"
}
absolute_proc_body()
{
	atf_check -s exit:0 -o "inline:${HELPER}\n" -e "inline:" \
		${HELPER} -1
}

atf_test_case interpeter_proc
interpreter_proc_head()
{
	atf_set "descr" "Test interpreter proc contains interpreter"
}
interpreter_proc_body()
{
	atf_check -s exit:0 -o "inline:/bin/sh\n" -e "inline:" \
		${INTERP} interpreter ${HELPER}
}

atf_test_case relative_proc
relative_proc_head()
{
	atf_set "descr" "Test that masking the trapped signal get reset"
}
relative_proc_body()
{
	atf_check -s exit:0 -o "inline:" -e "inline:" \
		${INTERP} dot ${HELPER}
}

atf_init_test_cases()
{
	atf_add_test_case absolute_proc
	atf_add_test_case interpreter_proc
	atf_add_test_case relative_proc
}
