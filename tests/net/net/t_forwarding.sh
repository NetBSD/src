#	$NetBSD: t_forwarding.sh,v 1.2 2015/05/16 14:29:37 ozaki-r Exp $
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

inetserver="rump_server -lrumpnet -lrumpnet_net -lrumpnet_netinet -lrumpnet_shmif"
inet6server="rump_server -lrumpnet -lrumpnet_net -lrumpnet_netinet -lrumpnet_netinet6 -lrumpnet_shmif"

SOCKSRC=unix://commsock1
SOCKFWD=unix://commsock2
SOCKDST=unix://commsock3
IP4SRC=10.0.1.2
IP4SRCGW=10.0.1.1
IP4DSTGW=10.0.2.1
IP4DST=10.0.2.2
IP6SRC=fc00:0:0:1::2
IP6SRCGW=fc00:0:0:1::1
IP6DSTGW=fc00:0:0:2::1
IP6DST=fc00:0:0:2::2

DEBUG=false

atf_test_case basic cleanup
atf_test_case basic6 cleanup

basic_head()
{
	atf_set "descr" "Does IPv4 forwarding tests"
	atf_set "require.progs" "rump_server"
}

basic6_head()
{
	atf_set "descr" "Does IPv6 forwarding tests"
	atf_set "require.progs" "rump_server"
}

