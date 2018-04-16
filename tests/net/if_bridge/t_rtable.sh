#	$NetBSD: t_rtable.sh,v 1.1.14.1 2018/04/16 02:00:10 pgoyette Exp $
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

SOCK1=unix://commsock1
SOCK2=unix://commsock2
SOCK3=unix://commsock3
IP1=10.0.0.1
IP2=10.0.0.2

DEBUG=${DEBUG:-false}
TIMEOUT=5

setup_endpoint()
{
	local sock=${1}
	local addr=${2}
	local bus=${3}
	local mode=${4}

	rump_server_add_iface $sock shmif0 $bus
	export RUMP_SERVER=${sock}
	if [ $mode = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig shmif0 inet6 ${addr}
	else
		atf_check -s exit:0 rump.ifconfig shmif0 inet ${addr} netmask 0xffffff00
	fi

	atf_check -s exit:0 rump.ifconfig shmif0 up
	$DEBUG && rump.ifconfig shmif0
}

setup_bridge_server()
{

	rump_server_add_iface $SOCK2 shmif0 bus1
	rump_server_add_iface $SOCK2 shmif1 bus2
	export RUMP_SERVER=$SOCK2
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig shmif1 up
}

setup()
{

	rump_server_start $SOCK1 bridge
	rump_server_start $SOCK2 bridge
	rump_server_start $SOCK3 bridge

	setup_endpoint $SOCK1 $IP1 bus1 ipv4
	setup_endpoint $SOCK3 $IP2 bus2 ipv4
	setup_bridge_server
}

setup_bridge()
{

	export RUMP_SERVER=$SOCK2
	atf_check -s exit:0 rump.ifconfig bridge0 create
	atf_check -s exit:0 rump.ifconfig bridge0 up

	export LD_PRELOAD=/usr/lib/librumphijack.so
	atf_check -s exit:0 /sbin/brconfig bridge0 add shmif0
	atf_check -s exit:0 /sbin/brconfig bridge0 add shmif1
	/sbin/brconfig bridge0
	unset LD_PRELOAD
	rump.ifconfig shmif0
	rump.ifconfig shmif1
}

get_number_of_caches()
{

	export RUMP_SERVER=$SOCK2
	export LD_PRELOAD=/usr/lib/librumphijack.so
	echo $(($(/sbin/brconfig bridge0 |grep -A 100 "Address cache" |wc -l) - 1))
	unset LD_PRELOAD
}


atf_test_case bridge_rtable_basic cleanup
bridge_rtable_basic_head()
{

	atf_set "descr" "Tests basic opearaions of bridge's learning table"
	atf_set "require.progs" "rump_server"
}

bridge_rtable_basic_body()
{
	local addr1= addr3=

	setup
	setup_bridge

	# Get MAC addresses of the endpoints.
	addr1=$(get_macaddr $SOCK1 shmif0)
	addr3=$(get_macaddr $SOCK3 shmif0)

	# Confirm there is no MAC address caches.
	export RUMP_SERVER=$SOCK2
	export LD_PRELOAD=/usr/lib/librumphijack.so
	$DEBUG && /sbin/brconfig bridge0
	atf_check -s exit:0 -o not-match:"$addr1" /sbin/brconfig bridge0
	atf_check -s exit:0 -o not-match:"$addr3" /sbin/brconfig bridge0
	unset LD_PRELOAD

	# Make the bridge learn the MAC addresses of the endpoints.
	export RUMP_SERVER=$SOCK1
	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 $IP2
	unset RUMP_SERVER

	# Tests the addresses are in the cache.
	export RUMP_SERVER=$SOCK2
	export LD_PRELOAD=/usr/lib/librumphijack.so
	$DEBUG && /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr1 shmif0" /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr3 shmif1" /sbin/brconfig bridge0

	# Tests brconfig deladdr
	atf_check -s exit:0 -o ignore /sbin/brconfig bridge0 deladdr "$addr1"
	atf_check -s exit:0 -o not-match:"$addr1 shmif0" /sbin/brconfig bridge0
	atf_check -s exit:0 -o ignore /sbin/brconfig bridge0 deladdr "$addr3"
	atf_check -s exit:0 -o not-match:"$addr3 shmif1" /sbin/brconfig bridge0
	unset LD_PRELOAD

	rump_server_destroy_ifaces
}

bridge_rtable_basic_cleanup()
{

	$DEBUG && dump
	cleanup
}


atf_test_case bridge_rtable_flush cleanup
bridge_rtable_flush_head()
{

	atf_set "descr" "Tests brconfig flush"
	atf_set "require.progs" "rump_server"
}

bridge_rtable_flush_body()
{
	local addr1= addr3=

	setup
	setup_bridge

	# Get MAC addresses of the endpoints.
	addr1=$(get_macaddr $SOCK1 shmif0)
	addr3=$(get_macaddr $SOCK3 shmif0)

	# Make the bridge learn the MAC addresses of the endpoints.
	export RUMP_SERVER=$SOCK1
	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 $IP2
	unset RUMP_SERVER

	# Tests the addresses are in the cache.
	export RUMP_SERVER=$SOCK2
	export LD_PRELOAD=/usr/lib/librumphijack.so
	$DEBUG && /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr1 shmif0" /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr3 shmif1" /sbin/brconfig bridge0

	# Tests brconfig flush.
	atf_check -s exit:0 -o ignore /sbin/brconfig bridge0 flush
	atf_check -s exit:0 -o not-match:"$addr1 shmif0" /sbin/brconfig bridge0
	atf_check -s exit:0 -o not-match:"$addr3 shmif1" /sbin/brconfig bridge0
	unset LD_PRELOAD

	rump_server_destroy_ifaces
}

bridge_rtable_flush_cleanup()
{

	$DEBUG && dump
	cleanup
}


atf_test_case bridge_rtable_timeout cleanup
bridge_rtable_timeout_head()
{

	atf_set "descr" "Tests cache timeout of bridge's learning table"
	atf_set "require.progs" "rump_server"
}

bridge_rtable_timeout_body()
{
	local addr1= addr3=
	local timeout=5

	setup
	setup_bridge

	# Get MAC addresses of the endpoints.
	addr1=$(get_macaddr $SOCK1 shmif0)
	addr3=$(get_macaddr $SOCK3 shmif0)

	# Tests brconfig timeout.
	export RUMP_SERVER=$SOCK2
	export LD_PRELOAD=/usr/lib/librumphijack.so
	atf_check -s exit:0 -o match:"timeout: 1200" /sbin/brconfig bridge0
	atf_check -s exit:0 -o ignore /sbin/brconfig bridge0 timeout $timeout
	atf_check -s exit:0 -o match:"timeout: $timeout" /sbin/brconfig bridge0
	unset LD_PRELOAD

	# Make the bridge learn the MAC addresses of the endpoints.
	export RUMP_SERVER=$SOCK1
	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 $IP2
	unset RUMP_SERVER

	# Tests the addresses are in the cache.
	export RUMP_SERVER=$SOCK2
	export LD_PRELOAD=/usr/lib/librumphijack.so
	$DEBUG && /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr1 shmif0" /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr3 shmif1" /sbin/brconfig bridge0

	# TODO: cache expiration
	# The initial timeout value of a cache is changed to $timeout and
	# after $timeout elapsed the cache is ready to be sweeped. However,
	# the GC of rtable runs every 5 minutes and the cache remains until
	# then. Should we have a sysctl to change the period?

	#sleep $(($timeout + 2))
	#
	## Tests the addresses are not in the cache.
	#export RUMP_SERVER=$SOCK2
	#export LD_PRELOAD=/usr/lib/librumphijack.so
	#$DEBUG && /sbin/brconfig bridge0
	#atf_check -s exit:0 -o not-match:"$addr1 shmif0" /sbin/brconfig bridge0
	#atf_check -s exit:0 -o not-match:"$addr3 shmif1" /sbin/brconfig bridge0

	rump_server_destroy_ifaces
}

bridge_rtable_timeout_cleanup()
{

	$DEBUG && dump
	cleanup
}


atf_test_case bridge_rtable_maxaddr cleanup
bridge_rtable_maxaddr_head()
{

	atf_set "descr" "Tests brconfig maxaddr"
	atf_set "require.progs" "rump_server"
}

bridge_rtable_maxaddr_body()
{
	local addr1= addr3=

	setup
	setup_bridge

	# Get MAC addresses of the endpoints.
	addr1=$(get_macaddr $SOCK1 shmif0)
	addr3=$(get_macaddr $SOCK3 shmif0)

	# Fill the MAC addresses of the endpoints.
	export RUMP_SERVER=$SOCK1
	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 $IP2
	export RUMP_SERVER=$SOCK2
	export LD_PRELOAD=/usr/lib/librumphijack.so
	/sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr1 shmif0" /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr3 shmif1" /sbin/brconfig bridge0

	# Check the default # of caches is 100
	atf_check -s exit:0 -o match:"max cache: 100" /sbin/brconfig bridge0

	# Test two MAC addresses are cached
	n=$(get_number_of_caches)
	atf_check_equal $n 2

	# Limit # of caches to one
	atf_check -s exit:0 -o ignore /sbin/brconfig bridge0 maxaddr 1
	atf_check -s exit:0 -o match:"max cache: 1" /sbin/brconfig bridge0
	/sbin/brconfig bridge0

	# Test just one address is cached
	n=$(get_number_of_caches)
	atf_check_equal $n 1

	# Increase # of caches to two
	atf_check -s exit:0 -o ignore /sbin/brconfig bridge0 maxaddr 2
	atf_check -s exit:0 -o match:"max cache: 2" /sbin/brconfig bridge0
	unset LD_PRELOAD

	# Test we can cache two addresses again
	export RUMP_SERVER=$SOCK1
	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 $IP2
	export RUMP_SERVER=$SOCK2
	export LD_PRELOAD=/usr/lib/librumphijack.so
	/sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr1 shmif0" /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr3 shmif1" /sbin/brconfig bridge0
	unset LD_PRELOAD

	rump_server_destroy_ifaces
}

bridge_rtable_maxaddr_cleanup()
{

	$DEBUG && dump
	cleanup
}


atf_test_case bridge_rtable_delete_member cleanup
bridge_rtable_delete_member_head()
{

	atf_set "descr" "Tests belonging rtable entries are removed on deleting an interface"
	atf_set "require.progs" "rump_server"
}

bridge_rtable_delete_member_body()
{
	local addr10= addr30= addr11= addr31=
	local n=

	setup
	setup_bridge

	# Add extra interfaces and addresses
	export RUMP_SERVER=$SOCK1
	rump_server_add_iface $SOCK1 shmif1 bus1
	atf_check -s exit:0 rump.ifconfig shmif1 10.0.0.11/24
	atf_check -s exit:0 rump.ifconfig -w 10

	export RUMP_SERVER=$SOCK3
	rump_server_add_iface $SOCK3 shmif1 bus2
	atf_check -s exit:0 rump.ifconfig shmif1 10.0.0.12/24
	atf_check -s exit:0 rump.ifconfig -w 10

	# Get MAC addresses of the endpoints.
	addr10=$(get_macaddr $SOCK1 shmif0)
	addr30=$(get_macaddr $SOCK3 shmif0)
	addr11=$(get_macaddr $SOCK1 shmif1)
	addr31=$(get_macaddr $SOCK3 shmif1)

	# Make the bridge learn the MAC addresses of the endpoints.
	export RUMP_SERVER=$SOCK1
	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 10.0.0.12
	export RUMP_SERVER=$SOCK3
	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 10.0.0.11

	export RUMP_SERVER=$SOCK2
	export LD_PRELOAD=/usr/lib/librumphijack.so
	$DEBUG && /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr10 shmif0" /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr11 shmif0" /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr30 shmif1" /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr31 shmif1" /sbin/brconfig bridge0

	atf_check -s exit:0 -o ignore /sbin/brconfig bridge0 delete shmif0
	atf_check -s exit:0 -o not-match:"$addr10 shmif0" /sbin/brconfig bridge0
	atf_check -s exit:0 -o not-match:"$addr11 shmif0" /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr30 shmif1" /sbin/brconfig bridge0
	atf_check -s exit:0 -o match:"$addr31 shmif1" /sbin/brconfig bridge0

	atf_check -s exit:0 -o ignore /sbin/brconfig bridge0 delete shmif1
	atf_check -s exit:0 -o not-match:"$addr10 shmif0" /sbin/brconfig bridge0
	atf_check -s exit:0 -o not-match:"$addr11 shmif0" /sbin/brconfig bridge0
	atf_check -s exit:0 -o not-match:"$addr30 shmif1" /sbin/brconfig bridge0
	atf_check -s exit:0 -o not-match:"$addr31 shmif1" /sbin/brconfig bridge0

	rump_server_destroy_ifaces
}

bridge_rtable_delete_member_cleanup()
{

	$DEBUG && dump
	cleanup
}


atf_init_test_cases()
{

	atf_add_test_case bridge_rtable_basic
	atf_add_test_case bridge_rtable_flush
	atf_add_test_case bridge_rtable_timeout
	atf_add_test_case bridge_rtable_maxaddr
	atf_add_test_case bridge_rtable_delete_member
	# TODO: brconfig static/flushall/discover/learn
}
