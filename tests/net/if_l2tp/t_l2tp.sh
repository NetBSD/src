#	$NetBSD: t_l2tp.sh,v 1.2.8.2 2018/02/26 00:41:13 snj Exp $
#
# Copyright (c) 2017 Internet Initiative Japan Inc.
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

LAC1SOCK=unix://commsock1
LAC2SOCK=unix://commsock2
CLIENT1SOCK=unix://commsock3
CLIENT2SOCK=unix://commsock4

WAN_LINK=bus0
LAC1_LAN_LINK=bus1
LAC2_LAN_LINK=bus2

LAC1_WANIP=10.0.0.1
LAC1_SESSION=1234
CLIENT1_LANIP=192.168.1.1
LAC2_WANIP=10.0.0.2
LAC2_SESSION=4321
CLIENT2_LANIP=192.168.1.2

LAC1_WANIP6=fc00::1
CLIENT1_LANIP6=fc00:1::1
LAC2_WANIP6=fc00::2
CLIENT2_LANIP6=fc00:1::2

TIMEOUT=5
DEBUG=${DEBUG:-false}

atf_test_case l2tp_create_destroy cleanup
l2tp_create_destroy_head()
{

	atf_set "descr" "Test creating/destroying l2tp interfaces"
	atf_set "require.progs" "rump_server"
}

l2tp_create_destroy_body()
{

	rump_server_start $LAC1SOCK l2tp

	test_create_destroy_common $LAC1SOCK l2tp0
}

l2tp_create_destroy_cleanup()
{

	$DEBUG && dump
	cleanup
}

