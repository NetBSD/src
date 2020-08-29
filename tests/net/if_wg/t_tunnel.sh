#	$NetBSD: t_tunnel.sh,v 1.2 2020/08/29 07:22:49 tih Exp $
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

BUS_LOCAL=bus_local
BUS_TUN=bus_tun
BUS_PEER=bus_peer
SOCK_LOCAL=unix://wg_local
SOCK_TUN_LOCAL=unix://wg_tun_local
SOCK_TUN_PEER=unix://wg_tun_peer
SOCK_PEER=unix://wg_peer

escape_key()
{

	echo $1 | sed 's/\+/\\+/g' | sed 's|\/|\\/|g'
}

setup_servers()
{

	rump_server_start $SOCK_LOCAL netinet6
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS_LOCAL

	rump_server_crypto_start $SOCK_TUN_LOCAL netinet6 wg
	rump_server_add_iface $SOCK_TUN_LOCAL shmif0 $BUS_LOCAL
	rump_server_add_iface $SOCK_TUN_LOCAL shmif1 $BUS_TUN

	rump_server_crypto_start $SOCK_TUN_PEER netinet6 wg
	rump_server_add_iface $SOCK_TUN_PEER shmif0 $BUS_PEER
	rump_server_add_iface $SOCK_TUN_PEER shmif1 $BUS_TUN

	rump_server_start $SOCK_PEER netinet6
	rump_server_add_iface $SOCK_PEER shmif0 $BUS_PEER
}

setup_edge()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local proto=$1
	local ip=$2
	local prefix=$3
	local gw=$4
	local ip_bad=$5
	local alias=

	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	$ifconfig shmif0 $proto $ip/$prefix
	atf_check -s exit:0 -o ignore rump.route add -$proto default $gw

	if [ -z "$ip_bad" ]; then
		return
	fi

	if [ $proto = inet ]; then
		alias="alias"
	fi

	$ifconfig shmif0 $proto $ip_bad/$prefix $alias
}

setup_ip()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local proto=$1
	local ip=$2
	local prefix=$3

	$ifconfig shmif0 $proto $ip/$prefix
}
setup_router()
{

	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwarding=1
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.forwarding=1
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
}

setup_wg()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local wgconfig="atf_check -s exit:0 $HIJACKING wgconfig"
	local proto=$1
	local ip=$2
	local prefix=$3
	local port=$4
	local key_priv="$5"
	local privfile=./tmp

	$ifconfig wg0 create
	$ifconfig wg0 $proto $ip/$prefix
	$DEBUG && rump.netstat -nr
	echo $key_priv > $privfile
	$wgconfig wg0 set private-key $privfile
	$wgconfig wg0 set listen-port $port
	rm -f $privfile
	$ifconfig wg0 up

	check_conf_port wg0 $port
	check_conf_privkey wg0 "$key_priv"
}

setup_wg_route()
{
	local proto=$1
	local subnet=$2
	local subnet_bad=$3

	atf_check -s exit:0 -o ignore rump.route add -$proto -net $subnet -link wg0 -iface
	if [ -n "$subnet_bad" ]; then
		atf_check -s exit:0 -o ignore rump.route add -$proto -net $subnet_bad -link wg0 -iface
	fi
}

prepare_file()
{
	local file=$1
	local data="0123456789"

	touch $file
	for i in `seq 1 200`
	do
		echo $data >> $file
	done
}

test_tcp()
{
	local proto=$1
	local ip_peer=$2
	local _proto=

	prepare_file ./file_send

	if [ $proto = inet ]; then
		_proto=ipv4
	else
		_proto=ipv6
	fi
	start_nc_server $SOCK_PEER 1234 ./file_recv $_proto

	export RUMP_SERVER=$SOCK_LOCAL
	# Send a file to the server
	# XXX Need a bit longer timeout value because the packet processing
	# of the implementation is quite inefficient...
	atf_check -s exit:0 $HIJACKING \
	    nc -N -w 20 $ip_peer 1234 < ./file_send
	$DEBUG && extract_new_packets $BUS > ./out
	$DEBUG && cat ./out
	stop_nc_server
	$DEBUG && ls -s ./file_send ./file_recv
	$DEBUG && wc -l ./file_send
	$DEBUG && wc -l ./file_recv
	$DEBUG && diff -u ./file_send ./file_recv
	atf_check -s exit:0 diff -q ./file_send ./file_recv
	rm -f ./out ./file_recv ./file_send
}

