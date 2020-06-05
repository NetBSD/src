#	$NetBSD: t_ipsec_natt.sh,v 1.5 2020/06/05 03:24:58 knakahara Exp $
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

SOCK_LOCAL=unix://ipsec_natt_local
SOCK_NAT=unix://ipsec_natt_nat
SOCK_REMOTE=unix://ipsec_natt_remote
BUS_LOCAL=./bus_ipsec_natt_local
BUS_NAT=./bus_ipsec_natt_nat
BUS_REMOTE=./bus_ipsec_natt_remote
BUS_GLOBAL=./bus_ipsec_natt_global

DEBUG=${DEBUG:-false}

setup_servers_ipv4()
{

	rump_server_crypto_start $SOCK_LOCAL netipsec
	rump_server_npf_start $SOCK_NAT
	rump_server_crypto_start $SOCK_REMOTE netipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS_LOCAL
	rump_server_add_iface $SOCK_NAT shmif0 $BUS_LOCAL
	rump_server_add_iface $SOCK_NAT shmif1 $BUS_NAT
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS_NAT
}

setup_servers_ipv6()
{

	rump_server_crypto_start $SOCK_LOCAL netipsec netinet6 ipsec
	rump_server_crypto_start $SOCK_REMOTE netipsec netinet6 ipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS_GLOBAL
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS_GLOBAL
}

setup_servers()
{
	local proto=$1

	setup_servers_$proto
}

setup_sp()
{
	local proto=$1
	local algo_args="$2"
	local ip_local=$3
	local ip_remote=$4
	local ip_nat_remote=$5
	local tmpfile=./tmp

	export RUMP_SERVER=$SOCK_LOCAL
	cat > $tmpfile <<-EOF
	spdadd $ip_local $ip_remote any -P out ipsec $proto/transport//require;
	spdadd $ip_remote $ip_local any -P in ipsec $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	#check_sp_entries $SOCK_LOCAL $ip_local $ip_remote

	export RUMP_SERVER=$SOCK_REMOTE
	cat > $tmpfile <<-EOF
	spdadd $ip_remote $ip_nat_remote any -P out ipsec $proto/transport//require;
	spdadd $ip_local $ip_remote any -P in ipsec $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	#check_sp_entries $SOCK_REMOTE $ip_remote $ip_local
}

