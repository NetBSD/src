#	$NetBSD: t_basic.sh,v 1.1 2020/08/26 16:03:42 riastradh Exp $
#
# Copyright (c) 2018 Ryota Ozaki <ozaki.ryota@gmail.com>
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
SOCK_LOCAL=unix://wg_local
SOCK_PEER=unix://wg_peer
SOCK_PEER2=unix://wg_peer2


check_ping_payload()
{
	local proto=$1
	local ip=$2
	local ping= size=

	if [ $proto = inet ]; then
		ping="atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w 1"
	else
		ping="atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 -X 1"
	fi

	for size in $(seq 1 100) $(seq 450 550) $(seq 1400 1500); do
		$ping -s $size $ip
	done
}

test_common()
{
	local type=$1
	local outer_proto=$2
	local inner_proto=$3
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local port=51820
	local ip_local= ip_peer=
	local ip_wg_local= ip_wg_peer=
	local outer_prefix= outer_prefixall=
	local inner_prefix= inner_prefixall=

	if [ $outer_proto = inet ]; then
		ip_local=192.168.1.1
		ip_peer=192.168.1.2
		outer_prefix=24
		outer_prefixall=32
	else
		ip_local=fc00::1
		ip_peer=fc00::2
		outer_prefix=64
		outer_prefixall=128
	fi

	if [ $inner_proto = inet ]; then
		ip_wg_local=10.0.0.1
		ip_wg_peer=10.0.0.2
		inner_prefix=24
		inner_prefixall=32
	else
		ip_wg_local=fd00::1
		ip_wg_peer=fd00::2
		inner_prefix=64
		inner_prefixall=128
	fi

	setup_servers

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 $outer_proto $ip_local $outer_prefix
	setup_wg_common wg0 $inner_proto $ip_wg_local $inner_prefix $port "$key_priv_local"

	export RUMP_SERVER=$SOCK_PEER
	setup_common shmif0 $outer_proto $ip_peer $outer_prefix
	setup_wg_common wg0 $inner_proto $ip_wg_peer $inner_prefix $port "$key_priv_peer"

	export RUMP_SERVER=$SOCK_LOCAL
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/$inner_prefixall

	export RUMP_SERVER=$SOCK_PEER
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/$inner_prefixall

	if [ $type = basic ]; then
		export RUMP_SERVER=$SOCK_LOCAL
		check_ping $inner_proto $ip_wg_peer
	elif [ $type = payload ]; then
		export RUMP_SERVER=$SOCK_LOCAL
		check_ping_payload $inner_proto $ip_wg_peer
	fi

	destroy_wg_interfaces
}