setup_lac()
{
	sock=${1}
	lanlink=${2}
	wan=${3}
	wan_mode=${4}


	rump_server_add_iface ${sock} shmif0 ${lanlink}
	rump_server_add_iface ${sock} shmif1 ${WAN_LINK}

	export RUMP_SERVER=${sock}

	if [ ${wan_mode} = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig shmif1 inet6 ${wan}
	else
		atf_check -s exit:0 rump.ifconfig shmif1 inet ${wan} netmask 0xff000000
	fi
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig shmif1 up

	unset RUMP_SERVER
}

test_lac()
{
	sock=${1}
	wan=${2}
	wan_mode=${3}

	export RUMP_SERVER=${sock}

	atf_check -s exit:0 -o match:shmif0 rump.ifconfig
	atf_check -s exit:0 -o match:shmif1 rump.ifconfig
	if [ ${wan_mode} = "ipv6" ]; then
		atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 -X $TIMEOUT ${wan}
	else
		atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w $TIMEOUT ${wan}
	fi

	unset RUMP_SERVER
}

setup_client()
{
	sock=${1}
	lanlink=${2}
	lan=${3}
	lan_mode=${4}

	rump_server_add_iface ${sock} shmif0 ${lanlink}

	export RUMP_SERVER=${sock}
	if [ ${lan_mode} = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig shmif0 inet6 ${lan}
	else
		atf_check -s exit:0 rump.ifconfig shmif0 inet ${lan} netmask 0xffffff00
	fi
	atf_check -s exit:0 rump.ifconfig shmif0 up

	unset RUMP_SERVER
}

test_client()
{
	sock=${1}
	lan=${2}
	lan_mode=${3}

	export RUMP_SERVER=${sock}

	atf_check -s exit:0 -o match:shmif0 rump.ifconfig
	if [ ${lan_mode} = "ipv6" ]; then
		atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 -X $TIMEOUT ${lan}
	else
		atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w $TIMEOUT ${lan}
	fi

	unset RUMP_SERVER
}

setup()
{
	lan_mode=${1}
	wan_mode=${2}

	rump_server_start $LAC1SOCK netinet6 bridge l2tp
	rump_server_start $LAC2SOCK netinet6 bridge l2tp
	rump_server_start $CLIENT1SOCK netinet6 bridge l2tp
	rump_server_start $CLIENT2SOCK netinet6 bridge l2tp

	client1_lan=""
	client2_lan=""
	if [ ${lan_mode} = "ipv6" ]; then
		client1_lan=${CLIENT1_LANIP6}
		client2_lan=${CLIENT2_LANIP6}
	else
		client1_lan=${CLIENT1_LANIP}
		client2_lan=${CLIENT2_LANIP}
	fi

	if [ ${wan_mode} = "ipv6" ]; then
		setup_lac $LAC1SOCK $LAC1_LAN_LINK $LAC1_WANIP6 ${wan_mode}
		setup_lac $LAC2SOCK $LAC2_LAN_LINK $LAC2_WANIP6 ${wan_mode}
		setup_client $CLIENT1SOCK $LAC1_LAN_LINK \
			     ${client1_lan} ${lan_mode}
		setup_client $CLIENT2SOCK $LAC2_LAN_LINK \
			     ${client2_lan} ${lan_mode}
	else
		setup_lac $LAC1SOCK $LAC1_LAN_LINK $LAC1_WANIP ${wan_mode}
		setup_lac $LAC2SOCK $LAC2_LAN_LINK $LAC2_WANIP ${wan_mode}
		setup_client $CLIENT1SOCK $LAC1_LAN_LINK \
			     ${client1_lan} ${lan_mode}
		setup_client $CLIENT2SOCK $LAC2_LAN_LINK \
			     ${client2_lan} ${lan_mode}
	fi
}

test_setup()
{
	lan_mode=${1}
	wan_mode=${2}

	client1_lan=""
	client2_lan=""
	if [ ${lan_mode} = "ipv6" ]; then
		client1_lan=$CLIENT1_LANIP6
		client2_lan=$CLIENT2_LANIP6
	else
		client1_lan=$CLIENT1_LANIP
		client2_lan=$CLIENT2_LANIP
	fi
	if [ ${wan_mode} = "ipv6" ]; then
		test_lac ${LAC1SOCK} $LAC1_WANIP6 ${wan_mode}
		test_lac ${LAC2SOCK} $LAC2_WANIP6 ${wan_mode}
		test_client ${CLIENT1SOCK} ${client1_lan} ${lan_mode}
		test_client ${CLIENT2SOCK} ${client2_lan} ${lan_mode}
	else
		test_lac ${LAC1SOCK} $LAC1_WANIP ${wan_mode}
		test_lac ${LAC2SOCK} $LAC2_WANIP ${wan_mode}
		test_client ${CLIENT1SOCK} ${client1_lan} ${lan_mode}
		test_client ${CLIENT2SOCK} ${client2_lan} ${lan_mode}
	fi
}

setup_if_l2tp()
{
	sock=${1}
	src=${2}
	dst=${3}
	src_session=${4}
	dst_session=${5}

	export RUMP_SERVER=${sock}

	atf_check -s exit:0 rump.ifconfig l2tp0 create
	atf_check -s exit:0 rump.ifconfig l2tp0 tunnel ${src} ${dst}
	atf_check -s exit:0 rump.ifconfig l2tp0 session ${src_session} ${dst_session}
	atf_check -s exit:0 rump.ifconfig l2tp0 up

	atf_check -s exit:0 rump.ifconfig bridge0 create
	atf_check -s exit:0 rump.ifconfig bridge0 up
	export LD_PRELOAD=/usr/lib/librumphijack.so
	atf_check -s exit:0 brconfig bridge0 add shmif0
	atf_check -s exit:0 brconfig bridge0 add l2tp0
	unset LD_PRELOAD

	$DEBUG && rump.ifconfig -v l2tp0
	$DEBUG && rump.ifconfig -v bridge0

	unset RUMP_SERVER
}

setup_tunnel()
{
	wan_mode=${1}

	src=""
	dst=""
	src_session=""
	dst_session=""

	if [ ${wan_mode} = "ipv6" ]; then
		src=$LAC1_WANIP6
		dst=$LAC2_WANIP6
	else
		src=$LAC1_WANIP
		dst=$LAC2_WANIP
	fi
	src_session=${LAC1_SESSION}
	dst_session=${LAC2_SESSION}
	setup_if_l2tp $LAC1SOCK ${src} ${dst} ${src_session} ${dst_session}

	if [ ${wan_mode} = "ipv6" ]; then
		src=$LAC2_WANIP6
		dst=$LAC1_WANIP6
	else
		src=$LAC2_WANIP
		dst=$LAC1_WANIP
	fi
	src_session=${LAC2_SESSION}
	dst_session=${LAC1_SESSION}
	setup_if_l2tp $LAC2SOCK ${src} ${dst} ${src_session} ${dst_session}
}

test_setup_tunnel()
{
	mode=${1}

	if [ ${mode} = "ipv6" ]; then
		lac1_wan=$LAC1_WANIP6
		lac2_wan=$LAC2_WANIP6
	else
		lac1_wan=$LAC1_WANIP
		lac2_wan=$LAC2_WANIP
	fi
	export RUMP_SERVER=$LAC1SOCK
	atf_check -s exit:0 -o match:l2tp0 rump.ifconfig
	if [ ${mode} = "ipv6" ]; then
	    atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 -X $TIMEOUT ${lac2_wan}
	else
	    atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w $TIMEOUT ${lac2_wan}
	fi

	export RUMP_SERVER=$LAC2SOCK
	atf_check -s exit:0 -o match:l2tp0 rump.ifconfig
	if [ ${mode} = "ipv6" ]; then
	    atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 -X $TIMEOUT ${lac1_wan}
	else
	    atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w $TIMEOUT ${lac1_wan}
	fi

	unset RUMP_SERVER
}

teardown_tunnel()
{
	export RUMP_SERVER=$LAC1SOCK
	atf_check -s exit:0 rump.ifconfig bridge0 destroy
	atf_check -s exit:0 rump.ifconfig l2tp0 deletetunnel
	atf_check -s exit:0 rump.ifconfig l2tp0 destroy

	export RUMP_SERVER=$LAC2SOCK
	atf_check -s exit:0 rump.ifconfig bridge0 destroy
	atf_check -s exit:0 rump.ifconfig l2tp0 deletetunnel
	atf_check -s exit:0 rump.ifconfig l2tp0 destroy

	unset RUMP_SERVER
}

test_ping_failure()
{
	mode=$1

	export RUMP_SERVER=$CLIENT1SOCK
	if [ ${mode} = "ipv6" ]; then
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping6 -n -X $TIMEOUT -c 1 $CLIENT2_LANIP6
	else
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping -n -w $TIMEOUT -c 1 $CLIENT2_LANIP
	fi

	export RUMP_SERVER=$CLIENT2SOCK
	if [ ${mode} = "ipv6" ]; then
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping6 -n -X $TIMEOUT -c 1 $CLIENT1_LANIP6
	else
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping -n -w $TIMEOUT -c 1 $CLIENT1_LANIP
	fi

	unset RUMP_SERVER
}

test_ping_success()
{
	mode=$1

	export RUMP_SERVER=$CLIENT1SOCK
	if [ ${mode} = "ipv6" ]; then
		# XXX
		# rump.ping6 rarely fails with the message that
		# "failed to get receiving hop limit".
		# This is a known issue being analyzed.
		atf_check -s exit:0 -o ignore \
			rump.ping6 -n -X $TIMEOUT -c 1 $CLIENT2_LANIP6
	else
		atf_check -s exit:0 -o ignore \
			rump.ping -n -w $TIMEOUT -c 1 $CLIENT2_LANIP
	fi
	export RUMP_SERVER=$LAC1SOCK
	$DEBUG && rump.ifconfig -v l2tp0
	$DEBUG && rump.ifconfig -v bridge0
	$DEBUG && rump.ifconfig -v shmif0

	export RUMP_SERVER=$CLIENT2SOCK
	if [ ${mode} = "ipv6" ]; then
		atf_check -s exit:0 -o ignore \
			rump.ping6 -n -X $TIMEOUT -c 1 $CLIENT1_LANIP6
	else
		atf_check -s exit:0 -o ignore \
			rump.ping -n -w $TIMEOUT -c 1 $CLIENT1_LANIP
	fi
	export RUMP_SERVER=$LAC2SOCK
	$DEBUG && rump.ifconfig -v l2tp0
	$DEBUG && rump.ifconfig -v bridge0
	$DEBUG && rump.ifconfig -v shmif0

	unset RUMP_SERVER
}

basic_setup()
{
	lan_mode=$1
	wan_mode=$2

	setup ${lan_mode} ${wan_mode}
	test_setup ${lan_mode} ${wan_mode}

	# Enable once PR kern/49219 is fixed
	#test_ping_failure

	setup_tunnel ${wan_mode}
	sleep 1
	test_setup_tunnel ${wan_mode}
}

basic_test()
{
	lan_mode=$1
	wan_mode=$2 # not use

	test_ping_success ${lan_mode}
}

basic_teardown()
{
	lan_mode=$1
	wan_mode=$2 # not use

	teardown_tunnel
	test_ping_failure ${lan_mode}
}

add_test()
{
	category=$1
	desc=$2
	lan_mode=$3
	wan_mode=$4

	name="l2tp_${category}_${lan_mode}over${wan_mode}"
	fulldesc="Does ${lan_mode} over ${wan_mode} if_l2tp ${desc}"

	atf_test_case ${name} cleanup
	eval "${name}_head() {
			atf_set descr \"${fulldesc}\"
			atf_set require.progs rump_server
		}
	    ${name}_body() {
			${category}_setup ${lan_mode} ${wan_mode}
			${category}_test ${lan_mode} ${wan_mode}
			${category}_teardown ${lan_mode} ${wan_mode}
			rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
			\$DEBUG && dump
			cleanup
		}"
	atf_add_test_case ${name}
}

add_test_allproto()
{
	category=$1
	desc=$2

	add_test ${category} "${desc}" ipv4 ipv4
	add_test ${category} "${desc}" ipv4 ipv6
	add_test ${category} "${desc}" ipv6 ipv4
	add_test ${category} "${desc}" ipv6 ipv6
}

atf_init_test_cases()
{

	atf_add_test_case l2tp_create_destroy

	add_test_allproto basic "basic tests"
#	add_test_allproto recursive "recursive check tests"
}
