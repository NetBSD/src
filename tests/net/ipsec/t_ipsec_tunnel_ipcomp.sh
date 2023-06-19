#	$NetBSD: t_ipsec_tunnel_ipcomp.sh,v 1.4 2023/06/19 08:28:09 knakahara Exp $
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

setup_servers()
{

	# See https://www.netbsd.org/docs/network/ipsec/#sample_vpn
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
}

check_tunnel_ipcomp_packets()
{
	local outfile=$1
	local osrc=$2
	local odst=$3
	local oproto=$4
	local isrc=$5
	local idst=$6
	local iproto=$7

	$DEBUG && cat $outfile

	if [ $oproto = ESP ]; then
		atf_check -s exit:0 \
		    -o match:"$osrc > $odst: $oproto" \
		    cat $outfile
		atf_check -s exit:0 \
		    -o match:"$odst > $osrc: $oproto" \
		    cat $outfile
		# TODO check the packet lengths to check IPComp is really used
		return
	fi

	# AH
	if [ $iproto = IPComp ]; then
		atf_check -s exit:0 \
		    -o match:"$osrc > $odst: $oproto.+: $iproto" \
		    cat $outfile
		atf_check -s exit:0 \
		    -o match:"$odst > $osrc: $oproto.+: $iproto" \
		    cat $outfile
	else
		atf_check -s exit:0 \
		    -o match:"$osrc > $odst: $oproto.+ $isrc > $idst: $iproto" \
		    cat $outfile
		atf_check -s exit:0 \
		    -o match:"$odst > $osrc: $oproto.+ $idst > $isrc: $iproto" \
		    cat $outfile
	fi
}

test_ipsec4_tunnel_ipcomp()
{
	local proto=$1
	local algo=$2
	local calgo=$3
	local ip_local=10.0.1.2
	local ip_gw_local=10.0.1.1
	local ip_gw_local_tunnel=20.0.0.1
	local ip_gw_remote_tunnel=20.0.0.2
	local ip_gw_remote=10.0.2.1
	local ip_remote=10.0.2.2
	local subnet_local=10.0.1.0
	local subnet_remote=10.0.2.0
	local tmpfile=./tmp
	local outfile=./out
	local pktproto=$(generate_pktproto $proto)
	local algo_args="$(generate_algo_args $proto $algo)"

	setup_servers

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local/24
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -net $subnet_remote $ip_gw_local

	export RUMP_SERVER=$SOCK_TUNNEL_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_gw_local/24
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_gw_local_tunnel/24
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwarding=1
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -net $subnet_remote $ip_gw_remote_tunnel
	rump.sysctl -a |grep ipsec

	export RUMP_SERVER=$SOCK_TUNNEL_REMOTE
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_gw_remote/24
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_gw_remote_tunnel/24
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwarding=1
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -net $subnet_local $ip_gw_local_tunnel

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_remote/24
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
	add $ip_gw_local_tunnel $ip_gw_remote_tunnel $proto 10000 $algo_args;
	add $ip_gw_remote_tunnel $ip_gw_local_tunnel $proto 10001 $algo_args;
	add $ip_gw_local_tunnel $ip_gw_remote_tunnel ipcomp 10000 -C $calgo;
	add $ip_gw_remote_tunnel $ip_gw_local_tunnel ipcomp 10001 -C $calgo;
	spdadd $subnet_local/24 $subnet_remote/24 any -P out ipsec
	    ipcomp/tunnel/$ip_gw_local_tunnel-$ip_gw_remote_tunnel/require
	    $proto/transport//require;
	spdadd $subnet_remote/24 $subnet_local/24 any -P in ipsec
	    ipcomp/tunnel/$ip_gw_remote_tunnel-$ip_gw_local_tunnel/require
	    $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	check_sa_entries $SOCK_TUNNEL_LOCAL $ip_gw_local_tunnel \
	    $ip_gw_remote_tunnel

	export RUMP_SERVER=$SOCK_TUNNEL_REMOTE
	cat > $tmpfile <<-EOF
	add $ip_gw_local_tunnel $ip_gw_remote_tunnel $proto 10000 $algo_args;
	add $ip_gw_remote_tunnel $ip_gw_local_tunnel $proto 10001 $algo_args;
	add $ip_gw_local_tunnel $ip_gw_remote_tunnel ipcomp 10000 -C $calgo;
	add $ip_gw_remote_tunnel $ip_gw_local_tunnel ipcomp 10001 -C $calgo;
	spdadd $subnet_remote/24 $subnet_local/24 any -P out ipsec
	    ipcomp/tunnel/$ip_gw_remote_tunnel-$ip_gw_local_tunnel/require
	    $proto/transport//require;
	spdadd $subnet_local/24 $subnet_remote/24 any -P in ipsec
	    ipcomp/tunnel/$ip_gw_local_tunnel-$ip_gw_remote_tunnel/require
	    $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	check_sa_entries $SOCK_TUNNEL_REMOTE $ip_gw_local_tunnel \
	    $ip_gw_remote_tunnel

	export RUMP_SERVER=$SOCK_LOCAL
	# IPComp sends a packet as-is if a compressed payload of
	# the packet is greater than or equal to the original payload.
	# So we have to fill a payload with 1 to let IPComp always send
	# a compressed packet.

	# pktsize == minlen - 1
	pktsize=$(($(get_minlen deflate) - 8 - 20 - 1))
	atf_check -s exit:0 -o ignore \
	    rump.ping -c 1 -n -w 3 -s $pktsize -p ff $ip_remote
	extract_new_packets $BUS_TUNNEL > $outfile
	check_tunnel_ipcomp_packets $outfile \
	    $ip_gw_local_tunnel $ip_gw_remote_tunnel $pktproto \
	    $ip_local $ip_remote ICMP

	# pktsize == minlen
	pktsize=$(($(get_minlen deflate) - 8 - 20))
	atf_check -s exit:0 -o ignore \
	    rump.ping -c 1 -n -w 3 -s $pktsize -p ff $ip_remote
	extract_new_packets $BUS_TUNNEL > $outfile
	check_tunnel_ipcomp_packets $outfile \
	    $ip_gw_local_tunnel $ip_gw_remote_tunnel $pktproto \
	    $ip_local $ip_remote IPComp

	test_flush_entries $SOCK_TUNNEL_LOCAL
	test_flush_entries $SOCK_TUNNEL_REMOTE
}

