#	$NetBSD: t_pppoe_ondemand.sh,v 1.6 2026/07/28 08:07:11 yamaguchi Exp $
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
SERVER_IP6=fc00::1
CLIENT_IP6=fc00::3
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

		# Set a generous idle timeout to prevent disconnection
		atf_pppoectl pppoe0 idle-timeout=$t_idle

		# Send trigger packet
		atf_sendpkt $SERVER_IP 80/tcp

		# The connection should be established
		atf_check -s exit:0 -o match:'UP.*RUNNING' \
		    rump.ifconfig pppoe0
		wait_for "IPCP" "opened"
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

	# Send trigger packet
	atf_sendpkt $SERVER_IP 80/tcp

	atf_check -s exit:0 -o match:'UP.*RUNNING' \
	    rump.ifconfig pppoe0
	wait_for "IPCP" "opened"
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

	# Send trigger packet
	atf_sendpkt $SERVER_IP 80/tcp

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

	# Send trigger packet
	atf_sendpkt $SERVER_IP 80/tcp

	# The connection should be established
	atf_check -s exit:0 -o match:'UP.*RUNNING' rump.ifconfig pppoe0
	wait_for "IPCP" "opened"
	atf_check -s exit:0 -o ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP
}

pppoe_ondemand_maxpadi_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case pppoe_sppp_filter cleanup
pppoe_sppp_filter_head()
{

	atf_set "descr" "Test for a part of SPPP_FILTER"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_sppp_filter_body()
{
	# A generous idle timeout to prevent accidental disconnects
	local t_idle=300

	local fragment="ip[6:2] & 0x3fff != 0"
	local icmp="ip proto 1"
	local igmp="ip proto 2"
	local icmp_echo="icmp[icmptype] == icmp-echo"
	local ports="port 67 or port 68 or port 123 or port 137 or port 520"
	local non_trigger_ports="\
	    port 67 or port 68 or port 123 or port 137 or port 520"
	local  non_active_ports="$non_trigger_ports or port 53"
	local filter_dialing="ip and ($fragment or $icmp_echo or \
	    (not $icmp and not $igmp and not ($non_trigger_ports)))"
	local filter_active_in="none"
	local filter_active_out="ip and ($fragment or $icmp_echo or \
	    (not $icmp and not $igmp and not ($non_active_ports)))"
	local NON_TRIGGER_PORTS=$(\
	    echo "$non_trigger_ports" | sed -E 's/(port|or)//g')

	export RUMP_PPPOE_KEEPALIVE_INTERVAL=1
	setup_pppoe_server_client $SERVER $CLIENT $BUS bpf
	setup_auth_params chap $AUTHNAME $SECRET
	setup_ipcp_addrs   $SERVER_IP $CLIENT_IP
	setup_ipv6cp_addrs $SERVER_IP6 $CLIENT_IP6

	# Enabled on-demand dialing
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 link1
	atf_check -s exit:0 -o ignore \
	    rump.route add -inet  default -ifp pppoe0 0.0.0.1
	atf_check -s exit:0 -o ignore \
	    rump.route add -inet6 default -ifp pppoe0 fe80::1

	pppoe_connect "expected-failure"

	export RUMP_SERVER=$CLIENT
	# Set a generous idle timeout
	atf_pppoectl pppoe0 idle-timeout=$t_idle

	echo 'Test default filter that is `all`'
	for P in $NON_TRIGGER_PORTS; do
		for PROTO in tcp udp; do
			# disconnect
			atf_ifconfig pppoe0 down
			atf_ifconfig pppoe0 up
			wait_for "LCP" "initial"

			# send packet
			atf_sendpkt $SERVER_IP $P/$PROTO
			atf_check -s exit:0 -o match:'UP.*RUNNING' \
			    rump.ifconfig pppoe0
			wait_for "IPCP" "opened"
		done
	done

	atf_ifconfig pppoe0 down
	atf_ifconfig pppoe0 up
	wait_for "LCP" "initial"

	# Set pass and active filters
	atf_pppoectl pppoe0 \
	    filter-dialing="\"$filter_dialing\"" \
	    filter-active-in="\"$filter_active_in\"" \
	    filter-active-out="\"$filter_active_out\""

	#
	# Test trigger packet types (SPPPIOCSOPASS)
	#
	echo "Trigger Packet Type Testing (IPv4)"

	export RUMP_SERVER=$CLIENT
	# TFTP, SSH, TELNET, HTTP(s), syslog, IMAPS, RDP
	TRIGGER_TEST_PORT="21 22 23 80 443 514 993 3389"
	for P in $TRIGGER_TEST_PORT; do
		for PROTO in tcp udp; do
			# disconnect
			atf_ifconfig pppoe0 down
			atf_ifconfig pppoe0 up
			wait_for "LCP" "initial"

			# send packet
			atf_sendpkt $SERVER_IP $P/$PROTO
			atf_check -s exit:0 -o match:'UP.*RUNNING' \
			    rump.ifconfig pppoe0
			wait_for "IPCP" "opened"
		done
	done

	atf_ifconfig pppoe0 down
	atf_ifconfig pppoe0 up
	wait_for "LCP" "initial"

	echo "Non-Trigger IPv4 Packet Testing"
	for P in $NON_TRIGGER_PORTS; do
		for PROTO in tcp udp; do
			atf_sendpkt $SERVER_IP $P/$PROTO
			atf_check -s exit:0 -o     match:'UP'\
			    rump.ifconfig pppoe0
			atf_check -s exit:0 -o not-match:'RUNNING' \
			    rump.ifconfig pppoe0
			wait_for "LCP" "initial"
		done
	done

	echo "Test that not all IPv6 packets are triggers"
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping6 -c 1 -X $TIMEOUT $SERVER_IP6
	atf_check -s exit:0 -o     match:'UP'      rump.ifconfig pppoe0
	atf_check -s exit:0 -o not-match:'RUNNING' rump.ifconfig pppoe0
	wait_for "LCP" "initial"

	for P in $TRIGGER_TEST_PORT; do
		for PROTO in tcp udp; do
			atf_sendpkt $SERVER_IP6 $P/$PROTO
			atf_check -s exit:0 -o     match:'UP' \
			    rump.ifconfig pppoe0
			atf_check -s exit:0 -o not-match:'RUNNING' \
			    rump.ifconfig pppoe0
			wait_for "LCP" "initial"
		done
	done

	#
	# Test output packet types for activity detection (SPPPIOCSOACTIVE)
	#
	echo "Test output packet types for activity detection"

	export RUMP_SERVER=$CLIENT
	# Set a generous idle timeout to prevent disconnection
	atf_pppoectl pppoe0 idle-timeout=$t_idle

	# Send a trigger packet
	atf_sendpkt $SERVER_IP 80/tcp

	# Wait for connection established
	atf_check -s exit:0 -o match:'UP.*RUNNING' \
	    rump.ifconfig pppoe0
	wait_for "IPCP"   "opened"
	wait_for "IPv6CP" "opened"
	atf_check -s exit:0 -o ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP
	atf_check -s exit:0 -o ignore -e ignore \
	    rump.ping6 -c 1 -X $TIMEOUT $SERVER_IP6

	# Set a short timeout for testing
	atf_pppoectl pppoe0 idle-timeout=3

	# ICMPv4 packet updates pp_last_activity
	atf_check -s exit:0 -o ignore \
	    rump.ping  -i 1 -c 5 -w 5 $SERVER_IP

	# ICMPv6 doesn't updates pp_last_activity
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping6 -i 1 -c 5 -X 5 $SERVER_IP6

	atf_check -s exit:0 -o     match:'UP'      rump.ifconfig pppoe0
	atf_check -s exit:0 -o not-match:'RUNNING' rump.ifconfig pppoe0
	wait_for "LCP" "initial"

	#
	# Test input packet types for activity detection (SPPPIOCSIACTIVE)
	#
	echo "Test input packet types for activity detection"

	export RUMP_SERVER=$CLIENT
	# Set a generous idle timeout to prevent disconnection
	atf_pppoectl pppoe0 idle-timeout=$t_idle

	# Send a trigger packet
	atf_sendpkt $SERVER_IP 80/tcp

	# Wait for connection established
	atf_check -s exit:0 -o match:'UP.*RUNNING' \
	    rump.ifconfig pppoe0
	wait_for "IPCP" "opened"
	atf_check -s exit:0 -o ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP

	# Set a short timeout for testing
	atf_pppoectl pppoe0 idle-timeout=3

	# Generate input traffic
	export RUMP_SERVER=$SERVER
	for n in $(seq 1 5); do
		atf_sendpkt $CLIENT_IP 80/tcp
		sleep 1
	done
	wait_for "LCP" "starting"

	export RUMP_SERVER=$CLIENT
	# Set a generous idle timeout to prevent disconnection
	atf_pppoectl pppoe0 idle-timeout=$t_idle

	atf_check -s exit:0 -o     match:'UP'      rump.ifconfig pppoe0
	atf_check -s exit:0 -o not-match:'RUNNING' rump.ifconfig pppoe0
	wait_for "LCP" "initial"

	# Set all pass filter that is default value
	echo "Test disabling output pass filter"
	atf_pppoectl pppoe0 \
	    filter-dialing="all" \
	    filter-active-in="all" \
	    filter-active-out="all"

	for P in $NON_TRIGGER_PORTS; do
		for PROTO in tcp udp; do
			# disconnect
			atf_ifconfig pppoe0 down
			atf_ifconfig pppoe0 up
			wait_for "LCP" "initial"

			# send packet
			atf_sendpkt $SERVER_IP $P/$PROTO
			atf_check -s exit:0 -o match:'UP.*RUNNING' \
			    rump.ifconfig pppoe0
			wait_for "IPCP" "opened"
		done
	done

	atf_ifconfig pppoe0 down
	atf_ifconfig pppoe0 up
	wait_for "LCP" "initial"
}

pppoe_sppp_filter_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case pppoe_ondemand
	atf_add_test_case pppoe_ondemand_maxpadi
	atf_add_test_case pppoe_sppp_filter
}
