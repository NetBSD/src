#	$NetBSD: t_vether.sh,v 1.1 2020/09/29 19:41:48 roy Exp $
#
# Copyright (c) 2016 Internet Initiative Japan Inc.
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

SOCK_LOCAL=unix://commsock1
SOCK_REMOTE=unix://commsock2
BUS=bus1
IP4_LOCAL=10.0.0.1
IP4_VETHER=10.0.0.2
IP4_REMOTE=10.0.0.3
IP6_LOCAL=fc00::1
IP6_VETHER=fc00::2
IP6_REMOTE=fc00::3

DEBUG=${DEBUG:-false}
TIMEOUT=1

atf_test_case vether_create_destroy cleanup
vether_create_destroy_head()
{

	atf_set "descr" "tests of creation and deletion of vether interface"
	atf_set "require.progs" "rump_server"
}

vether_create_destroy_body()
{

	rump_server_fs_start $SOCK_LOCAL netinet6 vether

	test_create_destroy_common $SOCK_LOCAL vether0 true
}

vether_create_destroy_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case vether_stand_alone cleanup
vether_create_destroy_head()
{

	atf_set "descr" "tests of alone vether interface"
	atf_set "require.progs" "rump_server"
}

vether_stand_alone_body()
{

	rump_server_fs_start $SOCK_LOCAL netinet6 vether
	rump_server_fs_start $SOCK_REMOTE netinet6 vether

	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS

	export RUMP_SERVER=${SOCK_LOCAL}
	atf_check -s exit:0 rump.ifconfig shmif0 $IP4_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $IP6_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	export RUMP_SERVER=${SOCK_REMOTE}
	atf_check -s exit:0 rump.ifconfig shmif0 $IP4_REMOTE
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $IP6_REMOTE
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 $IP4_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -n -X $TIMEOUT -c 1 $IP6_LOCAL

	export RUMP_SERVER=${SOCK_LOCAL}
	atf_check -s exit:0 rump.ifconfig shmif0 $IP4_LOCAL delete
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $IP6_LOCAL delete
	rump_server_add_iface $SOCK_LOCAL vether0
	atf_check -s exit:0 rump.ifconfig vether0 $IP4_VETHER
	atf_check -s exit:0 rump.ifconfig vether0 inet6 $IP6_VETHER
	atf_check -s exit:0 rump.ifconfig vether0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	export RUMP_SERVER=${SOCK_REMOTE}
	# Cannot reach to an alone vether
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping6 -n -X $TIMEOUT -c 1 $IP6_VETHER
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping -n -w $TIMEOUT -c 1 $IP4_VETHER

	rump_server_destroy_ifaces
}

vether_stand_alone_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case vether_bridged cleanup
vether_bridged_head()
{

	atf_set "descr" "tests of alone vether interface"
	atf_set "require.progs" "rump_server"
}

vether_bridged_body()
{

	rump_server_fs_start $SOCK_LOCAL netinet6 vether bridge
	rump_server_fs_start $SOCK_REMOTE netinet6 vether

	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS

	export RUMP_SERVER=${SOCK_LOCAL}
	atf_check -s exit:0 rump.ifconfig shmif0 $IP4_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $IP6_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 up
	rump_server_add_iface $SOCK_LOCAL vether0
	atf_check -s exit:0 rump.ifconfig vether0 $IP4_VETHER
	atf_check -s exit:0 rump.ifconfig vether0 inet6 $IP6_VETHER
	atf_check -s exit:0 rump.ifconfig vether0 up

	rump_server_add_iface $SOCK_LOCAL bridge0
	atf_check -s exit:0 rump.ifconfig bridge0 up
	export LD_PRELOAD=/usr/lib/librumphijack.so
	atf_check -s exit:0 brconfig bridge0 add shmif0
	atf_check -s exit:0 brconfig bridge0 add vether0
	unset LD_PRELOAD

	atf_check -s exit:0 rump.ifconfig -w 10

	export RUMP_SERVER=${SOCK_REMOTE}
	atf_check -s exit:0 rump.ifconfig shmif0 $IP4_REMOTE
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $IP6_REMOTE
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 $IP4_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 $IP4_VETHER

	atf_check -s exit:0 -o ignore rump.ping6 -n -X $TIMEOUT -c 1 $IP6_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -n -X $TIMEOUT -c 1 $IP6_VETHER

	rump_server_destroy_ifaces
}

vether_bridged_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case vether_create_destroy
	atf_add_test_case vether_stand_alone
	atf_add_test_case vether_bridged
}
