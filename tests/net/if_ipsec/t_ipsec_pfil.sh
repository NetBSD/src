#	$NetBSD: t_ipsec_pfil.sh,v 1.3 2020/08/05 01:10:50 knakahara Exp $
#
# Copyright (c) 2019 Internet Initiative Japan Inc.
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

SOCK_ROUTER1=unix://router1
SOCK_ROUTER2=unix://router2
ROUTER1_LANIP=192.168.1.1
ROUTER1_LANNET=192.168.1.0/24
ROUTER1_WANIP=10.0.0.1
ROUTER1_IPSECIP=172.16.1.1
ROUTER2_LANIP=192.168.2.1
ROUTER2_LANNET=192.168.2.0/24
ROUTER2_WANIP=10.0.0.2
ROUTER2_IPSECIP=172.16.2.1

DEBUG=${DEBUG:-false}
TIMEOUT=7
HIJACKING_NPF="${HIJACKING},blanket=/dev/npf"

setup_router()
{
	local sock=$1
	local lan=$2
	local wan=$3

	rump_server_add_iface $sock shmif0 bus0
	rump_server_add_iface $sock shmif1 bus1

	export RUMP_SERVER=${sock}
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0

	atf_check -s exit:0 rump.ifconfig shmif0 inet ${lan} netmask 0xffffff00
	atf_check -s exit:0 rump.ifconfig shmif0 up
	# Ensure shmif0 is running
	atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w $TIMEOUT ${lan}
	$DEBUG && rump.ifconfig shmif0

	atf_check -s exit:0 rump.ifconfig shmif1 inet ${wan} netmask 0xff000000
	atf_check -s exit:0 rump.ifconfig shmif1 up
	# Ensure shmif1 is running
	atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w $TIMEOUT ${wan}
	$DEBUG && rump.ifconfig shmif1

	unset RUMP_SERVER
}

setup_if_ipsec()
{
	local addr=$1
	local remote=$2
	local src=$3
	local dst=$4
	local peernet=$5

	rump_server_add_iface $RUMP_SERVER ipsec0
	atf_check -s exit:0 rump.ifconfig ipsec0 tunnel $src $dst
	atf_check -s exit:0 rump.ifconfig ipsec0 inet ${addr}/32 $remote
	atf_check -s exit:0 -o ignore rump.route add -inet $peernet $addr

	$DEBUG && rump.ifconfig ipsec0
	$DEBUG && rump.route -nL show -inet
}

get_if_ipsec_unique()
{
	local src=$1
	local proto=$2
	local unique=""

	unique=`$HIJACKING setkey -DP | grep -A2 "^${src}.*(${proto})$" | grep unique | sed 's/.*unique#//'`

	echo $unique
}

setup_if_ipsec_sa()
{
	local src=$1
	local dst=$2
	local inid=$3
	local outid=$4
	local proto=$5
	local algo=$6

	local tmpfile=./tmp
	local inunique=""
	local outunique=""
	local algo_args="$(generate_algo_args $proto $algo)"

	inunique=`get_if_ipsec_unique $dst "ipv4"`
	atf_check -s exit:0 test "X$inunique" != "X"
	outunique=`get_if_ipsec_unique $src "ipv4"`
	atf_check -s exit:0 test "X$outunique" != "X"

	cat > $tmpfile <<-EOF
	add $dst $src $proto $inid -u $inunique -m transport $algo_args;
	add $src $dst $proto $outid -u $outunique -m transport $algo_args;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
	$DEBUG && $HIJACKING setkey -DP
}

setup_tunnel()
{
	local proto=$1
	local algo=$2

	local addr= remote= src= dst= peernet=

	export RUMP_SERVER=$SOCK_ROUTER1
	addr=$ROUTER1_IPSECIP
	remote=$ROUTER2_IPSECIP
	src=$ROUTER1_WANIP
	dst=$ROUTER2_WANIP
	peernet=$ROUTER2_LANNET
	setup_if_ipsec $addr $remote $src $dst $peernet
	setup_if_ipsec_sa $src $dst "10000" "10001" $proto $algo

	export RUMP_SERVER=$SOCK_ROUTER2
	addr=$ROUTER2_IPSECIP
	remote=$ROUTER1_IPSECIP
	src=$ROUTER2_WANIP
	dst=$ROUTER1_WANIP
	peernet=$ROUTER1_LANNET
	setup_if_ipsec $addr $remote $src $dst $peernet
	setup_if_ipsec_sa $src $dst "10001" "10000" $proto $algo

	# Ensure ipsecif(4) settings have completed.
	export RUMP_SERVER=$SOCK_ROUTER1
	atf_check -s exit:0 -o ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER1_LANIP \
			$ROUTER2_LANIP

	export RUMP_SERVER=$SOCK_ROUTER2
	atf_check -s exit:0 -o ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER2_LANIP \
			$ROUTER1_LANIP

	unset RUMP_SERVER
}

