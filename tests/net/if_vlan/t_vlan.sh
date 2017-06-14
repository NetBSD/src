#	$NetBSD: t_vlan.sh,v 1.2 2017/06/14 02:32:29 ozaki-r Exp $
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
IP6_LOCAL0=fc00:0::1
IP6_LOCAL1=fc00:1::1
IP6_REMOTE0=fc00:0::2
IP6_REMOTE1=fc00:1::2

DEBUG=${DEBUG:-false}

vlan_create_destroy_body_common()
{
	export RUMP_SERVER=${SOCK_LOCAL}

	atf_check -s exit:0 rump.ifconfig vlan0 create
	atf_check -s exit:0 rump.ifconfig vlan0 up
	atf_check -s exit:0 rump.ifconfig vlan0 down
	atf_check -s exit:0 rump.ifconfig vlan0 destroy

	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig vlan0 create
	atf_check -s exit:0 rump.ifconfig vlan0 vlan 1 vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan0 up
	atf_check -s exit:0 rump.ifconfig vlan0 destroy

	# more than one vlan interface with a same parent interface
	atf_check -s exit:0 rump.ifconfig shmif1 create
	atf_check -s exit:0 rump.ifconfig vlan0 create
	atf_check -s exit:0 rump.ifconfig vlan0 vlan 10 vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan1 create
	atf_check -s exit:0 rump.ifconfig vlan1 vlan 11 vlanif shmif0

	# more than one interface with another parent interface
	atf_check -s exit:0 rump.ifconfig vlan2 create
	atf_check -s exit:0 rump.ifconfig vlan2 vlan 12 vlanif shmif1
	atf_check -s exit:0 rump.ifconfig vlan3 create
	atf_check -s exit:0 rump.ifconfig vlan3 vlan 13 vlanif shmif1
	atf_check -s exit:0 rump.ifconfig shmif0 destroy
	atf_check -s exit:0 -o not-match:'shmif0' rump.ifconfig vlan0
	atf_check -s exit:0 -o not-match:'shmif0' rump.ifconfig vlan1
	atf_check -s exit:0 -o match:'shmif1' rump.ifconfig vlan2
	atf_check -s exit:0 -o match:'shmif1' rump.ifconfig vlan3
	atf_check -s exit:0 rump.ifconfig vlan0 destroy
	atf_check -s exit:0 rump.ifconfig vlan1 destroy
	atf_check -s exit:0 rump.ifconfig vlan2 destroy
	atf_check -s exit:0 rump.ifconfig vlan3 destroy

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
	atf_check -s exit:0 rump.ifconfig shmif0 up
	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.ifconfig shmif0 up

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.ifconfig vlan0 create
	atf_check -s exit:0 rump.ifconfig vlan0 vlan 10 vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan0 $af $local0/$prefix
	atf_check -s exit:0 rump.ifconfig vlan0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.ifconfig vlan0 create
	atf_check -s exit:0 rump.ifconfig vlan0 vlan 10 vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan0 $af $remote0/$prefix
	atf_check -s exit:0 rump.ifconfig vlan0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	extract_new_packets $BUS > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore $ping_cmd $remote0

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:'vlan 10' cat $outfile

	atf_check -s exit:0 rump.ifconfig vlan0 -vlanif
	atf_check -s exit:0 rump.ifconfig vlan0 vlan 20 vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan0 $af $local0/$prefix
	atf_check -s exit:0 rump.ifconfig vlan0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	extract_new_packets $BUS > $outfile
	atf_check -s not-exit:0 -o ignore $ping_cmd $remote0

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:'vlan 20' cat $outfile
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

vlanid_config_and_ping()
{
	local vlanid=$1
	shift

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.ifconfig vlan0 vlan $vlanid vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan0 $IP_LOCAL0/24
	atf_check -s exit:0 rump.ifconfig vlan0 up

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.ifconfig vlan0 vlan $vlanid vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan0 $IP_REMOTE0/24
	atf_check -s exit:0 rump.ifconfig vlan0 up

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -n -w 1 -c 1 $IP_REMOTE0
	atf_check -s exit:0 rump.ifconfig vlan0 -vlanif

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.ifconfig vlan0 -vlanif
}

vlanid_config_and_ping6()
{
	local vlanid=$1
	shift

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.ifconfig vlan0 vlan $vlanid vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan0 inet6 $IP6_LOCAL0/64
	atf_check -s exit:0 rump.ifconfig vlan0 up

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.ifconfig vlan0 vlan $vlanid vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan0 inet6 $IP6_REMOTE0/64
	atf_check -s exit:0 rump.ifconfig vlan0 up

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 $IP6_REMOTE0
	atf_check -s exit:0 rump.ifconfig vlan0 -vlanif

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.ifconfig vlan0 -vlanif
}

vlan_vlanid_body_common()
{
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
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig vlan0 create

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 -o ignore rump.sysctl -w $sysctl_param
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig vlan0 create

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s not-exit:0 -e ignore\
	    rump.ifconfig vlan0 vlan -1 vlanif shmif0

	$config_and_ping 0 # reserved vlan id
	$config_and_ping 1
	$config_and_ping 4094
	$config_and_ping 4095 #reserved vlan id

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s not-exit:0 -e ignore \
	    rump.ifconfig vlan0 vlan 4096 vlanif shmif0

	atf_check -s exit:0 rump.ifconfig vlan0 vlan 1 vlanif shmif0
	atf_check -s not-exit:0 -e ignore \
	    rump.ifconfig vlan0 vlan 2 vlanif shmif0

	atf_check -s not-exit:0 -e ignore \
	    rump.ifconfig vlan0 vlan 1 vlanif shmif1

	atf_check -s exit:0 rump.ifconfig vlan0 -vlanif
	atf_check -s not-exit:0 -e ignore \
	    rump.ifconfig vlan0 $local0/$prefix

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.ifconfig vlan0 vlan 10 vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan0 $af $local0/$prefix
	atf_check -s exit:0 rump.ifconfig vlan0 up
	atf_check -s exit:0 rump.ifconfig vlan1 create
	atf_check -s exit:0 rump.ifconfig vlan1 vlan 11 vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan1 $af $local1/$prefix
	atf_check -s exit:0 rump.ifconfig vlan1 up

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.ifconfig vlan0 -vlanif
	atf_check -s exit:0 rump.ifconfig vlan0 vlan 10 vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan0 $af $remote0/$prefix
	atf_check -s exit:0 rump.ifconfig vlan0 up
	atf_check -s exit:0 rump.ifconfig vlan1 create
	atf_check -s exit:0 rump.ifconfig vlan1 vlan 11 vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan1 $af $remote1/$prefix
	atf_check -s exit:0 rump.ifconfig vlan1 up

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
	export RUMP_SERVER=${SOCK_LOCAL}

	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif1 create
	atf_check -s exit:0 rump.ifconfig vlan0 create

	atf_check -s exit:0 rump.ifconfig vlan0 vlan 10 vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan0 -vlanif

	atf_check -s exit:0 rump.ifconfig vlan0 vlan 10 vlanif shmif0
	atf_check -s exit:0 rump.ifconfig vlan0 -vlanif shmif0

	atf_check -s exit:0 rump.ifconfig vlan0 vlan 10 vlanif shmif0
	atf_check -s exit:0 -e ignore rump.ifconfig vlan0 -vlanif shmif1
	atf_check -s exit:0 -e ignore rump.ifconfig vlan0 -vlanif shmif2

	atf_check -s exit:0 rump.ifconfig vlan0 -vlanif

	atf_check -s exit:0 rump.ifconfig vlan0 vlan 10 vlanif shmif0
	atf_check -s exit:0 -e match:'Invalid argument' \
	    rump.ifconfig vlan0 mtu 1497
	atf_check -s exit:0 rump.ifconfig vlan0 mtu 1496
	atf_check -s exit:0 rump.ifconfig vlan0 mtu 42
	atf_check -s exit:0 -e match:'Invalid argument' \
	    rump.ifconfig vlan0 mtu 41
	atf_check -s exit:0 rump.ifconfig vlan0 -vlanif
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

atf_init_test_cases()
{

	atf_add_test_case vlan_create_destroy
	atf_add_test_case vlan_basic
	atf_add_test_case vlan_vlanid
	atf_add_test_case vlan_configs

	atf_add_test_case vlan_create_destroy6
	atf_add_test_case vlan_basic6
	atf_add_test_case vlan_vlanid6
	atf_add_test_case vlan_configs6
}
