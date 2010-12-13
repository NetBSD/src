#	$NetBSD: t_sp.sh,v 1.3 2010/12/13 13:39:42 pooka Exp $
#
# Copyright (c) 2010 The NetBSD Foundation, Inc.
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

atf_test_case basic cleanup
basic_head()
{

	atf_set "descr" "Check that basic remote server communication works"
}

basic_body()
{

	export RUMP_SERVER=unix://commsock

	atf_check -s exit:0 rump_server ${RUMP_SERVER}

	atf_check -s exit:0 $(atf_get_srcdir)/h_client/h_simplecli
}

basic_cleanup()
{

	docleanup
}

stresst=10
atf_test_case stress cleanup
stress_head()
{

	atf_set "descr" "Stress/robustness test (~${stresst}s)"
}

stress_body()
{

	export RUMP_SERVER=unix://commsock

	atf_check -s exit:0 rump_server ${RUMP_SERVER}

	atf_check -s exit:0 $(atf_get_srcdir)/h_client/h_stresscli ${stresst}
}

stress_cleanup()
{

	docleanup
}

docleanup()
{
	RUMP_SERVER=unix://commsock rump.halt
}

atf_init_test_cases()
{

	atf_add_test_case basic
	atf_add_test_case stress
}
