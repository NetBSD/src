#	$NetBSD: t_ipsec_unnumbered.sh,v 1.1.2.1 2023/10/02 12:58:50 martin Exp $
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
SOCK_REMOTE=unix://ipsec_remote
BUS_LOCAL_I=./bus_ipsec_local_inner
BUS_REMOTE_I=./bus_ipsec_remote_inner
BUS_GLOBAL=./bus_ipsec_global

DEBUG=${DEBUG:-false}
TIMEOUT=7

setup_servers_ipv4()
{

	rump_server_crypto_start $SOCK_LOCAL netipsec ipsec
	rump_server_crypto_start $SOCK_REMOTE netipsec ipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS_GLOBAL
	rump_server_add_iface $SOCK_LOCAL shmif1 $BUS_LOCAL_I
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS_GLOBAL
	rump_server_add_iface $SOCK_REMOTE shmif1 $BUS_REMOTE_I
}

setup_servers_ipv6()
{

	rump_server_crypto_start $SOCK_LOCAL netipsec netinet6 ipsec
	rump_server_crypto_start $SOCK_REMOTE netipsec netinet6 ipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS_GLOBAL
	rump_server_add_iface $SOCK_LOCAL shmif1 $BUS_LOCAL_I
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS_GLOBAL
	rump_server_add_iface $SOCK_REMOTE shmif1 $BUS_REMOTE_I
}

setup_servers()
{
	local proto=$1

	setup_servers_$proto
}

add_sa()
{
	local outer=$1
	local proto=$2
	local algo="$3"
	local src=$4
	local dst=$5
	local tmpfile=./tmp
	local spi=10000
	local algo_args="$(generate_algo_args esp $algo)"
	local uniq=8192 # 8192(reqid_base) + 2 * 0(unit id of "ipsec0")

	export RUMP_SERVER=$SOCK_LOCAL
	cat > $tmpfile <<-EOF
	add $src $dst $proto $((spi)) -u $uniq -m transport $algo_args;
	add $dst $src $proto $((spi + 1)) -u $uniq -m transport $algo_args;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
	# XXX it can be expired if $lifetime is very short
	#check_sa_entries $SOCK_LOCAL $ip_local $ip_remote

	export RUMP_SERVER=$SOCK_REMOTE
	cat > $tmpfile <<-EOF
	add $src $dst $proto $((spi)) -u $uniq -m transport $algo_args;
	add $dst $src $proto $((spi + 1)) -u $uniq -m transport $algo_args;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
}

test_ipsecif_unnumbered_ipv4()
{
	local algo=$1
	local ip_local_i=192.168.22.1
	local ip_local_i_subnet=192.168.22.0/24
	local ip_local_o=10.0.0.2
	local ip_remote_i=192.168.33.1
	local ip_remote_i_subnet=192.168.33.0/24
	local ip_remote_o=10.0.0.3
	local outfile=./out

	setup_servers ipv4

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.sysctl -q -w net.ipsecif.use_fixed_reqid=1
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local_o/24
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_local_i/24

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.sysctl -q -w net.ipsecif.use_fixed_reqid=1
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_remote_o/24
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_remote_i/24

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w $TIMEOUT $ip_remote_o

	# setup ipsecif(4) as unnumbered for local
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ifconfig ipsec0 create
	atf_check -s exit:0 -o ignore \
		  rump.ifconfig ipsec0 tunnel $ip_local_o $ip_remote_o
	atf_check -s exit:0 -o ignore rump.ifconfig ipsec0 unnumbered
	atf_check -s exit:0 -o ignore rump.ifconfig ipsec0 $ip_local_i/32
	atf_check -s exit:0 -o ignore \
		  rump.route add -inet $ip_remote_i_subnet -ifp ipsec0 $ip_local_i
	$DEBUG && rump.ifconfig -v ipsec0
	$DEBUG && $HIJACKING setkey -DP
	$DEBUG && rump.route -nL show

	# setup ipsecif(4) as unnumbered for remote
	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 -o ignore rump.ifconfig ipsec0 create
	atf_check -s exit:0 -o ignore \
		  rump.ifconfig ipsec0 tunnel $ip_remote_o $ip_local_o
	atf_check -s exit:0 -o ignore rump.ifconfig ipsec0 unnumbered
	atf_check -s exit:0 -o ignore rump.ifconfig ipsec0 $ip_remote_i/32
	atf_check -s exit:0 -o ignore \
		  rump.route add -inet $ip_local_i_subnet -ifp ipsec0 $ip_remote_i
	$DEBUG && rump.ifconfig -v ipsec0
	$DEBUG && $HIJACKING setkey -DP
	$DEBUG && rump.route -nL show

	add_sa ipv4 esp $algo $ip_local_o $ip_remote_o

	# test unnumbered ipsecif(4)
	extract_new_packets $BUS_GLOBAL > $outfile
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore \
		  rump.ping -c 1 -n -w $TIMEOUT -I $ip_local_i $ip_remote_i
	extract_new_packets $BUS_GLOBAL > $outfile
	$DEBUG && cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_local_o > $ip_remote_o: ESP" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote_o > $ip_local_o: ESP" \
	    cat $outfile
}

