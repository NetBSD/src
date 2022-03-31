#	$NetBSD: t_lagg.sh,v 1.8 2022/03/31 03:26:33 yamaguchi Exp $
#
# Copyright (c) 2021 Internet Initiative Japan Inc.
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

SOCK_HOST0=unix://commsock0
SOCK_HOST1=unix://commsock1
SOCK_HOST2=unix://commsock2
BUS0=bus0
BUS1=bus1
BUS2=bus2
IP4ADDR0=192.168.0.1
IP4ADDR1=192.168.0.2
IP4ADDR2=192.168.1.1
IP4ADDR3=192.168.1.2
IP6ADDR0=fc00::1
IP6ADDR1=fc00::2
IP6ADDR2=fc00:1::1
IP6ADDR3=fc00:1::2
WAITTIME=20

DEBUG=${DEBUG:-false}

wait_state()
{
	local state=$1
	local if_lagg=$2
	local if_port=$3

	local n=$WAITTIME
	local cmd_grep="grep -q ${state}"

	if [ x"$if_port" != x"" ]; then
		cmd_grep="grep $if_port | $cmd_grep"
	fi

	for i in $(seq $n); do
		rump.ifconfig $if_lagg | eval $cmd_grep
		if [ $? = 0 ] ; then
			$DEBUG && echo "wait for $i seconds."
			return 0
		fi

		sleep 1
	done

	$DEBUG && rump.ifconfig -v $if_lagg
	atf_fail "Couldn't be ${state} for $n seconds."
}
wait_for_distributing()
{

	wait_state "DISTRIBUTING" $*
}

expected_inactive()
{
	local if_lagg=$1
	local if_port=$2

	sleep 3 # wait a little
	atf_check -s exit:0 -o not-match:"${if_port}.*ACTIVE" \
	    rump.ifconfig ${if_lagg}
}

setup_l2tp_ipv4tunnel()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"

	local a_addr0=10.0.0.1
	local a_addr1=10.0.0.2
	local b_addr0=10.0.1.1
	local b_addr1=10.0.1.2
	local c_addr0=10.0.2.1
	local c_addr1=10.0.2.2

	local a_session0=1001
	local a_session1=1002
	local b_session0=1011
	local b_session1=1012
	local c_session0=1021
	local c_session1=1022

	rump_server_add_iface $SOCK_HOST0 l2tp0
	rump_server_add_iface $SOCK_HOST0 l2tp1
	rump_server_add_iface $SOCK_HOST0 l2tp2
	rump_server_add_iface $SOCK_HOST1 l2tp0
	rump_server_add_iface $SOCK_HOST1 l2tp1
	rump_server_add_iface $SOCK_HOST1 l2tp2


	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig shmif0 $a_addr0/24
	$atf_ifconfig l2tp0  tunnel  $a_addr0    $a_addr1
	$atf_ifconfig l2tp0  session $a_session0 $a_session1

	$atf_ifconfig shmif1 $b_addr0/24
	$atf_ifconfig l2tp1  tunnel  $b_addr0    $b_addr1
	$atf_ifconfig l2tp1  session $b_session0 $b_session1

	$atf_ifconfig shmif2 $c_addr0/24
	$atf_ifconfig l2tp2  tunnel  $c_addr0    $c_addr1
	$atf_ifconfig l2tp2  session $c_session0 $c_session1

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig shmif0 $a_addr1/24
	$atf_ifconfig l2tp0  tunnel  $a_addr1    $a_addr0
	$atf_ifconfig l2tp0  session $a_session1 $a_session0

	$atf_ifconfig shmif1 $b_addr1/24
	$atf_ifconfig l2tp1  tunnel  $b_addr1    $b_addr0
	$atf_ifconfig l2tp1  session $b_session1 $b_session0

	$atf_ifconfig shmif2 $c_addr1/24
	$atf_ifconfig l2tp2  tunnel  $c_addr1    $c_addr0
	$atf_ifconfig l2tp2  session $c_session1 $c_session0
}

atf_test_case lagg_ifconfig cleanup
lagg_ifconfig_head()
{

	atf_set "descr" "tests for create, destroy, and ioctl of lagg(4)"
	atf_set "require.progs" "rump_server"
}

lagg_ifconfig_body()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"

	rump_server_start $SOCK_HOST0 lagg

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 destroy

	$atf_ifconfig lagg0 create
	$atf_ifconfig shmif0 create

	$atf_ifconfig lagg0 laggproto none
	atf_check -s exit:0 -o match:'laggproto none' \
	    rump.ifconfig lagg0

	# cannot add a port while protocol is none
	atf_check -s not-exit:0 -e ignore \
	    rump.ifconfig lagg0 laggport shmif0

	$atf_ifconfig lagg0 laggproto lacp
	atf_check -s exit:0 -o match:'laggproto lacp' \
	    rump.ifconfig lagg0

	# add a port and an added port
	$atf_ifconfig lagg0 laggport shmif0
	atf_check -s not-exit:0 -e ignore \
	    rump.ifconfig lagg0 laggport shmif0

	# remove an added port and a removed port
	$atf_ifconfig lagg0 -laggport shmif0
	atf_check -s not-exit:0 -e ignore \
	    rump.ifconfig lagg0 -laggport shmif0

	# re-add a removed port
	$atf_ifconfig lagg0 laggport shmif0

	# detach protocol even if the I/F has ports
	$atf_ifconfig lagg0 laggproto none

	# destroy the interface while grouping ports
	$atf_ifconfig lagg0 destroy

	$atf_ifconfig lagg0 create
	$atf_ifconfig shmif1 create

	$atf_ifconfig lagg0 laggproto lacp
	$atf_ifconfig lagg0 laggport shmif0
	$atf_ifconfig lagg0 laggport shmif1

	$atf_ifconfig lagg0 -laggport shmif0
	$atf_ifconfig lagg0 laggport shmif0
	$atf_ifconfig lagg0 -laggport shmif1
	$atf_ifconfig lagg0 laggport shmif1

	# destroy a LAGed port
	atf_check -s exit:0 -o match:shmif0 rump.ifconfig lagg0
	atf_check -s exit:0 -o match:shmif1 rump.ifconfig lagg0
	$atf_ifconfig shmif0 destroy
	$atf_ifconfig shmif1 destroy

	$atf_ifconfig lagg0 laggproto none
	atf_check -s exit:0 -o ignore rump.ifconfig lagg0
}

