#	$NetBSD: t_ipsec_transport.sh,v 1.8 2023/06/19 08:28:09 knakahara Exp $
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
SOCK_PEER=unix://ipsec_peer
BUS=./bus_ipsec

DEBUG=${DEBUG:-false}

check_packets()
{
	local outfile=$1
	local src=$2
	local dst=$3
	local pktproto=$4

	atf_check -s exit:0 -o match:"$src > $dst: $pktproto" cat $outfile
	atf_check -s exit:0 -o match:"$dst > $src: $pktproto" cat $outfile
}

test_ipsec4_transport()
{
	local proto=$1
	local algo=$2
	local ip_local=10.0.0.1
	local ip_peer=10.0.0.2
	local tmpfile=./tmp
	local outfile=./out
	local pktproto=$(generate_pktproto $proto)
	local algo_args="$(generate_algo_args $proto $algo)"
	local pktsize=

	rump_server_crypto_start $SOCK_LOCAL netipsec
	rump_server_crypto_start $SOCK_PEER netipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_PEER shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local/24
	atf_check -s exit:0 rump.ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_peer/24
	atf_check -s exit:0 rump.ifconfig -w 10

	extract_new_packets $BUS > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:"$ip_local > $ip_peer: ICMP echo request" \
	    cat $outfile
	atf_check -s exit:0 -o match:"$ip_peer > $ip_local: ICMP echo reply" \
	    cat $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	# from https://www.netbsd.org/docs/network/ipsec/
	cat > $tmpfile <<-EOF
	add $ip_local $ip_peer $proto 10000 $algo_args;
	add $ip_peer $ip_local $proto 10001 $algo_args;
	spdadd $ip_local $ip_peer any -P out ipsec $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	check_sa_entries $SOCK_LOCAL $ip_local $ip_peer

	export RUMP_SERVER=$SOCK_PEER
	cat > $tmpfile <<-EOF
	add $ip_local $ip_peer $proto 10000 $algo_args;
	add $ip_peer $ip_local $proto 10001 $algo_args;
	spdadd $ip_peer $ip_local any -P out ipsec $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	check_sa_entries $SOCK_PEER $ip_local $ip_peer

	export RUMP_SERVER=$SOCK_LOCAL
	if [ $proto = ipcomp ]; then
		# IPComp sends a packet as-is if a compressed payload of
		# the packet is greater than or equal to the original payload.
		# So we have to fill a payload with 1 to let IPComp always send
		# a compressed packet.

		# pktsize == minlen - 1
		pktsize=$(($(get_minlen $algo) - 8 - 1))
		atf_check -s exit:0 -o ignore \
		    rump.ping -c 1 -n -w 3 -s $pktsize -p ff $ip_peer
		extract_new_packets $BUS > $outfile
		check_packets $outfile $ip_local $ip_peer ICMP

		# pktsize == minlen
		pktsize=$(($(get_minlen $algo) - 8))
		atf_check -s exit:0 -o ignore \
		    rump.ping -c 1 -n -w 3 -s $pktsize -p ff $ip_peer
		extract_new_packets $BUS > $outfile
		check_packets $outfile $ip_local $ip_peer $pktproto
	else
		atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer
		extract_new_packets $BUS > $outfile
		check_packets $outfile $ip_local $ip_peer $pktproto
	fi

	test_flush_entries $SOCK_LOCAL
	test_flush_entries $SOCK_PEER
}

test_ipsec6_transport()
{
	local proto=$1
	local algo=$2
	local ip_local=fd00::1
	local ip_peer=fd00::2
	local tmpfile=./tmp
	local outfile=./out
	local pktproto=$(generate_pktproto $proto)
	local algo_args="$(generate_algo_args $proto $algo)"

	rump_server_crypto_start $SOCK_LOCAL netinet6 netipsec
	rump_server_crypto_start $SOCK_PEER netinet6 netipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_PEER shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_local
	atf_check -s exit:0 rump.ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_peer
	atf_check -s exit:0 rump.ifconfig -w 10

	extract_new_packets $BUS > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X 3 $ip_peer

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:"$ip_local > $ip_peer: ICMP6, echo request" \
	    cat $outfile
	atf_check -s exit:0 -o match:"$ip_peer > $ip_local: ICMP6, echo reply" \
	    cat $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	# from https://www.netbsd.org/docs/network/ipsec/
	cat > $tmpfile <<-EOF
	add $ip_local $ip_peer $proto 10000 $algo_args;
	add $ip_peer $ip_local $proto 10001 $algo_args;
	spdadd $ip_local $ip_peer any -P out ipsec $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	check_sa_entries $SOCK_LOCAL $ip_local $ip_peer

	export RUMP_SERVER=$SOCK_PEER
	cat > $tmpfile <<-EOF
	add $ip_local $ip_peer $proto 10000 $algo_args;
	add $ip_peer $ip_local $proto 10001 $algo_args;
	spdadd $ip_peer $ip_local any -P out ipsec $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	check_sa_entries $SOCK_PEER $ip_local $ip_peer

	export RUMP_SERVER=$SOCK_LOCAL
	if [ $proto = ipcomp ]; then
		# IPComp sends a packet as-is if a compressed payload of
		# the packet is greater than or equal to the original payload.
		# So we have to fill a payload with 1 to let IPComp always send
		# a compressed packet.

		# pktsize == minlen - 1
		pktsize=$(($(get_minlen $algo) - 8 - 1))
		atf_check -s exit:0 -o ignore \
		    rump.ping6 -c 1 -n -X 3 -s $pktsize -p ff $ip_peer
		extract_new_packets $BUS > $outfile
		check_packets $outfile $ip_local $ip_peer ICMP6

		# pktsize == minlen
		pktsize=$(($(get_minlen $algo) - 8))
		atf_check -s exit:0 -o ignore \
		    rump.ping6 -c 1 -n -X 3 -s $pktsize -p ff $ip_peer
		extract_new_packets $BUS > $outfile
		check_packets $outfile $ip_local $ip_peer $pktproto
	else
		atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X 3 $ip_peer
		extract_new_packets $BUS > $outfile
		check_packets $outfile $ip_local $ip_peer $pktproto
	fi

	test_flush_entries $SOCK_LOCAL
	test_flush_entries $SOCK_PEER
}

test_transport_common()
{
	local ipproto=$1
	local proto=$2
	local algo=$3

	if [ $ipproto = ipv4 ]; then
		test_ipsec4_transport $proto $algo
	else
		test_ipsec6_transport $proto $algo
	fi
}

add_test_transport_mode()
{
	local ipproto=$1
	local proto=$2
	local algo=$3
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	name="ipsec_transport_${ipproto}_${proto}_${_algo}"
	desc="Tests of IPsec ($ipproto) transport mode with $proto ($algo)"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server setkey
	    }
	    ${name}_body() {
	        test_transport_common $ipproto $proto $algo
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
	local algo=

	for algo in $ESP_ENCRYPTION_ALGORITHMS; do
		add_test_transport_mode ipv4 esp $algo
		add_test_transport_mode ipv6 esp $algo
	done
	for algo in $AH_AUTHENTICATION_ALGORITHMS; do
		add_test_transport_mode ipv4 ah $algo
		add_test_transport_mode ipv6 ah $algo
	done
	for algo in $IPCOMP_COMPRESSION_ALGORITHMS; do
		add_test_transport_mode ipv4 ipcomp $algo
		add_test_transport_mode ipv6 ipcomp $algo
	done
}
