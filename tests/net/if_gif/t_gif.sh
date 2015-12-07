#	$NetBSD: t_gif.sh,v 1.2 2015/12/07 09:59:26 knakahara Exp $
#
# Copyright (c) 2015 Internet Initiative Japan Inc.
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

server="rump_server -v -lrumpnet -lrumpnet_net -lrumpnet_netinet \
		    -lrumpnet_netinet6 -lrumpnet_shmif -lrumpnet_gif"

SOCK1=unix://commsock1 # for ROUTER1
SOCK2=unix://commsock2 # for ROUTER2
ROUTER1_LANIP=192.168.1.1
ROUTER1_LANNET=192.168.1.0/24
ROUTER1_WANIP=10.0.0.1
ROUTER1_GIFIP=172.16.1.1
ROUTER2_LANIP=192.168.2.1
ROUTER2_LANNET=192.168.2.0/24
ROUTER2_WANIP=10.0.0.2
ROUTER2_GIFIP=172.16.2.1

ROUTER1_LANIP6=fc00:1::1
ROUTER1_LANNET6=fc00:1::/64
ROUTER1_WANIP6=fc00::1
ROUTER1_GIFIP6=fc00:3::1
ROUTER2_LANIP6=fc00:2::1
ROUTER2_LANNET6=fc00:2::/64
ROUTER2_WANIP6=fc00::2
ROUTER2_GIFIP6=fc00:4::1

TIMEOUT=5

atf_test_case basicv4overv4 cleanup
atf_test_case basicv4overv6 cleanup
atf_test_case basicv6overv4 cleanup
atf_test_case basicv6overv6 cleanup

basicv4overv4_head()
{
	atf_set "descr" "Does IPv4 over IPv4 if_gif tests"
	atf_set "require.progs" "rump_server"
}

basicv4overv6_head()
{
	atf_set "descr" "Does IPv4 over IPv6 if_gif tests"
	atf_set "require.progs" "rump_server"
}

basicv6overv4_head()
{
	atf_set "descr" "Does IPv6 over IPv4 if_gif tests"
	atf_set "require.progs" "rump_server"
}

basicv6overv6_head()
{
	atf_set "descr" "Does IPv6 over IPv6 if_gif tests"
	atf_set "require.progs" "rump_server"
}

