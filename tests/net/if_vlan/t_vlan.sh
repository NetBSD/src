#	$NetBSD: t_vlan.sh,v 1.24.2.1 2023/11/03 10:10:49 martin Exp $
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

BUS=bus
SOCK_LOCAL=unix://commsock1
SOCK_REMOTE=unix://commsock2
IP_LOCAL0=10.0.0.1
IP_LOCAL1=10.0.1.1
IP_REMOTE0=10.0.0.2
IP_REMOTE1=10.0.1.2
IP_MCADDR0=224.0.0.10
IP6_LOCAL0=fc00::1
IP6_LOCAL1=fc00:1::1
IP6_REMOTE0=fc00::2
IP6_REMOTE1=fc00:1::2
IP6_MCADDR0=ff11::10
ETH_IP_MCADDR0=01:00:5e:00:00:0a
ETH_IP6_MCADDR0=33:33:00:00:00:10

DEBUG=${DEBUG:-false}

vlan_create_destroy_body_common()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"

	export RUMP_SERVER=${SOCK_LOCAL}

	$atf_ifconfig vlan0 create
	$atf_ifconfig vlan0 destroy

	$atf_ifconfig vlan0 create
	$atf_ifconfig vlan0 up
	$atf_ifconfig vlan0 down
	$atf_ifconfig vlan0 destroy

	$atf_ifconfig shmif0 create
	$atf_ifconfig vlan0 create
	$atf_ifconfig vlan0 vlan 1 vlanif shmif0
	$atf_ifconfig vlan0 up
	$atf_ifconfig vlan0 destroy

	# more than one vlan interface with a same parent interface
	$atf_ifconfig shmif1 create
	$atf_ifconfig vlan0 create
	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan1 create
	$atf_ifconfig vlan1 vlan 11 vlanif shmif0

	# more than one interface with another parent interface
	$atf_ifconfig vlan2 create
	$atf_ifconfig vlan2 vlan 12 vlanif shmif1
	$atf_ifconfig vlan3 create
	$atf_ifconfig vlan3 vlan 13 vlanif shmif1
	$atf_ifconfig shmif0 destroy
	atf_check -s exit:0 -o not-match:'shmif0' rump.ifconfig vlan0
	atf_check -s exit:0 -o not-match:'shmif0' rump.ifconfig vlan1
	atf_check -s exit:0 -o match:'shmif1' rump.ifconfig vlan2
	atf_check -s exit:0 -o match:'shmif1' rump.ifconfig vlan3
	$atf_ifconfig vlan0 destroy
	$atf_ifconfig vlan1 destroy
	$atf_ifconfig vlan2 destroy
	$atf_ifconfig vlan3 destroy

}

atf_test_case vlan_create_destroy cleanup
vlan_create_destroy_head()
{

	atf_set "descr" "tests of creation and deletion of vlan interface"
	atf_set "require.progs" "rump_server"
}

vlan_create_destroy_body()
{
	rump_server_start $SOCK_LOCAL vlan

	vlan_create_destroy_body_common
}


vlan_create_destroy_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case vlan_create_destroy6 cleanup
vlan_create_destroy6_head()
{

	atf_set "descr" "tests of creation and deletion of vlan interface with IPv6"
	atf_set "require.progs" "rump_server"
}

vlan_create_destroy6_body()
{

	rump_server_start $SOCK_LOCAL vlan netinet6

	vlan_create_destroy_body_common
}

vlan_create_destroy6_cleanup()
{

	$DEBUG && dump
	cleanup
}

vlan_basic_body_common()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"
	local outfile=./out
	local af=inet
	local prefix=24
	local local0=$IP_LOCAL0
	local remote0=$IP_REMOTE0
	local ping_cmd="rump.ping -n -w 1 -c 1"

	if [ x"$1" = x"inet6" ]; then
		af="inet6"
		prefix=64
		local0=$IP6_LOCAL0
		remote0=$IP6_REMOTE0
		ping_cmd="rump.ping6 -n -c 1"
	fi

	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	$atf_ifconfig shmif0 up
	export RUMP_SERVER=$SOCK_REMOTE
	$atf_ifconfig shmif0 up

	export RUMP_SERVER=$SOCK_LOCAL
	$atf_ifconfig vlan0 create
	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan0 $af $local0/$prefix
	$atf_ifconfig vlan0 up
	$atf_ifconfig -w 10

	export RUMP_SERVER=$SOCK_REMOTE
	$atf_ifconfig vlan0 create
	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan0 $af $remote0/$prefix
	$atf_ifconfig vlan0 up
	$atf_ifconfig -w 10

	extract_new_packets $BUS > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore $ping_cmd $remote0

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:'vlan 10' cat $outfile

	$atf_ifconfig vlan0 -vlanif
	$atf_ifconfig vlan0 vlan 20 vlanif shmif0
	$atf_ifconfig vlan0 $af $local0/$prefix
	$atf_ifconfig vlan0 up
	$atf_ifconfig -w 10

	extract_new_packets $BUS > $outfile
	atf_check -s not-exit:0 -o ignore $ping_cmd $remote0

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:'vlan 20' cat $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	$atf_ifconfig vlan0 -vlanif
	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan0 $af $local0/$prefix
	$atf_ifconfig vlan0 up
	$atf_ifconfig -w 10

	atf_check -s exit:0 -o ignore rump.ifconfig -z vlan0
	atf_check -s exit:0 -o ignore $ping_cmd $remote0
	rump.ifconfig -v vlan0 > $outfile

	atf_check -s exit:0 -o not-match:' 0 packets' cat $outfile
	atf_check -s exit:0 -o not-match:' 0 bytes' cat $outfile
}