atf_test_case wg_create_destroy cleanup
wg_create_destroy_head()
{

	atf_set "descr" "tests to create/destroy wg(4) interfaces"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_create_destroy_body()
{

	rump_server_crypto_start $SOCK_LOCAL netinet6 wg

	test_create_destroy_common $SOCK_LOCAL wg0 true
}

wg_create_destroy_cleanup()
{

	$DEBUG && dump
	cleanup
}

wg_create_destroy_peers_common()
{
	local proto=$1
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local port=51820
	local ip_local= ip_peer=
	local ip_wg_local= ip_wg_peer=
	local outer_prefix= outer_prefixall=
	local inner_prefix= inner_prefixall=

	if [ $proto = inet ]; then
		ip_local=192.168.1.1
		ip_peer=192.168.1.2
		outer_prefix=24
		outer_prefixall=32
		ip_wg_local=10.0.0.1
		ip_wg_peer=10.0.0.2
		inner_prefix=24
		inner_prefixall=32
	else
		ip_local=fc00::1
		ip_peer=fc00::2
		outer_prefix=64
		outer_prefixall=128
		ip_wg_local=fd00::1
		ip_wg_peer=fd00::2
		inner_prefix=64
		inner_prefixall=128
	fi

	rump_server_crypto_start $SOCK_LOCAL netinet6 wg
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 $proto $ip_local $outer_prefix
	setup_wg_common wg0 $proto $ip_wg_local $inner_prefix $port "$key_priv_local"

	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/$inner_prefixall

	delete_peer wg0 peer0
}

atf_test_case wg_create_destroy_peers_ipv4 cleanup
wg_create_destroy_peers_ipv4_head()
{

	atf_set "descr" "tests to create/destroy peers (IPv4)"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_create_destroy_peers_ipv4_body()
{

	wg_create_destroy_peers_common inet
}

wg_create_destroy_peers_ipv4_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case wg_create_destroy_peers_ipv6 cleanup
wg_create_destroy_peers_ipv6_head()
{

	atf_set "descr" "tests to create/destroy peers (IPv6)"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_create_destroy_peers_ipv6_body()
{

	wg_create_destroy_peers_common inet6
}

wg_create_destroy_peers_ipv6_cleanup()
{

	$DEBUG && dump
	cleanup
}

add_basic_test()
{
	local inner=$1
	local outer=$2
	local ipv4=inet
	local ipv6=inet6

	name="wg_basic_${inner}_over_${outer}"
	fulldesc="Test wg(4) with ${inner} over ${outer}"

	eval inner=\$$inner
	eval outer=\$$outer

	atf_test_case ${name} cleanup
	eval "
		${name}_head() {
			atf_set descr \"${fulldesc}\"
			atf_set require.progs rump_server wgconfig wg-keygen
		}
		${name}_body() {
			test_common basic $outer $inner
			rump_server_destroy_ifaces
		}
		${name}_cleanup() {
			\$DEBUG && dump
			cleanup
		}"
	atf_add_test_case ${name}
}

add_payload_sizes_test()
{
	local inner=$1
	local outer=$2
	local ipv4=inet
	local ipv6=inet6

	name="wg_payload_sizes_${inner}_over_${outer}"
	fulldesc="Test wg(4) with ${inner} over ${outer} with various payload sizes"

	eval inner=\$$inner
	eval outer=\$$outer

	atf_test_case ${name} cleanup
	eval "
		${name}_head() {
			atf_set descr \"${fulldesc}\"
			atf_set require.progs rump_server wgconfig wg-keygen
		}
		${name}_body() {
			test_common payload $outer $inner
			rump_server_destroy_ifaces
		}
		${name}_cleanup() {
			\$DEBUG && dump
			cleanup
		}"
	atf_add_test_case ${name}
}

atf_test_case wg_multiple_interfaces cleanup
wg_multiple_interfaces_head()
{

	atf_set "descr" "tests multiple wg(4) interfaces"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_multiple_interfaces_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping -n -i 0.1 -c 3 -w 1"
	local ping_fail="atf_check -s not-exit:0 -o ignore rump.ping -n -c 1 -w 1"
	local key_priv_peer2=
	local key_pub_peer2=
	local ip_local=192.168.1.1
	local ip_local2=192.168.2.1
	local ip_peer=192.168.1.2
	local ip_peer2=192.168.2.2
	local ip_wg_local=10.0.0.1
	local ip_wg_local2=10.0.1.1
	local ip_wg_peer=10.0.0.2
	local ip_wg_peer2=10.0.1.2
	local port=51820
	local port2=51821
	local outfile=./out

	setup_servers
	rump_server_add_iface $SOCK_LOCAL shmif1 $BUS

	rump_server_crypto_start $SOCK_PEER2 netinet6 wg
	rump_server_add_iface $SOCK_PEER2 shmif0 $BUS

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys
	key_priv_peer2=$(wg-keygen)
	key_pub_peer2=$(echo $key_priv_peer2| wg-keygen --pub)

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 inet $ip_local 24
	setup_common shmif1 inet $ip_local2 24
	setup_wg_common wg0 inet $ip_wg_local 24 $port "$key_priv_local"
	setup_wg_common wg1 inet $ip_wg_local2 24 $port2 "$key_priv_local"

	export RUMP_SERVER=$SOCK_PEER
	setup_common shmif0 inet $ip_peer 24
	setup_wg_common wg0 inet $ip_wg_peer 24 $port "$key_priv_peer"

	export RUMP_SERVER=$SOCK_PEER2
	setup_common shmif0 inet $ip_peer2 24
	setup_wg_common wg0 inet $ip_wg_peer2 24 $port2 "$key_priv_peer2"

	export RUMP_SERVER=$SOCK_LOCAL
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/32
	add_peer wg1 peer0 $key_pub_peer2 $ip_peer2:$port2 $ip_wg_peer2/32

	export RUMP_SERVER=$SOCK_PEER
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32

	export RUMP_SERVER=$SOCK_PEER2
	add_peer wg0 peer0 $key_pub_local $ip_local2:$port2 $ip_wg_local2/32

	export RUMP_SERVER=$SOCK_LOCAL

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	$ping $ip_wg_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	$ping $ip_wg_peer2

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	$ifconfig wg0 destroy
	$ifconfig wg1 destroy
	export RUMP_SERVER=$SOCK_PEER
	$ifconfig wg0 destroy
	export RUMP_SERVER=$SOCK_PEER2
	$ifconfig wg0 destroy
}

wg_multiple_interfaces_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case wg_multiple_peers cleanup
wg_multiple_peers_head()
{

	atf_set "descr" "tests multiple wg(4) peers"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_multiple_peers_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping -n -i 0.1 -c 3 -w 1"
	local ping_fail="atf_check -s not-exit:0 -o ignore rump.ping -n -c 1 -w 1"
	local key_priv_peer2=
	local key_pub_peer2=
	local ip_local=192.168.1.1
	local ip_peer=192.168.1.2
	local ip_peer2=192.168.1.3
	local ip_wg_local=10.0.0.1
	local ip_wg_peer=10.0.0.2
	local ip_wg_peer2=10.0.0.3
	local port=51820
	local outfile=./out

	setup_servers
	rump_server_add_iface $SOCK_LOCAL shmif1 $BUS

	rump_server_crypto_start $SOCK_PEER2 netinet6 wg
	rump_server_add_iface $SOCK_PEER2 shmif0 $BUS

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys
	key_priv_peer2=$(wg-keygen)
	key_pub_peer2=$(echo $key_priv_peer2| wg-keygen --pub)

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 inet $ip_local 24
	setup_wg_common wg0 inet $ip_wg_local 24 $port "$key_priv_local"

	export RUMP_SERVER=$SOCK_PEER
	setup_common shmif0 inet $ip_peer 24
	setup_wg_common wg0 inet $ip_wg_peer 24 $port "$key_priv_peer"

	export RUMP_SERVER=$SOCK_PEER2
	setup_common shmif0 inet $ip_peer2 24
	setup_wg_common wg0 inet $ip_wg_peer2 24 $port "$key_priv_peer2"

	export RUMP_SERVER=$SOCK_LOCAL
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/32
	add_peer wg0 peer1 $key_pub_peer2 $ip_peer2:$port $ip_wg_peer2/32

	export RUMP_SERVER=$SOCK_PEER
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32

	export RUMP_SERVER=$SOCK_PEER2
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32

	export RUMP_SERVER=$SOCK_LOCAL

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	$ping $ip_wg_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	$ping $ip_wg_peer2

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	$ifconfig wg0 destroy
	export RUMP_SERVER=$SOCK_PEER
	$ifconfig wg0 destroy
	export RUMP_SERVER=$SOCK_PEER2
	$ifconfig wg0 destroy
}

wg_multiple_peers_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	add_basic_test ipv4 ipv4
	add_basic_test ipv4 ipv6
	add_basic_test ipv6 ipv4
	add_basic_test ipv6 ipv6

	add_payload_sizes_test ipv4 ipv4
	add_payload_sizes_test ipv4 ipv6
	add_payload_sizes_test ipv6 ipv4
	add_payload_sizes_test ipv6 ipv6

	atf_add_test_case wg_create_destroy
	atf_add_test_case wg_create_destroy_peers_ipv4
	atf_add_test_case wg_create_destroy_peers_ipv6
	atf_add_test_case wg_multiple_interfaces
	atf_add_test_case wg_multiple_peers
}
