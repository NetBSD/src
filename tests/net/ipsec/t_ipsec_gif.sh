#	$NetBSD: t_ipsec_gif.sh,v 1.2 2017/04/27 10:17:12 ozaki-r Exp $
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

SOCK_LOCAL=unix://ipsec_gif_local
SOCK_TUN_LOCAL=unix://ipsec_gif_tunel_local
SOCK_TUN_REMOTE=unix://ipsec_gif_tunnel_remote
SOCK_REMOTE=unix://ipsec_gif_remote
BUS_LOCAL=./bus_ipsec_local
BUS_TUNNEL=./bus_ipsec_tunnel
BUS_REMOTE=./bus_ipsec_remote

DEBUG=${DEBUG:-false}

make_gif_pktstr()
{
	local src=$1
	local dst=$2
	local src_inner=$3
	local dst_inner=$4
	local proto=$5
	local ipproto=$6
	local proto_cap= inner_str=

	if [ $proto = esp ]; then
		proto_cap=ESP
	else
		proto_cap=AH
		if [ $ipproto = ipv4 ]; then
			inner_str="$src_inner > $dst_inner:.+\(ipip-proto-4\)"
		else
			inner_str="$src_inner > $dst_inner"
		fi
	fi

	echo "$src > $dst: $proto_cap.+$inner_str"
}