atf_test_case vlan_basic cleanup
vlan_basic_head()
{

	atf_set "descr" "tests of communications over vlan interfaces"
	atf_set "require.progs" "rump_server"
}

vlan_basic_body()
{
	rump_server_start $SOCK_LOCAL vlan
	rump_server_start $SOCK_REMOTE vlan

	vlan_basic_body_common inet

}

vlan_basic_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case vlan_basic6 cleanup
vlan_basic6_head()
{

	atf_set "descr" "tests of communications over vlan interfaces using IPv6"
	atf_set "require.progs" "rump_server"
}

vlan_basic6_body()
{
	rump_server_start $SOCK_LOCAL vlan netinet6
	rump_server_start $SOCK_REMOTE vlan netinet6

	vlan_basic_body_common inet6
}

vlan_basic6_cleanup()
{

	$DEBUG && dump
	cleanup
}

vlan_auto_follow_mtu_body_common()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"
	local outfile=./out
	local af=inet
	local prefix=24
	local local0=$IP_LOCAL0
	local remote0=$IP_REMOTE0
	local ping_cmd="rump.ping -D -n -w 1 -c 1"
	local mtu=1500
	local vlan_mtu=`expr $mtu - 4`
	# ipv4 header=20bytes, icmp header=8bytes
	local padding=`expr $vlan_mtu - 20 - 8`
	local over_padding=`expr $vlan_mtu - 20 - 8 + 1`
	local nonfrag_msg="$local0 > $remote0: ICMP echo request"
	# unused for ipv4
	local frag_msg=""

	if [ x"$1" = x"inet6" ]; then
		af="inet6"
		prefix=64
		local0=$IP6_LOCAL0
		remote0=$IP6_REMOTE0
		# ipv6 header=40bytes, icmpv6 header=8bytes
		padding=`expr $vlan_mtu - 40 - 8`
		over_padding=`expr $vlan_mtu - 40 - 8 + 1`
		ping_cmd="rump.ping6 -mm -n -c 1 -i 1"
		nonfrag_msg="$local0 > $remote0: ICMP6, echo request"
		frag_msg="$local0 > $remote0: frag .* ICMP6, echo request"
	fi

	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	$atf_ifconfig shmif0 up
	export RUMP_SERVER=$SOCK_REMOTE
	$atf_ifconfig shmif0 up

	export RUMP_SERVER=$SOCK_LOCAL
	$atf_ifconfig vlan0 create

	# since upper bound of shmif's mtu is 1500,
	# so we lower vlan's mtu instead of raising shmif's.
	# to do this, we change the interface's parameter
	# such as ND_IFINFO(ifp)->maxmtu that is changed by SIOCSIFMTU.

	# $atf_config shmif0 mtu 1600
	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan0 mtu 1400
	$atf_ifconfig vlan0 -vlanif shmif0

	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan0 $af $local0/$prefix
	$atf_ifconfig vlan0 up
	$atf_ifconfig -w 10

	export RUMP_SERVER=$SOCK_REMOTE
	$atf_ifconfig vlan0 create
	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan0 $af $remote0/$prefix
	$atf_ifconfig vlan0 up
	$atf_ifconfig -w 10

	extract_new_packets $BUS > $outfile
	export RUMP_SERVER=$SOCK_LOCAL

	atf_check -s exit:0 -o ignore $ping_cmd -s $padding $remote0
	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:"$nonfrag_msg" cat $outfile

	if [ x"$1" = x"inet6" ]; then
		atf_check -s exit:0 -o ignore $ping_cmd -s $over_padding $remote0
		extract_new_packets $BUS > $outfile
		atf_check -s exit:0 -o match:"$frag_msg" cat $outfile
	else
		atf_check -s not-exit:0 -o ignore -e match:"Message too long" \
				$ping_cmd -s $over_padding $remote0
	fi
}

