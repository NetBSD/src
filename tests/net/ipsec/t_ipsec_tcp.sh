#	$NetBSD: t_ipsec_tcp.sh,v 1.2.2.2 2017/10/21 19:43:55 snj Exp $
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

DEBUG=${DEBUG:-true}

setup_sasp()
{
	local proto=$1
	local algo_args="$2"
	local ip_local=$3
	local ip_peer=$4
	local tmpfile=./tmp
	local extra=

	export RUMP_SERVER=$SOCK_LOCAL
	cat > $tmpfile <<-EOF
	add $ip_local $ip_peer $proto 10000 $algo_args;
	add $ip_peer $ip_local $proto 10001 $algo_args;
	spdadd $ip_local $ip_peer any -P out ipsec $proto/transport//require;
	$extra
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	# XXX it can be expired if $lifetime is very short
	#check_sa_entries $SOCK_LOCAL $ip_local $ip_peer

	export RUMP_SERVER=$SOCK_PEER
	cat > $tmpfile <<-EOF
	add $ip_local $ip_peer $proto 10000 $algo_args;
	add $ip_peer $ip_local $proto 10001 $algo_args;
	spdadd $ip_peer $ip_local any -P out ipsec $proto/transport//require;
	$extra
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	# XXX it can be expired if $lifetime is very short
	#check_sa_entries $SOCK_PEER $ip_local $ip_peer
}

prepare_file()
{
	local file=$1
	local data="0123456789"

	touch $file
	for i in `seq 1 512`
	do
		echo $data >> $file
	done
}

test_tcp()
{
	local local_proto=$1
	local ip_local=$2
	local peer_proto=$3
	local ip_peer=$4
	local port=1234
	local file_send=./file.send
	local file_recv=./file.recv
	local opts=

	if [ $local_proto = ipv4 ]; then
		opts="-N -w 3 -4"
	else
		opts="-N -w 3 -6"
	fi

	# Start nc server
	start_nc_server $SOCK_PEER $port $file_recv $peer_proto

	export RUMP_SERVER=$SOCK_LOCAL
	# Send a file to the server
	prepare_file $file_send
	atf_check -s exit:0 $HIJACKING nc $opts $ip_peer $port < $file_send

	# Check if the file is transferred correctly
	atf_check -s exit:0 diff -q $file_send $file_recv

	stop_nc_server
	rm -f $file_send $file_recv
}

test_tcp_ipv4()
{
	local proto=$1
	local algo=$2
	local ip_local=10.0.0.1
	local ip_peer=10.0.0.2
	local algo_args="$(generate_algo_args $proto $algo)"
	local proto_cap=$(echo $proto | tr 'a-z' 'A-Z')
	local outfile=./out

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

	if [ $proto != none ]; then
		setup_sasp $proto "$algo_args" $ip_local $ip_peer
	fi

	extract_new_packets $BUS > $outfile

	test_tcp ipv4 $ip_local ipv4 $ip_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	if [ $proto != none ]; then
		atf_check -s exit:0 \
		    -o match:"$ip_local > $ip_peer: $proto_cap" \
		    cat $outfile
		atf_check -s exit:0 \
		    -o match:"$ip_peer > $ip_local: $proto_cap" \
		    cat $outfile
	fi
}

test_tcp_ipv6()
{
	local proto=$1
	local algo=$2
	local ip_local=fd00::1
	local ip_peer=fd00::2
	local algo_args="$(generate_algo_args $proto $algo)"
	local proto_cap=$(echo $proto | tr 'a-z' 'A-Z')
	local outfile=./out

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

	if [ $proto != none ]; then
		setup_sasp $proto "$algo_args" $ip_local $ip_peer
	fi

	extract_new_packets $BUS > $outfile

	test_tcp ipv6 $ip_local ipv6 $ip_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	if [ $proto != none ]; then
		atf_check -s exit:0 \
		    -o match:"$ip_local > $ip_peer: $proto_cap" \
		    cat $outfile
		atf_check -s exit:0 \
		    -o match:"$ip_peer > $ip_local: $proto_cap" \
		    cat $outfile
	fi
}

test_tcp_ipv4mappedipv6()
{
	local proto=$1
	local algo=$2
	local ip_local=10.0.0.1
	local ip_peer=10.0.0.2
	local ip6_peer=::ffff:10.0.0.2
	local algo_args="$(generate_algo_args $proto $algo)"
	local proto_cap=$(echo $proto | tr 'a-z' 'A-Z')
	local outfile=./out

	rump_server_crypto_start $SOCK_LOCAL netipsec
	rump_server_crypto_start $SOCK_PEER netipsec netinet6
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_PEER shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local/24
	atf_check -s exit:0 rump.ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 -o ignore rump.sysctl -w net.inet6.ip6.v6only=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_peer/24
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip6_peer/96
	atf_check -s exit:0 rump.ifconfig -w 10

	if [ $proto != none ]; then
		setup_sasp $proto "$algo_args" $ip_local $ip_peer 100
	fi

	extract_new_packets $BUS > $outfile

	test_tcp ipv4 $ip_local ipv6 $ip_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	if [ $proto != none ]; then
		atf_check -s exit:0 \
		    -o match:"$ip_local > $ip_peer: $proto_cap" \
		    cat $outfile
		atf_check -s exit:0 \
		    -o match:"$ip_peer > $ip_local: $proto_cap" \
		    cat $outfile
	fi
}

add_test_tcp()
{
	local ipproto=$1
	local proto=$2
	local algo=$3
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	if [ $proto = none ]; then
		desc="Tests of TCP with IPsec enabled ($ipproto)"
		name="ipsec_tcp_${ipproto}_${proto}"
	else
		desc="Tests of TCP with IPsec ($ipproto) $proto $algo"
		name="ipsec_tcp_${ipproto}_${proto}_${_algo}"
	fi

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server setkey
	    }
	    ${name}_body() {
	        test_tcp_${ipproto} $proto $algo
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

	for algo in $ESP_ENCRYPTION_ALGORITHMS_MINIMUM; do
		add_test_tcp ipv4 esp $algo
		add_test_tcp ipv6 esp $algo
		add_test_tcp ipv4mappedipv6 esp $algo
	done
	for algo in $AH_AUTHENTICATION_ALGORITHMS_MINIMUM; do
		add_test_tcp ipv4 ah $algo
		add_test_tcp ipv6 ah $algo
		add_test_tcp ipv4mappedipv6 ah $algo
	done

	add_test_tcp ipv4 none
	add_test_tcp ipv6 none
	add_test_tcp ipv4mappedipv6 none
}