setup_router()
{
	sock=${1}
	lan=${2}
	lan_mode=${3}
	wan=${4}
	wan_mode=${5}

	export RUMP_SERVER=${sock}
	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr bus0
	if [ ${lan_mode} = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig shmif0 inet6 ${lan}
	else
		atf_check -s exit:0 rump.ifconfig shmif0 inet ${lan} netmask 0xffffff00
	fi
	atf_check -s exit:0 rump.ifconfig shmif0 up
	rump.ifconfig shmif0

	atf_check -s exit:0 rump.ifconfig shmif1 create
	atf_check -s exit:0 rump.ifconfig shmif1 linkstr bus1
	if [ ${wan_mode} = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig shmif1 inet6 ${wan}
	else
		atf_check -s exit:0 rump.ifconfig shmif1 inet ${wan} netmask 0xff000000
	fi
	atf_check -s exit:0 rump.ifconfig shmif1 up
	rump.ifconfig shmif1
}

test_router()
{
	sock=${1}
	lan=${2}
	lan_mode=${3}
	wan=${4}
	wan_mode=${5}

	export RUMP_SERVER=${sock}
	atf_check -s exit:0 -o match:shmif0 rump.ifconfig
	if [ ${lan_mode} = "ipv6" ]; then
		atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 -X $TIMEOUT ${lan}
	else
		atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w $TIMEOUT ${lan}
	fi

	atf_check -s exit:0 -o match:shmif1 rump.ifconfig
	if [ ${wan_mode} = "ipv6" ]; then
		atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 -X $TIMEOUT ${wan}
	else
		atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w $TIMEOUT ${wan}
	fi
}

setup()
{
	inner=${1}
	outer=${2}

	atf_check -s exit:0 ${server} $SOCK1
	atf_check -s exit:0 ${server} $SOCK2

	router1_lan=""
	router1_lan_mode=""
	router2_lan=""
	router2_lan_mode=""
	if [ ${inner} = "ipv6" ]; then
		router1_lan=$ROUTER1_LANIP6
		router1_lan_mode="ipv6"
		router2_lan=$ROUTER2_LANIP6
		router2_lan_mode="ipv6"
	else
		router1_lan=$ROUTER1_LANIP
		router1_lan_mode="ipv4"
		router2_lan=$ROUTER2_LANIP
		router2_lan_mode="ipv4"
	fi

	if [ ${outer} = "ipv6" ]; then
		setup_router $SOCK1 ${router1_lan} ${router1_lan_mode} \
			$ROUTER1_WANIP6 ipv6
		setup_router $SOCK2 ${router2_lan} ${router2_lan_mode} \
			$ROUTER2_WANIP6 ipv6
	else
		setup_router $SOCK1 ${router1_lan} ${router1_lan_mode} \
			$ROUTER1_WANIP ipv4
		setup_router $SOCK2 ${router2_lan} ${router2_lan_mode} \
			$ROUTER2_WANIP ipv4
	fi
}

test_setup()
{
	inner=${1}
	outer=${2}

	router1_lan=""
	router1_lan_mode=""
	router2_lan=""
	router2_lan_mode=""
	if [ ${inner} = "ipv6" ]; then
		router1_lan=$ROUTER1_LANIP6
		router1_lan_mode="ipv6"
		router2_lan=$ROUTER2_LANIP6
		router2_lan_mode="ipv6"
	else
		router1_lan=$ROUTER1_LANIP
		router1_lan_mode="ipv4"
		router2_lan=$ROUTER2_LANIP
		router2_lan_mode="ipv4"
	fi
	if [ ${outer} = "ipv6" ]; then
		test_router $SOCK1 ${router1_lan} ${router1_lan_mode} \
			$ROUTER1_WANIP6 ipv6
		test_router $SOCK2 ${router2_lan} ${router2_lan_mode} \
			$ROUTER2_WANIP6 ipv6
	else
		test_router $SOCK1 ${router1_lan} ${router1_lan_mode} \
			$ROUTER1_WANIP ipv4
		test_router $SOCK2 ${router2_lan} ${router2_lan_mode} \
			$ROUTER2_WANIP ipv4
	fi
}

setup_if_gif()
{
	sock=${1}
	addr=${2}
	remote=${3}
	inner=${4}
	src=${5}
	dst=${6}
	peernet=${7}

	export RUMP_SERVER=${sock}
	atf_check -s exit:0 rump.ifconfig gif0 create
	atf_check -s exit:0 rump.ifconfig gif0 tunnel ${src} ${dst}
	if [ ${inner} = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig gif0 inet6 ${addr}/128 ${remote}
		atf_check -s exit:0 -o ignore rump.route add -inet6 ${peernet} ${addr}
	else
		atf_check -s exit:0 rump.ifconfig gif0 inet ${addr}/32 ${remote}
		atf_check -s exit:0 -o ignore rump.route add -inet ${peernet} ${addr}
	fi

	rump.ifconfig gif0
	rump.route -nL show
}

setup_tunnel()
{
	inner=${1}
	outer=${2}

	addr=""
	remote=""
	src=""
	dst=""
	peernet=""

	if [ ${inner} = "ipv6" ]; then
		addr=$ROUTER1_GIFIP6
		remote=$ROUTER2_GIFIP6
		peernet=$ROUTER2_LANNET6
	else
		addr=$ROUTER1_GIFIP
		remote=$ROUTER2_GIFIP
		peernet=$ROUTER2_LANNET
	fi
	if [ ${outer} = "ipv6" ]; then
		src=$ROUTER1_WANIP6
		dst=$ROUTER2_WANIP6
	else
		src=$ROUTER1_WANIP
		dst=$ROUTER2_WANIP
	fi
	setup_if_gif $SOCK1 ${addr} ${remote} ${inner} \
		     ${src} ${dst} ${peernet}

	if [ $inner = "ipv6" ]; then
		addr=$ROUTER2_GIFIP6
		remote=$ROUTER1_GIFIP6
		peernet=$ROUTER1_LANNET6
	else
		addr=$ROUTER2_GIFIP
		remote=$ROUTER1_GIFIP
		peernet=$ROUTER1_LANNET
	fi
	if [ $outer = "ipv6" ]; then
		src=$ROUTER2_WANIP6
		dst=$ROUTER1_WANIP6
	else
		src=$ROUTER2_WANIP
		dst=$ROUTER1_WANIP
	fi
	setup_if_gif $SOCK2 ${addr} ${remote} ${inner} \
		     ${src} ${dst} ${peernet}
}

test_setup_tunnel()
{
	mode=${1}

	peernet=""
	opt=""
	if [ ${mode} = "ipv6" ]; then
		peernet=$ROUTER2_LANNET6
		opt="-inet6"
	else
		peernet=$ROUTER2_LANNET
		opt="-inet"
	fi
	export RUMP_SERVER=$SOCK1
	atf_check -s exit:0 -o match:gif0 rump.ifconfig
	atf_check -s exit:0 -o match:gif0 rump.route -nL get ${opt} ${peernet}

	if [ ${mode} = "ipv6" ]; then
		peernet=$ROUTER1_LANNET6
		opt="-inet6"
	else
		peernet=$ROUTER1_LANNET
		opt="-inet"
	fi
	export RUMP_SERVER=$SOCK2
	atf_check -s exit:0 -o match:gif0 rump.ifconfig
	atf_check -s exit:0 -o match:gif0 rump.route -nL get ${opt} ${peernet}
}

teardown_tunnel()
{
	export RUMP_SERVER=$SOCK1
	atf_check -s exit:0 rump.ifconfig gif0 deletetunnel
	atf_check -s exit:0 rump.ifconfig gif0 destroy

	export RUMP_SERVER=$SOCK2
	atf_check -s exit:0 rump.ifconfig gif0 deletetunnel
	atf_check -s exit:0 rump.ifconfig gif0 destroy
}

cleanup()
{
	env RUMP_SERVER=$SOCK1 rump.halt
	env RUMP_SERVER=$SOCK2 rump.halt
}

dump_bus()
{
	/usr/bin/shmif_dumpbus -p - bus0 2>/dev/null| /usr/sbin/tcpdump -n -e -r -
	/usr/bin/shmif_dumpbus -p - bus1 2>/dev/null| /usr/sbin/tcpdump -n -e -r -
}

test_ping_failure()
{
	mode=$1

	export RUMP_SERVER=$SOCK1
	if [ ${mode} = "ipv6" ]; then
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping6 -n -X $TIMEOUT -c 1 -S $ROUTER1_LANIP6 \
			$ROUTER2_LANIP6
	else
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER1_LANIP \
			$ROUTER2_LANIP
	fi

	export RUMP_SERVER=$SOCK2
	if [ ${mode} = "ipv6" ]; then
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping6 -n -X $TIMEOUT -c 1 -S $ROUTER2_LANIP6 \
			$ROUTER1_LANIP6
	else
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER1_LANIP \
			$ROUTER2_LANIP
	fi
}

test_ping_success()
{
	mode=$1

	export RUMP_SERVER=$SOCK1
	rump.ifconfig -v gif0
	if [ ${mode} = "ipv6" ]; then
		# XXX
		# rump.ping6 rarely fails with the message that
		# "failed to get receiving hop limit".
		# This is a known issue being analyzed.
		atf_check -s exit:0 -o ignore \
			rump.ping6 -n -X $TIMEOUT -c 1 -S $ROUTER1_LANIP6 \
			$ROUTER2_LANIP6
	else
		atf_check -s exit:0 -o ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER1_LANIP \
			$ROUTER2_LANIP
	fi
	rump.ifconfig -v gif0

	export RUMP_SERVER=$SOCK2
	rump.ifconfig -v gif0
	if [ ${mode} = "ipv6" ]; then
		atf_check -s exit:0 -o ignore \
			rump.ping6 -n -X $TIMEOUT -c 1 -S $ROUTER2_LANIP6 \
			$ROUTER1_LANIP6
	else
		atf_check -s exit:0 -o ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER2_LANIP \
			$ROUTER1_LANIP
	fi
	rump.ifconfig -v gif0
}

basicv4overv4_body()
{
	setup ipv4 ipv4
	test_setup ipv4 ipv4

	# Enable once PR kern/49219 is fixed
	#test_ping_failure

	setup_tunnel ipv4 ipv4
	sleep 1
	test_setup_tunnel ipv4
	test_ping_success ipv4

	teardown_tunnel
	test_ping_failure ipv4
}

basicv4overv6_body()
{
	setup ipv4 ipv6
	test_setup ipv4 ipv6

	# Enable once PR kern/49219 is fixed
	#test_ping_failure

	setup_tunnel ipv4 ipv6
	sleep 1
	test_setup_tunnel ipv4
	test_ping_success ipv4

	teardown_tunnel
	test_ping_failure ipv4
}

basicv6overv4_body()
{
	setup ipv6 ipv4
	test_setup ipv6 ipv4

	# Enable once PR kern/49219 is fixed
	#test_ping_failure

	setup_tunnel ipv6 ipv4
	sleep 1
	test_setup_tunnel ipv6
	test_ping_success ipv6

	teardown_tunnel
	test_ping_failure ipv6
}

basicv6overv6_body()
{
	setup ipv6 ipv6
	test_setup ipv6 ipv6

	# Enable once PR kern/49219 is fixed
	#test_ping_failure

	setup_tunnel ipv6 ipv6
	sleep 1
	test_setup_tunnel ipv6
	test_ping_success ipv6

	teardown_tunnel
	test_ping_failure ipv6
}

basicv4overv4_cleanup()
{
	dump_bus
	cleanup
}

basicv4overv6_cleanup()
{
	dump_bus
	cleanup
}

basicv6overv4_cleanup()
{
	dump_bus
	cleanup
}

basicv6overv6_cleanup()
{
	dump_bus
	cleanup
}

atf_init_test_cases()
{
	atf_add_test_case basicv4overv4
	atf_add_test_case basicv4overv6
	atf_add_test_case basicv6overv4
	atf_add_test_case basicv6overv6
}