atf_test_case vlan_auto_follow_mtu cleanup
vlan_auto_follow_mtu_head()
{

	atf_set "descr" "tests of setting vlan mtu using IPv4"
	atf_set "require.progs" "rump_server"
}

vlan_auto_follow_mtu_body()
{
	rump_server_start $SOCK_LOCAL vlan
	rump_server_start $SOCK_REMOTE vlan

	vlan_auto_follow_mtu_body_common inet
}

vlan_auto_follow_mtu_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case vlan_auto_follow_mtu6 cleanup
vlan_auto_follow_mtu6_head()
{

	atf_set "descr" "tests of setting vlan mtu using IPv6"
	atf_set "require.progs" "rump_server"
}

vlan_auto_follow_mtu6_body()
{
	rump_server_start $SOCK_LOCAL vlan netinet6
	rump_server_start $SOCK_REMOTE vlan netinet6

	vlan_auto_follow_mtu_body_common inet6
}

vlan_auto_follow_mtu6_cleanup()
{

	$DEBUG && dump
	cleanup
}

vlanid_config_and_ping()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"
	local vlanid=$1
	shift

	export RUMP_SERVER=$SOCK_LOCAL
	$atf_ifconfig vlan0 vlan $vlanid vlanif shmif0
	$atf_ifconfig vlan0 $IP_LOCAL0/24
	$atf_ifconfig vlan0 up

	export RUMP_SERVER=$SOCK_REMOTE
	$atf_ifconfig vlan0 vlan $vlanid vlanif shmif0
	$atf_ifconfig vlan0 $IP_REMOTE0/24
	$atf_ifconfig vlan0 up

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -n -w 1 -c 1 $IP_REMOTE0
	$atf_ifconfig vlan0 -vlanif

	export RUMP_SERVER=$SOCK_REMOTE
	$atf_ifconfig vlan0 -vlanif
}

vlanid_config_and_ping6()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"
	local vlanid=$1
	shift

	export RUMP_SERVER=$SOCK_LOCAL
	$atf_ifconfig vlan0 vlan $vlanid vlanif shmif0
	$atf_ifconfig vlan0 inet6 $IP6_LOCAL0/64
	$atf_ifconfig vlan0 up

	export RUMP_SERVER=$SOCK_REMOTE
	$atf_ifconfig vlan0 vlan $vlanid vlanif shmif0
	$atf_ifconfig vlan0 inet6 $IP6_REMOTE0/64
	$atf_ifconfig vlan0 up

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 $IP6_REMOTE0
	$atf_ifconfig vlan0 -vlanif

	export RUMP_SERVER=$SOCK_REMOTE
	$atf_ifconfig vlan0 -vlanif
}

