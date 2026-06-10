#	$NetBSD: t_pppoe.sh,v 1.34 2026/06/10 05:22:57 yamaguchi Exp $
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

SERVER=unix://pppoe_server
CLIENT=unix://pppoe_client

SERVER_IP=10.3.3.1
CLIENT_IP=10.3.3.3
SERVER_IP6=fc00::1
CLIENT_IP6=fc00::3
AUTHNAME=foobar@baz.com
SECRET=oink
BUS=bus0
TIMEOUT=3
WAITTIME=10
DEBUG=${DEBUG:-false}

atf_test_case pppoe_create_destroy cleanup
pppoe_create_destroy_head()
{

	atf_set "descr" "Test creating/destroying pppoe interfaces"
	atf_set "require.progs" "rump_server"
}

pppoe_create_destroy_body()
{

	rump_server_start $CLIENT netinet6 pppoe

	test_create_destroy_common $CLIENT pppoe0 true
}

pppoe_create_destroy_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case pppoe_ncp cleanup
pppoe_ncp_head()
{
	atf_set "descr" "Test for Network Configuration Protocols (NCPs)"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_ncp_body()
{
	setup_pppoe_server_client $SERVER $CLIENT $BUS
	setup_auth_params chap $AUTHNAME $SECRET
	setup_ipcp_addrs   $SERVER_IP $CLIENT_IP
	setup_ipv6cp_addrs $SERVER_IP6 $CLIENT_IP6

	#
	# test of default settings
	#
	echo "Test of default settings"

	# ipcp & ipv6cp are enabled by default
	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o match:'ipcp: enable' \
	    -x "$HIJACKING pppoectl pppoe0"
	atf_check -s exit:0 -o match:'ipv6cp: enable' \
	    -x "$HIJACKING pppoectl pppoe0"
	
	pppoe_connect
	pppoe_disconnect_by_client

	#
	# Test the disabling IPCP
	#
	atf_pppoectl pppoe0 noipcp
	pppoe_connect_ncp "IPv6CP"
	wait_for "IPCP" "closed"
	pppoe_disconnect_by_client

	#
	# Test the enabling IPCP
	#
	atf_pppoectl pppoe0 ipcp
	pppoe_connect
	pppoe_disconnect_by_client

	#
	# Test the disabling IPv6CP
	#
	atf_pppoectl pppoe0 noipv6cp
	pppoe_connect_ncp "IPCP"
	wait_for "IPv6CP" "closed"
	pppoe_disconnect_by_client

	#
	# Test the enabling IPv6CP
	#
	atf_pppoectl pppoe0 ipv6cp
	pppoe_connect
	pppoe_disconnect_by_client
}

pppoe_ncp_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case pppoe_mtu cleanup
pppoe_mtu_head()
{

	atf_set "descr" "Test for mtu"
	atf_set "require.progs" "rump_server"
}

pppoe_mtu_body()
{
	setup_pppoe_server_client $SERVER $CLIENT $BUS
	setup_auth_params chap $AUTHNAME $SECRET
	setup_ipcp_addrs   $SERVER_IP $CLIENT_IP

	#
	# Test that the smaller MTU is preferred (1400 and 1450)
	#
	echo "Test MTU arbitration between 1400 and 1450"
	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 mtu 1400
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 mtu 1450

	pppoe_connect

	export RUMP_SERVER=$SERVER
	atf_check -s exit:0 -o match:'mtu 1400' rump.ifconfig pppoe0
	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o match:'mtu 1400' rump.ifconfig pppoe0

	#
	# Test that MTU applies up to peer's MTU without re-negotiation
	#
	echo "Test that MTU applies up to peer's MTU without re-negotiation"
	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 mtu 1470
	atf_check -s exit:0 -o match:'mtu 1450' rump.ifconfig pppoe0
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 mtu 1460
	atf_check -s exit:0 -o match:'mtu 1400' rump.ifconfig pppoe0

	#
	# Test that MTU takes effect upon LCP re-negotiation
	#
	echo "Test that MTU takes effect upon LCP re-negotiation"

	pppoe_disconnect_by_client
	pppoe_connect

	export RUMP_SERVER=$SERVER
	atf_check -s exit:0 -o match:'mtu 1460' rump.ifconfig pppoe0
	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o match:'mtu 1460' rump.ifconfig pppoe0

	#
	# Test that MTU reverts to the original value upon interface down
	#
	pppoe_disconnect_by_client

	export RUMP_SERVER=$SERVER
	atf_check -s exit:0 -o match:'mtu 1470' rump.ifconfig pppoe0
	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o match:'mtu 1460' rump.ifconfig pppoe0

	#
	# Test MTU boundary conditions (1492 vs 1493)
	#
	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 mtu 1492
	atf_check -s exit:0 -o ignore \
	    -e match:'SIOCSIFMTU: Invalid argument' \
	    rump.ifconfig pppoe0 mtu 1493
}

pppoe_mtu_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case pppoe_create_destroy
	atf_add_test_case pppoe_ncp
	atf_add_test_case pppoe_mtu
}