test_ipsecif_unnumbered_ipv6()
{
	local algo=$1
	local ip_local_i=192.168.22.1
	local ip_local_i_subnet=192.168.22.0/24
	local ip_local_o=fc00::2
	local ip_remote_i=192.168.33.1
	local ip_remote_i_subnet=192.168.33.0/24
	local ip_remote_o=fc00::3
	local outfile=./out

	setup_servers ipv6

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.sysctl -q -w net.ipsecif.use_fixed_reqid=1
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_local_o/64
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_local_i/24

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.sysctl -q -w net.ipsecif.use_fixed_reqid=1
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_remote_o/64
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_remote_i/24

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X $TIMEOUT $ip_remote_o

	# setup ipsecif(4) as unnumbered for local
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ifconfig ipsec0 create
	atf_check -s exit:0 -o ignore \
		  rump.ifconfig ipsec0 tunnel $ip_local_o $ip_remote_o
	atf_check -s exit:0 -o ignore rump.ifconfig ipsec0 unnumbered
	atf_check -s exit:0 -o ignore rump.ifconfig ipsec0 $ip_local_i/32
	atf_check -s exit:0 -o ignore \
		  rump.route add -inet $ip_remote_i_subnet -ifp ipsec0 $ip_local_i
	$DEBUG && rump.ifconfig -v ipsec0
	$DEBUG && rump.route -nL show

	# setup ipsecif(4) as unnumbered for remote
	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 -o ignore rump.ifconfig ipsec0 create
	atf_check -s exit:0 -o ignore \
		  rump.ifconfig ipsec0 tunnel $ip_remote_o $ip_local_o
	atf_check -s exit:0 -o ignore rump.ifconfig ipsec0 unnumbered
	atf_check -s exit:0 -o ignore rump.ifconfig ipsec0 $ip_remote_i/32
	atf_check -s exit:0 -o ignore \
		  rump.route add -inet $ip_local_i_subnet -ifp ipsec0 $ip_remote_i
	$DEBUG && rump.ifconfig -v ipsec0
	$DEBUG && rump.route -nL show

	add_sa ipv6 esp $algo $ip_local_o $ip_remote_o

	# test unnumbered gif(4)
	extract_new_packets $BUS_GLOBAL > $outfile
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore \
		  rump.ping -c 1 -n -w $TIMEOUT -I $ip_local_i $ip_remote_i
	extract_new_packets $BUS_GLOBAL > $outfile
	$DEBUG && cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_local_o > $ip_remote_o: ESP" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote_o > $ip_local_o: ESP" \
	    cat $outfile
}

add_test_ipsecif_unnumbered()
{
	local outer=$1
	local algo=$2
	local _algo=$(echo $algo | sed 's/-//g')
	local name=
	local desc=

	name="ipsecif_unnumbered_over${outer}_${_algo}"
	desc="Does unnumbered ipsecif over ${outer} $algo"

	atf_test_case ${name} cleanup
	eval "
	     ${name}_head() {
		atf_set descr \"${desc}\"
		atf_set require.progs rump_server setkey
	    }
	    ${name}_body() {
		test_ipsecif_unnumbered_${outer} $algo
		rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
			\$DEBUG && dump
			cleanup
	    }"
	atf_add_test_case ${name}
}

atf_init_test_cases()
{
	local algo=

	for algo in $ESP_ENCRYPTION_ALGORITHMS_MINIMUM; do
		add_test_ipsecif_unnumbered ipv4 $algo
		add_test_ipsecif_unnumbered ipv6 $algo
	done
}