vlan_vlanid_body_common()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"
	local af=inet
	local prefix=24
	local sysctl_param="net.inet.ip.dad_count=0"
	local ping_cmd="rump.ping -n -w 1 -c 1"
	local config_and_ping=vlanid_config_and_ping
	local local0=$IP_LOCAL0
	local local1=$IP_LOCAL1
	local remote0=$IP_REMOTE0
	local remote1=$IP_REMOTE1

	if [ x"$1" = x"inet6" ]; then
		af=inet6
		prefix=64
		sysctl_param="net.inet6.ip6.dad_count=0"
		ping_cmd="rump.ping6 -n -c 1"
		config_and_ping=vlanid_config_and_ping6
		local0=$IP6_LOCAL0
		local1=$IP6_LOCAL1
		remote0=$IP6_REMOTE0
		remote1=$IP6_REMOTE1
	fi

	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.sysctl -w $sysctl_param
	$atf_ifconfig shmif0 up
	$atf_ifconfig vlan0 create

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 -o ignore rump.sysctl -w $sysctl_param
	$atf_ifconfig shmif0 up
	$atf_ifconfig vlan0 create

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s not-exit:0 -e match:"^usage: rump.ifconfig" \
	    rump.ifconfig vlan0 vlan -1 vlanif shmif0

	# $config_and_ping 0 # reserved vlan id
	$config_and_ping 1
	$config_and_ping 4094
	# $config_and_ping 4095 #reserved vlan id

	if [ "${RANDOM:-0}" != "${RANDOM:-0}" ]
	then
		for TAG in $(( ${RANDOM:-0} % 4092 + 2 )) \
			   $(( ${RANDOM:-0} % 4092 + 2 )) \
			   $(( ${RANDOM:-0} % 4092 + 2 ))
		do
			$config_and_ping "${TAG}"
		done
	fi

	export RUMP_SERVER=$SOCK_LOCAL
	for TAG in 0 4095 4096 $((4096*4 + 1)) 65536 65537 $((65536 + 4095))
	do
		atf_check -s not-exit:0 -e not-empty \
		    rump.ifconfig vlan0 vlan "${TAG}" vlanif shmif0
	done

	$atf_ifconfig vlan0 vlan 1 vlanif shmif0
	atf_check -s not-exit:0 -e match:"SIOCSETVLAN: Device busy" \
	    rump.ifconfig vlan0 vlan 2 vlanif shmif0

	atf_check -s not-exit:0 -e match:"SIOCSETVLAN: Device busy" \
	    rump.ifconfig vlan0 vlan 1 vlanif shmif1

	$atf_ifconfig vlan0 -vlanif
	atf_check -s not-exit:0 -e match:"Invalid argument" \
	    rump.ifconfig vlan0 $af $local0/$prefix

	export RUMP_SERVER=$SOCK_LOCAL
	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan0 $af $local0/$prefix
	$atf_ifconfig vlan0 up
	$atf_ifconfig vlan1 create
	$atf_ifconfig vlan1 vlan 11 vlanif shmif0
	$atf_ifconfig vlan1 $af $local1/$prefix
	$atf_ifconfig vlan1 up

	export RUMP_SERVER=$SOCK_REMOTE
	$atf_ifconfig vlan0 -vlanif
	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan0 $af $remote0/$prefix
	$atf_ifconfig vlan0 up
	$atf_ifconfig vlan1 create
	$atf_ifconfig vlan1 vlan 11 vlanif shmif0
	$atf_ifconfig vlan1 $af $remote1/$prefix
	$atf_ifconfig vlan1 up

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore $ping_cmd $remote0
	atf_check -s exit:0 -o ignore $ping_cmd $remote1
}

atf_test_case vlan_vlanid cleanup
vlan_vlanid_head()
{

	atf_set "descr" "tests of configuration for vlan id"
	atf_set "require.progs" "rump_server"
}

vlan_vlanid_body()
{
	rump_server_start $SOCK_LOCAL vlan
	rump_server_start $SOCK_REMOTE vlan

	vlan_vlanid_body_common inet
}

vlan_vlanid_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case vlan_vlanid6 cleanup
vlan_vlanid6_head()
{

	atf_set "descr" "tests of configuration for vlan id using IPv6"
	atf_set "require.progs" "rump_server"
}


vlan_vlanid6_body()
{
	rump_server_start $SOCK_LOCAL vlan netinet6
	rump_server_start $SOCK_REMOTE vlan netinet6

	vlan_vlanid_body_common inet6
}

vlan_vlanid6_cleanup()
{

	$DEBUG && dump
	cleanup
}

vlan_configs_body_common()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"

	export RUMP_SERVER=${SOCK_LOCAL}

	$atf_ifconfig shmif0 create
	$atf_ifconfig shmif1 create
	# unset U/L bit to detect a bug fixed by if_vlan.c:r1.132
	$atf_ifconfig shmif0 link b0:a0:75:00:01:00 active
	$atf_ifconfig shmif1 link b0:a0:75:00:01:01 active
	$atf_ifconfig vlan0 create

	atf_check -s exit:0 -o match:'status: +down' \
	    rump.ifconfig vlan0
	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan0 -vlanif
	atf_check -s exit:0 -o match:'status: +down' \
	    rump.ifconfig vlan0

	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan0 -vlanif shmif0

	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan0 -vlanif shmif1
	atf_check -s exit:0 rump.ifconfig vlan0 -vlanif shmif2

	$atf_ifconfig vlan0 -vlanif

	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	atf_check -s exit:0 -e match:'Invalid argument' \
	    rump.ifconfig vlan0 mtu 1497
	$atf_ifconfig vlan0 mtu 1496
	$atf_ifconfig vlan0 mtu 42
	atf_check -s exit:0 -e match:'Invalid argument' \
	    rump.ifconfig vlan0 mtu 41
	$atf_ifconfig vlan0 -vlanif

	$atf_ifconfig vlan1 create
	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	atf_check -s not-exit:0 -e match:'File exists' \
	    rump.ifconfig vlan1 vlan 10 vlanif shmif0
	$atf_ifconfig vlan1 vlan 10 vlanif shmif1

	$atf_ifconfig vlan1 -vlanif shmif1
	$atf_ifconfig vlan1 vlan 10 vlanif shmif1

	$atf_ifconfig vlan0 -vlanif shmif0
	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
}

