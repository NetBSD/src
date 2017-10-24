#	$NetBSD: t_rtcache.sh,v 1.1.2.2 2017/10/24 08:55:56 snj Exp $
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

SOCK_SRC=unix://rtcache_src
SOCK_FWD=unix://rtcache_fwd
SOCK_DST1=unix://rtcache_dst1
SOCK_DST2=unix://rtcache_dst2

BUS_SRC=./src
BUS_DST1=./dst1
BUS_DST2=./dst2

DEBUG=${DEBUG:-false}

atf_test_case rtcache_invalidation cleanup
rtcache_invalidation_head()
{

	atf_set "descr" "Tests for rtcache invalidation"
	atf_set "require.progs" "rump_server"
}

rtcache_invalidation_body()
{
	local ip_src=10.0.0.2
	local ip_gwsrc=10.0.0.1
	local ip_gwdst1=10.0.1.1
	local ip_gwdst2=10.0.2.1
	local ip_dst1=10.0.1.2
	local ip_dst2=10.0.2.2
	local ip_dst=10.0.3.1
	local subnet_src=10.0.0.0
	local subnet_dst=10.0.3.0

	rump_server_start $SOCK_SRC
	rump_server_start $SOCK_FWD
	rump_server_start $SOCK_DST1
	rump_server_start $SOCK_DST2

	rump_server_add_iface $SOCK_SRC shmif0 $BUS_SRC

	rump_server_add_iface $SOCK_FWD shmif0 $BUS_SRC
	rump_server_add_iface $SOCK_FWD shmif1 $BUS_DST1
	rump_server_add_iface $SOCK_FWD shmif2 $BUS_DST2

	rump_server_add_iface $SOCK_DST1 shmif0 $BUS_DST1
	rump_server_add_iface $SOCK_DST2 shmif0 $BUS_DST2

	export RUMP_SERVER=$SOCK_SRC
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_src/24
	atf_check -s exit:0 -o ignore rump.route add default $ip_gwsrc

	export RUMP_SERVER=$SOCK_FWD
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwarding=1
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_gwsrc/24
	atf_check -s exit:0 rump.ifconfig shmif1 $ip_gwdst1/24
	atf_check -s exit:0 rump.ifconfig shmif2 $ip_gwdst2/24

	export RUMP_SERVER=$SOCK_DST1
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_dst1/24
	atf_check -s exit:0 -o ignore rump.route add default $ip_gwdst1
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_dst/24 alias

	export RUMP_SERVER=$SOCK_DST2
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_dst2/24
	atf_check -s exit:0 -o ignore rump.route add default $ip_gwdst2
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_dst/24 alias

	export RUMP_SERVER=$SOCK_SRC
	atf_check -s exit:0 -o ignore rump.ping -n -w 3 -c 1 $ip_dst1
	atf_check -s exit:0 -o ignore rump.ping -n -w 3 -c 1 $ip_dst2
	# It fails because there is no route to $ip_dst
	atf_check -s not-exit:0 -o ignore rump.ping -n -w 3 -c 1 $ip_dst

	extract_new_packets $BUS_DST1 > ./outfile

	export RUMP_SERVER=$SOCK_FWD
	# Add the default route so that $ip_dst @ dst1 is reachable now
	atf_check -s exit:0 -o ignore rump.route add default $ip_dst1
	export RUMP_SERVER=$SOCK_DST1
	# ...but don't response ICMP requests to avoid rtcache pollution
	atf_check -s exit:0 -o ignore \
	    rump.route add -net $subnet_src/24 127.0.0.1 -blackhole

	export RUMP_SERVER=$SOCK_SRC
	# ping fails expectedly
	atf_check -s not-exit:0 -o ignore rump.ping -n -w 3 -c 1 $ip_dst

	# An ICMP request should come to dst1
	extract_new_packets $BUS_DST1 > ./outfile
	atf_check -s exit:0 -o match:"$ip_src > $ip_dst: ICMP echo request" \
	    cat ./outfile

	export RUMP_SERVER=$SOCK_FWD
	# Teach the subnet of $ip_dst is at dst2
	atf_check -s exit:0 -o ignore rump.route add -net $subnet_dst/24 $ip_dst2
	export RUMP_SERVER=$SOCK_DST2
	# ...but don't response ICMP requests to avoid rtcache pollution
	atf_check -s exit:0 -o ignore \
	    rump.route add -net $subnet_src/24 127.0.0.1 -blackhole

	export RUMP_SERVER=$SOCK_SRC
	# ping fails expectedly
	atf_check -s not-exit:0 -o ignore rump.ping -n -w 3 -c 1 $ip_dst

	# An ICMP request should come to dst2. If rtcaches aren't invalidated
	# correctly, the ICMP request should appear at dst1
	extract_new_packets $BUS_DST1 > ./outfile
	atf_check -s exit:0 -o not-match:"$ip_src > $ip_dst: ICMP echo request" \
	    cat ./outfile

	export RUMP_SERVER=$SOCK_FWD
	# Delete the route so the packets heading to $ip_dst should go dst1 again
	atf_check -s exit:0 -o ignore rump.route delete -net $subnet_dst/24 $ip_dst2

	export RUMP_SERVER=$SOCK_SRC
	# ping fails expectedly
	atf_check -s not-exit:0 -o ignore rump.ping -n -w 3 -c 1 $ip_dst

	# An ICMP request should come to dst1
	extract_new_packets $BUS_DST1 > ./outfile
	atf_check -s exit:0 -o match:"$ip_src > $ip_dst: ICMP echo request" \
	    cat ./outfile

	rump_server_destroy_ifaces
}

rtcache_invalidation_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case rtcache_invalidation
}
