#	$NetBSD: t_gif_unnumbered.sh,v 1.1 2022/11/25 08:43:16 knakahara Exp $
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

SOCK_LOCAL=unix://gif_local
SOCK_REMOTE=unix://gif_remote
BUS_LOCAL_I=./bus_gif_local_inner
BUS_REMOTE_I=./bus_gif_remote_inner
BUS_GLOBAL=./bus_gif_global

DEBUG=${DEBUG:-false}
TIMEOUT=5

setup_servers_ipv4()
{

	rump_server_start $SOCK_LOCAL gif
	rump_server_start $SOCK_REMOTE gif
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS_GLOBAL
	rump_server_add_iface $SOCK_LOCAL shmif1 $BUS_LOCAL_I
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS_GLOBAL
	rump_server_add_iface $SOCK_REMOTE shmif1 $BUS_REMOTE_I
}

setup_servers_ipv6()
{

	rump_server_start $SOCK_LOCAL netinet6 gif
	rump_server_start $SOCK_REMOTE netinet6 gif
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

test_gif_unnumbered_ipv4()
{
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
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local_o/24
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_local_i/24

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_remote_o/24
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_remote_i/24

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w $TIMEOUT $ip_remote_o

	# setup gif(4) as unnumbered for local
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ifconfig gif0 create
	atf_check -s exit:0 -o ignore \
		  rump.ifconfig gif0 tunnel $ip_local_o $ip_remote_o
	atf_check -s exit:0 -o ignore rump.ifconfig gif0 unnumbered
	atf_check -s exit:0 -o ignore rump.ifconfig gif0 $ip_local_i/32
	atf_check -s exit:0 -o ignore \
		  rump.route add -inet $ip_remote_i_subnet -ifp gif0 $ip_local_i
	$DEBUG && rump.ifconfig -v gif0
	$DEBUG && rump.route -nL show

	# setup gif(4) as unnumbered for remote
	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 -o ignore rump.ifconfig gif0 create
	atf_check -s exit:0 -o ignore \
		  rump.ifconfig gif0 tunnel $ip_remote_o $ip_local_o
	atf_check -s exit:0 -o ignore rump.ifconfig gif0 unnumbered
	atf_check -s exit:0 -o ignore rump.ifconfig gif0 $ip_remote_i/32
	atf_check -s exit:0 -o ignore \
		  rump.route add -inet $ip_local_i_subnet -ifp gif0 $ip_remote_i
	$DEBUG && rump.ifconfig -v gif0
	$DEBUG && rump.route -nL show

	# test unnumbered gif(4)
	extract_new_packets $BUS_GLOBAL > $outfile
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore \
		  rump.ping -c 1 -n -w $TIMEOUT -I $ip_local_i $ip_remote_i
	extract_new_packets $BUS_GLOBAL > $outfile
	$DEBUG && cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_local_o > $ip_remote_o: $ip_local_i > $ip_remote_i: ICMP echo request" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote_o > $ip_local_o: $ip_remote_i > $ip_local_i: ICMP echo reply" \
	    cat $outfile
}

test_gif_unnumbered_ipv6()
{
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
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_local_o/64
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_local_i/24

	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_remote_o/64
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_remote_i/24

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X $TIMEOUT $ip_remote_o

	# setup gif(4) as unnumbered for local
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ifconfig gif0 create
	atf_check -s exit:0 -o ignore \
		  rump.ifconfig gif0 tunnel $ip_local_o $ip_remote_o
	atf_check -s exit:0 -o ignore rump.ifconfig gif0 unnumbered
	atf_check -s exit:0 -o ignore rump.ifconfig gif0 $ip_local_i/32
	atf_check -s exit:0 -o ignore \
		  rump.route add -inet $ip_remote_i_subnet -ifp gif0 $ip_local_i
	$DEBUG && rump.ifconfig -v gif0
	$DEBUG && rump.route -nL show

	# setup gif(4) as unnumbered for remote
	export RUMP_SERVER=$SOCK_REMOTE
	atf_check -s exit:0 -o ignore rump.ifconfig gif0 create
	atf_check -s exit:0 -o ignore \
		  rump.ifconfig gif0 tunnel $ip_remote_o $ip_local_o
	atf_check -s exit:0 -o ignore rump.ifconfig gif0 unnumbered
	atf_check -s exit:0 -o ignore rump.ifconfig gif0 $ip_remote_i/32
	atf_check -s exit:0 -o ignore \
		  rump.route add -inet $ip_local_i_subnet -ifp gif0 $ip_remote_i
	$DEBUG && rump.ifconfig -v gif0
	$DEBUG && rump.route -nL show

	# test unnumbered gif(4)
	extract_new_packets $BUS_GLOBAL > $outfile
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore \
		  rump.ping -c 1 -n -w $TIMEOUT -I $ip_local_i $ip_remote_i
	extract_new_packets $BUS_GLOBAL > $outfile
	$DEBUG && cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_local_o > $ip_remote_o: $ip_local_i > $ip_remote_i: ICMP echo request" \
	    cat $outfile
	atf_check -s exit:0 \
	    -o match:"$ip_remote_o > $ip_local_o: $ip_remote_i > $ip_local_i: ICMP echo reply" \
	    cat $outfile
}

add_test_gif_unnumbered()
{
	outer=$1

	name="gif_unnumbered_over${outer}"
	desc="Does unnumbered gif over ${outer}"

	atf_test_case ${name} cleanup
	eval "
	     ${name}_head() {
		atf_set descr \"${desc}\"
		atf_set require.progs rump_server
	    }
	    ${name}_body() {
		test_gif_unnumbered_${outer}
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

	add_test_gif_unnumbered ipv4
    	add_test_gif_unnumbered ipv6
}