test_ipsec4_gif()
{
	local mode=$1
	local proto=$2
	local algo=$3
	local ip_local=10.0.1.2
	local ip_gw_local=10.0.1.1
	local ip_gwlo_tun=20.0.0.1
	local ip_gwlo_gif=20.1.0.1
	local ip_gwre_gif=20.1.0.2
	local ip_gwre_tun=20.0.0.2
	local ip_gw_remote=10.0.2.1
	local ip_remote=10.0.2.2
	local subnet_local=10.0.1.0
	local subnet_remote=10.0.2.0
	local keylen=$(get_one_valid_keylen $algo)
	local key=$(generate_key $keylen)
	local tmpfile=./tmp
	local outfile=./out
	local opt= str=

	if [ $proto = esp ]; then
		opt=-E
	else
		opt=-A
	fi

	rump_server_crypto_start $SOCK_LOCAL
	rump_server_crypto_start $SOCK_TUN_LOCAL netipsec gif
	rump_server_crypto_start $SOCK_TUN_REMOTE netipsec gif
	rump_server_crypto_start $SOCK_REMOTE
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS_LOCAL
	rump_server_add_iface $SOCK_TUN_LOCAL shmif0 $BUS_LOCAL
	rump_server_add_iface $SOCK_TUN_LOCAL shmif1 $BUS_TUNNEL
	rump_server_add_iface $SOCK_TUN_REMOTE shmif0 $BUS_REMOTE
	rump_server_add_iface $SOCK_TUN_REMOTE shmif1 $BUS_TUNNEL
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS_REMOTE

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local/24
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -net $subnet_remote $ip_gw_local

	export RUMP_SERVER=$SOCK_TUN_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_gw_local/24
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_gwlo_tun/24
	atf_check -s exit:0 rump.ifconfig gif0 create
	atf_check -s exit:0 rump.ifconfig gif0 \
	    tunnel $ip_gwlo_tun $ip_gwre_tun
	atf_check -s exit:0 rump.ifconfig gif0 \
	    inet $ip_gwlo_gif/32 $ip_gwre_gif
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwarding=1
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -net $subnet_remote $ip_gwre_gif

	export RUMP_SERVER=$SOCK_TUN_REMOTE
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_gw_remote/24
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_gwre_tun/24
	atf_check -s exit:0 rump.ifconfig gif0 create
	atf_check -s exit:0 rump.ifconfig gif0 \
	    tunnel $ip_gwre_tun $ip_gwlo_tun
	atf_check -s exit:0 rump.ifconfig gif0 \
	    inet $ip_gwre_gif/32 $ip_gwlo_gif
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwarding=1
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -net $subnet_local $ip_gwlo_gif

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
	str="$ip_gwlo_tun > $ip_gwre_tun:"
	str="$str $ip_local > $ip_remote: ICMP echo request,"
	str="$str .+ \(ipip-proto-4\)"
	atf_check -s exit:0 -o match:"$str" cat $outfile
	str="$ip_gwre_tun > $ip_gwlo_tun:"
	str="$str $ip_remote > $ip_local: ICMP echo reply,"
	str="$str .+ \(ipip-proto-4\)"
	atf_check -s exit:0 -o match:"$str" cat $outfile

	if [ $mode = tunnel ]; then
		export RUMP_SERVER=$SOCK_TUN_LOCAL
		# from https://www.netbsd.org/docs/network/ipsec/
		cat > $tmpfile <<-EOF
		add $ip_gwlo_tun $ip_gwre_tun $proto 10000 $opt $algo $key;
		add $ip_gwre_tun $ip_gwlo_tun $proto 10001 $opt $algo $key;
		spdadd $subnet_local/24 $subnet_remote/24 any -P out ipsec
		    $proto/tunnel/$ip_gwlo_tun-$ip_gwre_tun/require;
		spdadd $subnet_remote/24 $subnet_local/24 any -P in ipsec
		    $proto/tunnel/$ip_gwre_tun-$ip_gwlo_tun/require;
		EOF
		$DEBUG && cat $tmpfile
		atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
		$DEBUG && $HIJACKING setkey -D
		atf_check -s exit:0 -o match:"$ip_gwlo_tun $ip_gwre_tun" \
		    $HIJACKING setkey -D
		atf_check -s exit:0 -o match:"$ip_gwre_tun $ip_gwlo_tun" \
		    $HIJACKING setkey -D
		# TODO: more detail checks

		export RUMP_SERVER=$SOCK_TUN_REMOTE
		cat > $tmpfile <<-EOF
		add $ip_gwlo_tun $ip_gwre_tun $proto 10000 $opt $algo $key;
		add $ip_gwre_tun $ip_gwlo_tun $proto 10001 $opt $algo $key;
		spdadd $subnet_remote/24 $subnet_local/24 any -P out ipsec
		    $proto/tunnel/$ip_gwre_tun-$ip_gwlo_tun/require;
		spdadd $subnet_local/24 $subnet_remote/24 any -P in ipsec
		    $proto/tunnel/$ip_gwlo_tun-$ip_gwre_tun/require;
		EOF
		$DEBUG && cat $tmpfile
		atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
		$DEBUG && $HIJACKING setkey -D
		atf_check -s exit:0 -o match:"$ip_gwlo_tun $ip_gwre_tun" \
		    $HIJACKING setkey -D
		atf_check -s exit:0 -o match:"$ip_gwre_tun $ip_gwlo_tun" \
		    $HIJACKING setkey -D
		# TODO: more detail checks
	else # transport mode
		export RUMP_SERVER=$SOCK_TUN_LOCAL
		# from https://www.netbsd.org/docs/network/ipsec/
		cat > $tmpfile <<-EOF
		add $ip_gwlo_tun $ip_gwre_tun $proto 10000 $opt $algo $key;
		add $ip_gwre_tun $ip_gwlo_tun $proto 10001 $opt $algo $key;
		spdadd $ip_gwlo_tun/32 $ip_gwre_tun/32 any -P out ipsec
		    $proto/transport//require;
		spdadd $ip_gwre_tun/32 $ip_gwlo_tun/32 any -P in ipsec
		    $proto/transport//require;
		EOF
		$DEBUG && cat $tmpfile
		atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
		$DEBUG && $HIJACKING setkey -D
		atf_check -s exit:0 -o match:"$ip_gwlo_tun $ip_gwre_tun" \
		    $HIJACKING setkey -D
		atf_check -s exit:0 -o match:"$ip_gwre_tun $ip_gwlo_tun" \
		    $HIJACKING setkey -D
		# TODO: more detail checks

		export RUMP_SERVER=$SOCK_TUN_REMOTE
		cat > $tmpfile <<-EOF
		add $ip_gwlo_tun $ip_gwre_tun $proto 10000 $opt $algo $key;
		add $ip_gwre_tun $ip_gwlo_tun $proto 10001 $opt $algo $key;
		spdadd $ip_gwre_tun/32 $ip_gwlo_tun/32 any -P out ipsec
		    $proto/transport//require;
		spdadd $ip_gwlo_tun/32 $ip_gwre_tun/32 any -P in ipsec
		    $proto/transport//require;
		EOF
		$DEBUG && cat $tmpfile
		atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
		$DEBUG && $HIJACKING setkey -D
		atf_check -s exit:0 -o match:"$ip_gwlo_tun $ip_gwre_tun" \
		    $HIJACKING setkey -D
		atf_check -s exit:0 -o match:"$ip_gwre_tun $ip_gwlo_tun" \
		    $HIJACKING setkey -D
		# TODO: more detail checks
	fi

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_remote

	extract_new_packets $BUS_TUNNEL > $outfile
	str=$(make_gif_pktstr $ip_gwlo_tun $ip_gwre_tun \
	    $ip_local $ip_remote $proto ipv4)
	atf_check -s exit:0 -o match:"$str" cat $outfile
	str=$(make_gif_pktstr $ip_gwre_tun $ip_gwlo_tun \
	    $ip_remote $ip_local $proto ipv4)
	atf_check -s exit:0 -o match:"$str" cat $outfile
}

