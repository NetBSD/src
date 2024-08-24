# $NetBSD: t_shmif.sh,v 1.1.2.2 2024/08/24 16:42:25 martin Exp $
#
# Copyright (c) 2024 Internet Initiative Japan Inc.
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
RUMP_SERVER2=unix://./r2

TIMEOUT=3

atf_test_case shmif_linkstate cleanup
shmif_linkstate_head()
{
	atf_set "descr" "tests of ifconfig media on shmif"
	atf_set "require.progs" "rump_server"
}

shmif_linkstate_body()
{
	local auto="Ethernet autoselect"
	local none="Ethernet none"

	rump_server_start $RUMP_SERVER1
	rump_server_add_iface $RUMP_SERVER1 shmif0 bus1

	export RUMP_SERVER=$RUMP_SERVER1
	# After ifconfig linkstr, the state becomes UP
	atf_check -o match:'linkstate: up' \
	          -o match:"media: $auto" \
	          -o not-match:"<UP" rump.ifconfig -v shmif0
	atf_check rump.ifconfig shmif0 up
	atf_check -o match:'linkstate: up' \
	          -o match:"media: $auto" \
	          -o match:"<UP" rump.ifconfig -v shmif0
	# ifconfig media none makes the state DOWN
	atf_check rump.ifconfig shmif0 media none
	atf_check -o match:'linkstate: down' \
	          -o match:"media: $none" \
	          -o match:"<UP" rump.ifconfig -v shmif0
	# ifconfig media auto makes the state UP
	atf_check rump.ifconfig shmif0 media auto
	atf_check -o match:'linkstate: up' \
	          -o match:"media: $auto" \
	          -o match:"<UP" rump.ifconfig -v shmif0
	atf_check rump.ifconfig shmif0 down
	atf_check -o match:'linkstate: up' \
	          -o match:"media: $auto" \
	          -o not-match:"<UP" rump.ifconfig -v shmif0
	# After ifconfig -linkstr, the state becomes UNKNOWN
	atf_check rump.ifconfig shmif0 -linkstr
	atf_check -o match:'linkstate: unknown' \
	          -o match:"media: $auto" \
	          -o not-match:"<UP" rump.ifconfig -v shmif0

	rump_server_destroy_ifaces
}

shmif_linkstate_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case shmif_linkstate_down cleanup
shmif_linkstate_down_head()
{
	atf_set "descr" "tests of behaviors of down shmif"
	atf_set "require.progs" "rump_server"
}

shmif_linkstate_down_body()
{

	rump_server_start $RUMP_SERVER1
	rump_server_start $RUMP_SERVER2
	rump_server_add_iface $RUMP_SERVER1 shmif0 bus1
	rump_server_add_iface $RUMP_SERVER2 shmif0 bus1

	export RUMP_SERVER=$RUMP_SERVER1
	atf_check rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check rump.ifconfig shmif0 10.0.0.1/24 up
	export RUMP_SERVER=$RUMP_SERVER2
	atf_check rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check rump.ifconfig shmif0 10.0.0.2/24 up

	export RUMP_SERVER=$RUMP_SERVER1
	atf_check -o ignore rump.ping -c 1 -w $TIMEOUT 10.0.0.2

	atf_check rump.ifconfig shmif0 media none
	atf_check -o match:'linkstate: down' rump.ifconfig -v shmif0

	# shmif doesn't send any packets on link down
	atf_check -s not-exit:0 -o ignore rump.ping -c 1 -w $TIMEOUT 10.0.0.2

	atf_check rump.ifconfig shmif0 media auto
	atf_check -o match:'linkstate: up' rump.ifconfig -v shmif0

	atf_check -o ignore rump.ping -c 1 -w $TIMEOUT 10.0.0.2

	rump_server_destroy_ifaces
}

shmif_linkstate_down_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case shmif_linkstate
	atf_add_test_case shmif_linkstate_down
}
