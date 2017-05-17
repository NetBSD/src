#	$NetBSD: t_ipsec_misc.sh,v 1.2 2017/05/17 06:30:15 ozaki-r Exp $
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

setup_sasp()
{
	local proto=$1
	local algo_args="$2"
	local ip_local=$3
	local ip_peer=$4
	local lifetime=$5
	local tmpfile=./tmp

	export RUMP_SERVER=$SOCK_LOCAL
	cat > $tmpfile <<-EOF
	add $ip_local $ip_peer $proto 10000 -lh $lifetime -ls $lifetime $algo_args;
	add $ip_peer $ip_local $proto 10001 -lh $lifetime -ls $lifetime $algo_args;
	spdadd $ip_local $ip_peer any -P out ipsec $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	check_sa_entries $SOCK_LOCAL $ip_local $ip_peer

	export RUMP_SERVER=$SOCK_PEER
	cat > $tmpfile <<-EOF
	add $ip_local $ip_peer $proto 10000 -lh $lifetime -ls $lifetime $algo_args;
	add $ip_peer $ip_local $proto 10001 -lh $lifetime -ls $lifetime $algo_args;
	spdadd $ip_peer $ip_local any -P out ipsec $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	check_sa_entries $SOCK_PEER $ip_local $ip_peer
}

test_ipsec4_lifetime()
{
	local proto=$1
	local algo=$2
	local ip_local=10.0.0.1
	local ip_peer=10.0.0.2
	local outfile=./out
	local proto_cap=$(echo $proto | tr 'a-z' 'A-Z')
	local algo_args="$(generate_algo_args $proto $algo)"
	local lifetime=3

	rump_server_crypto_start $SOCK_LOCAL netipsec
	rump_server_crypto_start $SOCK_PEER netipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_PEER shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local/24
	#atf_check -s exit:0 -o ignore rump.sysctl -w net.key.debug=0xff

	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_peer/24
	#atf_check -s exit:0 -o ignore rump.sysctl -w net.key.debug=0xff

	extract_new_packets $BUS > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:"$ip_local > $ip_peer: ICMP echo request" \
	    cat $outfile
	atf_check -s exit:0 -o match:"$ip_peer > $ip_local: ICMP echo reply" \
	    cat $outfile

	# Set up SAs with lifetime 1 sec.
	setup_sasp $proto "$algo_args" $ip_local $ip_peer 1

	# Wait for the SAs to be expired
	atf_check -s exit:0 sleep 2

	# Check the SAs have been expired
	export RUMP_SERVER=$SOCK_LOCAL
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 -o match:'No SAD entries.' $HIJACKING setkey -D
	export RUMP_SERVER=$SOCK_PEER
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 -o match:'No SAD entries.' $HIJACKING setkey -D

	# Clean up SPs
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o empty $HIJACKING setkey -F -P
	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 -o empty $HIJACKING setkey -F -P

	# Set up SAs with lifetime with $lifetime
	setup_sasp $proto "$algo_args" $ip_local $ip_peer $lifetime

	# Use the SAs; this will create a reference from an SP to an SA
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:"$ip_local > $ip_peer: $proto_cap" \
	    cat $outfile
	atf_check -s exit:0 -o match:"$ip_peer > $ip_local: $proto_cap" \
	    cat $outfile

	atf_check -s exit:0 sleep $((lifetime + 1))

	export RUMP_SERVER=$SOCK_LOCAL
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 -o empty $HIJACKING setkey -D
	# The SA on output remain because sp/isr still refers it
	atf_check -s exit:0 -o match:"$ip_local $ip_peer" \
	    $HIJACKING setkey -D -a
	atf_check -s exit:0 -o not-match:"$ip_peer $ip_local" \
	    $HIJACKING setkey -D -a

	export RUMP_SERVER=$SOCK_PEER
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 -o empty $HIJACKING setkey -D
	atf_check -s exit:0 -o not-match:"$ip_local $ip_peer" \
	    $HIJACKING setkey -D -a
	# The SA on output remain because sp/isr still refers it
	atf_check -s exit:0 -o match:"$ip_peer $ip_local" \
	    $HIJACKING setkey -D -a

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s not-exit:0 -o match:'0 packets received' \
	    rump.ping -c 1 -n -w 1 $ip_peer

	test_flush_entries $SOCK_LOCAL
	test_flush_entries $SOCK_PEER
}