test_ipsec6_gif()
{
	local mode=$1
	local proto=$2
	local algo=$3
	local ip_local=fd00:1::2
	local ip_gw_local=fd00:1::1
	local ip_gwlo_tun=fc00::1
	local ip_gwlo_gif=fc01::1
	local ip_gwre_gif=fc01::2
	local ip_gwre_tun=fc00::2
	local ip_gw_remote=fd00:2::1
	local ip_remote=fd00:2::2
	local subnet_local=fd00:1::
	local subnet_remote=fd00:2::
	local keylen=$(get_one_valid_keylen $algo)
	local key=$(generate_key $keylen)
	local tmpfile=./tmp
	local outfile=./out
	local opt= str=

	if [ $proto = esp ]; then
		opt=-E
	else
		opt=-A
	fi

	rump_server_crypto_start $SOCK_LOCAL netinet6
	rump_server_crypto_start $SOCK_TUN_LOCAL netipsec netinet6 gif
	rump_server_crypto_start $SOCK_TUN_REMOTE netipsec netinet6 gif
	rump_server_crypto_start $SOCK_REMOTE netinet6
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS_LOCAL
	rump_server_add_iface $SOCK_TUN_LOCAL shmif0 $BUS_LOCAL
	rump_server_add_iface $SOCK_TUN_LOCAL shmif1 $BUS_TUNNEL
	rump_server_add_iface $SOCK_TUN_REMOTE shmif0 $BUS_REMOTE
	rump_server_add_iface $SOCK_TUN_REMOTE shmif1 $BUS_TUNNEL
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS_REMOTE

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_local/64
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -inet6 -net $subnet_remote/64 $ip_gw_local

	export RUMP_SERVER=$SOCK_TUN_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_gw_local/64
	atf_check -s exit:0 rump.ifconfig shmif1 inet6 $ip_gwlo_tun/64
	atf_check -s exit:0 rump.ifconfig gif0 create
	atf_check -s exit:0 rump.ifconfig gif0 \
	    tunnel $ip_gwlo_tun $ip_gwre_tun
	atf_check -s exit:0 rump.ifconfig gif0 \
	    inet6 $ip_gwlo_gif/128 $ip_gwre_gif
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.forwarding=1
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -inet6 -net $subnet_remote/64 $ip_gwlo_gif

	export RUMP_SERVER=$SOCK_TUN_REMOTE
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_gw_remote/64
	atf_check -s exit:0 rump.ifconfig shmif1 inet6 $ip_gwre_tun/64
	atf_check -s exit:0 rump.ifconfig gif0 create
	atf_check -s exit:0 rump.ifconfig gif0 \
	    tunnel $ip_gwre_tun $ip_gwlo_tun
	atf_check -s exit:0 rump.ifconfig gif0 \
	    inet6 $ip_gwre_gif/128 $ip_gwlo_gif
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.forwarding=1
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -inet6 -net $subnet_local/64 $ip_gwre_gif

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
	str="$ip_gwlo_tun > $ip_gwre_tun:"
	str="$str $ip_local > $ip_remote: ICMP6, echo request"
	atf_check -s exit:0 -o match:"$str" cat $outfile
	str="$ip_gwre_tun > $ip_gwlo_tun:"
	str="$str $ip_remote > $ip_local: ICMP6, echo reply,"
	atf_check -s exit:0 -o match:"$str" cat $outfile

	if [ $mode = tunnel ]; then
		export RUMP_SERVER=$SOCK_TUN_LOCAL
		# from https://www.netbsd.org/docs/network/ipsec/
		cat > $tmpfile <<-EOF
		add $ip_gwlo_tun $ip_gwre_tun $proto 10000 $opt $algo $key;
		add $ip_gwre_tun $ip_gwlo_tun $proto 10001 $opt $algo $key;
		spdadd $subnet_local/64 $subnet_remote/64 any -P out ipsec
		    $proto/tunnel/$ip_gwlo_tun-$ip_gwre_tun/require;
		spdadd $subnet_remote/64 $subnet_local/64 any -P in ipsec
		    $proto/tunnel/$ip_gwre_tun-$ip_gwlo_tun/require;
		EOF
		$DEBUG && cat $tmpfile
		atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile

		export RUMP_SERVER=$SOCK_TUN_REMOTE
		cat > $tmpfile <<-EOF
		add $ip_gwlo_tun $ip_gwre_tun $proto 10000 $opt $algo $key;
		add $ip_gwre_tun $ip_gwlo_tun $proto 10001 $opt $algo $key;
		spdadd $subnet_remote/64 $subnet_local/64 any -P out ipsec
		    $proto/tunnel/$ip_gwre_tun-$ip_gwlo_tun/require;
		spdadd $subnet_local/64 $subnet_remote/64 any -P in ipsec
		    $proto/tunnel/$ip_gwlo_tun-$ip_gwre_tun/require;
		EOF
		$DEBUG && cat $tmpfile
		atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	else # transport mode
		export RUMP_SERVER=$SOCK_TUN_LOCAL
		# from https://www.netbsd.org/docs/network/ipsec/
		cat > $tmpfile <<-EOF
		add $ip_gwlo_tun $ip_gwre_tun $proto 10000 $opt $algo $key;
		add $ip_gwre_tun $ip_gwlo_tun $proto 10001 $opt $algo $key;
		spdadd $ip_gwlo_tun/128 $ip_gwre_tun/128 any -P out ipsec
		    $proto/transport//require;
		spdadd $ip_gwre_tun/128 $ip_gwlo_tun/128 any -P in ipsec
		    $proto/transport//require;
		EOF
		$DEBUG && cat $tmpfile
		atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile

		export RUMP_SERVER=$SOCK_TUN_REMOTE
		cat > $tmpfile <<-EOF
		add $ip_gwlo_tun $ip_gwre_tun $proto 10000 $opt $algo $key;
		add $ip_gwre_tun $ip_gwlo_tun $proto 10001 $opt $algo $key;
		spdadd $ip_gwre_tun/128 $ip_gwlo_tun/128 any -P out ipsec
		    $proto/transport//require;
		spdadd $ip_gwlo_tun/128 $ip_gwre_tun/128 any -P in ipsec
		    $proto/transport//require;
		EOF
		$DEBUG && cat $tmpfile
		atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	fi

	export RUMP_SERVER=$SOCK_TUN_LOCAL
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 -o match:"$ip_gwlo_tun $ip_gwre_tun" \
	    $HIJACKING setkey -D
	atf_check -s exit:0 -o match:"$ip_gwre_tun $ip_gwlo_tun" \
	    $HIJACKING setkey -D
	# TODO: more detail checks

	export RUMP_SERVER=$SOCK_TUN_REMOTE
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 -o match:"$ip_gwlo_tun $ip_gwre_tun" \
	    $HIJACKING setkey -D
	atf_check -s exit:0 -o match:"$ip_gwre_tun $ip_gwlo_tun" \
	    $HIJACKING setkey -D
	# TODO: more detail checks

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X 3 $ip_remote

	extract_new_packets $BUS_TUNNEL > $outfile
	str=$(make_gif_pktstr $ip_gwlo_tun $ip_gwre_tun \
	    $ip_local $ip_remote $proto ipv6)
	atf_check -s exit:0 -o match:"$str" cat $outfile
	str=$(make_gif_pktstr $ip_gwre_tun $ip_gwlo_tun \
	    $ip_remote $ip_local $proto ipv6)
	atf_check -s exit:0 -o match:"$str" cat $outfile
}

