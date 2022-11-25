#	$NetBSD: t_pppoe_unnumbered.sh,v 1.1 2022/11/25 08:43:16 knakahara Exp $
#
# Copyright (c) 2022 Internet Initiative Japan Inc.
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

SERVER_IP=10.0.0.1
CLIENT_IP=10.1.1.1
AUTHNAME=foobar@baz.com
SECRET=oink
BUS_G=bus_global
BUS_CLIENT_L=bus_client_local
TIMEOUT=3
WAITTIME=10
DEBUG=${DEBUG:-false}

atf_ifconfig()
{

	atf_check -s exit:0 rump.ifconfig $*
}

atf_pppoectl()
{

	atf_check -s exit:0 -x "$HIJACKING pppoectl $*"
}

setup_ifaces()
{

	rump_server_add_iface $SERVER shmif0 $BUS_G
	rump_server_add_iface $CLIENT shmif0 $BUS_G
	rump_server_add_iface $CLIENT shmif1 $BUS_CLIENT_L
	rump_server_add_iface $SERVER pppoe0
	rump_server_add_iface $CLIENT pppoe0

	export RUMP_SERVER=$SERVER

	atf_ifconfig shmif0 up
	$inet && atf_ifconfig pppoe0 \
	    inet $SERVER_IP $CLIENT_IP down
	atf_ifconfig pppoe0 link0

	$DEBUG && rump.ifconfig pppoe0 debug
	$DEBUG && rump.ifconfig
	$DEBUG && $HIJACKING pppoectl -d pppoe0
	unset RUMP_SERVER

	export RUMP_SERVER=$CLIENT
	atf_ifconfig shmif0 up

	$inet && atf_ifconfig pppoe0 \
	    inet $CLIENT_IP/32 0.0.0.1 down

	$DEBUG && rump.ifconfig pppoe0 debug
	$DEBUG && rump.ifconfig
	$DEBUG && $HIJACKING pppoectl -d pppoe0

	atf_ifconfig shmif1 inet $CLIENT_IP/29
	unset RUMP_SERVER
}

setup()
{
	inet=true

	if [ $# -ne 0 ]; then
		eval $@
	fi

	rump_server_start $SERVER pppoe
	rump_server_start $CLIENT pppoe

	setup_ifaces

	export RUMP_SERVER=$SERVER
	atf_pppoectl -e shmif0 pppoe0
	unset RUMP_SERVER

	export RUMP_SERVER=$CLIENT
	atf_pppoectl -e shmif0 pppoe0
	unset RUMP_SERVER
}

wait_for_opened()
{
	local cp=$1
	local dontfail=$2
	local n=$WAITTIME

	for i in $(seq $n); do
		$HIJACKING pppoectl -dd pppoe0 | grep -q "$cp state: opened"
		if [ $? = 0 ]; then
			rump.ifconfig -w 10
			return
		fi
		sleep 1
	done

	if [ "$dontfail" != "dontfail" ]; then
		atf_fail "Couldn't connect to the server for $n seconds."
	fi
}

wait_for_disconnected()
{
	local dontfail=$1
	local n=$WAITTIME

	for i in $(seq $n); do
		# If PPPoE client is disconnected by PPPoE server, then
		# the LCP state will of the client is in a starting to send PADI.
		$HIJACKING pppoectl -dd pppoe0 | grep -q \
		    -e "LCP state: initial" -e "LCP state: starting"
		[ $? = 0 ] && return

		sleep 1
	done

	if [ "$dontfail" != "dontfail" ]; then
		atf_fail "Couldn't disconnect for $n seconds."
	fi
}

run_test_unnumbered()
{
	local auth="chap"
	local cp="IPCP"
	setup

	# As pppoe client doesn't support rechallenge yet.
	local server_optparam=""
	if [ $auth = "chap" ]; then
		server_optparam="norechallenge"
	fi

	export RUMP_SERVER=$SERVER
	atf_pppoectl pppoe0 "hisauthproto=$auth" \
	    "hisauthname=$AUTHNAME" "hisauthsecret=$SECRET" \
	    "myauthproto=none" $server_optparam
	atf_ifconfig pppoe0 up
	unset RUMP_SERVER

	export RUMP_SERVER=$CLIENT
	atf_pppoectl pppoe0 \
	    "myauthname=$AUTHNAME" "myauthsecret=$SECRET" \
	    "myauthproto=$auth" "hisauthproto=none"
	atf_ifconfig pppoe0 unnumbered
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	$DEBUG && rump.route -nL show
	wait_for_opened $cp
	atf_check -s exit:0 -o ignore \
		  rump.route add -inet default -ifp pppoe0 $CLIENT_IP
	atf_check -s exit:0 -o ignore \
		  rump.ping -c 1 -w $TIMEOUT -I $CLIENT_IP $SERVER_IP
	unset RUMP_SERVER

	# test for disconnection from client just in case
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	export RUMP_SERVER=$SERVER
	wait_for_disconnected
	$DEBUG && $HIJACKING pppoectl -d pppoe0
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping -c 1 -w $TIMEOUT $CLIENT_IP
	atf_check -s exit:0 -o match:'initial' -x "$HIJACKING pppoectl -d pppoe0"
	unset RUMP_SERVER
}

atf_test_case pppoe_unnumbered cleanup

pppoe_unnumbered_head()
{

	atf_set "descr" "Does pppoe unnumbered tests"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_unnumbered_body()
{

	run_test_unnumbered
}

pppoe_unnumbered_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case pppoe_unnumbered
}