atf_test_case vlan_configs cleanup
vlan_configs_head()
{
	atf_set "descr" "tests of configuration except vlan id"
	atf_set "require.progs" "rump_server"
}

vlan_configs_body()
{

	rump_server_start $SOCK_LOCAL vlan

	vlan_configs_body_common

}

vlan_configs_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case vlan_configs6 cleanup
vlan_configs6_head()
{
	atf_set "descr" "tests of configuration except vlan id using IPv6"
	atf_set "require.progs" "rump_server"
}

vlan_configs6_body()
{
	rump_server_start $SOCK_LOCAL vlan netinet6

	vlan_configs_body_common
}

vlan_configs6_cleanup()
{
	$DEBUG && dump
	cleanup
}

vlan_bridge_body_common()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"
	local atf_brconfig="atf_check -s exit:0 $HIJACKING /sbin/brconfig"

	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	$atf_ifconfig shmif0 up

	$atf_ifconfig vlan0 create
	$DEBUG && rump.ifconfig vlan0

	$atf_ifconfig bridge0 create
	$atf_ifconfig bridge0 up

	#
	# Add vlan to bridge member
	#
	$atf_ifconfig bridge0 mtu 1496

	# vlan0 can not add to bridge member
	# because it is not an ethernet device
	atf_check -s not-exit:0 -e match:'Invalid argument' \
	    $HIJACKING /sbin/brconfig bridge0 add vlan0

	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan0 up
	atf_check -s exit:0 -o match:'mtu 1496' rump.ifconfig vlan0

	# vlan0 becomes an ethernet device
	# after attaching the parent interface
	$atf_brconfig bridge0 add vlan0
	$DEBUG && $HIJACKING /sbin/brconfig bridge0

	$atf_brconfig bridge0 delete vlan0

	$atf_brconfig bridge0 add vlan0
	$atf_ifconfig vlan0 -vlanif
	atf_check -s exit:0 -o not-match:'vlan0' \
	    $HIJACKING /sbin/brconfig bridge0
	atf_check -s not-exit:0 -e match:'No such' \
	    $HIJACKING /sbin/brconfig bridge0 delete vlan0

	#
	# decrease MTU on adding to bridge member
	#
	$atf_ifconfig bridge0 mtu 1495
	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan0 up
	atf_check -s exit:0 -o match:'mtu 1496' rump.ifconfig vlan0

	$atf_brconfig bridge0 add vlan0
	$DEBUG && $HIJACKING /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:'mtu 1495' rump.ifconfig vlan0
	$atf_brconfig bridge0 delete vlan0

	#
	# increase MTU on adding to bridge member
	#
	$atf_ifconfig bridge0 mtu 1496
	$atf_ifconfig vlan0 mtu 1495
	$atf_brconfig bridge0 add vlan0

	$DEBUG && $HIJACKING /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:'mtu 1496' rump.ifconfig vlan0
	$atf_brconfig bridge0 delete vlan0

	$atf_ifconfig bridge0 mtu 1497
	atf_check -s not-exit:0 -o ignore -e ignore \
	    $HIJACKING /sbin/brconfig bridge0 add vlan0

	#
	# Destroy a vlan interface that is bridge member
	#
	$atf_ifconfig bridge0 mtu 1496
	$atf_brconfig bridge0 add vlan0
	$atf_ifconfig vlan0 destroy

	rump_server_destroy_ifaces
}

atf_test_case vlan_bridge cleanup
vlan_bridge_head()
{

	atf_set "descr" "tests of vlan interfaces with bridges (IPv4)"
	atf_set "require.progs" "rump_server"
}

vlan_bridge_body()
{

	rump_server_start $SOCK_LOCAL vlan bridge
	vlan_bridge_body_common
}

vlan_bridge_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case vlan_bridge6 cleanup
vlan_bridge6_head()
{

	atf_set "descr" "tests of vlan interfaces with bridges (IPv6)"
	atf_set "require.progs" "rump_server"
}

vlan_bridge6_body()
{

	rump_server_start $SOCK_LOCAL vlan netinet6 bridge
	vlan_bridge_body_common
}

vlan_bridge6_cleanup()
{

	$DEBUG && dump
	cleanup
}

