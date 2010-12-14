#	$NetBSD: t_cgd.sh,v 1.2 2010/12/14 17:48:31 pooka Exp $
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
basic_head()
{

	atf_set "descr" "Tests that encrypt/decrypt works"
}

basic_body()
{

	d=$(atf_get_srcdir)
	atf_check -s exit:0 \
	    rump_allserver -d key=/dev/dk,hostpath=dk.img,size=1m unix://csock

	export RUMP_SERVER=unix://csock
	atf_check -s exit:0 sh -c "echo 12345 | \
	    rump.cgdconfig -p cgd0 /dev/dk ${d}/paramsfile"
	atf_check -s exit:0 -e ignore dd if=${d}/t_cgd rof=/dev/rcgd0d count=2
	atf_check -s exit:0 -e ignore dd if=${d}/t_cgd of=testfile count=2
	atf_check -s exit:0 -e ignore -o file:testfile \
	    dd rif=/dev/rcgd0d count=2
}

basic_cleanup()
{

	env RUMP_SERVER=unix://csock rump.halt
}

atf_test_case wrongpass
wrongpass_head()
{

	atf_set "descr" "Tests that wrong password does not give original " \
	    "plaintext"
}

wrongpass_body()
{

	d=$(atf_get_srcdir)
	atf_check -s exit:0 \
	    rump_allserver -d key=/dev/dk,hostpath=dk.img,size=1m unix://csock

	export RUMP_SERVER=unix://csock
	atf_check -s exit:0 sh -c "echo 12345 | \
	    rump.cgdconfig -p cgd0 /dev/dk ${d}/paramsfile"
	atf_check -s exit:0 -e ignore dd if=${d}/t_cgd rof=/dev/rcgd0d count=2

	# unconfig and reconfig cgd
	atf_check -s exit:0 rump.cgdconfig -u cgd0
	atf_check -s exit:0 sh -c "echo 54321 | \
	    rump.cgdconfig -p cgd0 /dev/dk ${d}/paramsfile"

	atf_check -s exit:0 -e ignore dd if=${d}/t_cgd of=testfile count=2
	atf_check -s exit:0 -e ignore -o not-file:testfile \
	    dd rif=/dev/rcgd0d count=2
}

atf_init_test_cases()
{

	atf_add_test_case basic
	atf_add_test_case wrongpass
}