lagg_ifconfig_cleanup()
{
	$DEBG && dump
	cleanup
}

atf_test_case lagg_macaddr cleanup
lagg_macaddr_head()
{
	atf_set "descr" "tests for a MAC address to assign to lagg(4)"
	atf_set "require.progs" "rump_server"
}

lagg_macaddr_body()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"
	local lnkaddr0="02:00:00:00:00:01" # 02: I/G = 0, G/L = 1

	rump_server_start $SOCK_HOST0 lagg

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig lagg0 create
	rump_server_add_iface $SOCK_HOST0 shmif0 $BUS0
	rump_server_add_iface $SOCK_HOST0 shmif1 $BUS1

	maddr=$(get_macaddr $SOCK_HOST0 lagg0)
	maddr0=$(get_macaddr $SOCK_HOST0 shmif0)
	maddr1=$(get_macaddr $SOCK_HOST0 shmif1)

	$atf_ifconfig lagg0 laggproto lacp

	#
	# Copy MAC address from shmif0 that is
	# the first port to lagg.
	# (laggport: (none) => shmif0)
	#
	$atf_ifconfig lagg0 laggport shmif0
	atf_check -s exit:0 -o match:$maddr0 rump.ifconfig lagg0
	atf_check -s exit:0 -o match:$maddr0 rump.ifconfig shmif0

	#
	# Copy MAC address assigned to lagg0 to shmif1
	# (laggport: shmif0 => shmif0, shmif1)
	#
	$atf_ifconfig lagg0 laggport shmif1
	atf_check -s exit:0 -o match:$maddr0 rump.ifconfig lagg0
	atf_check -s exit:0 -o match:$maddr0 rump.ifconfig shmif1

	#
	# Change MAC address on the detaching
	# the first port (shmif0) from lagg0
	# (laggport: shmif0, shmif1 => shmif1)
	#
	$atf_ifconfig lagg0 -laggport shmif0
	atf_check -s exit:0 -o match:$maddr1 rump.ifconfig lagg0
	atf_check -s exit:0 -o match:$maddr0 rump.ifconfig shmif0
	atf_check -s exit:0 -o match:$maddr1 rump.ifconfig shmif1

	#
	# Copy lagg0's MAC address to shmif0 even if
	# lagg0 had used shmif0's MAC address
	# (laggport: shmif1 => shmif1, shmif0)
	#
	$atf_ifconfig lagg0 laggport shmif0
	atf_check -s exit:0 -o match:$maddr1 rump.ifconfig lagg0
	atf_check -s exit:0 -o match:$maddr1 rump.ifconfig shmif0

	#
	# should not change MAC address of lagg0 on detaching
	# shmif0 that copied mac address from lagg0
	# (laggport: shmif1, shmif0 => shmif1)
	#
	$atf_ifconfig lagg0 -laggport shmif0
	atf_check -s exit:0 -o match:$maddr1 rump.ifconfig lagg0
	atf_check -s exit:0 -o match:$maddr0 rump.ifconfig shmif0

	#
	# Use the generated MAC address
	# when all port detached from lagg0
	# (laggport: shmif1 => (none))
	$atf_ifconfig lagg0 -laggport shmif1
	atf_check -s exit:0 -o match:$maddr rump.ifconfig lagg0

	#
	# Copy the active MAC address from shmif0 to lagg0
	# when shmif0 has two MAC addresses
	#
	$atf_ifconfig shmif0 link $lnkaddr0
	$atf_ifconfig lagg0 laggport shmif0
	atf_check -s exit:0 -o match:$maddr0 rump.ifconfig lagg0
	atf_check -s exit:0 -o not-match:$lnkaddr0 rump.ifconfig lagg0
	atf_check -s exit:0 -o match:$maddr0 rump.ifconfig shmif0
	atf_check -s exit:0 -o match:$lnkaddr0 rump.ifconfig shmif0
}

lagg_macaddr_cleanup()
{
	$DEBUG && dump
	cleanup
}

atf_test_case lagg_ipv6lla cleanup
lagg_ipv6lla_head()
{
	atf_set "descr" "tests for a IPV6 LLA to assign to lagg(4)"
	atf_set "require.progs" "rump_server"
}

lagg_ipv6lla_body()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"

	rump_server_start $SOCK_HOST0 netinet6 lagg

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig lagg0 create
	rump_server_add_iface $SOCK_HOST0 shmif0 $BUS0
	rump_server_add_iface $SOCK_HOST0 shmif1 $BUS1

	$atf_ifconfig lagg0 laggproto lacp

	$atf_ifconfig shmif0 up
	atf_check -s exit:0 -o match:'inet6 fe80:' rump.ifconfig shmif0

	$atf_ifconfig lagg0 laggproto lacp laggport shmif0
	atf_check -s exit:0 -o not-match:'inet6 fe80:' rump.ifconfig shmif0

	$atf_ifconfig lagg0 laggport shmif1
	$atf_ifconfig shmif1 up
	atf_check -s exit:0 -o not-match:'inet6 fe80:' rump.ifconfig shmif1

	$atf_ifconfig lagg0 -laggport shmif0
	atf_check -s exit:0 -o match:'inet6 fe80:' rump.ifconfig shmif0

	$atf_ifconfig shmif1 down
	$atf_ifconfig lagg0 -laggport shmif1
	atf_check -s exit:0 -o not-match:'inet fe80:' rump.ifconfig shmif1
}

