#	$NetBSD: common.sh,v 1.2 2026/07/14 05:05:21 yamaguchi Exp $
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

RETRY=5
PPPOE_RECON_PADTRCVD=5 # defined in if_pppoe.c

atf_ifconfig()
{

	atf_check -s exit:0 rump.ifconfig $*
}

atf_pppoectl()
{

	atf_check -s exit:0 -x "$HIJACKING pppoectl $*"
}

dump_bus()
{

	shmif_dumpbus -p - ${BUS_} | tcpdump -n -e -r -
}

setup_pppoe_server_client()
{
	# set globals
	SERVER_=$1
	CLIENT_=$2
	BUS_=$3

	# IPCP is disabled and IPv6CP is enabled by default
	NCP_="IPv6CP"

	rump_server_start $SERVER_ netinet6 pppoe
	rump_server_start $CLIENT_ netinet6 pppoe

	for S in $SERVER_ $CLIENT_; do
		export RUMP_SERVER=$S
		rump_server_add_iface $S shmif0 $BUS_
		rump_server_add_iface $S pppoe0
		atf_ifconfig shmif0 up
		atf_pppoectl -e shmif0 pppoe0
	done

	if $DEBUG; then
		for S in $SERVER_ $CLIENT_; do
			export RUMP_SERVER=$S
			atf_ifconfig pppoe0 debug
			rump.ifconfig
			$HIJACKING pppoectl -d pppoe0
		done
	fi

	export RUMP_SERVER=$SERVER_
	atf_ifconfig pppoe0 link0
}

setup_auth_params()
{
	local auth=$1
	local   id=$2
	local pass=$3

	# As pppoe client doesn't support rechallenge yet.
	local server_optparam=""
	if [ "$auth" = "chap" ]; then
		server_optparam="norechallenge"
	fi

	export RUMP_SERVER=$SERVER_
	atf_pppoectl pppoe0 "hisauthproto=$auth" \
	    "hisauthname=$id" "hisauthsecret=$pass" \
	    "myauthproto=none" $server_optparam
	$DEBUG && $HIJACKING pppoectl pppoe0

	export RUMP_SERVER=$CLIENT_
	atf_pppoectl pppoe0 \
	    "myauthname=$id" "myauthsecret=$pass" \
	    "myauthproto=$auth" "hisauthproto=none"
	$DEBUG && $HIJACKING pppoectl pppoe0
}

setup_ipcp_addrs()
{
	local serverip=$1
	local clientip=$2

	NCP_="IPCP IPv6CP"

	export RUMP_SERVER=$SERVER_
	atf_ifconfig pppoe0 inet $serverip $clientip down

	export RUMP_SERVER=$CLIENT_
	atf_ifconfig pppoe0 inet 0.0.0.0 0.0.0.1 down
}

setup_ipv6cp_addrs()
{
	local serverip6=$1
	local clientip6=$2

	export RUMP_SERVER=$SERVER_
	atf_ifconfig pppoe0 inet6 $serverip6/64 down

	export RUMP_SERVER=$CLIENT_
	atf_ifconfig pppoe0 inet6 $clientip6/64 down
}

pppoe_connect()
{
	local expected_failure=$1

	export RUMP_SERVER=$SERVER_
	atf_ifconfig pppoe0 up
	wait_for "LCP" "starting"

	export RUMP_SERVER=$CLIENT_
	atf_ifconfig pppoe0 up
	for ncp in $NCP_; do
		test -n "$ncp" && \
		    wait_for $ncp "opened" $expected_failure
	done

	for S in $SERVER_ $CLIENT_; do
		export RUMP_SERVER=$S
		atf_ifconfig -w 10
		$DEBUG && rump.ifconfig
	done
}

pppoe_connect_ncp()
{
	local ncp=$1

	export RUMP_SERVER=$SERVER_
	atf_ifconfig pppoe0 up
	wait_for "LCP" "starting"

	export RUMP_SERVER=$CLIENT_
	atf_ifconfig pppoe0 up
	wait_for $ncp "opened"

	for S in $SERVER_ $CLIENT_; do
		export RUMP_SERVER=$S
		atf_ifconfig -w 10
		$DEBUG && rump.ifconfig
	done
}

pppoe_disconnect_by_client()
{

	export RUMP_SERVER=$CLIENT_
	atf_ifconfig pppoe0 down
	wait_for "LCP" "initial"

	export RUMP_SERVER=$SERVER_
	wait_for "LCP" "starting"
	atf_ifconfig pppoe0 down
	wait_for "LCP" "initial"

	echo "Waiting for PPPOE_RECON_PADTRCVD"
	sleep $PPPOE_RECON_PADTRCVD
}

pppoe_disconnect_by_server()
{

	export RUMP_SERVER=$SERVER_
	atf_ifconfig pppoe0 down
	wait_for "LCP" "initial"

	export RUMP_SERVER=$CLIENT_
	wait_for "LCP" "starting"
	atf_ifconfig pppoe0 down
	wait_for "LCP" "initial"

	echo "Waiting for PPPOE_RECON_PADTRCVD"
	sleep $PPPOE_RECON_PADTRCVD
}

wait_for()
{
	local cp=$1
	local state=$2
	local fail_ok=$3

	for i in $(seq $RETRY); do
		$HIJACKING pppoectl -dd pppoe0 | grep -q "$cp state: $state"
		if [ $? = 0 ]; then
			break
		fi
		sleep 1
		$DEBUG && echo "Waiting for $cp $state, retry=$i"
	done

	if [ "$fail_ok" != "expected-failure" ]; then
		# Double check by using atf_check
		atf_check -s exit:0 -o match:"$cp state: +$state" \
		    -x "$HIJACKING pppoectl -dd pppoe0"
	else
		atf_check -s exit:0 -o not-match:"$cp state: +$state" \
		    -x "$HIJACKING pppoectl -dd pppoe0"
	fi
}

cleanup_bus()
{

	for S in $SERVER_ $CLIENT_; do
		export RUMP_SERVER=$S
		atf_ifconfig shmif0 down
		atf_ifconfig shmif0 -linkstr
	done

	rm $BUS_

	for S in $SERVER_ $CLIENT_; do
		export RUMP_SERVER=$S
		atf_ifconfig shmif0 linkstr $BUS_
		atf_ifconfig shmif0 up
	done
}

