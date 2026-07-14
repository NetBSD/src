#	$NetBSD: t_pppoe_ondemand.sh,v 1.3 2026/07/14 05:05:21 yamaguchi Exp $
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
TIMEOUT=3

atf_test_case pppoe_ondemand cleanup
pppoe_ondemand_head()
{

	atf_set "descr" "Test dial-on-demand connection"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_ondemand_body()
{
	# A generous idle timeout to prevent accidental disconnects
	local t_idle=300

	export RUMP_PPPOE_KEEPALIVE_INTERVAL=1
	setup_pppoe_server_client $SERVER $CLIENT $BUS
	setup_auth_params chap $AUTHNAME $SECRET
	setup_ipcp_addrs   $SERVER_IP $CLIENT_IP

	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 link1
	atf_check -s exit:0 -o ignore \
	    rump.route add -inet default -ifp pppoe0 0.0.0.1

	pppoe_connect "expected-failure"

	# The interface should NOT become RUNNING
	# until the trigger packet is sent.
	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o     match:'UP'      rump.ifconfig pppoe0
	atf_check -s exit:0 -o not-match:'RUNNING' rump.ifconfig pppoe0
	wait_for "LCP" "initial"

	#
	# Test idle timer disconnection 3 times
	#  - 1st: Initial connection (post-config)
	#  - 2nd: After single idle timeout
	#  - 3rd: After multiple idle timeouts
	#
	local t=3
	for n in $(seq 1 3); do
		echo "Test disconnect triggered by idle timer (n=$n)"

		export RUMP_SERVER=$CLIENT

		# Set a generous idle timeout for `ifconfig -w 10`
		atf_pppoectl pppoe0 idle-timeout=$t_idle

		# Due to implementation constraints,
		# the first packet is dropped.
		atf_check -s not-exit:0 -o ignore -e ignore \
		    rump.ping -c 1 -w $TIMEOUT $SERVER_IP

		# The connection should be established
		atf_check -s exit:0 -o match:'UP.*RUNNING' \
		    rump.ifconfig pppoe0
		wait_for "IPCP" "opened"
		atf_ifconfig -w 10
		atf_check -s exit:0 -o ignore \
		    rump.ping -c 1 -w $TIMEOUT $SERVER_IP

		# Set a short timeout for testing
		atf_pppoectl pppoe0 idle-timeout=$t
		echo "Waiting for idle timeout disconnection ($t * 2s)"
		sleep $((t * 2))

		atf_check -s exit:0 -o     match:'UP'      rump.ifconfig pppoe0
		atf_check -s exit:0 -o not-match:'RUNNING' rump.ifconfig pppoe0
		wait_for "LCP" "initial"
	done

	#
	# Test session reset due to no echo reply
	#
	echo "Test session reset due to no echo reply"

	local n=1
	export RUMP_SERVER=$CLIENT
	# Set a generous idle timeout
	atf_pppoectl pppoe0 idle-timeout=$t_idle

	# Set keepalive parameters
	atf_pppoectl pppoe0     \
	    alive-interval=1    \
	    max-alive-missed=$n \
	    max-noreceive=0

	# Connect by the trigger packet
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP
	atf_check -s exit:0 -o match:'UP.*RUNNING' \
	    rump.ifconfig pppoe0
	wait_for "IPCP" "opened"
	atf_ifconfig -w 10
	atf_check -s exit:0 -o ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP

	# Stop all packet including keepalive
	export RUMP_SERVER=$SERVER
	atf_ifconfig shmif0 down

	# Waiting for missed keepalive packets
	T=$((RUMP_PPPOE_KEEPALIVE_INTERVAL * n * 3))
	echo "sleep PPPOE_KEEPALIVE_INTERVAL * $n pkts * 3 (${T}s)"
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
	atf_check -s exit:0 -o ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP

	#
	# Test session reset after PADT received
	#
	echo "Test session reset after PADT received"

	# Set a generous missed count to prevent accidental disconnects
	export RUMP_SERVER=$CLIENT
	atf_pppoectl pppoe0     \
	    alive-interval=1    \
	    max-alive-missed=99

	# Send PADT from server
	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 down

	export RUMP_SERVER=$CLIENT
	wait_for "LCP" "starting"
	atf_check -s exit:0 -o match:'UP.*RUNNING' rump.ifconfig pppoe0

	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 up

	# automatically reconnect
	T=$((PPPOE_RECON_PADTRCVD * 2))
	echo "sleep \$PPPOE_RECON_PADTRCVD * 2 (${T}s)"
	sleep $T
	wait_for "IPCP" "opened"
	atf_check -s exit:0 -o ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP
}

pppoe_ondemand_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case pppoe_ondemand_maxpadi cleanup
pppoe_ondemand_maxpadi_head()
{

	atf_set "descr" "Test for stop connection by PPPOE_DISC_MAXPADI"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_ondemand_maxpadi_body()
{
	# A generous idle timeout to prevent accidental disconnects
	local t_idle=300

	# Skipped by default as it involves a long sleep duration.
	if [ "$ATF_NET_IF_PPPOE_FULLTEST" != "yes" ]; then
		atf_skip "set ATF_NET_IF_PPPOE_FULLTEST=yes to run the test"
	fi

	# constants defined in if_pppoe.c
	local PPPOE_DISC_MAXPADI=4
	local PPPOE_DISC_TIMEOUT=5

	# Calculate total number of
	#`retry_wait = PPPOE_DISC_TIMEOUT * (1 + sc->sc_padi_retried);`
	local n=$((PPPOE_DISC_MAXPADI + 1))
	local sum=$((n / 2 * (n + 1)))
	local total_retry_wait=$((sum * PPPOE_DISC_TIMEOUT))

	export RUMP_PPPOE_KEEPALIVE_INTERVAL=1
	setup_pppoe_server_client $SERVER $CLIENT $BUS
	setup_auth_params chap $AUTHNAME $SECRET
	setup_ipcp_addrs   $SERVER_IP $CLIENT_IP

	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 link1
	atf_pppoectl pppoe0 idle-timeout=$t_idle
	atf_check -s exit:0 -o ignore \
	    rump.route add -inet default -ifp pppoe0 0.0.0.1

	pppoe_connect "expected-failure"

	# Stop PPPoE Server
	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 down

	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o     match:'UP'      rump.ifconfig pppoe0
	atf_check -s exit:0 -o not-match:'RUNNING' rump.ifconfig pppoe0
	wait_for "LCP" "initial"

	# Due to implementation constraints, the first packet is dropped.
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP

	# The client starts connection
	atf_check -s exit:0 -o match:'UP.*RUNNING' rump.ifconfig pppoe0
	wait_for "LCP" "starting"

	local t=$((total_retry_wait + 10))
	echo "Waiting for timeout of PADI retries (${t}s)"
	sleep $t

	atf_check -s exit:0 -o     match:'UP'      rump.ifconfig pppoe0
	atf_check -s exit:0 -o not-match:'RUNNING' rump.ifconfig pppoe0
	wait_for "LCP" "initial"

	# Start PPPoE Server
	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 up

	export RUMP_SERVER=$CLIENT

	# Trigger packet
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP

	# The connection should be established
	atf_check -s exit:0 -o match:'UP.*RUNNING' rump.ifconfig pppoe0
	wait_for "IPCP" "opened"
	atf_ifconfig -w 10
	atf_check -s exit:0 -o ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP
}

pppoe_ondemand_maxpadi_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case pppoe_ondemand
	atf_add_test_case pppoe_ondemand_maxpadi
}