lagg_ipv6lla_cleanup()
{
	$DEBUG && dump
	cleanup
}

atf_test_case lagg_mtu cleanup
lagg_mtu_head()
{
	atf_set "descr" "tests for MTU"
	atf_set "require.progs" "rump_server"
}

lagg_mtu_body()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"
	local mtu_1st=1450
	local mtu_big=1460
	local mtu_small=1440

	rump_server_start $SOCK_HOST0 lagg

	export RUMP_SERVER=$SOCK_HOST0
	rump_server_add_iface $SOCK_HOST0 shmif0 $BUS0
	rump_server_add_iface $SOCK_HOST0 shmif1 $BUS1
	rump_server_add_iface $SOCK_HOST0 shmif2 $BUS2
	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto lacp

	$atf_ifconfig shmif0 mtu $mtu_1st
	$atf_ifconfig shmif1 mtu $mtu_big
	$atf_ifconfig shmif2 mtu $mtu_small

	atf_check -s exit:0 -o match:"mtu *1500" rump.ifconfig lagg0
	atf_check -s exit:0 -o match:"mtu *$mtu_1st" rump.ifconfig shmif0
	atf_check -s exit:0 -o match:"mtu *$mtu_big" rump.ifconfig shmif1
	atf_check -s exit:0 -o match:"mtu *$mtu_small" rump.ifconfig shmif2

	# copy MTU from 1st port
	$atf_ifconfig lagg0 laggport shmif0
	atf_check -s exit:0 -o match:"mtu *$mtu_1st" rump.ifconfig lagg0
	atf_check -s exit:0 -o match:"mtu *$mtu_1st" rump.ifconfig shmif0

	# copy MTU to added port
	$atf_ifconfig lagg0 laggport shmif1
	$atf_ifconfig lagg0 laggport shmif2
	atf_check -s exit:0 -o match:"mtu *$mtu_1st" rump.ifconfig lagg0
	atf_check -s exit:0 -o match:"mtu *$mtu_1st" rump.ifconfig shmif0
	atf_check -s exit:0 -o match:"mtu *$mtu_1st" rump.ifconfig shmif1
	atf_check -s exit:0 -o match:"mtu *$mtu_1st" rump.ifconfig shmif2

	# reset MTU after detaching from lagg0
	$atf_ifconfig lagg0 -laggport shmif2
	atf_check -s exit:0 -o match:"mtu *$mtu_small" rump.ifconfig shmif2

	# change MTU of lagg0
	$atf_ifconfig lagg0 mtu 1500
	atf_check -s exit:0 -o match:"mtu *1500" rump.ifconfig lagg0
	atf_check -s exit:0 -o match:"mtu *1500" rump.ifconfig shmif0
	atf_check -s exit:0 -o match:"mtu *1500" rump.ifconfig shmif1

	# reset MTU after detching from lagg0
	$atf_ifconfig lagg0 -laggport shmif0
	$atf_ifconfig lagg0 -laggport shmif1
	atf_check -s exit:0 -o match:"mtu *$mtu_1st" rump.ifconfig shmif0
	atf_check -s exit:0 -o match:"mtu *$mtu_big" rump.ifconfig shmif1

	# MTU should not be changed
	atf_check -s exit:0 -o match:"mtu *1500" rump.ifconfig lagg0

	# copy MTU from 1st port even when MTU of lagg0 is changhed
	$atf_ifconfig lagg0 mtu 1400
	atf_check -s exit:0 -o match:"mtu *1400" rump.ifconfig lagg0
	$atf_ifconfig lagg0 laggport shmif0
	atf_check -s exit:0 -o match:"mtu *$mtu_1st" rump.ifconfig lagg0
	atf_check -s exit:0 -o match:"mtu *$mtu_1st" rump.ifconfig shmif0

	# MTU of lagg0 need not reset
	$atf_ifconfig lagg0 -laggport shmif0
	atf_check -s exit:0 -o match:"mtu *$mtu_1st" rump.ifconfig lagg0
	atf_check -s exit:0 -o match:"mtu *$mtu_1st" rump.ifconfig shmif0
}

lagg_mtu_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case lagg_lacp_basic cleanup
lagg_lacp_basic_head()
{

	atf_set "descr" "tests for LACP basic functions"
	atf_set "require.progs" "rump_server"
}

