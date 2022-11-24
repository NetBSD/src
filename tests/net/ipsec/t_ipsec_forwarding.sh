#       $NetBSD: t_ipsec_forwarding.sh,v 1.2 2022/11/24 02:58:28 knakahara Exp $
#
# Copyright (c) 2022 Internet Initiative Japan Inc.
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
SOCK_FORWARD=unix://ipsec_forward
SOCK_REMOTE=unix://ipsec_remote
BUS_LOCAL_I=./bus_ipsec_local
BUS_LOCAL_F=./bus_ipsec_local_forward
BUS_REMOTE_F=./bus_ipsec_remote_forward
BUS_REMOTE_I=./bus_ipsec_remote

DEBUG=${DEBUG:-false}

setup_servers_ipv4()
{

	rump_server_crypto_start $SOCK_LOCAL netipsec
	rump_server_crypto_start $SOCK_FORWARD netipsec
	rump_server_crypto_start $SOCK_REMOTE netipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS_LOCAL_F
	rump_server_add_iface $SOCK_LOCAL shmif1 $BUS_LOCAL_I
	rump_server_add_iface $SOCK_FORWARD shmif0 $BUS_LOCAL_F
	rump_server_add_iface $SOCK_FORWARD shmif1 $BUS_REMOTE_F
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS_REMOTE_F
	rump_server_add_iface $SOCK_REMOTE shmif1 $BUS_REMOTE_I
}

setup_servers_ipv6()
{

	rump_server_crypto_start $SOCK_LOCAL netipsec netinet6
	rump_server_crypto_start $SOCK_FORWARD netipsec netinet6
	rump_server_crypto_start $SOCK_REMOTE netipsec netinet6
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS_LOCAL_F
	rump_server_add_iface $SOCK_LOCAL shmif1 $BUS_LOCAL_I
	rump_server_add_iface $SOCK_FORWARD shmif0 $BUS_LOCAL_F
	rump_server_add_iface $SOCK_FORWARD shmif1 $BUS_REMOTE_F
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS_REMOTE_F
	rump_server_add_iface $SOCK_REMOTE shmif1 $BUS_REMOTE_I
}

setup_servers()
{
	local proto=$1

	setup_servers_$proto
}

setup_sp_port()
{
	local proto=$1
	local algo_args="$2"
	local tunnel_src=$3
	local tunnel_dst=$4
	local subnet_src=$5
	local subnet_dst=$6
	local port_src=$7
	local port_dst=$8
	local tmpfile=./tmp

	export RUMP_SERVER=$SOCK_LOCAL
	cat > $tmpfile <<-EOF
	spdadd $subnet_src[$port_src] $subnet_dst[$port_dst] tcp -P out ipsec $proto/tunnel/$tunnel_src-$tunnel_dst/require;
	spdadd $subnet_dst[$port_dst] $subnet_src[$port_src] tcp -P in ipsec $proto/tunnel/$tunnel_dst-$tunnel_src/require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -DP

	export RUMP_SERVER=$SOCK_FORWARD
	cat > $tmpfile <<-EOF
	spdadd $subnet_dst[$port_dst] $subnet_src[$port_src] tcp -P out ipsec $proto/tunnel/$tunnel_dst-$tunnel_src/require;
	spdadd $subnet_src[$port_src] $subnet_dst[$port_dst] tcp -P in ipsec $proto/tunnel/$tunnel_src-$tunnel_dst/require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -DP
}