add_sa()
{
	local proto=$1
	local algo_args="$2"
	local ip_local=$3
	local ip_remote=$4
	local ip_nat_remote=$5
	local spi=$6
	local port=$7
	local tmpfile=./tmp

	export RUMP_SERVER=$SOCK_LOCAL
	cat > $tmpfile <<-EOF
	add $ip_local [4500] $ip_remote [4500] $proto $((spi)) $algo_args;
	add $ip_remote [4500] $ip_local [4500] $proto $((spi + 1)) $algo_args;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
	# XXX it can be expired if $lifetime is very short
	#check_sa_entries $SOCK_LOCAL $ip_local $ip_remote

	export RUMP_SERVER=$SOCK_REMOTE
	cat > $tmpfile <<-EOF
	add $ip_local [$port] $ip_remote [4500] $proto $((spi)) $algo_args;
	add $ip_remote [4500] $ip_nat_remote [$port] $proto $((spi + 1)) $algo_args;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
	# XXX it can be expired if $lifetime is very short
	#check_sa_entries $SOCK_PEER $ip_local $ip_remote
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

build_npf_conf()
{
	local outfile=$1
	local localnet=$2

	cat > $outfile <<-EOF
	set bpf.jit off
	\$int_if = inet4(shmif0)
	\$ext_if = inet4(shmif1)
	\$localnet = { $localnet }
	map \$ext_if dynamic \$localnet -> \$ext_if
	group "external" on \$ext_if {
		pass stateful out final all
	}
	group "internal" on \$int_if {
		block in all
		pass in final from \$localnet
		pass out final all
	}
	group default {
		pass final on lo0 all
		block all
	}
	EOF
}

PIDSFILE=./terminator.pids
start_natt_terminator()
{
	local sock=$1
	local proto=$2
	local ip=$3
	local port=$4
	local pidsfile=$5
	local backup=$RUMP_SERVER
	local pid= opt=
	local terminator="$(atf_get_srcdir)/natt_terminator"

	if [ "$proto" = "ipv6" ]; then
	    opt="-6"
	else
	    opt="-4"
	fi

	export RUMP_SERVER=$sock

	env LD_PRELOAD=/usr/lib/librumphijack.so \
	    $terminator $opt $ip $port &
	pid=$!
	if [ ! -f $PIDSFILE ]; then
		touch $PIDSFILE
	fi
	echo $pid >> $PIDSFILE

	$DEBUG && rump.netstat -a -f inet

	export RUMP_SERVER=$backup

	sleep 1
}

stop_natt_terminators()
{
	local pid=

	if [ ! -f $PIDSFILE ]; then
		return
	fi

	for pid in $(cat $PIDSFILE); do
		kill -9 $pid
	done
	rm -f $PIDSFILE
}

test_ipsec_natt_transport_ipv4()
{
	local algo=$1
	local ip_local=10.0.1.2
	local ip_nat_local=10.0.1.1
	local ip_nat_remote=20.0.0.1
	local ip_remote=20.0.0.2
	local subnet_local=10.0.1.0
	local outfile=./out
	local npffile=./npf.conf
	local file_send=./file.send
	local file_recv=./file.recv
	local algo_args="$(generate_algo_args esp-udp $algo)"
	local pid= port=

	setup_servers ipv4

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local/24
	atf_check -s exit:0 -o ignore \
	    rump.route -n add default $ip_nat_local

	export RUMP_SERVER=$SOCK_NAT
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_nat_local/24
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_nat_remote/24
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwarding=1

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_remote/24
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -net $subnet_local $ip_nat_remote

	extract_new_packets $BUS_NAT > $outfile

	# There is no NAT/NAPT. ping should just work.
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_remote

	extract_new_packets $BUS_NAT > $outfile
	$DEBUG && cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_local > $ip_remote: ICMP echo request" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote > $ip_local: ICMP echo reply" \
	    cat $outfile

	# Setup an NAPT with npf
	build_npf_conf $npffile "$subnet_local/24"

	export RUMP_SERVER=$SOCK_NAT
	atf_check -s exit:0 $HIJACKING_NPF npfctl reload $npffile
	atf_check -s exit:0 $HIJACKING_NPF npfctl start
	$DEBUG && ${HIJACKING},"blanket=/dev/npf" npfctl show

	# There is an NAPT. ping works but source IP/port are translated
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_remote

	extract_new_packets $BUS_NAT > $outfile
	$DEBUG && cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_nat_remote > $ip_remote: ICMP echo request" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote > $ip_nat_remote: ICMP echo reply" \
	    cat $outfile

	# Try TCP communications just in case
	start_nc_server $SOCK_REMOTE 4501 $file_recv ipv4
	prepare_file $file_send
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 $HIJACKING nc -w 3 $ip_remote 4501 < $file_send
	atf_check -s exit:0 diff -q $file_send $file_recv
	stop_nc_server

	extract_new_packets $BUS_NAT > $outfile
	$DEBUG && cat $outfile
	atf_check -s exit:0 \
	    -o match:"${ip_nat_remote}\.[0-9]+ > ${ip_remote}\.4501" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"${ip_remote}\.4501 > ${ip_nat_remote}\.[0-9]+" \
	    cat $outfile

	# Launch a nc server as a terminator of NAT-T on outside the NAPT
	start_natt_terminator $SOCK_REMOTE ipv4 $ip_remote 4500
	echo zzz > $file_send

	export RUMP_SERVER=$SOCK_LOCAL
	# Send a UDP packet to the remote server at port 4500 from the local
	# host of port 4500. This makes a mapping on the NAPT between them
	atf_check -s exit:0 $HIJACKING \
	    nc -u -w 3 -p 4500 $ip_remote 4500 < $file_send
	# Launch a nc server as a terminator of NAT-T on inside the NAPT,
	# taking over port 4500 of the local host.
	start_natt_terminator $SOCK_LOCAL ipv4 $ip_local 4500

	# We need to keep the servers for NAT-T

	export RUMP_SERVER=$SOCK_LOCAL
	$DEBUG && rump.netstat -na -f inet
	export RUMP_SERVER=$SOCK_REMOTE
	$DEBUG && rump.netstat -na -f inet

	# Get a translated port number from 4500 on the NAPT
	export RUMP_SERVER=$SOCK_NAT
	$DEBUG && $HIJACKING_NPF npfctl list
	#          10.0.1.2:4500    20.0.0.2:4500  via shmif1:9696
	port=$(get_natt_port $ip_local $ip_nat_remote)
	$DEBUG && echo port=$port
	if [ -z "$port" ]; then
		atf_fail "Failed to get a translated port on NAPT"
	fi

	# Create ESP-UDP IPsec connections
	setup_sp esp "$algo_args" $ip_local $ip_remote $ip_nat_remote
	add_sa "esp-udp" "$algo_args" $ip_local $ip_remote $ip_nat_remote 10000 $port

	# ping should still work
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_remote

	# Try TCP communications over the ESP-UDP connections
	start_nc_server $SOCK_REMOTE 4501 $file_recv ipv4
	prepare_file $file_send
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore $HIJACKING nc -w 3 $ip_remote 4501 < $file_send
	atf_check -s exit:0 diff -q $file_send $file_recv
	stop_nc_server

	# Check both ports and UDP encapsulation
	extract_new_packets $BUS_NAT > $outfile
	$DEBUG && cat $outfile
	atf_check -s exit:0 \
	    -o match:"${ip_nat_remote}\.$port > ${ip_remote}\.4500: UDP-encap" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"${ip_remote}\.4500 > ${ip_nat_remote}\.$port: UDP-encap" \
	    cat $outfile

	# Kill the NAT-T terminator
	stop_natt_terminators
}

test_ipsec_natt_transport_ipv6_without_nat()
{
	local algo=$1
	local ip_local_phys=fc00::1
	local ip_local_ipsecif=fc00:1111::1
	local ip_remote_phys=fc00::2
	local ip_remote_ipsecif=fc00:2222::1
	local outfile=./out
	local npffile=./npf.conf
	local file_send=./file.send
	local file_recv=./file.recv
	local algo_args="$(generate_algo_args esp-udp $algo)"
	local pid=
	local port=4500

	setup_servers ipv6

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_local_phys/64

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_remote_phys/64

	extract_new_packets $BUS_GLOBAL > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X 3 $ip_remote_phys

	extract_new_packets $BUS_GLOBAL > $outfile
	$DEBUG && cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_local_phys > $ip_remote_phys: ICMP6, echo request" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote_phys > $ip_local_phys: ICMP6, echo reply" \
	    cat $outfile

	# Create ESP-UDP ipsecif(4) connections
	export RUMP_SERVER=$SOCK_LOCAL
	rump_server_add_iface $SOCK_LOCAL ipsec0
	atf_check -s exit:0 rump.ifconfig ipsec0 link0 # enable nat-t
	atf_check -s exit:0 rump.ifconfig ipsec0 link2 # ensure IPv6 forward
	atf_check -s exit:0 rump.ifconfig ipsec0 tunnel $ip_local_phys $ip_remote_phys
	atf_check -s exit:0 rump.ifconfig ipsec0 inet6 $ip_local_ipsecif
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -inet6 $ip_remote_ipsecif $ip_local_ipsecif
	start_natt_terminator $SOCK_LOCAL ipv6 $ip_local_phys $port

	add_sa "esp-udp" "$algo_args" $ip_local_phys $ip_remote_phys \
	    $ip_local_phys 10000 $port

	export RUMP_SERVER=$SOCK_REMOTE
	rump_server_add_iface $SOCK_REMOTE ipsec0
	atf_check -s exit:0 rump.ifconfig ipsec0 link0 # enable nat-t
	atf_check -s exit:0 rump.ifconfig ipsec0 link2 # ensure IPv6 forward
	atf_check -s exit:0 rump.ifconfig ipsec0 tunnel $ip_remote_phys $ip_local_phys
	atf_check -s exit:0 rump.ifconfig ipsec0 inet6 $ip_remote_ipsecif
	atf_check -s exit:0 -o ignore \
	    rump.route -n add -inet6 $ip_local_ipsecif $ip_remote_ipsecif
	start_natt_terminator $SOCK_REMOTE ipv6 $ip_remote_phys $port

	# ping should still work
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X 5 $ip_remote_ipsecif

	# Check UDP encapsulation
	extract_new_packets $BUS_GLOBAL > $outfile
	$DEBUG && cat $outfile

	atf_check -s exit:0 \
	    -o match:"${ip_local_phys}\.$port > ${ip_remote_phys}\.4500: UDP-encap" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"${ip_remote_phys}\.4500 > ${ip_local_phys}\.$port: UDP-encap" \
	    cat $outfile

	# Kill the NAT-T terminator
	stop_natt_terminators
	export RUMP_SERVER=$SOCK_REMOTE
	stop_natt_terminators
}

test_ipsec_natt_transport_ipv6()
{
	local algo=$1

	test_ipsec_natt_transport_ipv6_without_nat $algo
}

add_test_ipsec_natt_transport()
{
	local proto=$1
	local algo=$2
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	desc="Test IPsec $proto NAT-T ($algo)"
	name="ipsec_natt_transport_${proto}_${_algo}"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server setkey nc
	    }
	    ${name}_body() {
	        test_ipsec_natt_transport_$proto $algo
	        rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
		stop_nc_server
		stop_natt_terminators
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
		add_test_ipsec_natt_transport ipv4 $algo
		add_test_ipsec_natt_transport ipv6 $algo
	done
}