lagg_lacp_basic_body()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"

	rump_server_start $SOCK_HOST0 lagg
	rump_server_start $SOCK_HOST1 lagg
	rump_server_start $SOCK_HOST2 lagg

	export RUMP_SERVER=$SOCK_HOST0

	# added running interface
	$atf_ifconfig shmif0 create
	$atf_ifconfig shmif0 linkstr $BUS0

	$atf_ifconfig shmif1 create
	$atf_ifconfig shmif1 linkstr $BUS1

	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto lacp

	$atf_ifconfig shmif0 up
	$atf_ifconfig shmif1 up
	$atf_ifconfig lagg0 up

	$atf_ifconfig lagg0 laggport shmif0
	$atf_ifconfig lagg0 laggport shmif1
	$atf_ifconfig -w 10

	$atf_ifconfig lagg0 -laggport shmif0
	$atf_ifconfig lagg0 -laggport shmif1
	$atf_ifconfig lagg0 down

	# add the same interfaces again
	$atf_ifconfig lagg0 up
	$atf_ifconfig lagg0 laggport shmif0
	$atf_ifconfig lagg0 laggport shmif1

	# detach and re-attach protocol
	$atf_ifconfig lagg0 laggproto none
	$atf_ifconfig lagg0 laggproto lacp \
	    laggport shmif0 laggport shmif1

	$atf_ifconfig lagg0 -laggport shmif0 -laggport shmif1
	$atf_ifconfig lagg0 destroy
	$atf_ifconfig shmif0 destroy
	$atf_ifconfig shmif1 destroy

	# tests for a loopback condition
	$atf_ifconfig shmif0 create
	$atf_ifconfig shmif0 linkstr $BUS0
	$atf_ifconfig shmif1 create
	$atf_ifconfig shmif1 linkstr $BUS0
	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto lacp \
	    laggport shmif0 laggport shmif1
	$atf_ifconfig shmif0 up
	$atf_ifconfig shmif1 up
	$atf_ifconfig lagg0 up

	expected_inactive lagg0

	$atf_ifconfig shmif0 down
	$atf_ifconfig shmif0 destroy
	$atf_ifconfig shmif1 down
	$atf_ifconfig shmif1 destroy
	$atf_ifconfig lagg0 down
	$atf_ifconfig lagg0 destroy

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig shmif0 create
	$atf_ifconfig shmif0 linkstr $BUS0
	$atf_ifconfig shmif0 up

	$atf_ifconfig shmif1 create
	$atf_ifconfig shmif1 linkstr $BUS1
	$atf_ifconfig shmif1 up

	$atf_ifconfig shmif2 create
	$atf_ifconfig shmif2 linkstr $BUS2
	$atf_ifconfig shmif2 up

	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto lacp laggport shmif0 \
	    laggport shmif1 laggport shmif2
	$atf_ifconfig lagg0 up

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig shmif0 create
	$atf_ifconfig shmif0 linkstr $BUS0
	$atf_ifconfig shmif0 up

	$atf_ifconfig shmif1 create
	$atf_ifconfig shmif1 linkstr $BUS1
	$atf_ifconfig shmif1 up

	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto lacp
	$atf_ifconfig lagg1 create
	$atf_ifconfig lagg1 laggproto lacp

	$atf_ifconfig lagg0 laggport shmif0
	$atf_ifconfig lagg0 up
	wait_for_distributing lagg0 shmif0

	$atf_ifconfig lagg1 laggport shmif1
	$atf_ifconfig lagg1 up

	export RUMP_SERVER=$SOCK_HOST2
	$atf_ifconfig shmif0 create
	$atf_ifconfig shmif0 linkstr $BUS2
	$atf_ifconfig shmif0 up

	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto lacp laggport shmif0
	$atf_ifconfig lagg0 up

	export RUMP_SERVER=$SOCK_HOST0
	wait_for_distributing lagg0 shmif0
	expected_inactive lagg0 shmif1
	expected_inactive lagg0 shmif2
}

lagg_lacp_basic_cleanup()
{

	$DEBUG && dump
	cleanup
}

lagg_lacp_ping()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"

	local af=$1
	local l2proto=$2
	local atf_ping="atf_check -s exit:0 -o ignore rump.ping -c 1"
	local ping=rump.ping
	local rumplib=""
	local pfx=24
	local addr_host0=$IP4ADDR0
	local addr_host1=$IP4ADDR1

	case $af in
	"inet")
		# do nothing
		;;
	"inet6")
		atf_ping="atf_check -s exit:0 -o ignore rump.ping6 -c 1"
		rumplib="netinet6"
		pfx=64
		addr_host0=$IP6ADDR0
		addr_host1=$IP6ADDR1
		;;
	esac

	case $l2proto in
	"ether")
		iface0=shmif0
		iface1=shmif1
		iface2=shmif2
		;;
	"l2tp")
		rumplib="$rumplib l2tp"
		iface0=l2tp0
		iface1=l2tp1
		iface2=l2tp2
		;;
	esac

	rump_server_start $SOCK_HOST0 lagg $rumplib
	rump_server_start $SOCK_HOST1 lagg $rumplib

	rump_server_add_iface $SOCK_HOST0 shmif0 $BUS0
	rump_server_add_iface $SOCK_HOST0 shmif1 $BUS1
	rump_server_add_iface $SOCK_HOST0 shmif2 $BUS2

	rump_server_add_iface $SOCK_HOST1 shmif0 $BUS0
	rump_server_add_iface $SOCK_HOST1 shmif1 $BUS1
	rump_server_add_iface $SOCK_HOST1 shmif2 $BUS2

	if [ x"$l2proto" = x"l2tp" ];then
		setup_l2tp_ipv4tunnel
	fi

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto lacp laggport $iface0
	$atf_ifconfig lagg0 $af $addr_host0/$pfx
	$atf_ifconfig $iface0 up
	$atf_ifconfig lagg0 up

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto lacp laggport $iface0
	$atf_ifconfig lagg0 $af $addr_host1/$pfx
	$atf_ifconfig $iface0 up
	$atf_ifconfig lagg0 up

	export RUMP_SERVER=$SOCK_HOST0
	wait_for_distributing lagg0
	$atf_ifconfig -w 10

	export RUMP_SERVER=$SOCK_HOST1
	wait_for_distributing lagg0
	$atf_ifconfig -w 10

	$atf_ping $addr_host0

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig $iface1 up
	$atf_ifconfig lagg0 laggport $iface1 laggport $iface2
	$atf_ifconfig $iface2 up

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig $iface1 up
	$atf_ifconfig lagg0 laggport $iface1 laggport $iface2
	$atf_ifconfig $iface2 up

	export RUMP_SERVER=$SOCK_HOST0
	wait_for_distributing lagg0 $iface1
	wait_for_distributing lagg0 $iface2

	export RUMP_SERVER=$SOCK_HOST1
	wait_for_distributing lagg0 $iface1
	wait_for_distributing lagg0 $iface2

	$atf_ping $addr_host0
}

