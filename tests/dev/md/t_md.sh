#	$NetBSD: t_md.sh,v 1.1 2010/11/23 15:38:54 pooka Exp $
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

atf_test_case basic
basic()
{

	atf_set "descr" "Check that md can be created, read and written"
}

basic_body()
{

	RUMPSOCK=unix://commsock
	env RUMP_SP_SERVER=${RUMPSOCK} $(atf_get_srcdir)/h_mdserv &

	sleep 1 # XXX: wait for server to start

	export RUMP_SP_CLIENT=${RUMPSOCK}
	atf_check -s exit:0 -e ignore dd if=/bin/ls rof=/dev/rmd0d seek=100 count=10
	atf_check -s exit:0 -e ignore dd of=testfile rif=/dev/rmd0d skip=100 count=10
	atf_check -s exit:0 -e ignore -o file:testfile dd if=/bin/ls count=10
}

atf_init_test_cases()
{

	atf_add_test_case basic
}
