#	$NetBSD: t_ipsec_natt.sh,v 1.2.2.2 2018/12/26 14:02:10 pgoyette Exp $
#
# Copyright (c) 2018 Internet Initiative Japan Inc.
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

DEBUG=${DEBUG:-false}
HIJACKING_NPF="${HIJACKING},blanket=/dev/npf"

setup_servers()
{

	rump_server_crypto_start $SOCK_LOCAL netipsec ipsec
	rump_server_npf_start $SOCK_NAT
	rump_server_crypto_start $SOCK_REMOTE netipsec ipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS_LOCAL
	rump_server_add_iface $SOCK_NAT shmif0 $BUS_LOCAL
	rump_server_add_iface $SOCK_NAT shmif1 $BUS_NAT
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS_NAT
}

setup_ipsecif()
{
	local sock=$1
	local ifid=$2
	local src_ip=$3
	local src_port=$4
	local dst_ip=$5
	local dst_port=$6
	local ipsecif_ip=$7
	local peer_ip=$8

	export RUMP_SERVER=$sock
	atf_check -s exit:0 rump.ifconfig ipsec$ifid create
	atf_check -s exit:0 rump.ifconfig ipsec$ifid link0 # enable NAT-T
	atf_check -s exit:0 rump.ifconfig ipsec$ifid tunnel ${src_ip},${src_port} ${dst_ip},${dst_port}
	atf_check -s exit:0 rump.ifconfig ipsec$ifid ${ipsecif_ip}/32
	atf_check -s exit:0 -o ignore \
	    rump.route -n add ${peer_ip}/32 $ipsecif_ip
}