atf_test_case lagg_lacp_ipv4 cleanup
lagg_lacp_ipv4_head()
{

	atf_set "descr" "tests for IPv4 with LACP"
	atf_set "require.progs" "rump_server"
}

lagg_lacp_ipv4_body()
{

	lagg_lacp_ping "inet" "ether"
}

lagg_lacp_ipv4_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case lagg_lacp_ipv6 cleanup
lagg_lacp_ipv6_head()
{

	atf_set "descr" "tests for IPv6 with LACP"
	atf_set "require.progs" "rump_server"
}

lagg_lacp_ipv6_body()
{

	lagg_lacp_ping "inet6" "ether"
}

lagg_lacp_ipv6_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case lagg_lacp_l2tp_ipv4 cleanup
lagg_lacp_l2tp_ipv4_head()
{

	atf_set "descr" "tests for LACP over l2tp by using IPv4"
	atf_set "require.progs" "rump_server"
}

lagg_lacp_l2tp_ipv4_body()
{

	lagg_lacp_ping "inet" "l2tp"
}

lagg_lacp_l2tp_ipv4_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case lagg_lacp_l2tp_ipv6 cleanup
lagg_lacp_l2tp_ipv6_head()
{

	atf_set "descr" "tests for LACP over l2tp using IPv6"
	atf_set "require.progs" "rump_server"
}

lagg_lacp_l2tp_ipv6_body()
{

	lagg_lacp_ping "inet6" "l2tp"
}

lagg_lacp_l2tp_ipv6_cleanup()
{

	$DEBUG && dump
	cleanup
}

lagg_lacp_vlan()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"

	local af=$1
	local l2proto=$2
	local atf_ping="atf_check -s exit:0 -o ignore rump.ping -c 1"
	local rumplib="vlan"
	local pfx=24
	local vlan0_addr_host0=$IP4ADDR0
	local host0addr0=$IP4ADDR0
	local host1addr0=$IP4ADDR1
	local host0addr1=$IP4ADDR2
	local host1addr1=$IP4ADDR3

	case $af in
	"inet")
		# do nothing
		;;
	"inet6")
		atf_ping="atf_check -s exit:0 -o ignore rump.ping6 -c 1"
		rumplib="$rumplib netinet6"
		pfx=64
		host0addr0=$IP6ADDR0
		host1addr0=$IP6ADDR1
		host0addr1=$IP6ADDR2
		host1addr1=$IP6ADDR3
		;;
	esac

	case $l2proto in
	"ether")
		iface0=shmif0
		iface1=shmif1
		iface2=shmif2
		;;
	"l2tp")
		rumplib="$rumplib l2tp"
		iface0=l2tp0
		iface1=l2tp1
		iface2=l2tp2
	esac

	rump_server_start $SOCK_HOST0 lagg $rumplib
	rump_server_start $SOCK_HOST1 lagg $rumplib

	rump_server_add_iface $SOCK_HOST0 shmif0 $BUS0
	rump_server_add_iface $SOCK_HOST0 shmif1 $BUS1
	rump_server_add_iface $SOCK_HOST0 shmif2 $BUS2

	rump_server_add_iface $SOCK_HOST1 shmif0 $BUS0
	rump_server_add_iface $SOCK_HOST1 shmif1 $BUS1
	rump_server_add_iface $SOCK_HOST1 shmif2 $BUS2

	if [ x"$l2proto" = x"l2tp" ]; then
		setup_l2tp_ipv4tunnel
	fi

	for sock in $SOCK_HOST0 $SOCK_HOST1; do
		export RUMP_SERVER=$sock
		$atf_ifconfig lagg0 create
		$atf_ifconfig lagg0 laggproto lacp laggport $iface0

		$atf_ifconfig vlan0 create
		$atf_ifconfig vlan0 vlan 10 vlanif lagg0
		$atf_ifconfig vlan1 create
		$atf_ifconfig vlan1 vlan 11 vlanif lagg0

		$atf_ifconfig $iface0 up
		$atf_ifconfig lagg0 up
	done

	export RUMP_SERVER=$SOCK_HOST0
	wait_for_distributing lagg0
	$atf_ifconfig vlan0 $af $host0addr0/$pfx
	$atf_ifconfig vlan0 up
	$atf_ifconfig vlan1 $af $host0addr1/$pfx
	$atf_ifconfig vlan1 up

	export RUMP_SERVER=$SOCK_HOST1
	wait_for_distributing lagg0
	$atf_ifconfig vlan0 $af $host1addr0/$pfx
	$atf_ifconfig vlan0 up
	$atf_ifconfig vlan1 $af $host1addr1/$pfx
	$atf_ifconfig vlan1 up

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig -w 10
	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig -w 10

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ping $host1addr0
	$atf_ping $host1addr1

	$atf_ifconfig lagg0 laggport $iface1
	$atf_ifconfig $iface1 up

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig lagg0 laggport $iface1
	$atf_ifconfig $iface1 up

	export RUMP_SERVER=$SOCK_HOST0
	wait_for_distributing lagg0 $iface1

	export RUMP_SERVER=$SOCK_HOST1
	wait_for_distributing lagg0 $iface1

	$atf_ping $host0addr0
	$atf_ping $host0addr1
}

atf_test_case lagg_lacp_vlan_ipv4 cleanup
lagg_lacp_vlan_ipv4_head()
{

	atf_set "descr" "tests for IPv4 VLAN frames over LACP LAG"
	atf_set "require.progs" "rump_server"
}