ipsecif_pfil_setup()
{
	local proto=$1
	local algo=$2

	rump_server_crypto_npf_start $SOCK_ROUTER1 netipsec ipsec
	rump_server_crypto_npf_start $SOCK_ROUTER2 netipsec ipsec

	setup_router $SOCK_ROUTER1 $ROUTER1_LANIP $ROUTER1_WANIP
	setup_router $SOCK_ROUTER2 $ROUTER2_LANIP $ROUTER2_WANIP

	setup_tunnel $proto $algo
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
	local subnet=$2
	local direction=$3

	local reverse=
	if [ "X${direction}" = "Xin" ] ; then
		reverse="out"
	else
		reverse="in"
	fi

	cat > $outfile <<-EOF
	set bpf.jit off
	\$if = inet4(ipsec0)
	\$subnet = { $subnet }

	procedure "log0" {
		log: npflog0
	}

	group default {
		block $direction on \$if proto tcp from \$subnet apply "log0"
		pass $reverse on \$if proto tcp from \$subnet
		pass in on \$if proto icmp from 0.0.0.0/0
		pass out on \$if proto icmp from 0.0.0.0/0
		pass final on shmif0 all
		pass final on shmif1 all
	}
	EOF
}

ipsecif_pfil_test()
{
	local outfile=./out
	local npffile=./npf.conf
	local file_send=./file.send
	local file_recv=./file.recv

	local subnet="172.16.0.0/16"

	# Try TCP communications just in case.
	start_nc_server $SOCK_ROUTER2 8888 $file_recv ipv4
	prepare_file $file_send
	export RUMP_SERVER=$SOCK_ROUTER1
	atf_check -s exit:0 $HIJACKING nc -w 3 $ROUTER2_IPSECIP 8888 < $file_send
	atf_check -s exit:0 diff -q $file_send $file_recv
	stop_nc_server

	# Setup npf to block *out* direction for ipsecif(4).
	build_npf_conf $npffile $subnet "out"
	$DEBUG && cat $npffile

	export RUMP_SERVER=$SOCK_ROUTER1
	atf_check -s exit:0 $HIJACKING_NPF npfctl reload $npffile
	atf_check -s exit:0 $HIJACKING_NPF npfctl start
	$DEBUG && ${HIJACKING},"blanket=/dev/npf" npfctl show

	# ping should still work
	export RUMP_SERVER=$SOCK_ROUTER1
	atf_check -s exit:0 -o ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER1_LANIP \
			$ROUTER2_LANIP

	export RUMP_SERVER=$SOCK_ROUTER2
	atf_check -s exit:0 -o ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER2_LANIP \
			$ROUTER1_LANIP

	# TCP communications should be blocked.
	start_nc_server $SOCK_ROUTER2 8888 $file_recv ipv4
	prepare_file $file_send
	export RUMP_SERVER=$SOCK_ROUTER1
	atf_check -s exit:1 -o ignore $HIJACKING nc -w 3 $ROUTER2_IPSECIP 8888 < $file_send
	stop_nc_server

	atf_check -s exit:0 $HIJACKING_NPF npfctl stop
	$DEBUG && ${HIJACKING},"blanket=/dev/npf" npfctl show

	# Setup npf to block *in* direction for ipsecif(4).
	build_npf_conf $npffile $subnet "in"
	$DEBUG && cat $npffile

	export RUMP_SERVER=$SOCK_ROUTER2
	atf_check -s exit:0 $HIJACKING_NPF npfctl reload $npffile
	atf_check -s exit:0 $HIJACKING_NPF npfctl start
	$DEBUG && ${HIJACKING},"blanket=/dev/npf" npfctl show

	# ping should still work.
	export RUMP_SERVER=$SOCK_ROUTER1
	atf_check -s exit:0 -o ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER1_LANIP \
			$ROUTER2_LANIP

	export RUMP_SERVER=$SOCK_ROUTER2
	atf_check -s exit:0 -o ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER2_LANIP \
			$ROUTER1_LANIP

	# TCP communications should be blocked.
	start_nc_server $SOCK_ROUTER2 8888 $file_recv ipv4
	prepare_file $file_send
	export RUMP_SERVER=$SOCK_ROUTER1
	atf_check -s exit:1 -o ignore $HIJACKING nc -w 3 $ROUTER2_IPSECIP 8888 < $file_send
	stop_nc_server

	atf_check -s exit:0 $HIJACKING_NPF npfctl stop
	$DEBUG && ${HIJACKING},"blanket=/dev/npf" npfctl show


	unset RUMP_SERVER
}

ipsecif_pfil_teardown()
{

	export RUMP_SERVER=$SOCK_ROUTER1
	atf_check -s exit:0 rump.ifconfig ipsec0 deletetunnel
	atf_check -s exit:0 rump.ifconfig ipsec0 destroy
	$HIJACKING setkey -F

	export RUMP_SERVER=$SOCK_ROUTER2
	atf_check -s exit:0 rump.ifconfig ipsec0 deletetunnel
	atf_check -s exit:0 rump.ifconfig ipsec0 destroy
	$HIJACKING setkey -F

	unset RUMP_SERVER
}

add_test()
{
	local proto=$1
	local algo=$2
	local _algo=$(echo $algo | sed 's/-//g')

	name="ipsecif_pfil_${proto}_${_algo}"
	desc="Does ipsecif filter tests"

	atf_test_case ${name} cleanup
	eval "${name}_head() {
			atf_set descr \"${desc}\"
			atf_set require.progs rump_server setkey
		}
	    ${name}_body() {
			ipsecif_pfil_setup ${proto} ${algo}
			ipsecif_pfil_test
			ipsecif_pfil_teardown
			rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
			\$DEBUG && dump
			cleanup
		}"
	atf_add_test_case ${name}
}

add_test_allalgo()
{
	local desc=$1

	for algo in $ESP_ENCRYPTION_ALGORITHMS_MINIMUM; do
		add_test esp $algo
	done

	# ah does not support yet
}

atf_init_test_cases()
{

	add_test_allalgo ipsecif_pfil
}
