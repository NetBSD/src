#	$NetBSD: t_ipsec_tunnel.sh,v 1.3.2.2 2017/04/16 10:34:50 ozaki-r Exp $
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

SOCK_LOCAL=unix://ipsec_local
SOCK_TUNNEL_LOCAL=unix://ipsec_tunel_local
SOCK_TUNNEL_REMOTE=unix://ipsec_tunnel_remote
SOCK_REMOTE=unix://ipsec_remote
BUS_LOCAL=./bus_ipsec_local
BUS_TUNNEL=./bus_ipsec_tunnel
BUS_REMOTE=./bus_ipsec_remote

DEBUG=${DEBUG:-false}

test_ipsec4_tunnel()
{
	local proto=$1
	local algo=$2
	local ip_local=10.0.1.2
	local ip_gw_local=10.0.1.1
	local ip_gw_local_tunnel=20.0.0.1
	local ip_gw_remote_tunnel=20.0.0.2
	local ip_gw_remote=10.0.2.1
	local ip_remote=10.0.2.2
	local subnet_local=10.0.1.0
	local subnet_remote=10.0.2.0
	local keylen=$(get_one_valid_keylen $algo)
	local key=$(generate_key $keylen)
	local tmpfile=./tmp
	local outfile=./out
	local opt= proto_cap=

	if [ $proto = esp ]; then
		opt=-E
		proto_cap=ESP
	else
		opt=-A
		proto_cap=AH
	fi

	# See https://www.netbsd.org/docs/network/ipsec/#sample_vpn
	rump_server_crypto_start $SOCK_LOCAL
	rump_server_crypto_start $SOCK_TUNNEL_LOCAL netipsec
	rump_server_crypto_start $SOCK_TUNNEL_REMOTE netipsec
	rump_server_crypto_start $SOCK_REMOTE
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS_LOCAL
	rump_server_add_iface $SOCK_TUNNEL_LOCAL shmif0 $BUS_LOCAL
	rump_server_add_iface $SOCK_TUNNEL_LOCAL shmif1 $BUS_TUNNEL
	rump_server_add_iface $SOCK_TUNNEL_REMOTE shmif0 $BUS_REMOTE
	rump_server_add_iface $SOCK_TUNNEL_REMOTE shmif1 $BUS_TUNNEL
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS_REMOTE

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local/24
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -net $subnet_remote $ip_gw_local

	export RUMP_SERVER=$SOCK_TUNNEL_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_gw_local/24
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_gw_local_tunnel/24
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwarding=1
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -net $subnet_remote $ip_gw_remote_tunnel

	export RUMP_SERVER=$SOCK_TUNNEL_REMOTE
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_gw_remote/24
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_gw_remote_tunnel/24
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwarding=1
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -net $subnet_local $ip_gw_local_tunnel

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_remote/24
	# Run ifconfig -w 10 just once for optimization
	atf_check -s exit:0 rump.ifconfig -w 10
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -net $subnet_local $ip_gw_remote

	extract_new_packets $BUS_TUNNEL > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_remote

	extract_new_packets $BUS_TUNNEL > $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_local > $ip_remote: ICMP echo request" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote > $ip_local: ICMP echo reply" \
	    cat $outfile

	export RUMP_SERVER=$SOCK_TUNNEL_LOCAL
	# from https://www.netbsd.org/docs/network/ipsec/
	cat > $tmpfile <<-EOF
	add $ip_gw_local_tunnel $ip_gw_remote_tunnel $proto 10000 $opt $algo $key;
	add $ip_gw_remote_tunnel $ip_gw_local_tunnel $proto 10001 $opt $algo $key;
	spdadd $subnet_local/24 $subnet_remote/24 any -P out ipsec
	    $proto/tunnel/$ip_gw_local_tunnel-$ip_gw_remote_tunnel/require;
	spdadd $subnet_remote/24 $subnet_local/24 any -P in ipsec
	    $proto/tunnel/$ip_gw_remote_tunnel-$ip_gw_local_tunnel/require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 \
	    -o match:"$ip_gw_local_tunnel $ip_gw_remote_tunnel" \
	    $HIJACKING setkey -D
	atf_check -s exit:0 \
	    -o match:"$ip_gw_remote_tunnel $ip_gw_local_tunnel" \
	    $HIJACKING setkey -D
	# TODO: more detail checks

	export RUMP_SERVER=$SOCK_TUNNEL_REMOTE
	cat > $tmpfile <<-EOF
	add $ip_gw_local_tunnel $ip_gw_remote_tunnel $proto 10000 $opt $algo $key;
	add $ip_gw_remote_tunnel $ip_gw_local_tunnel $proto 10001 $opt $algo $key;
	spdadd $subnet_remote/24 $subnet_local/24 any -P out ipsec
	    $proto/tunnel/$ip_gw_remote_tunnel-$ip_gw_local_tunnel/require;
	spdadd $subnet_local/24 $subnet_remote/24 any -P in ipsec
	    $proto/tunnel/$ip_gw_local_tunnel-$ip_gw_remote_tunnel/require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 \
	    -o match:"$ip_gw_local_tunnel $ip_gw_remote_tunnel" \
	    $HIJACKING setkey -D
	atf_check -s exit:0 \
	    -o match:"$ip_gw_remote_tunnel $ip_gw_local_tunnel" \
	    $HIJACKING setkey -D
	# TODO: more detail checks

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_remote

	extract_new_packets $BUS_TUNNEL > $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_gw_local_tunnel > $ip_gw_remote_tunnel: $proto_cap" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_gw_remote_tunnel > $ip_gw_local_tunnel: $proto_cap" \
	    cat $outfile
}