lagg_lacp_vlan_ipv4_body()
{

	lagg_lacp_vlan "inet" "ether"
}

lagg_lacp_vlan_ipv4_cleanup()
{
	$DEBUG && dump
	cleanup
}

atf_test_case lagg_lacp_vlan_ipv6 cleanup
lagg_lacp_vlan_ipv6_head()
{

	atf_set "descr" "tests for IPv6 VLAN frames over LACP LAG"
	atf_set "require.progs" "rump_server"
}

lagg_lacp_vlan_ipv6_body()
{

	lagg_lacp_vlan "inet6" "ether"
}

lagg_lacp_vlan_ipv6_cleanup()
{
	$DEBUG && dump
	cleanup
}

atf_test_case lagg_lacp_vlanl2tp_ipv4 cleanup
lagg_lacp_vlanl2tp_ipv4_head()
{

	atf_set "descr" "tests for IPv4 VLAN frames over LACP L2TP LAG"
	atf_set "require.progs" "rump_server"
}

lagg_lacp_vlanl2tp_ipv4_body()
{

	lagg_lacp_vlan "inet" "l2tp"
}

lagg_lacp_vlanl2tp_ipv4_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case lagg_lacp_vlanl2tp_ipv6 cleanup
lagg_lacp_vlanl2tp_ipv6_head()
{

	atf_set "descr" "tests for IPv6 VLAN frames over LACP L2TP LAG"
	atf_set "require.progs" "rump_server"
}

lagg_lacp_vlanl2tp_ipv6_body()
{

	lagg_lacp_vlan "inet6" "l2tp"
}

lagg_lacp_vlanl2tp_ipv6_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case lagg_lacp_portpri cleanup
lagg_lacp_portpri_head()
{

	atf_set "descr" "tests for LACP port priority"
	atf_set "require.progs" "rump_server"
}

lagg_lacp_portpri_body()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"

	rump_server_start $SOCK_HOST0 lagg
	rump_server_start $SOCK_HOST1 lagg

	rump_server_add_iface $SOCK_HOST0 shmif0 $BUS0
	rump_server_add_iface $SOCK_HOST0 shmif1 $BUS1
	rump_server_add_iface $SOCK_HOST0 shmif2 $BUS2

	rump_server_add_iface $SOCK_HOST1 shmif0 $BUS0
	rump_server_add_iface $SOCK_HOST1 shmif1 $BUS1
	rump_server_add_iface $SOCK_HOST1 shmif2 $BUS2

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto lacp
	$atf_ifconfig lagg0 lagglacp maxports 2

	$atf_ifconfig shmif0 up
	$atf_ifconfig shmif1 up
	$atf_ifconfig shmif2 up

	$atf_ifconfig lagg0 laggport shmif0 pri 1000
	$atf_ifconfig lagg0 laggport shmif1 pri 2000
	$atf_ifconfig lagg0 laggport shmif2 pri 3000
	$atf_ifconfig lagg0 up

	atf_check -s exit:0 -o match:'shmif0 pri=1000' rump.ifconfig lagg0
	atf_check -s exit:0 -o match:'shmif1 pri=2000' rump.ifconfig lagg0
	atf_check -s exit:0 -o match:'shmif2 pri=3000' rump.ifconfig lagg0

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto lacp

	$atf_ifconfig shmif0 up
	$atf_ifconfig shmif1 up
	$atf_ifconfig shmif2 up

	$atf_ifconfig lagg0 laggport shmif0 pri 300
	$atf_ifconfig lagg0 laggport shmif1 pri 200
	$atf_ifconfig lagg0 laggport shmif2 pri 100
	$atf_ifconfig lagg0 up

	atf_check -s exit:0 -o match:'shmif0 pri=300' rump.ifconfig lagg0
	atf_check -s exit:0 -o match:'shmif1 pri=200' rump.ifconfig lagg0
	atf_check -s exit:0 -o match:'shmif2 pri=100' rump.ifconfig lagg0

	export RUMP_SERVER=$SOCK_HOST0
	wait_for_distributing lagg0 shmif0
	wait_for_distributing lagg0 shmif1
	wait_state "STANDBY" lagg0 shmif2

	$atf_ifconfig shmif0 down
	wait_for_distributing lagg0 shmif2

	$atf_ifconfig shmif0 up
	wait_for_distributing lagg0 shmif0

	$atf_ifconfig lagg0 laggportpri shmif0 5000
	$atf_ifconfig lagg0 laggportpri shmif1 5000
	$atf_ifconfig lagg0 laggportpri shmif2 5000

	atf_check -s exit:0 -o match:'shmif0 pri=5000' rump.ifconfig lagg0
	atf_check -s exit:0 -o match:'shmif1 pri=5000' rump.ifconfig lagg0
	atf_check -s exit:0 -o match:'shmif2 pri=5000' rump.ifconfig lagg0

	wait_state "STANDBY" lagg0 shmif0
	wait_for_distributing lagg0 shmif1
	wait_for_distributing lagg0 shmif2
}

lagg_lacp_portpri_cleanup()
{

	$DEBUG && dump
	cleanup
}