test_ipsec6_lifetime()
{
	local proto=$1
	local algo=$2
	local ip_local=fd00::1
	local ip_peer=fd00::2
	local tmpfile=./tmp
	local outfile=./out
	local proto_cap=$(echo $proto | tr 'a-z' 'A-Z')
	local algo_args="$(generate_algo_args $proto $algo)"
	local lifetime=3

	rump_server_crypto_start $SOCK_LOCAL netinet6 netipsec
	rump_server_crypto_start $SOCK_PEER netinet6 netipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_PEER shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_local

	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_peer

	extract_new_packets $BUS > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X 3 $ip_peer

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:"$ip_local > $ip_peer: ICMP6, echo request" \
	    cat $outfile
	atf_check -s exit:0 -o match:"$ip_peer > $ip_local: ICMP6, echo reply" \
	    cat $outfile

	# Set up SAs with lifetime 1 sec.
	setup_sasp $proto "$algo_args" $ip_local $ip_peer 1

	# Wait for the SAs to be expired
	atf_check -s exit:0 sleep 2

	# Check the SAs have been expired
	export RUMP_SERVER=$SOCK_LOCAL
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 -o match:'No SAD entries.' $HIJACKING setkey -D
	export RUMP_SERVER=$SOCK_PEER
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 -o match:'No SAD entries.' $HIJACKING setkey -D

	# Clean up SPs
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o empty $HIJACKING setkey -F -P
	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 -o empty $HIJACKING setkey -F -P

	# Set up SAs with lifetime with $lifetime
	setup_sasp $proto "$algo_args" $ip_local $ip_peer $lifetime

	# Use the SAs; this will create a reference from an SP to an SA
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X 3 $ip_peer

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:"$ip_local > $ip_peer: $proto_cap" \
	    cat $outfile
	atf_check -s exit:0 -o match:"$ip_peer > $ip_local: $proto_cap" \
	    cat $outfile

	atf_check -s exit:0 sleep $((lifetime + 1))

	export RUMP_SERVER=$SOCK_LOCAL
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 -o empty $HIJACKING setkey -D
	# The SA on output remain because sp/isr still refers it
	atf_check -s exit:0 -o match:"$ip_local $ip_peer" \
	    $HIJACKING setkey -D -a
	atf_check -s exit:0 -o not-match:"$ip_peer $ip_local" \
	    $HIJACKING setkey -D -a

	export RUMP_SERVER=$SOCK_PEER
	$DEBUG && $HIJACKING setkey -D
	atf_check -s exit:0 -o empty $HIJACKING setkey -D
	atf_check -s exit:0 -o not-match:"$ip_local $ip_peer" \
	    $HIJACKING setkey -D -a
	# The SA on output remain because sp/isr still refers it
	atf_check -s exit:0 -o match:"$ip_peer $ip_local" \
	    $HIJACKING setkey -D -a

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s not-exit:0 -o match:'0 packets received' \
	    rump.ping6 -c 1 -n -X 1 $ip_peer

	test_flush_entries $SOCK_LOCAL
	test_flush_entries $SOCK_PEER
}

test_lifetime_common()
{
	local ipproto=$1
	local proto=$2
	local algo=$3

	if [ $ipproto = ipv4 ]; then
		test_ipsec4_lifetime $proto $algo
	else
		test_ipsec6_lifetime $proto $algo
	fi
}

add_test_lifetime()
{
	local ipproto=$1
	local proto=$2
	local algo=$3
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	name="ipsec_lifetime_${ipproto}_${proto}_${_algo}"
	desc="Tests of lifetime of IPsec ($ipproto) with $proto ($algo)"

	atf_test_case ${name} cleanup
	eval "								\
	    ${name}_head() {						\
	        atf_set \"descr\" \"$desc\";				\
	        atf_set \"require.progs\" \"rump_server\" \"setkey\";	\
	    };								\
	    ${name}_body() {						\
	        test_lifetime_common $ipproto $proto $algo;		\
	        rump_server_destroy_ifaces;				\
	    };								\
	    ${name}_cleanup() {						\
	        $DEBUG && dump;						\
	        cleanup;						\
	    }								\
	"
	atf_add_test_case ${name}
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
	local proto=$1
	local ip_local=$2
	local ip_peer=$3
	local port=1234
	local file_send=./file.send
	local file_recv=./file.recv
	local opts=

	if [ $proto = ipv4 ]; then
		opts="-N -w 3 -4"
	else
		opts="-N -w 3 -6"
	fi

	# Start nc server
	start_nc_server $SOCK_PEER $port $file_recv $proto

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
	local ip_local=10.0.0.1
	local ip_peer=10.0.0.2

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

	test_tcp ipv4 $ip_local $ip_peer
}

test_tcp_ipv6()
{
	local ip_local=fd00::1
	local ip_peer=fd00::2

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

	test_tcp ipv6 $ip_local $ip_peer
}

add_test_tcp()
{
	local ipproto=$1
	local name= desc=

	name="ipsec_tcp_${ipproto}"
	desc="Tests of TCP with IPsec enabled ($ipproto)"

	atf_test_case ${name} cleanup
	eval "								\
	    ${name}_head() {						\
	        atf_set \"descr\" \"$desc\";				\
	        atf_set \"require.progs\" \"rump_server\" \"setkey\";	\
	    };								\
	    ${name}_body() {						\
	        test_tcp_${ipproto};					\
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
		add_test_lifetime ipv4 esp $algo
		add_test_lifetime ipv6 esp $algo
	done
	for algo in $AH_AUTHENTICATION_ALGORITHMS_MINIMUM; do
		add_test_lifetime ipv4 ah $algo
		add_test_lifetime ipv6 ah $algo
	done

	add_test_tcp ipv4
	add_test_tcp ipv6
}