test_ipsec6_tunnel()
{
	local proto=$1
	local algo=$2
	local ip_local=fd00:1::2
	local ip_gw_local=fd00:1::1
	local ip_gw_local_tunnel=fc00::1
	local ip_gw_remote_tunnel=fc00::2
	local ip_gw_remote=fd00:2::1
	local ip_remote=fd00:2::2
	local subnet_local=fd00:1::
	local subnet_remote=fd00:2::
	local keylen=$(get_one_valid_keylen $algo)
	local key=$(generate_key $keylen)
	local tmpfile=./tmp
	local outfile=./out
	local opt= proto_cap=

	if [ $proto = esp ]; then
		opt=-E
		proto_cap=ESP
	else
		opt=-A
		proto_cap=AH
	fi

	rump_server_crypto_start $SOCK_LOCAL netinet6
	rump_server_crypto_start $SOCK_TUNNEL_LOCAL netipsec netinet6
	rump_server_crypto_start $SOCK_TUNNEL_REMOTE netipsec netinet6
	rump_server_crypto_start $SOCK_REMOTE netinet6
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS_LOCAL
	rump_server_add_iface $SOCK_TUNNEL_LOCAL shmif0 $BUS_LOCAL
	rump_server_add_iface $SOCK_TUNNEL_LOCAL shmif1 $BUS_TUNNEL
	rump_server_add_iface $SOCK_TUNNEL_REMOTE shmif0 $BUS_REMOTE
	rump_server_add_iface $SOCK_TUNNEL_REMOTE shmif1 $BUS_TUNNEL
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS_REMOTE

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_local/64
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -inet6 -net $subnet_remote/64 $ip_gw_local

	export RUMP_SERVER=$SOCK_TUNNEL_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_gw_local/64
	atf_check -s exit:0 rump.ifconfig shmif1 inet6 $ip_gw_local_tunnel/64
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.forwarding=1
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -inet6 -net $subnet_remote/64 $ip_gw_remote_tunnel

	export RUMP_SERVER=$SOCK_TUNNEL_REMOTE
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_gw_remote/64
	atf_check -s exit:0 rump.ifconfig shmif1 inet6 $ip_gw_remote_tunnel/64
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.forwarding=1
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -inet6 -net $subnet_local/64 $ip_gw_local_tunnel

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_remote
	# Run ifconfig -w 10 just once for optimization
	atf_check -s exit:0 rump.ifconfig -w 10
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -inet6 -net $subnet_local/64 $ip_gw_remote

	extract_new_packets $BUS_TUNNEL > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X 3 $ip_remote

	extract_new_packets $BUS_TUNNEL > $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_local > $ip_remote: ICMP6, echo request" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote > $ip_local: ICMP6, echo reply" \
	    cat $outfile

	export RUMP_SERVER=$SOCK_TUNNEL_LOCAL
	# from https://www.netbsd.org/docs/network/ipsec/
	cat > $tmpfile <<-EOF
	add $ip_gw_local_tunnel $ip_gw_remote_tunnel $proto 10000 $opt $algo $key;
	add $ip_gw_remote_tunnel $ip_gw_local_tunnel $proto 10001 $opt $algo $key;
	spdadd $subnet_local/64 $subnet_remote/64 any -P out ipsec
	    $proto/tunnel/$ip_gw_local_tunnel-$ip_gw_remote_tunnel/require;
	spdadd $subnet_remote/64 $subnet_local/64 any -P in ipsec
	    $proto/tunnel/$ip_gw_remote_tunnel-$ip_gw_local_tunnel/require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 \
	    -o match:"$ip_gw_local_tunnel $ip_gw_remote_tunnel" \
	    $HIJACKING setkey -D
	atf_check -s exit:0 \
	    -o match:"$ip_gw_remote_tunnel $ip_gw_local_tunnel" \
	    $HIJACKING setkey -D
	# TODO: more detail checks

	export RUMP_SERVER=$SOCK_TUNNEL_REMOTE
	cat > $tmpfile <<-EOF
	add $ip_gw_local_tunnel $ip_gw_remote_tunnel $proto 10000 $opt $algo $key;
	add $ip_gw_remote_tunnel $ip_gw_local_tunnel $proto 10001 $opt $algo $key;
	spdadd $subnet_remote/64 $subnet_local/64 any -P out ipsec
	    $proto/tunnel/$ip_gw_remote_tunnel-$ip_gw_local_tunnel/require;
	spdadd $subnet_local/64 $subnet_remote/64 any -P in ipsec
	    $proto/tunnel/$ip_gw_local_tunnel-$ip_gw_remote_tunnel/require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 \
	    -o match:"$ip_gw_local_tunnel $ip_gw_remote_tunnel" \
	    $HIJACKING setkey -D
	atf_check -s exit:0 \
	    -o match:"$ip_gw_remote_tunnel $ip_gw_local_tunnel" \
	    $HIJACKING setkey -D
	# TODO: more detail checks

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X 3 $ip_remote

	extract_new_packets $BUS_TUNNEL > $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_gw_local_tunnel > $ip_gw_remote_tunnel: $proto_cap" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_gw_remote_tunnel > $ip_gw_local_tunnel: $proto_cap" \
	    cat $outfile
}