lagg_failover()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"

	local af=$1
	local l2proto=$2
	local ping="rump.ping -c 1"
	local rumplib=""
	local pfx=24
	local addr_host0=$IP4ADDR0
	local addr_host1=$IP4ADDR1

	case $af in
	"inet")
		# do nothing
		;;
	"inet6")
		ping="rump.ping6 -c 1"
		rumplib="netinet6"
		pfx=64
		addr_host0=$IP6ADDR0
		addr_host1=$IP6ADDR1
		;;
	esac

	case $l2proto in
	"ether")
		iface0="shmif0"
		iface1="shmif1"
		iface2="shmif2"
		;;
	"l2tp")
		rumplib="$rumplib l2tp"
		iface0="l2tp0"
		iface1="l2tp1"
		iface2="l2tp2"
		;;
	esac

	local atf_ping="atf_check -s exit:0 -o ignore ${ping}"

	rump_server_start $SOCK_HOST0 lagg $rumplib
	rump_server_start $SOCK_HOST1 lagg $rumplib

	rump_server_add_iface $SOCK_HOST0 shmif0 $BUS0
	rump_server_add_iface $SOCK_HOST0 shmif1 $BUS1
	rump_server_add_iface $SOCK_HOST0 shmif2 $BUS2

	rump_server_add_iface $SOCK_HOST1 shmif0 $BUS0
	rump_server_add_iface $SOCK_HOST1 shmif1 $BUS1
	rump_server_add_iface $SOCK_HOST1 shmif2 $BUS2

	if [ x"$l2proto" = x"l2tp" ]; then
		setup_l2tp_ipv4tunnel
	fi

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto failover

	$atf_ifconfig lagg0 laggport $iface0 pri 1000
	$atf_ifconfig lagg0 laggport $iface1 pri 2000
	$atf_ifconfig lagg0 laggport $iface2 pri 3000
	$atf_ifconfig lagg0 $af $addr_host0/$pfx

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto failover

	$atf_ifconfig lagg0 laggport $iface0 pri 1000
	$atf_ifconfig lagg0 laggport $iface1 pri 3000
	$atf_ifconfig lagg0 laggport $iface2 pri 2000
	$atf_ifconfig lagg0 $af $addr_host1/$pfx

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig $iface0 up
	$atf_ifconfig $iface1 up
	$atf_ifconfig $iface2 up
	$atf_ifconfig lagg0 up

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig $iface0 up
	$atf_ifconfig $iface1 up
	$atf_ifconfig $iface2 up
	$atf_ifconfig lagg0 up

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig -w 10
	wait_for_distributing lagg0 $iface0
	wait_state "COLLECTING" lagg0 $iface0
	wait_state "COLLECTING" lagg0 $iface1
	wait_state "COLLECTING" lagg0 $iface2

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig -w 10
	wait_for_distributing lagg0 $iface0
	wait_state "COLLECTING" lagg0 $iface0
	wait_state "COLLECTING" lagg0 $iface1
	wait_state "COLLECTING" lagg0 $iface2

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ping $addr_host1

	$atf_ifconfig $iface0 down
	wait_for_distributing lagg0 $iface1
	wait_state "COLLECTING" lagg0 $iface1
	wait_state "COLLECTING" lagg0 $iface2

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig $iface0 down
	wait_for_distributing lagg0 $iface2
	wait_state "COLLECTING" lagg0 $iface2
	wait_state "COLLECTING" lagg0 $iface1

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ping $addr_host1

	$atf_ifconfig lagg0 laggfailover -rx-all
	atf_check -s exit:0 -o not-match:'$iface2.+COLLECTING' rump.ifconfig lagg0

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig lagg0 laggfailover -rx-all
	atf_check -s exit:0 -o not-match:'$iface1.+COLLECTING' rump.ifconfig lagg0

	export RUMP_SERVER=$SOCK_HOST0
	atf_check -s not-exit:0 -o ignore -e ignore $ping -c 1 $addr_host1
}

atf_test_case lagg_failover_ipv4 cleanup
lagg_failover_ipv4_head()
{

	atf_set "descr" "tests for failover using IPv4"
	atf_set "require.progs" "rump_server"
}

lagg_failover_ipv4_body()
{

	lagg_failover "inet" "ether"
}

lagg_failover_ipv4_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case lagg_failover_ipv6 cleanup
lagg_failover_ipv6_head()
{

	atf_set "descr" "tests for failover using IPv6"
	atf_set "require.progs" "rump_server"
}

lagg_failover_ipv6_body()
{

	lagg_failover "inet6" "ether"
}

lagg_failover_ipv6_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case lagg_failover_l2tp_ipv4 cleanup
lagg_failover_l2tp_ipv4_head()
{

	atf_set "descr" "tests for failover over l2tp using IPv4"
	atf_set "require.progs" "rump_server"
}

lagg_failover_l2tp_ipv4_body()
{

	lagg_failover "inet" "l2tp"
}

lagg_failover_l2tp_ipv4_cleanup()
{
	$DEBUG && dump
	cleanup
}

atf_test_case lagg_failover_l2tp_ipv6 cleanup
lagg_failover_l2tp_ipv6_head()
{

	atf_set "descr" "tests for failover over l2tp using IPv6"
	atf_set "require.progs" "rump_server"
}

lagg_failover_l2tp_ipv6_body()
{

	lagg_failover "inet6" "l2tp"
}

lagg_failover_l2tp_ipv6_cleanup()
{
	$DEBUG && dump
	cleanup
}