vlan_multicast_body_common()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"
	local af="inet"
	local local0=$IP_LOCAL0
	local local1=$IP_LOCAL1
	local mcaddr=$IP_MCADDR0
	local eth_mcaddr=$ETH_IP_MCADDR0
	local prefix=24
	local siocXmulti="$(atf_get_srcdir)/siocXmulti"
	local atf_siocXmulti="atf_check -s exit:0 $HIJACKING $siocXmulti"

	if [ x"$1" =  x"inet6" ]; then
		af="inet6"
		prefix=64
		local0=$IP6_LOCAL0
		local1=$IP6_LOCAL1
		mcaddr=$IP6_MCADDR0
		eth_mcaddr=$ETH_IP6_MCADDR0
	fi

	export RUMP_SERVER=$SOCK_LOCAL

	$atf_ifconfig shmif0 create
	$atf_ifconfig shmif0 linkstr net0 up
	$atf_ifconfig vlan0 create
	$atf_ifconfig vlan0 vlan 10 vlanif shmif0
	$atf_ifconfig vlan0 $af $local0/$prefix up
	$atf_ifconfig vlan1 create
	$atf_ifconfig vlan1 vlan 11 vlanif shmif0
	$atf_ifconfig vlan1 $af $local1/$prefix up
	$atf_ifconfig -w 10

	# check the initial state
	atf_check -s exit:0 -o not-match:"$eth_mcaddr" $HIJACKING ifmcstat

	# add a multicast address
	$atf_siocXmulti add vlan0 $mcaddr
	atf_check -s exit:0 -o match:"$eth_mcaddr" $HIJACKING ifmcstat

	# delete the address
	$atf_siocXmulti del vlan0 $mcaddr
	atf_check -s exit:0 -o not-match:"$eth_mcaddr" $HIJACKING ifmcstat

	# delete a non-existing address
	atf_check -s not-exit:0 -e match:"Invalid argument" \
	    $HIJACKING $siocXmulti del vlan0 $mcaddr

	# add an address to different interfaces
	$atf_siocXmulti add vlan0 $mcaddr
	$atf_siocXmulti add vlan1 $mcaddr
	atf_check -s exit:0 -o match:"${eth_mcaddr} refcount 2" $HIJACKING ifmcstat
	$atf_siocXmulti del vlan0 $mcaddr

	# delete the address with invalid interface
	atf_check -s not-exit:0 -e match:"Invalid argument" \
	    $HIJACKING $siocXmulti del vlan0 $mcaddr

	$atf_siocXmulti del vlan1 $mcaddr

	# add and delete a same address more than once
	$atf_siocXmulti add vlan0 $mcaddr
	$atf_siocXmulti add vlan0 $mcaddr
	$atf_siocXmulti add vlan0 $mcaddr
	atf_check -s exit:0 -o match:"${eth_mcaddr} refcount 3" $HIJACKING ifmcstat
	$atf_siocXmulti del vlan0 $mcaddr
	$atf_siocXmulti del vlan0 $mcaddr
	$atf_siocXmulti del vlan0 $mcaddr
	atf_check -s exit:0 -o not-match:"$eth_mcaddr" $HIJACKING ifmcstat

	# delete all address added to parent device when remove
	# the config of parent interface
	$atf_siocXmulti add vlan0 $mcaddr
	$atf_siocXmulti add vlan0 $mcaddr
	$atf_siocXmulti add vlan0 $mcaddr
	$atf_ifconfig vlan0 -vlanif shmif0
	atf_check -s exit:0 -o not-match:"$eth_mcaddr" $HIJACKING ifmcstat
}

atf_test_case vlan_multicast cleanup
vlan_multicast_head()
{
	atf_set "descr" "tests of multicast address adding and deleting"
	atf_set "require.progs" "rump_server"
}

vlan_multicast_body()
{
	rump_server_start $SOCK_LOCAL vlan

	vlan_multicast_body_common inet
}

vlan_multicast_cleanup()
{
	$DEBUG && dump
	cleanup
}

atf_test_case vlan_multicast6 cleanup
vlan_multicast6_head()
{
	atf_set "descr" "tests of multicast address adding and deleting with IPv6"
	atf_set "require.progs" "rump_server"
}

vlan_multicast6_body()
{
	rump_server_start $SOCK_LOCAL vlan netinet6

	vlan_multicast_body_common inet6
}

vlan_multicast6_cleanup()
{
	$DEBUG && dump
	cleanup
}

atf_test_case vlan_promisc cleanup
vlan_promisc_head()
{

	atf_set "descr" "tests of IFF_PROMISC of vlan"
	atf_set "require.progs" "rump_server"
}