test_tunnel_common()
{
	local ipproto=$1
	local proto=$2
	local algo=$3

	if [ $ipproto = ipv4 ]; then
		test_ipsec4_tunnel $proto $algo
	else
		test_ipsec6_tunnel $proto $algo
	fi
}

add_test_tunnel_mode()
{
	local ipproto=$1
	local proto=$2
	local algo=$3
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	name="ipsec_tunnel_${ipproto}_${proto}_${_algo}"
	desc="Tests of IPsec ($ipproto) tunnel mode with $proto ($algo)"

	atf_test_case ${name} cleanup
	eval "								\
	    ${name}_head() {						\
	        atf_set \"descr\" \"$desc\";				\
	        atf_set \"require.progs\" \"rump_server\" \"setkey\";	\
	    };								\
	    ${name}_body() {						\
	        test_tunnel_common $ipproto $proto $algo;		\
	        rump_server_destroy_ifaces;				\
	    };								\
	    ${name}_cleanup() {						\
	        $DEBUG && dump;						\
	        cleanup;						\
	    }								\
	"
	atf_add_test_case ${name}
}

atf_init_test_cases()
{
	local algo=

	for algo in $ESP_ENCRYPTION_ALGORITHMS; do
		add_test_tunnel_mode ipv4 esp $algo
		add_test_tunnel_mode ipv6 esp $algo
	done

	for algo in $AH_AUTHENTICATION_ALGORITHMS; do
		add_test_tunnel_mode ipv4 ah $algo
		add_test_tunnel_mode ipv6 ah $algo
	done
}
