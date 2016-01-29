#	$NetBSD: t_route.sh,v 1.1 2016/01/29 04:15:46 ozaki-r Exp $
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

RUMP_LIBS="-lrumpnet -lrumpnet_net -lrumpnet_netinet -lrumpnet_shmif"
SOCK_CLIENT=unix://commsock1
SOCK_GW=unix://commsock2
BUS=bus1

DEBUG=false
TIMEOUT=1
PING_OPTS="-n -c 1 -w $TIMEOUT"

atf_test_case non_subnet_gateway cleanup
non_subnet_gateway_head()
{

	atf_set "descr" "tests of a gateway not on the local subnet"
	atf_set "require.progs" "rump_server"
}

non_subnet_gateway_body()
{

	atf_check -s exit:0 rump_server ${RUMP_LIBS} ${SOCK_CLIENT}
	atf_check -s exit:0 rump_server ${RUMP_LIBS} ${SOCK_GW}

	export RUMP_SERVER=${SOCK_GW}

	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr $BUS
	atf_check -s exit:0 rump.ifconfig shmif0 192.168.0.1
	atf_check -s exit:0 rump.ifconfig shmif0 up

	# The gateway knows the client
	atf_check -s exit:0 -o match:'add net 10.0.0.1: gateway shmif0' \
	    rump.route add -net 10.0.0.1/32 -link -cloning -iface shmif0

	$DEBUG && rump.netstat -nr -f inet

	export RUMP_SERVER=${SOCK_CLIENT}

	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr $BUS
	atf_check -s exit:0 rump.ifconfig shmif0 10.0.0.1/32
	atf_check -s exit:0 rump.ifconfig shmif0 up

	$DEBUG && rump.netstat -nr -f inet

	# Don't know a route to the gateway yet
	atf_check -s not-exit:0 -o match:'100.0% packet loss' \
	    -e match:'No route to host' rump.ping $PING_OPTS 192.168.0.1

	# Teach a route to the gateway
	atf_check -s exit:0 -o match:'add net 192.168.0.1: gateway shmif0' \
	    rump.route add -net 192.168.0.1/32 -link -cloning -iface shmif0
	atf_check -s exit:0 -o match:'add net default: gateway 192.168.0.1' \
	    rump.route add default -ifa 10.0.0.1 192.168.0.1

	$DEBUG && rump.netstat -nr -f inet

	# Be reachable to the gateway
	atf_check -s exit:0 -o ignore rump.ping $PING_OPTS 192.168.0.1

	unset RUMP_SERVER
}

non_subnet_gateway_cleanup()
{

	$DEBUG && shmif_dumpbus -p - $BUS 2>/dev/null | tcpdump -n -e -r -
	env RUMP_SERVER=$SOCK_CLIENT rump.halt
	env RUMP_SERVER=$SOCK_GW rump.halt
}

atf_init_test_cases()
{

	atf_add_test_case non_subnet_gateway
}