vlan_promisc_body()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"
	local atf_brconfig="atf_check -s exit:0 $HIJACKING /sbin/brconfig"
	local atf_arp="atf_check -s exit:0 rump.arp"
	local bpfopen="$HIJACKING $(atf_get_srcdir)/bpfopen"
	bpfopen="$bpfopen -dv -b /rump/dev/bpf"
	local pidfile="./bpfopen.pid"
	local macaddr=""

	rump_server_bpf_start $SOCK_LOCAL vlan bridge
	rump_server_start $SOCK_REMOTE vlan

	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_LOCAL shmif1
	rump_server_add_iface $SOCK_LOCAL vlan0
	rump_server_add_iface $SOCK_LOCAL vlan1
	rump_server_add_iface $SOCK_LOCAL bridge0

	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS
	rump_server_add_iface $SOCK_REMOTE vlan0

	macaddr=$(get_macaddr $SOCK_LOCAL shmif1)

	export RUMP_SERVER=$SOCK_REMOTE
	$atf_ifconfig vlan0 vlan 1 vlanif shmif0
	$atf_ifconfig shmif0 up
	$atf_ifconfig vlan0 inet $IP_REMOTE0/24
	$atf_ifconfig vlan0 up
	$atf_ifconfig -w 10
	$atf_arp -s $IP_LOCAL0 $macaddr

	export RUMP_SERVER=$SOCK_LOCAL
	$atf_ifconfig bridge0 mtu 1496
	#
	# When vlan IF is PROMISC, the parent is also PROMISC
	#
	$atf_ifconfig vlan0 vlan 1 vlanif shmif0
	$atf_ifconfig shmif0 up
	$atf_ifconfig vlan0 up

	atf_check -s exit:0 -o not-match:'PROMISC' rump.ifconfig vlan0
	atf_check -s exit:0 -o not-match:'PROMISC' rump.ifconfig shmif0

	$atf_brconfig bridge0 add vlan0
	$atf_ifconfig bridge0 up
	atf_check -s exit:0 -o match:'PROMISC' rump.ifconfig vlan0
	atf_check -s exit:0 -o match:'PROMISC' rump.ifconfig shmif0

	$atf_ifconfig bridge0 down
	$atf_brconfig bridge0 delete vlan0
	atf_check -s exit:0 -o not-match:'PROMISC' rump.ifconfig vlan0
	atf_check -s exit:0 -o not-match:'PROMISC' rump.ifconfig shmif0
	$atf_ifconfig vlan0 -vlanif

	#
	# drop unicast packets that is not for the host
	#
	$atf_ifconfig vlan0 vlan 1 vlanif shmif0
	$atf_ifconfig -w 10

	atf_check -s exit:0 -e match:'bpf opened' $bpfopen -p $pidfile shmif0

	atf_check -s exit:0 -o not-match:'PROMISC' rump.ifconfig vlan0
	atf_check -s exit:0 -o match:'PROMISC' rump.ifconfig shmif0
	atf_check -s exit:0 -o ignore rump.ifconfig -z vlan0
	atf_check -s exit:0 -o not-match:'input:.*errors' \
	    rump.ifconfig -v vlan0

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping -c 3 -i 0.2 $IP_LOCAL0

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o match:'input:.*errors' \
	    rump.ifconfig -v vlan0

	atf_check -s exit:0 kill -TERM $(cat $pidfile)
	sleep 2

	atf_check -s exit:0 -o not-match:'PROMISC' rump.ifconfig vlan0
	atf_check -s exit:0 -o not-match:'PROMISC' rump.ifconfig shmif0
	$atf_ifconfig vlan0 -vlanif

	#
	# clear IFF_PROMISC after bpf_detach called from ether_ifdetach
	#
	$atf_ifconfig vlan0 vlan 1 vlanif shmif0
	$atf_ifconfig vlan0 up

	atf_check -s exit:0 -e match:'bpf opened' $bpfopen -p $pidfile vlan0

	atf_check -s exit:0 -o match:'PROMISC' rump.ifconfig vlan0
	atf_check -s exit:0 -o match:'PROMISC' rump.ifconfig shmif0

	$atf_ifconfig vlan0 -vlanif

	atf_check -s exit:0 -o not-match:'PROMISC' rump.ifconfig vlan0
	atf_check -s exit:0 -o not-match:'PROMISC' rump.ifconfig shmif0

	atf_check -s exit:0 kill -TERM $(cat $pidfile)
	sleep 2
	atf_check -s exit:0 -o not-match:'PROMISC' rump.ifconfig vlan0
}

vlan_promisc_cleanup()
{

	$DEBUG && dump
	cleanup
}