test_ipsec6_tunnel_ipcomp()
{
	local proto=$1
	local algo=$2
	local calgo=$3
	local ip_local=fd00:1::2
	local ip_gw_local=fd00:1::1
	local ip_gw_local_tunnel=fc00::1
	local ip_gw_remote_tunnel=fc00::2
	local ip_gw_remote=fd00:2::1
	local ip_remote=fd00:2::2
	local subnet_local=fd00:1::
	local subnet_remote=fd00:2::
	local tmpfile=./tmp
	local outfile=./out
	local pktproto=$(generate_pktproto $proto)
	local algo_args="$(generate_algo_args $proto $algo)"

	setup_servers

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_local/64
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -inet6 -net $subnet_remote/64 $ip_gw_local

	export RUMP_SERVER=$SOCK_TUNNEL_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_gw_local/64
	atf_check -s exit:0 rump.ifconfig shmif1 inet6 $ip_gw_local_tunnel/64
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.forwarding=1
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -inet6 -net $subnet_remote/64 $ip_gw_remote_tunnel

	export RUMP_SERVER=$SOCK_TUNNEL_REMOTE
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_gw_remote/64
	atf_check -s exit:0 rump.ifconfig shmif1 inet6 $ip_gw_remote_tunnel/64
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.forwarding=1
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -inet6 -net $subnet_local/64 $ip_gw_local_tunnel

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_remote
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
	add $ip_gw_local_tunnel $ip_gw_remote_tunnel $proto 10000 $algo_args;
	add $ip_gw_remote_tunnel $ip_gw_local_tunnel $proto 10001 $algo_args;
	add $ip_gw_local_tunnel $ip_gw_remote_tunnel ipcomp 10000 -C $calgo;
	add $ip_gw_remote_tunnel $ip_gw_local_tunnel ipcomp 10001 -C $calgo;
	spdadd $subnet_local/64 $subnet_remote/64 any -P out ipsec
	    ipcomp/tunnel/$ip_gw_local_tunnel-$ip_gw_remote_tunnel/require
	    $proto/transport//require;
	spdadd $subnet_remote/64 $subnet_local/64 any -P in ipsec
	    ipcomp/tunnel/$ip_gw_remote_tunnel-$ip_gw_local_tunnel/require
	    $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	check_sa_entries $SOCK_TUNNEL_LOCAL $ip_gw_local_tunnel \
	    $ip_gw_remote_tunnel

	export RUMP_SERVER=$SOCK_TUNNEL_REMOTE
	cat > $tmpfile <<-EOF
	add $ip_gw_local_tunnel $ip_gw_remote_tunnel $proto 10000 $algo_args;
	add $ip_gw_remote_tunnel $ip_gw_local_tunnel $proto 10001 $algo_args;
	add $ip_gw_local_tunnel $ip_gw_remote_tunnel ipcomp 10000 -C $calgo;
	add $ip_gw_remote_tunnel $ip_gw_local_tunnel ipcomp 10001 -C $calgo;
	spdadd $subnet_remote/64 $subnet_local/64 any -P out ipsec
	    ipcomp/tunnel/$ip_gw_remote_tunnel-$ip_gw_local_tunnel/require
	    $proto/transport//require;
	spdadd $subnet_local/64 $subnet_remote/64 any -P in ipsec
	    ipcomp/tunnel/$ip_gw_local_tunnel-$ip_gw_remote_tunnel/require
	    $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	check_sa_entries $SOCK_TUNNEL_REMOTE $ip_gw_local_tunnel \
	    $ip_gw_remote_tunnel

	export RUMP_SERVER=$SOCK_LOCAL

	# IPComp sends a packet as-is if a compressed payload of
	# the packet is greater than or equal to the original payload.
	# So we have to fill a payload with 1 to let IPComp always send
	# a compressed packet.

	# pktsize == minlen - 1

	pktsize=$(($(get_minlen deflate) - 8 - 40 - 1))
	atf_check -s exit:0 -o ignore \
	    rump.ping6 -c 1 -n -X 3 -s $pktsize -p ff $ip_remote
	extract_new_packets $BUS_TUNNEL > $outfile
	check_tunnel_ipcomp_packets $outfile \
	    $ip_gw_local_tunnel $ip_gw_remote_tunnel $pktproto \
	    $ip_local $ip_remote ICMP6

	# pktsize == minlen
	pktsize=$(($(get_minlen deflate) - 8 - 40))
	atf_check -s exit:0 -o ignore \
	    rump.ping6 -c 1 -n -X 3 -s $pktsize -p ff $ip_remote
	extract_new_packets $BUS_TUNNEL > $outfile
	check_tunnel_ipcomp_packets $outfile \
	    $ip_gw_local_tunnel $ip_gw_remote_tunnel $pktproto \
	    $ip_local $ip_remote IPComp

	test_flush_entries $SOCK_TUNNEL_LOCAL
	test_flush_entries $SOCK_TUNNEL_REMOTE
}