wg_tunnel_common()
{
	local outer_proto=$1
	local inner_proto=$2
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local wgconfig="atf_check -s exit:0 $HIJACKING wgconfig"
	local port=51820
	local ip_local= ip_peer=
	local ip_wg_local= ip_wg_peer=
	local outer_prefix= outer_prefixall=
	local inner_prefix= inner_prefixall=

	if [ $outer_proto = inet ]; then
		ip_tun_local_tun=192.168.10.1
		ip_tun_peer_tun=192.168.10.2
		outer_prefix=24
		outer_prefixall=32
	else
		ip_tun_local_tun=fc00:10::1
		ip_tun_peer_tun=fc00:10::2
		outer_prefix=64
		outer_prefixall=128
	fi

	if [ $inner_proto = inet ]; then
		ip_local=192.168.1.2
		ip_tun_local=192.168.1.1
		ip_wg_local=10.0.0.1
		ip_wg_peer=10.0.0.2
		ip_tun_peer=192.168.2.1
		ip_peer=192.168.2.2
		ip_peer_bad=192.168.3.2
		inner_prefix=24
		inner_prefixall=32
		subnet_local=192.168.1.0/24
		subnet_peer=192.168.2.0/24
		subnet_peer_bad=192.168.3.0/24
	else
		ip_tun_local=fc00:1::1
		ip_local=fc00:1::2
		ip_wg_local=fd00::1
		ip_wg_peer=fd00::2
		ip_tun_peer=fc00:2::1
		ip_peer=fc00:2::2
		ip_peer_bad=fc00:3::2
		inner_prefix=64
		inner_prefixall=128
		subnet_local=fc00:1::/64
		subnet_peer=fc00:2::/64
		subnet_peer_bad=fc00:3::/64
	fi

	setup_servers

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys

	export RUMP_SERVER=$SOCK_LOCAL
	setup_edge $inner_proto $ip_local $inner_prefix $ip_tun_local

	export RUMP_SERVER=$SOCK_TUN_LOCAL
	setup_router
	$ifconfig shmif0 $inner_proto $ip_tun_local/$inner_prefix
	$ifconfig shmif1 $outer_proto $ip_tun_local_tun/$outer_prefix
	setup_wg $inner_proto $ip_wg_local $inner_prefix $port "$key_priv_local"
	setup_wg_route $inner_proto $subnet_peer $subnet_peer_bad

	export RUMP_SERVER=$SOCK_TUN_PEER
	setup_router
	$ifconfig shmif0 $inner_proto $ip_tun_peer/$inner_prefix
	$ifconfig shmif1 $outer_proto $ip_tun_peer_tun/$outer_prefix
	setup_wg $inner_proto $ip_wg_peer $inner_prefix $port "$key_priv_peer"
	setup_wg_route $inner_proto $subnet_local

	export RUMP_SERVER=$SOCK_PEER
	setup_edge $inner_proto $ip_peer $inner_prefix $ip_tun_peer $ip_peer_bad

	export RUMP_SERVER=$SOCK_TUN_LOCAL
	add_peer wg0 peer0 $key_pub_peer $ip_tun_peer_tun:$port \
	    $ip_wg_peer/$inner_prefixall,$subnet_peer

	export RUMP_SERVER=$SOCK_TUN_PEER
	add_peer wg0 peer0 $key_pub_local $ip_tun_local_tun:$port \
	    $ip_wg_local/$inner_prefixall,$subnet_local

	export RUMP_SERVER=$SOCK_TUN_LOCAL
	atf_check -s exit:0 -o match:"latest-handshake: \(never\)" \
	    $HIJACKING wgconfig wg0 show peer peer0

	export RUMP_SERVER=$SOCK_LOCAL
	check_ping $inner_proto $ip_peer

	export RUMP_SERVER=$SOCK_TUN_LOCAL
	atf_check -s exit:0 -o not-match:"latest-handshake: \(never\)" \
	    $HIJACKING wgconfig wg0 show peer peer0

	export RUMP_SERVER=$SOCK_LOCAL
	# ping fails because the subnet of the IP is not allowed
	check_ping_fail $inner_proto $ip_peer_bad

	#
	# Test TCP stream over the tunnel
	#
	test_tcp $inner_proto $ip_peer

	export RUMP_SERVER=$SOCK_TUN_LOCAL
	$ifconfig wg0 destroy
	export RUMP_SERVER=$SOCK_TUN_PEER
	$ifconfig wg0 destroy
}

add_tunnel_test()
{
	local inner=$1
	local outer=$2
	local ipv4=inet
	local ipv6=inet6

	name="wg_tunnel_${inner}_over_${outer}"
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
			wg_tunnel_common $outer $inner
			rump_server_destroy_ifaces
		}
		${name}_cleanup() {
			\$DEBUG && dump
			cleanup
		}"
	atf_add_test_case ${name}
}

atf_init_test_cases()
{

	add_tunnel_test ipv4 ipv4
	add_tunnel_test ipv4 ipv6
	add_tunnel_test ipv6 ipv4
	add_tunnel_test ipv6 ipv6
}