add_sa()
{
	local sock=$1
	local proto=$2
	local algo_args="$3"
	local src_ip=$4
	local src_port=$5
	local dst_ip=$6
	local dst_port=$7
	local out_spi=$8
	local in_spi=$9
	local tmpfile=./tmp

	export RUMP_SERVER=$sock
	cat > $tmpfile <<-EOF
	add $src_ip [$src_port] $dst_ip [$dst_port] $proto $out_spi -m transport $algo_args;
	add $dst_ip [$dst_port] $src_ip [$src_port] $proto $in_spi -m transport $algo_args;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	# XXX it can be expired if $lifetime is very short
	#check_sa_entries $SOCK_LOCAL $ip_local $ip_remote
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
	local ip=$2
	local port=$3
	local pidsfile=$4
	local backup=$RUMP_SERVER
	local pid=
	local terminator="$(atf_get_srcdir)/../ipsec/natt_terminator"

	export RUMP_SERVER=$sock

	env LD_PRELOAD=/usr/lib/librumphijack.so \
	    $terminator $ip $port &
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

check_ping_packets()
{
	local sock=$1
	local bus=$2
	local from_ip=$3
	local to_ip=$4

	local outfile=./out.ping

	extract_new_packets $bus > $outfile

	export RUMP_SERVER=$sock
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $to_ip

	extract_new_packets $bus > $outfile
	$DEBUG && cat $outfile
	atf_check -s exit:0 \
	    -o match:"$from_ip > $to_ip: ICMP echo request" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"$to_ip > $from_ip: ICMP echo reply" \
	    cat $outfile
}

check_ping_packets_over_ipsecif()
{
	local sock=$1
	local bus=$2
	local to_ip=$3
	local nat_from_ip=$4
	local nat_from_port=$5
	local nat_to_ip=$6
	local nat_to_port=$7

	local outfile=./out.ping_over_ipsecif

	extract_new_packets $bus > $outfile

	export RUMP_SERVER=$sock
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 7 $to_ip

	# Check both ports and UDP encapsulation
	extract_new_packets $bus > $outfile
	$DEBUG && cat $outfile
	atf_check -s exit:0 \
	    -o match:"${nat_from_ip}\.$nat_from_port > ${nat_to_ip}\.${nat_to_port}: UDP-encap" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"${nat_to_ip}\.${nat_to_port} > ${nat_from_ip}\.${nat_from_port}: UDP-encap" \
	    cat $outfile
}

check_tcp_com_prepare()
{
	local server_sock=$1
	local client_sock=$2
	local bus=$3
	local to_ip=$4
	local nat_from_ip=$5
	local nat_to_ip=$6

	local outfile=./out.prepare
	local file_send=./file.send.prepare
	local file_recv=./file.recv.prepare

	extract_new_packets $bus > $outfile

	start_nc_server $server_sock 4501 $file_recv ipv4

	prepare_file $file_send
	export RUMP_SERVER=$client_sock
	atf_check -s exit:0 $HIJACKING nc -w 3 $to_ip 4501 < $file_send
	atf_check -s exit:0 diff -q $file_send $file_recv
	extract_new_packets $bus > $outfile
	$DEBUG && cat $outfile
	atf_check -s exit:0 \
	    -o match:"${nat_from_ip}\.[0-9]+ > ${nat_to_ip}\.4501" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"${nat_to_ip}\.4501 > ${nat_from_ip}\.[0-9]+" \
	    cat $outfile

	stop_nc_server
}

check_tcp_com_over_ipsecif()
{
	local server_sock=$1
	local client_sock=$2
	local bus=$3
	local to_ip=$4
	local nat_from_ip=$5
	local nat_from_port=$6
	local nat_to_ip=$7
	local nat_to_port=$8

	local outfile=./out.ipsecif
	local file_send=./file.send.ipsecif
	local file_recv=./file.recv.ipsecif

	extract_new_packets $bus > $outfile

	start_nc_server $server_sock 4501 $file_recv ipv4
	prepare_file $file_send
	export RUMP_SERVER=$client_sock
	atf_check -s exit:0 -o ignore $HIJACKING nc -w 7 $to_ip 4501 < $file_send
	atf_check -s exit:0 diff -q $file_send $file_recv
	stop_nc_server

	# Check both ports and UDP encapsulation
	extract_new_packets $bus > $outfile
	$DEBUG && cat $outfile
	atf_check -s exit:0 \
	    -o match:"${nat_from_ip}\.$nat_from_port > ${nat_to_ip}\.${nat_to_port}: UDP-encap" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"${nat_to_ip}\.${nat_to_port} > ${nat_from_ip}\.${nat_from_port}: UDP-encap" \
	    cat $outfile
}

test_ipsecif_natt_transport()
{
	local algo=$1
	local ip_local=192.168.0.2
	local ip_nat_local=192.168.0.1
	local ip_nat_remote=10.0.0.1
	local ip_remote=10.0.0.2
	local subnet_local=192.168.0.0
	local ip_local_ipsecif=172.16.100.1
	local ip_remote_ipsecif=172.16.10.1

	local npffile=./npf.conf
	local file_send=./file.send
	local algo_args="$(generate_algo_args esp-udp $algo)"
	local pid= port=

	setup_servers

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

	# There is no NAT/NAPT. ping should just work.
	check_ping_packets $SOCK_LOCAL $BUS_NAT $ip_local $ip_remote

	# Setup an NAPT with npf
	build_npf_conf $npffile "$subnet_local/24"

	export RUMP_SERVER=$SOCK_NAT
	atf_check -s exit:0 $HIJACKING_NPF npfctl reload $npffile
	atf_check -s exit:0 $HIJACKING_NPF npfctl start
	$DEBUG && ${HIJACKING},"blanket=/dev/npf" npfctl show

	# There is an NAPT. ping works but source IP/port are translated
	check_ping_packets $SOCK_LOCAL $BUS_NAT $ip_nat_remote $ip_remote

	# Try TCP communications just in case
	check_tcp_com_prepare $SOCK_REMOTE $SOCK_LOCAL $BUS_NAT \
			      $ip_remote $ip_nat_remote $ip_remote

	# Launch a nc server as a terminator of NAT-T on outside the NAPT
	start_natt_terminator $SOCK_REMOTE $ip_remote 4500
	echo zzz > $file_send


	export RUMP_SERVER=$SOCK_LOCAL
	# Send a UDP packet to the remote server at port 4500 from the local
	# host of port 4500. This makes a mapping on the NAPT between them
	atf_check -s exit:0 $HIJACKING \
	    nc -u -w 3 -p 4500 $ip_remote 4500 < $file_send
	# Launch a nc server as a terminator of NAT-T on inside the NAPT,
	# taking over port 4500 of the local host.
	start_natt_terminator $SOCK_LOCAL $ip_local 4500

	# We need to keep the servers for NAT-T

	export RUMP_SERVER=$SOCK_LOCAL
	$DEBUG && rump.netstat -na -f inet
	export RUMP_SERVER=$SOCK_REMOTE
	$DEBUG && rump.netstat -na -f inet

	# Get a translated port number from 4500 on the NAPT
	export RUMP_SERVER=$SOCK_NAT
	$DEBUG && $HIJACKING_NPF npfctl list
	#          192.168.0.2:4500 10.0.0.2:4500  via shmif1:65248
	port=$($HIJACKING_NPF npfctl list | grep $ip_local | awk -F 'shmif1:' '/4500/ {print $2;}')
	$DEBUG && echo port=$port
	if [ -z "$port" ]; then
		atf_fail "Failed to get a traslated port on NAPT"
	fi

	# Setup ESP-UDP ipsecif(4) for first client under NAPT
	setup_ipsecif $SOCK_LOCAL 0 $ip_local 4500 $ip_remote 4500 \
		      $ip_local_ipsecif $ip_remote_ipsecif
	setup_ipsecif $SOCK_REMOTE 0 $ip_remote 4500 $ip_nat_remote $port \
		      $ip_remote_ipsecif $ip_local_ipsecif

	add_sa $SOCK_LOCAL "esp-udp" "$algo_args" \
	       $ip_local 4500 $ip_remote 4500 10000 10001
	add_sa $SOCK_REMOTE "esp-udp" "$algo_args" \
	       $ip_remote 4500 $ip_nat_remote $port 10001 10000

	export RUMP_SERVER=$SOCK_LOCAL
	# ping should still work
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_remote

	# Try ping over the ESP-UDP ipsecif(4)
	check_ping_packets_over_ipsecif $SOCK_LOCAL $BUS_NAT \
					 $ip_remote_ipsecif $ip_nat_remote $port $ip_remote 4500

	# Try TCP communications over the ESP-UDP ipsecif(4)
	check_tcp_com_over_ipsecif $SOCK_REMOTE $SOCK_LOCAL $BUS_NAT \
				   $ip_remote_ipsecif $ip_nat_remote $port $ip_remote 4500

	# Kill the NAT-T terminator
	stop_natt_terminators
}

add_test_ipsecif_natt_transport()
{
	local algo=$1
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	desc="Test ipsecif(4) NAT-T ($algo)"
	name="ipsecif_natt_transport_${_algo}"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server setkey nc
	    }
	    ${name}_body() {
	        test_ipsecif_natt_transport $algo
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
		add_test_ipsecif_natt_transport $algo
	done
}
