# $NetBSD: t_ifconfig.sh,v 1.1 2015/07/01 08:33:31 ozaki-r Exp $
#
# Copyright (c) 2015 The NetBSD Foundation, Inc.
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

RUMP_SERVER1=unix://./r1

RUMP_FLAGS=\
"-lrumpnet -lrumpnet_net -lrumpnet_netinet -lrumpnet_netinet6 -lrumpnet_shmif"

atf_test_case create_destroy cleanup
create_destroy_head()
{

	atf_set "descr" "tests of ifconfig create and destroy"
	atf_set "require.progs" "rump_server"
}

create_destroy_body()
{
	atf_check -s exit:0 rump_server ${RUMP_FLAGS} ${RUMP_SERVER1}

	export RUMP_SERVER=${RUMP_SERVER1}

	# Create and destroy (no address)
	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 destroy

	# Create and destroy (with an IPv4 address)
	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr shmbus
	atf_check -s exit:0 rump.ifconfig shmif0 192.168.0.1/24
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig shmif0 destroy

	# Create and destroy (with an IPv6 address)
	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr shmbus
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 fc00::1
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig shmif0 destroy

	unset RUMP_SERVER
}

create_destroy_cleanup()
{

	RUMP_SERVER=${RUMP_SERVER1} rump.halt
}

atf_init_test_cases()
{

	atf_add_test_case create_destroy
}