vlan_l2tp_body_common()
{
	local atf_ifconfig="atf_check -s exit:0 rump.ifconfig"

	local af=$1
	local ping_cmd="rump.ping -c 1"
	local pfx=24
	local local0=$IP_LOCAL0
	local local1=$IP_LOCAL1
	local remote0=$IP_REMOTE0
	local remote1=$IP_REMOTE1
	local sysctl_param="net.inet.ip.dad_count=0"
	local vid0=10
	local vid1=11

	local l2tp_laddr=10.222.222.1
	local l2tp_lsession=1001
	local l2tp_raddr=10.222.222.2
	local l2tp_rsession=1002

	if [ x"$af" = x"inet6" ]; then
		ping_cmd="rump.ping6 -c 1"
		rumplib="netinet6"
		pfx=64
		local0=$IP6_LOCAL0
		local1=$IP6_LOCAL1
		remote0=$IP6_REMOTE0
		remote1=$IP6_REMOTE1
		sysctl_param="net.inet6.ip6.dad_count=0"
	fi

	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_LOCAL l2tp0
	rump_server_add_iface $SOCK_LOCAL vlan0
	rump_server_add_iface $SOCK_LOCAL vlan1

	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS
	rump_server_add_iface $SOCK_REMOTE l2tp0
	rump_server_add_iface $SOCK_REMOTE vlan0
	rump_server_add_iface $SOCK_REMOTE vlan1

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.sysctl -w $sysctl_param
	$atf_ifconfig shmif0 $l2tp_laddr/24
	$atf_ifconfig l2tp0 tunnel  $l2tp_laddr $l2tp_raddr
	$atf_ifconfig l2tp0 session $l2tp_lsession $l2tp_rsession
	$atf_ifconfig l2tp0 up

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 -o ignore rump.sysctl -w $sysctl_param
	$atf_ifconfig shmif0 $l2tp_raddr/24
	$atf_ifconfig l2tp0 tunnel  $l2tp_raddr $l2tp_laddr
	$atf_ifconfig l2tp0 session $l2tp_rsession $l2tp_lsession
	$atf_ifconfig l2tp0 up

	# configure vlans on l2tp(4)
	export RUMP_SERVER=$SOCK_LOCAL
	$atf_ifconfig vlan0 vlan $vid0 vlanif l2tp0
	$atf_ifconfig vlan0 $af $local0/$pfx
	$atf_ifconfig vlan1 vlan $vid1 vlanif l2tp0
	$atf_ifconfig vlan1 $af $local1/$pfx
	export RUMP_SERVER=$SOCK_REMOTE
	$atf_ifconfig vlan0 vlan $vid0 vlanif l2tp0
	$atf_ifconfig vlan0 $af $remote0/$pfx
	$atf_ifconfig vlan1 vlan $vid1 vlanif l2tp0
	$atf_ifconfig vlan1 $af $remote1/$pfx

	# test for VLAN frame transfer
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore $ping_cmd $remote0
	atf_check -s exit:0 -o ignore $ping_cmd $remote1

	# unconfig vlans
	export RUMP_SERVER=$SOCK_LOCAL
	$atf_ifconfig vlan0 -vlanif
	export RUMP_SERVER=$SOCK_REMOTE
	$atf_ifconfig vlan0 -vlanif

	# remove l2tp0 that has vlan1
	export RUMP_SERVER=$SOCK_LOCAL
	$atf_ifconfig l2tp0 destroy
	export RUMP_SERVER=$SOCK_REMOTE
	$atf_ifconfig l2tp0 destroy
}

atf_test_case vlan_l2tp cleanup
vlan_l2tp_head()
{

	atf_set "descr" "tests of vlan(IPv4) over l2tp(IPv4)"
	atf_set "require.progs" "rump_server"
}

vlan_l2tp_body()
{

	rump_server_start $SOCK_LOCAL  vlan l2tp
	rump_server_start $SOCK_REMOTE vlan l2tp

	vlan_l2tp_body_common "inet"
}

vlan_l2tp_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case vlan_l2tp6 cleanup
vlan_l2tp6_head()
{

	atf_set "descr" "tests of vlan(IPv6) over l2tp(IPv4)"
	atf_set "require.progs" "rump_server"
}

vlan_l2tp6_body()
{

	rump_server_start $SOCK_LOCAL  vlan l2tp netinet6
	rump_server_start $SOCK_REMOTE vlan l2tp netinet6

	vlan_l2tp_body_common "inet6"
}

vlan_l2tp6_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case vlan_create_destroy
	atf_add_test_case vlan_basic
	atf_add_test_case vlan_auto_follow_mtu
	atf_add_test_case vlan_vlanid
	atf_add_test_case vlan_configs
	atf_add_test_case vlan_bridge
	atf_add_test_case vlan_multicast
	atf_add_test_case vlan_promisc
	atf_add_test_case vlan_l2tp

	atf_add_test_case vlan_create_destroy6
	atf_add_test_case vlan_basic6
	atf_add_test_case vlan_auto_follow_mtu6
	atf_add_test_case vlan_vlanid6
	atf_add_test_case vlan_configs6
	atf_add_test_case vlan_bridge6
	atf_add_test_case vlan_multicast6
	atf_add_test_case vlan_l2tp6
}