add_sa()
{
	local proto=$1
	local algo_args="$2"
	local tunnel_src=$3
	local tunnel_dst=$4
	local spi=$5
	local port_src=$6
	local port_dst=$7
	local tmpfile=./tmp

	export RUMP_SERVER=$SOCK_LOCAL
	cat > $tmpfile <<-EOF
	add $tunnel_src [$port_src] $tunnel_dst [$port_dst] $proto $((spi)) $algo_args;
	add $tunnel_dst [$port_dst] $tunnel_src [$port_src] $proto $((spi + 1)) $algo_args;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
	# XXX it can be expired if $lifetime is very short
	#check_sa_entries $SOCK_LOCAL $ip_local $ip_remote

	export RUMP_SERVER=$SOCK_FORWARD
	cat > $tmpfile <<-EOF
	add $tunnel_src [$port_src] $tunnel_dst [$port_dst] $proto $((spi)) $algo_args;
	add $tunnel_dst [$port_dst] $tunnel_src [$port_src] $proto $((spi + 1)) $algo_args;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
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

test_ipsec_sp_port_ipv4()
{

	local algo=$1
	local ip_local_i=192.168.11.1
	local ip_local_i_subnet=192.168.11.0/24
	local ip_local_f=10.22.22.2
	local ip_local_f_subnet=10.22.22.0/24
	local ip_forward_l=10.22.22.1
	local ip_forward_l_subnet=10.22.22.0/24
	local ip_forward_r=10.33.33.1
	local ip_forward_r_subnet=10.33.33.0/24
	local ip_remote_f=10.33.33.2
	local ip_remote_f_subnet=10.33.33.0/24
	local ip_remote_i=192.168.44.1
	local ip_remote_i_subnet=192.168.44.0/24
	local port=1234
	local loutfile=./out_local
	local routfile=./out_remote
	local file_send=./file.send
	local file_recv=./file.recv
	local algo_args="$(generate_algo_args esp $algo)"
	local pid=

	setup_servers ipv4

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet $ip_local_f/24
	atf_check -s exit:0 rump.ifconfig shmif1 inet $ip_local_i/24
	atf_check -s exit:0 -o ignore \
	    rump.route add -inet default $ip_forward_l

	export RUMP_SERVER=$SOCK_FORWARD
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwarding=1
	atf_check -s exit:0 rump.ifconfig shmif0 inet $ip_forward_l/24
	atf_check -s exit:0 rump.ifconfig shmif1 inet $ip_forward_r/24
	atf_check -s exit:0 -o ignore \
	    rump.route add -inet $ip_local_i_subnet $ip_local_f
	atf_check -s exit:0 -o ignore \
	    rump.route add -inet $ip_remote_i_subnet $ip_remote_f

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet $ip_remote_f/24
	atf_check -s exit:0 rump.ifconfig shmif1 inet $ip_remote_i/24
	atf_check -s exit:0 -o ignore \
	    rump.route add -inet default $ip_forward_r

	extract_new_packets $BUS_LOCAL_F > $loutfile
	extract_new_packets $BUS_REMOTE_F > $routfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 -I $ip_local_i \
		  $ip_remote_i

	extract_new_packets $BUS_LOCAL_F > $loutfile
	extract_new_packets $BUS_REMOTE_F > $routfile
	$DEBUG && cat $loutfile
	atf_check -s exit:0 \
	    -o match:"$ip_local_i > $ip_remote_i: ICMP echo request" \
	    cat $loutfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote_i > $ip_local_i: ICMP echo reply" \
	    cat $loutfile
	$DEBUG && cat $routfile
	atf_check -s exit:0 \
	    -o match:"$ip_local_i > $ip_remote_i: ICMP echo request" \
	    cat $routfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote_i > $ip_local_i: ICMP echo reply" \
	    cat $routfile

	# Try TCP communications just in case
	start_nc_server $SOCK_REMOTE $port $file_recv ipv4
	prepare_file $file_send
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 $HIJACKING nc -w 7 -s $ip_local_i \
		  $ip_remote_i $port < $file_send
	atf_check -s exit:0 diff -q $file_send $file_recv
	stop_nc_server

	extract_new_packets $BUS_LOCAL_F > $loutfile
	extract_new_packets $BUS_REMOTE_F > $routfile
	$DEBUG && cat $loutfile
	atf_check -s exit:0 \
	    -o match:"${ip_local_i}\.[0-9]+ > ${ip_remote_i}\.$port" \
	    cat $loutfile
	atf_check -s exit:0 \
	    -o match:"${ip_remote_i}\.$port > ${ip_local_i}\.[0-9]+" \
	    cat $loutfile
	$DEBUG && cat $routfile
	atf_check -s exit:0 \
	    -o match:"${ip_local_i}\.[0-9]+ > ${ip_remote_i}\.$port" \
	    cat $routfile
	atf_check -s exit:0 \
	    -o match:"${ip_remote_i}\.$port > ${ip_local_i}\.[0-9]+" \
	    cat $routfile

	# Create IPsec connections
	setup_sp_port esp "$algo_args" $ip_local_i $ip_forward_r \
		      $ip_local_i_subnet $ip_remote_i_subnet any $port
	add_sa esp "$algo_args" $ip_local_i $ip_forward_r \
	       10000 any $port

	extract_new_packets $BUS_LOCAL_F > $loutfile
	extract_new_packets $BUS_REMOTE_F > $routfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 -I $ip_local_i \
		  $ip_remote_i

	extract_new_packets $BUS_LOCAL_F > $loutfile
	extract_new_packets $BUS_REMOTE_F > $routfile
	$DEBUG && cat $loutfile
	atf_check -s exit:0 \
	    -o match:"$ip_local_i > $ip_remote_i: ICMP echo request" \
	    cat $loutfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote_i > $ip_local_i: ICMP echo reply" \
	    cat $loutfile
	$DEBUG && cat $routfile
	atf_check -s exit:0 \
	    -o match:"$ip_local_i > $ip_remote_i: ICMP echo request" \
	    cat $routfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote_i > $ip_local_i: ICMP echo reply" \
	    cat $routfile

	# Check TCP communications from local to remote
	start_nc_server $SOCK_REMOTE $port $file_recv ipv4
	prepare_file $file_send
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 $HIJACKING nc -w 15 -s $ip_local_i \
		  $ip_remote_i $port < $file_send
	atf_check -s exit:0 diff -q $file_send $file_recv
	stop_nc_server

	extract_new_packets $BUS_LOCAL_F > $loutfile
	extract_new_packets $BUS_REMOTE_F > $routfile
	$DEBUG && cat $loutfile
	atf_check -s exit:0 \
	    -o match:"${ip_local_i} > ${ip_forward_r}: ESP" \
	    cat $loutfile
	atf_check -s exit:0 \
	    -o match:"${ip_forward_r} > ${ip_local_i}: ESP" \
	    cat $loutfile
	$DEBUG && cat $routfile
	atf_check -s exit:0 \
	    -o match:"${ip_local_i}\.[0-9]+ > ${ip_remote_i}\.$port" \
	    cat $routfile
	atf_check -s exit:0 \
	    -o match:"${ip_remote_i}\.$port > ${ip_local_i}\.[0-9]+" \
	    cat $routfile
}

test_ipsec_sp_port_ipv6()
{
	local algo=$1
	local ip_local_i=fc00:1111::1
	local ip_local_i_subnet=fc00:1111::/64
	local ip_local_f=fc00:2222::2
	local ip_local_f_subnet=fc00:2222::/64
	local ip_forward_l=fc00:2222::1
	local ip_forward_l_subnet=fc00:2222::/64
	local ip_forward_r=fc00:3333::1
	local ip_forward_r_subnet=fc00:3333::/64
	local ip_remote_f=fc00:3333::2
	local ip_remote_f_subnet=fc00:3333::/64
	local ip_remote_i=fc00:4444::1
	local ip_remote_i_subnet=fc00:4444::/64
	local port=1234
	local loutfile=./out_local
	local routfile=./out_remote
	local file_send=./file.send
	local file_recv=./file.recv
	local algo_args="$(generate_algo_args esp $algo)"
	local pid=

	setup_servers ipv6

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_local_f/64
	atf_check -s exit:0 rump.ifconfig shmif1 inet6 $ip_local_i/64
	atf_check -s exit:0 -o ignore \
	    rump.route add -inet6 default $ip_forward_l

	export RUMP_SERVER=$SOCK_FORWARD
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.forwarding=1
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_forward_l/64
	atf_check -s exit:0 rump.ifconfig shmif1 inet6 $ip_forward_r/64
	atf_check -s exit:0 -o ignore \
	    rump.route add -inet6 $ip_local_i_subnet $ip_local_f
	atf_check -s exit:0 -o ignore \
	    rump.route add -inet6 $ip_remote_i_subnet $ip_remote_f

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_remote_f/64
	atf_check -s exit:0 rump.ifconfig shmif1 inet6 $ip_remote_i/64
	atf_check -s exit:0 -o ignore \
	    rump.route add -inet6 default $ip_forward_r

	extract_new_packets $BUS_LOCAL_F > $loutfile
	extract_new_packets $BUS_REMOTE_F > $routfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X 3 -S $ip_local_i \
		  $ip_remote_i

	extract_new_packets $BUS_LOCAL_F > $loutfile
	extract_new_packets $BUS_REMOTE_F > $routfile
	$DEBUG && cat $loutfile
	atf_check -s exit:0 \
	    -o match:"$ip_local_i > $ip_remote_i: ICMP6, echo request" \
	    cat $loutfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote_i > $ip_local_i: ICMP6, echo reply" \
	    cat $loutfile
	$DEBUG && cat $routfile
	atf_check -s exit:0 \
	    -o match:"$ip_local_i > $ip_remote_i: ICMP6, echo request" \
	    cat $routfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote_i > $ip_local_i: ICMP6, echo reply" \
	    cat $routfile

	# Try TCP communications just in case
	start_nc_server $SOCK_REMOTE $port $file_recv ipv6
	prepare_file $file_send
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 $HIJACKING nc -w 7 -s $ip_local_i \
		  $ip_remote_i $port < $file_send
	atf_check -s exit:0 diff -q $file_send $file_recv
	stop_nc_server

	extract_new_packets $BUS_LOCAL_F > $loutfile
	extract_new_packets $BUS_REMOTE_F > $routfile
	$DEBUG && cat $loutfile
	atf_check -s exit:0 \
	    -o match:"${ip_local_i}\.[0-9]+ > ${ip_remote_i}\.$port" \
	    cat $loutfile
	atf_check -s exit:0 \
	    -o match:"${ip_remote_i}\.$port > ${ip_local_i}\.[0-9]+" \
	    cat $loutfile
	$DEBUG && cat $routfile
	atf_check -s exit:0 \
	    -o match:"${ip_local_i}\.[0-9]+ > ${ip_remote_i}\.$port" \
	    cat $routfile
	atf_check -s exit:0 \
	    -o match:"${ip_remote_i}\.$port > ${ip_local_i}\.[0-9]+" \
	    cat $routfile

	# Create IPsec connections
	setup_sp_port esp "$algo_args" $ip_local_i $ip_forward_r \
		      $ip_local_i_subnet $ip_remote_i_subnet any $port
	add_sa esp "$algo_args" $ip_local_i $ip_forward_r \
	       10000 any $port

	extract_new_packets $BUS_LOCAL_F > $loutfile
	extract_new_packets $BUS_REMOTE_F > $routfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X 3 -S $ip_local_i \
		  $ip_remote_i

	extract_new_packets $BUS_LOCAL_F > $loutfile
	extract_new_packets $BUS_REMOTE_F > $routfile
	$DEBUG && cat $loutfile
	atf_check -s exit:0 \
	    -o match:"$ip_local_i > $ip_remote_i: ICMP6, echo request" \
	    cat $loutfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote_i > $ip_local_i: ICMP6, echo reply" \
	    cat $loutfile
	$DEBUG && cat $routfile
	atf_check -s exit:0 \
	    -o match:"$ip_local_i > $ip_remote_i: ICMP6, echo request" \
	    cat $routfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote_i > $ip_local_i: ICMP6, echo reply" \
	    cat $routfile

	# Check TCP communications from local to remote
	start_nc_server $SOCK_REMOTE $port $file_recv ipv6
	prepare_file $file_send
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 $HIJACKING nc -w 7 -s $ip_local_i \
		  $ip_remote_i $port < $file_send
	atf_check -s exit:0 diff -q $file_send $file_recv
	stop_nc_server

	extract_new_packets $BUS_LOCAL_F > $loutfile
	extract_new_packets $BUS_REMOTE_F > $routfile
	$DEBUG && cat $loutfile
	atf_check -s exit:0 \
	    -o match:"${ip_local_i} > ${ip_forward_r}: ESP" \
	    cat $loutfile
	atf_check -s exit:0 \
	    -o match:"${ip_forward_r} > ${ip_local_i}: ESP" \
	    cat $loutfile
	$DEBUG && cat $routfile
	atf_check -s exit:0 \
	    -o match:"${ip_local_i}\.[0-9]+ > ${ip_remote_i}\.$port" \
	    cat $routfile
	atf_check -s exit:0 \
	    -o match:"${ip_remote_i}\.$port > ${ip_local_i}\.[0-9]+" \
	    cat $routfile
}

add_test_ipsec_sp_port()
{
	local proto=$1
	local algo=$2
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	desc="Test IPsec $proto forwarding SP port ($algo)"
	name="ipsec_sp_port_${proto}_${_algo}"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server setkey nc
	    }
	    ${name}_body() {
	        test_ipsec_sp_port_$proto $algo
	        rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
	    	stop_nc_server
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
		add_test_ipsec_sp_port ipv4 $algo
		add_test_ipsec_sp_port ipv6 $algo
	done
}
