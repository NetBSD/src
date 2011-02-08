#	$NetBSD: t_cgd.sh,v 1.5.2.1 2011/02/08 16:20:08 bouyer Exp $
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

rawpart=`sysctl -n kern.rawpartition | tr '01234' 'abcde'`
rawcgd=/dev/rcgd0${rawpart}
cgdserver=\
"rump_server -lrumpvfs -lrumpkern_crypto -lrumpdev -lrumpdev_disk -lrumpdev_cgd"

atf_test_case basic cleanup
basic_head()
{

	atf_set "descr" "Tests that encrypt/decrypt works"
}

basic_body()
{

	d=$(atf_get_srcdir)
	atf_check -s exit:0 \
	    ${cgdserver} -d key=/dev/dk,hostpath=dk.img,size=1m unix://csock

	export RUMP_SERVER=unix://csock
	atf_check -s exit:0 sh -c "echo 12345 | \
	    rump.cgdconfig -p cgd0 /dev/dk ${d}/paramsfile"
	atf_check -s exit:0 -e ignore sh -c \
	    "dd if=${d}/t_cgd count=2 | rump.dd of=${rawcgd}"
	atf_check -s exit:0 -e ignore dd if=${d}/t_cgd of=testfile count=2
	atf_check -s exit:0 -e ignore -o file:testfile \
	    rump.dd if=${rawcgd} count=2
}

basic_cleanup()
{

	env RUMP_SERVER=unix://csock rump.halt
}

atf_test_case wrongpass cleanup
wrongpass_head()
{

	atf_set "descr" "Tests that wrong password does not give original " \
	    "plaintext"
}

wrongpass_body()
{

	d=$(atf_get_srcdir)
	atf_check -s exit:0 \
	    ${cgdserver} -d key=/dev/dk,hostpath=dk.img,size=1m unix://csock

	export RUMP_SERVER=unix://csock
	atf_check -s exit:0 sh -c "echo 12345 | \
	    rump.cgdconfig -p cgd0 /dev/dk ${d}/paramsfile"
	atf_check -s exit:0 -e ignore sh -c \
	    "dd if=${d}/t_cgd | rump.dd of=${rawcgd} count=2"

	# unconfig and reconfig cgd
	atf_check -s exit:0 rump.cgdconfig -u cgd0
	atf_check -s exit:0 sh -c "echo 54321 | \
	    rump.cgdconfig -p cgd0 /dev/dk ${d}/paramsfile"

	atf_check -s exit:0 -e ignore dd if=${d}/t_cgd of=testfile count=2
	atf_check -s exit:0 -e ignore -o not-file:testfile \
	    rump.dd if=${rawcgd} count=2
}

wrongpass_cleanup()
{

	env RUMP_SERVER=unix://csock rump.halt
}


atf_test_case non512 cleanup
non512_head()
{

	atf_set "descr" "Write a non-512 block to a raw cgd device"
}

non512_body()
{
	d=$(atf_get_srcdir)
	atf_check -s exit:0 \
	    ${cgdserver} -d key=/dev/dk,hostpath=dk.img,size=1m unix://csock

	export RUMP_SERVER=unix://csock
	atf_check -s exit:0 sh -c "echo 12345 | \
	    rump.cgdconfig -p cgd0 /dev/dk ${d}/paramsfile"

	atf_expect_fail "PR kern/44515"
	atf_check -s exit:0 -e ignore sh -c \
	    "echo die hard | rump.dd of=${rawcgd} bs=123 conv=sync"
}

non512_cleanup()
{
	env RUMP_SERVER=unix://csock rump.halt
}

atf_init_test_cases()
{

	atf_add_test_case basic
	atf_add_test_case wrongpass
	atf_add_test_case non512
}