setup_endpoint()
{
	sock=${1}
	addr=${2}
	bus=${3}
	mode=${4}
	gw=${5}

	export RUMP_SERVER=${sock}
	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr ${bus}
	if [ $mode = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig shmif0 inet6 ${addr}
		atf_check -s exit:0 -o ignore rump.route add -inet6 default ${gw}
	else
		atf_check -s exit:0 rump.ifconfig shmif0 inet ${addr} netmask 0xffffff00
		atf_check -s exit:0 -o ignore rump.route add default ${gw}
	fi
	atf_check -s exit:0 rump.ifconfig shmif0 up

	if $DEBUG; then
		rump.ifconfig shmif0
		rump.netstat -nr
	fi	
}

test_endpoint()
{
	sock=${1}
	addr=${2}
	bus=${3}
	mode=${4}

	export RUMP_SERVER=${sock}
	atf_check -s exit:0 -o match:shmif0 rump.ifconfig
	if [ $mode = "ipv6" ]; then
		export LD_PRELOAD=/usr/lib/librumphijack.so
		atf_check -s exit:0 -o ignore ping6 -n -c 1 ${addr}
		unset LD_PRELOAD
	else
		atf_check -s exit:0 -o ignore rump.ping -n -w 1 -c 1 ${addr}
	fi
}

setup_forwarder()
{
	mode=${1}

	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr bus1

	atf_check -s exit:0 rump.ifconfig shmif1 create
	atf_check -s exit:0 rump.ifconfig shmif1 linkstr bus2

	if [ $mode = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig shmif0 inet6 ${IP6SRCGW}
		atf_check -s exit:0 rump.ifconfig shmif1 inet6 ${IP6DSTGW}
	else
		atf_check -s exit:0 rump.ifconfig shmif0 inet ${IP4SRCGW} netmask 0xffffff00
		atf_check -s exit:0 rump.ifconfig shmif1 inet ${IP4DSTGW} netmask 0xffffff00
	fi

	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig shmif1 up

	if $DEBUG; then
		rump.netstat -nr
		if [ $mode = "ipv6" ]; then
			rump.sysctl net.inet6.ip6.forwarding
		else
			rump.sysctl net.inet.ip.forwarding
		fi
	fi
}

setup()
{
	atf_check -s exit:0 ${inetserver} $SOCKSRC
	atf_check -s exit:0 ${inetserver} $SOCKFWD
	atf_check -s exit:0 ${inetserver} $SOCKDST

	setup_endpoint $SOCKSRC $IP4SRC bus1 ipv4 $IP4SRCGW
	setup_endpoint $SOCKDST $IP4DST bus2 ipv4 $IP4DSTGW
	setup_forwarder ipv4
}

setup6()
{
	atf_check -s exit:0 ${inet6server} $SOCKSRC
	atf_check -s exit:0 ${inet6server} $SOCKFWD
	atf_check -s exit:0 ${inet6server} $SOCKDST

	setup_endpoint $SOCKSRC $IP6SRC bus1 ipv6 $IP6SRCGW
	setup_endpoint $SOCKDST $IP6DST bus2 ipv6 $IP6DSTGW
	setup_forwarder ipv6
}

test_setup()
{
	test_endpoint $SOCKSRC $IP4SRC bus1 ipv4
	test_endpoint $SOCKDST $IP4DST bus2 ipv4

	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 -o match:shmif0 rump.ifconfig
	atf_check -s exit:0 -o match:shmif1 rump.ifconfig

	atf_check -s exit:0 -o ignore rump.ping -n -w 1 -c 1 ${IP4SRCGW}
	atf_check -s exit:0 -o ignore rump.ping -n -w 1 -c 1 ${IP4DSTGW}
}

test_setup6()
{
	test_endpoint $SOCKSRC $IP6SRC bus1 ipv6
	test_endpoint $SOCKDST $IP6DST bus2 ipv6

	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 -o match:shmif0 rump.ifconfig
	atf_check -s exit:0 -o match:shmif1 rump.ifconfig

	export LD_PRELOAD=/usr/lib/librumphijack.so
	atf_check -s exit:0 -o ignore ping6 -n -c 1 ${IP6SRCGW}
	atf_check -s exit:0 -o ignore ping6 -n -c 1 ${IP6DSTGW}
	unset LD_PRELOAD
}

setup_forwarding()
{
	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 -o ignore rump.sysctl -w net.inet.ip.forwarding=1
}

setup_forwarding6()
{
	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 -o ignore rump.sysctl -w net.inet6.ip6.forwarding=1
}

teardown_forwarding()
{
	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 -o ignore rump.sysctl -w net.inet.ip.forwarding=0
}

teardown_forwarding6()
{
	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 -o ignore rump.sysctl -w net.inet6.ip6.forwarding=0
}

test_setup_forwarding()
{
	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 -o match:"net.inet.ip.forwarding = 1" \
	    rump.sysctl net.inet.ip.forwarding
}
test_setup_forwarding6()
{
	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 -o match:"net.inet6.ip6.forwarding = 1" \
	    rump.sysctl net.inet6.ip6.forwarding
}

test_teardown_forwarding()
{
	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 -o match:"net.inet.ip.forwarding = 0" \
	    rump.sysctl net.inet.ip.forwarding
}
test_teardown_forwarding6()
{
	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 -o match:"net.inet6.ip6.forwarding = 0" \
	    rump.sysctl net.inet6.ip6.forwarding
}

cleanup()
{
	env RUMP_SERVER=$SOCKSRC rump.halt
	env RUMP_SERVER=$SOCKFWD rump.halt
	env RUMP_SERVER=$SOCKDST rump.halt
}

dump()
{
	env RUMP_SERVER=$SOCKSRC rump.netstat -nr
	env RUMP_SERVER=$SOCKFWD rump.netstat -nr
	env RUMP_SERVER=$SOCKDST rump.netstat -nr

	/usr/bin/shmif_dumpbus -p - bus1 2>/dev/null| /usr/sbin/tcpdump -n -e -r -
	/usr/bin/shmif_dumpbus -p - bus2 2>/dev/null| /usr/sbin/tcpdump -n -e -r -
}

test_ping_failure()
{
	export RUMP_SERVER=$SOCKSRC
	atf_check -s not-exit:0 -o ignore rump.ping -q -n -w 1 -c 1 $IP4DST
	export RUMP_SERVER=$SOCKDST
	atf_check -s not-exit:0 -o ignore rump.ping -q -n -w 1 -c 1 $IP4SRC
}

test_ping_success()
{
	export RUMP_SERVER=$SOCKSRC
	$DEBUG && rump.ifconfig -v shmif0
	atf_check -s exit:0 -o ignore rump.ping -q -n -w 1 -c 1 $IP4SRCGW
	atf_check -s exit:0 -o ignore rump.ping -q -n -w 1 -c 1 $IP4DST
	$DEBUG && rump.ifconfig -v shmif0

	export RUMP_SERVER=$SOCKDST
	$DEBUG && rump.ifconfig -v shmif0
	atf_check -s exit:0 -o ignore rump.ping -q -n -w 1 -c 1 $IP4DSTGW
	atf_check -s exit:0 -o ignore rump.ping -q -n -w 1 -c 1 $IP4SRC
	$DEBUG && rump.ifconfig -v shmif0
}

test_ttl()
{
	export RUMP_SERVER=$SOCKSRC
	$DEBUG && rump.ifconfig -v shmif0
	atf_check -s exit:0 -o ignore rump.ping -q -n -w 1 -c 1 -T 1 $IP4SRCGW
	atf_check -s not-exit:0 -o ignore rump.ping -q -n -w 1 -c 1 -T 1 $IP4DST
	atf_check -s exit:0 -o ignore rump.ping -q -n -w 1 -c 1 -T 2 $IP4DST
	$DEBUG && rump.ifconfig -v shmif0
}

test_ping6_failure()
{
	export LD_PRELOAD=/usr/lib/librumphijack.so
	export RUMP_SERVER=$SOCKSRC
	atf_check -s not-exit:0 -o ignore ping6 -q -n -c 1 -X 1 $IP6DST
	export RUMP_SERVER=$SOCKDST
	atf_check -s not-exit:0 -o ignore ping6 -q -n -c 1 -X 1 $IP6SRC
	unset LD_PRELOAD
}

test_ping6_success()
{
	export RUMP_SERVER=$SOCKSRC
	$DEBUG && rump.ifconfig -v shmif0
	export LD_PRELOAD=/usr/lib/librumphijack.so
	atf_check -s exit:0 -o ignore ping6 -q -n -c 1 -X 1 $IP6SRCGW
	atf_check -s exit:0 -o ignore ping6 -q -n -c 1 -X 1 $IP6DST
	unset LD_PRELOAD
	$DEBUG && rump.ifconfig -v shmif0

	export RUMP_SERVER=$SOCKDST
	$DEBUG && rump.ifconfig -v shmif0
	export LD_PRELOAD=/usr/lib/librumphijack.so
	atf_check -s exit:0 -o ignore ping6 -q -n -c 1 -X 1 $IP6DSTGW
	atf_check -s exit:0 -o ignore ping6 -q -n -c 1 -X 1 $IP6SRC
	unset LD_PRELOAD
	$DEBUG && rump.ifconfig -v shmif0
}

test_hoplimit()
{
	export RUMP_SERVER=$SOCKSRC
	$DEBUG && rump.ifconfig -v shmif0
	export LD_PRELOAD=/usr/lib/librumphijack.so
	atf_check -s exit:0 -o ignore ping6 -q -n -c 1 -h 1 -X 1 $IP6SRCGW
	atf_check -s not-exit:0 -o ignore ping6 -q -n -c 1 -h 1 -X 1 $IP6DST
	atf_check -s exit:0 -o ignore ping6 -q -n -c 1 -h 2 -X 1 $IP6DST
	unset LD_PRELOAD
	$DEBUG && rump.ifconfig -v shmif0
}

basic_body()
{
	setup
	test_setup

	setup_forwarding
	test_setup_forwarding
	test_ping_success
	test_ttl

	teardown_forwarding
	test_teardown_forwarding
	test_ping_failure
}

basic6_body()
{
	setup6
	test_setup6

	setup_forwarding6
	test_setup_forwarding6
	test_ping6_success
	test_hoplimit

	teardown_forwarding6
	test_teardown_forwarding6
	test_ping6_failure
}

basic_cleanup()
{
	dump
	cleanup
}

basic6_cleanup()
{
	dump
	cleanup
}

atf_init_test_cases()
{
	atf_add_test_case basic
	atf_add_test_case basic6
}
