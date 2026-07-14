#	$NetBSD: t_pppoe_keepalive.sh,v 1.4 2026/07/14 05:05:21 yamaguchi Exp $
#
# Copyright (c) Internet Initiative Japan Inc.
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
AUTHNAME=foobar@baz.com
SECRET=oink
BUS=bus0
DEBUG=${DEBUG:-false}

atf_test_case pppoe_keepalive_reset cleanup
pppoe_keepalive_reset_head()
{

	atf_set "descr" "Test reconnection triggered by keepalive"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_keepalive_reset_body()
{
	export RUMP_PPPOE_KEEPALIVE_INTERVAL=1
	setup_pppoe_server_client $SERVER $CLIENT $BUS
	setup_auth_params chap $AUTHNAME $SECRET
	setup_ipcp_addrs   $SERVER_IP $CLIENT_IP

	export RUMP_SERVER=$CLIENT
	atf_pppoectl pppoe0  \
	    alive-interval=1   \
	    max-alive-missed=1 \
	    max-noreceive=0

	pppoe_connect

	export RUMP_SERVER=$SERVER
	atf_ifconfig shmif0 down

	# wait for no keepalive detaction
	T=$((RUMP_PPPOE_KEEPALIVE_INTERVAL * 3))
	echo "sleep PPPOE_KEEPALIVE_INTERVAL * 3 (${T}s)"
	sleep $T

	export RUMP_SERVER=$CLIENT
	wait_for "LCP" "starting"
	atf_check -s exit:0 -o match:'UP.*RUNNING' rump.ifconfig pppoe0

	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 down
	atf_ifconfig shmif0 up
	atf_ifconfig pppoe0 up

	# automatically reconnect
	echo "sleep \$PPPOE_RECON_PADTRCVD (${PPPOE_RECON_PADTRCVD}s)"
	sleep $PPPOE_RECON_PADTRCVD
	wait_for "IPCP" "opened"
}

pppoe_keepalive_reset_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case pppoe_keepalive_ifdown cleanup
pppoe_keepalive_ifdown_head()
{

	atf_set "descr" "Test for PP_IFDOWN flag"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_keepalive_ifdown_body()
{
	export RUMP_PPPOE_KEEPALIVE_INTERVAL=1
	setup_pppoe_server_client $SERVER $CLIENT $BUS
	setup_auth_params chap $AUTHNAME $SECRET
	setup_ipcp_addrs   $SERVER_IP $CLIENT_IP

	export RUMP_SERVER=$CLIENT
	atf_pppoectl pppoe0  \
	    alive-interval=1   \
	    max-alive-missed=1 \
	    max-noreceive=0

	atf_check -s exit:0 -o match:"ifdown: 0 -> 1" \
	    rump.sysctl -w net.sppp.pppoe0.ifdown=1

	pppoe_connect

	#
	# Test IFF_UP cleared after disconnect
	#
	echo "Test IFF_UP cleared during disconnection and reconnection"

	export RUMP_SERVER=$SERVER
	atf_ifconfig shmif0 down

	# wait for no keepalive detection
	T=$((RUMP_PPPOE_KEEPALIVE_INTERVAL * 3))
	echo "sleep PPPOE_KEEPALIVE_INTERVAL * 3 (${T}s)"
	sleep $T

	export RUMP_SERVER=$CLIENT
	wait_for "LCP" "starting"
	atf_check -s exit:0 -o not-match:'UP'      rump.ifconfig pppoe0
	atf_check -s exit:0 -o     match:'RUNNING' rump.ifconfig pppoe0

	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 down
	atf_ifconfig shmif0 up
	atf_ifconfig pppoe0 up

	# automatically reconnect
	echo "sleep \$PPPOE_RECON_PADTRCVD (${PPPOE_RECON_PADTRCVD}s)"
	sleep $PPPOE_RECON_PADTRCVD
	wait_for "IPCP" "opened"
	atf_check -s exit:0 -o match:'UP.*RUNNING' rump.ifconfig pppoe0

	#
	# Test for taking interface down during if_down() and if_up()
	#
	echo "Test for taking interface down during if_down() and if_up()"

	export RUMP_SERVER=$SERVER
	atf_ifconfig shmif0 down

	# wait for no keepalive detection
	T=$((RUMP_PPPOE_KEEPALIVE_INTERVAL * 3))
	echo "sleep PPPOE_KEEPALIVE_INTERVAL * 3 (${T}s)"
	sleep $T
	export RUMP_SERVER=$CLIENT
	wait_for "LCP" "starting"

	atf_check -s exit:0 -o not-match:'UP'      rump.ifconfig pppoe0
	atf_check -s exit:0 -o     match:'RUNNING' rump.ifconfig pppoe0

	atf_ifconfig pppoe0 down

	atf_check -s exit:0 -o not-match:'UP'      rump.ifconfig pppoe0
	atf_check -s exit:0 -o not-match:'RUNNING' rump.ifconfig pppoe0

	# restart server
	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 down
	atf_ifconfig shmif0 up
	atf_ifconfig pppoe0 up

	echo "sleep \$PPPOE_RECON_PADTRCVD (${PPPOE_RECON_PADTRCVD}s)"
	sleep $PPPOE_RECON_PADTRCVD

	# pppoe0 should NOT reconnect after `ifconfig pppoe0 down`
	export RUMP_SERVER=$CLIENT
	wait_for "LCP" "initial"
	atf_check -s exit:0 -o not-match:'UP'      rump.ifconfig pppoe0
	atf_check -s exit:0 -o not-match:'RUNNING' rump.ifconfig pppoe0
}

pppoe_keepalive_ifdown_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case pppoe_keepalive_reset
	atf_add_test_case pppoe_keepalive_ifdown
}