lagg_loadbalance()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"

	local af=$1
	local l2proto=$2
	local ping="rump.ping -c 1"
	local rumplib=""
	local pfx=24
	local addr_host0=$IP4ADDR0
	local addr_host1=$IP4ADDR1

	case $af in
	"inet")
		# do nothing
		;;
	"inet6")
		ping="rump.ping6 -c 1"
		rumplib="netinet6"
		pfx=64
		addr_host0=$IP6ADDR0
		addr_host1=$IP6ADDR1
		;;
	esac

	case $l2proto in
	"ether")
		iface0=shmif0
		iface1=shmif1
		iface2=shmif2
		;;
	"l2tp")
		rumplib="$rumplib l2tp"
		iface0=l2tp0
		iface1=l2tp1
		iface2=l2tp2
		;;
	esac

	local atf_ping="atf_check -s exit:0 -o ignore ${ping}"

	rump_server_start $SOCK_HOST0 lagg $rumplib
	rump_server_start $SOCK_HOST1 lagg $rumplib

	rump_server_add_iface $SOCK_HOST0 shmif0 $BUS0
	rump_server_add_iface $SOCK_HOST0 shmif1 $BUS1
	rump_server_add_iface $SOCK_HOST0 shmif2 $BUS2

	rump_server_add_iface $SOCK_HOST1 shmif0 $BUS0
	rump_server_add_iface $SOCK_HOST1 shmif1 $BUS1
	rump_server_add_iface $SOCK_HOST1 shmif2 $BUS2

	if [ x"$l2proto" = x"l2tp" ]; then
		setup_l2tp_ipv4tunnel
	fi

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto loadbalance

	$atf_ifconfig lagg0 laggport $iface0 pri 1000
	$atf_ifconfig lagg0 laggport $iface1 pri 2000
	$atf_ifconfig lagg0 laggport $iface2 pri 3000
	$atf_ifconfig lagg0 $af $addr_host0/$pfx

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig lagg0 create
	$atf_ifconfig lagg0 laggproto loadbalance

	$atf_ifconfig lagg0 laggport $iface0 pri 1000
	$atf_ifconfig lagg0 laggport $iface1 pri 3000
	$atf_ifconfig lagg0 laggport $iface2 pri 2000
	$atf_ifconfig lagg0 $af $addr_host1/$pfx

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig $iface0 up
	$atf_ifconfig $iface1 up
	$atf_ifconfig $iface2 up
	$atf_ifconfig lagg0 up

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig $iface0 up
	$atf_ifconfig $iface1 up
	$atf_ifconfig $iface2 up
	$atf_ifconfig lagg0 up

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ifconfig -w 10
	wait_for_distributing lagg0 $iface0
	wait_state "COLLECTING" lagg0 $iface0
	wait_state "COLLECTING" lagg0 $iface1
	wait_state "COLLECTING" lagg0 $iface2

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig -w 10
	wait_state "COLLECTING,DISTRIBUTING" lagg0 $iface0
	wait_state "COLLECTING,DISTRIBUTING" lagg0 $iface1
	wait_state "COLLECTING,DISTRIBUTING" lagg0 $iface2

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ping $addr_host1

	$atf_ifconfig $iface0 down
	wait_state "COLLECTING,DISTRIBUTING" lagg0 $iface1
	wait_state "COLLECTING,DISTRIBUTING" lagg0 $iface2

	export RUMP_SERVER=$SOCK_HOST1
	$atf_ifconfig $iface0 down
	wait_state "COLLECTING,DISTRIBUTING" lagg0 $iface1
	wait_state "COLLECTING,DISTRIBUTING" lagg0 $iface2

	export RUMP_SERVER=$SOCK_HOST0
	$atf_ping $addr_host1
}

atf_test_case lagg_loadbalance_ipv4 cleanup
lagg_loadbalance_ipv4_head()
{

	atf_set "descr" "tests for loadbalance using IPv4"
	atf_set "require.progs" "rump_server"
}

lagg_loadbalance_ipv4_body()
{

	lagg_loadbalance "inet" "ether"
}

lagg_loadbalance_ipv4_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case lagg_loadbalance_ipv6 cleanup
lagg_loadbalance_ipv6_head()
{

	atf_set "descr" "tests for loadbalance using IPv6"
	atf_set "require.progs" "rump_server"
}

lagg_loadbalance_ipv6_body()
{

	lagg_loadbalance "inet6" "ether"
}

lagg_loadbalance_ipv6_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case lagg_loadbalance_l2tp_ipv4 cleanup
lagg_loadbalance_l2tp_ipv4_head()
{

	atf_set "descr" "tests for loadbalance over l2tp using IPv4"
	atf_set "require.progs" "rump_server"
}

lagg_loadbalance_l2tp_ipv4_body()
{

	lagg_loadbalance "inet" "l2tp"
}

lagg_loadbalance_l2tp_ipv4_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case lagg_loadbalance_l2tp_ipv6 cleanup
lagg_loadbalance_l2tp_ipv4_head()
{

	atf_set "descr" "tests for loadbalance over l2tp using IPv6"
	atf_set "require.progs" "rump_server"
}

lagg_loadbalance_l2tp_ipv6_body()
{

	lagg_loadbalance "inet6" "l2tp"
}

lagg_loadbalance_l2tp_ipv6_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case lagg_ifconfig
	atf_add_test_case lagg_macaddr
	atf_add_test_case lagg_ipv6lla
	atf_add_test_case lagg_mtu
	atf_add_test_case lagg_lacp_basic
	atf_add_test_case lagg_lacp_ipv4
	atf_add_test_case lagg_lacp_ipv6
	atf_add_test_case lagg_lacp_l2tp_ipv4
	atf_add_test_case lagg_lacp_l2tp_ipv6
	atf_add_test_case lagg_lacp_vlan_ipv4
	atf_add_test_case lagg_lacp_vlan_ipv6
	atf_add_test_case lagg_lacp_vlanl2tp_ipv4
	atf_add_test_case lagg_lacp_vlanl2tp_ipv6
	atf_add_test_case lagg_lacp_portpri
	atf_add_test_case lagg_failover_ipv4
	atf_add_test_case lagg_failover_ipv6
	atf_add_test_case lagg_failover_l2tp_ipv4
	atf_add_test_case lagg_failover_l2tp_ipv6
	atf_add_test_case lagg_loadbalance_ipv4
	atf_add_test_case lagg_loadbalance_ipv6
	atf_add_test_case lagg_loadbalance_l2tp_ipv4
	atf_add_test_case lagg_loadbalance_l2tp_ipv6
}