test_tunnel_ipcomp_common()
{
	local ipproto=$1
	local proto=$2
	local algo=$3
	local calgo=$4

	if [ $ipproto = ipv4 ]; then
		test_ipsec4_tunnel_ipcomp $proto $algo $calgo
	else
		test_ipsec6_tunnel_ipcomp $proto $algo $calgo
	fi
}

add_test_tunnel_mode()
{
	local ipproto=$1
	local proto=$2
	local algo=$3
	local calgo=$4
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	name="ipsec_tunnel_ipcomp_${calgo}_${ipproto}_${proto}_${_algo}"
	desc="Tests of IPsec ($ipproto) tunnel mode with $proto ($algo) and ipcomp ($calgo)"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server setkey
	    }
	    ${name}_body() {
	        test_tunnel_ipcomp_common $ipproto $proto $algo $calgo
	        rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
	        \$DEBUG && dump
	        cleanup
	    }
	"
	atf_add_test_case ${name}
}

atf_init_test_cases()
{
	local calgo= algo=

	for calgo in $IPCOMP_COMPRESSION_ALGORITHMS; do
		for algo in $ESP_ENCRYPTION_ALGORITHMS_MINIMUM; do
			add_test_tunnel_mode ipv4 esp $algo $calgo
			add_test_tunnel_mode ipv6 esp $algo $calgo
		done

		for algo in $AH_AUTHENTICATION_ALGORITHMS_MINIMUM; do
			add_test_tunnel_mode ipv4 ah $algo $calgo
			add_test_tunnel_mode ipv6 ah $algo $calgo
		done
	done
}