test_ipsec_gif_common()
{
	local ipproto=$1
	local mode=$2
	local proto=$3
	local algo=$4

	if [ $ipproto = ipv4 ]; then
		test_ipsec4_gif $mode $proto $algo
	else
		test_ipsec6_gif $mode $proto $algo
	fi
}

add_test_ipsec_gif()
{
	local ipproto=$1
	local mode=$2
	local proto=$3
	local algo=$4
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	name="ipsec_gif_${ipproto}_${mode}_${proto}_${_algo}"
	desc="Tests of gif/IPsec ($ipproto) ${mode} mode with $proto ($algo)"

	atf_test_case ${name} cleanup
	eval "								\
	    ${name}_head() {						\
	        atf_set \"descr\" \"$desc\";				\
	        atf_set \"require.progs\" \"rump_server\" \"setkey\";	\
	    };								\
	    ${name}_body() {						\
	        test_ipsec_gif_common $ipproto $mode $proto $algo;	\
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

	for algo in $ESP_ENCRYPTION_ALGORITHMS_MINIMUM; do
		add_test_ipsec_gif ipv4 tunnel esp $algo
		add_test_ipsec_gif ipv6 tunnel esp $algo
		add_test_ipsec_gif ipv4 transport esp $algo
		add_test_ipsec_gif ipv6 transport esp $algo
	done

	for algo in $AH_AUTHENTICATION_ALGORITHMS_MINIMUM; do
		add_test_ipsec_gif ipv4 tunnel ah $algo
		add_test_ipsec_gif ipv6 tunnel ah $algo
		add_test_ipsec_gif ipv4 transport ah $algo
		add_test_ipsec_gif ipv6 transport ah $algo
	done
}
