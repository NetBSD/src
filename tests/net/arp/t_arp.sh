#	$NetBSD: t_arp.sh,v 1.1 2015/07/29 06:10:10 ozaki-r Exp $
#
# Copyright (c) 2015 The NetBSD Foundation, Inc.
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

inetserver="rump_server -lrumpnet -lrumpnet_net -lrumpnet_netinet -lrumpnet_shmif"
HIJACKING="env LD_PRELOAD=/usr/lib/librumphijack.so RUMPHIJACK=sysctl=yes"

SOCKSRC=unix://commsock1
SOCKDST=unix://commsock2
IP4SRC=10.0.1.1
IP4DST=10.0.1.2

DEBUG=false
TIMEOUT=1

atf_test_case cache_expiration_5s cleanup
atf_test_case cache_expiration_10s cleanup
atf_test_case command cleanup

cache_expiration_5s_head()
{
	atf_set "descr" "Tests for ARP cache expiration (5s)"
	atf_set "require.progs" "rump_server"
}

cache_expiration_10s_head()
{
	atf_set "descr" "Tests for ARP cache expiration (10s)"
	atf_set "require.progs" "rump_server"
}

command_head()
{
	atf_set "descr" "Tests for commands of arp(8)"
	atf_set "require.progs" "rump_server"
}

setup_dst_server()
{
	export RUMP_SERVER=$SOCKDST
	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr bus1
	atf_check -s exit:0 rump.ifconfig shmif0 inet $IP4DST/24
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	$DEBUG && rump.ifconfig shmif0
	$DEBUG && rump.arp -n -a
}

setup_src_server()
{
	local prune=$1
	local keep=$2

	export RUMP_SERVER=$SOCKSRC

	# Adjust ARP parameters
	atf_check -s exit:0 -o ignore rump.sysctl -w net.inet.arp.prune=$prune
	atf_check -s exit:0 -o ignore rump.sysctl -w net.inet.arp.keep=$keep
	# Don't refresh to test expiration easily
	atf_check -s exit:0 -o ignore rump.sysctl -w net.inet.arp.refresh=0

	# Setup an interface
	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr bus1
	atf_check -s exit:0 rump.ifconfig shmif0 inet $IP4SRC/24
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	# Sanity check
	$DEBUG && rump.ifconfig shmif0
	$DEBUG && rump.arp -n -a
	atf_check -s exit:0 -o ignore rump.arp -n $IP4SRC
	atf_check -s not-exit:0 -e ignore rump.arp -n $IP4DST
}

test_cache_expiration()
{
	local arp_prune=1
	local arp_keep=$1
	local bonus=2

	atf_check -s exit:0 ${inetserver} $SOCKSRC
	atf_check -s exit:0 ${inetserver} $SOCKDST

	setup_dst_server
	setup_src_server $arp_prune $arp_keep

	#
	# Check if a cache is expired expectedly
	#
	export RUMP_SERVER=$SOCKSRC
	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 $IP4DST

	$DEBUG && rump.arp -n -a
	atf_check -s exit:0 -o ignore rump.arp -n $IP4SRC
	# Should be cached
	atf_check -s exit:0 -o ignore rump.arp -n $IP4DST

	atf_check -s exit:0 sleep $(($arp_keep + $arp_prune + $bonus))

	$DEBUG && rump.arp -n -a
	atf_check -s exit:0 -o ignore rump.arp -n $IP4SRC
	# Should be expired
	#atf_check -s not-exit:0 -e ignore rump.arp -n $IP4DST
	atf_check -s exit:0 -o match:'incomplete' rump.arp -n $IP4DST
}

cache_expiration_5s_body()
{
	test_cache_expiration 5
}

cache_expiration_10s_body()
{
	test_cache_expiration 10
}

command_body()
{
	local arp_prune=1
	local arp_keep=5
	local bonus=2

	atf_check -s exit:0 ${inetserver} $SOCKSRC
	atf_check -s exit:0 ${inetserver} $SOCKDST

	setup_dst_server
	setup_src_server $arp_prune $arp_keep

	export RUMP_SERVER=$SOCKSRC

	# Add and delete a static entry
	$DEBUG && rump.arp -n -a
	atf_check -s exit:0 -o ignore rump.arp -s 10.0.1.10 b2:a0:20:00:00:10
	$DEBUG && rump.arp -n -a
	atf_check -s exit:0 -o match:'permanent' rump.arp -n 10.0.1.10
	atf_check -s exit:0 -o ignore rump.arp -d 10.0.1.10
	$DEBUG && rump.arp -n -a
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.10

	# Add multiple entries via a file
	cat - > ./list <<-EOF
	10.0.1.11 b2:a0:20:00:00:11
	10.0.1.12 b2:a0:20:00:00:12
	10.0.1.13 b2:a0:20:00:00:13
	10.0.1.14 b2:a0:20:00:00:14
	10.0.1.15 b2:a0:20:00:00:15
	EOF
	$DEBUG && rump.arp -n -a
	atf_check -s exit:0 -o ignore rump.arp -f ./list
	$DEBUG && rump.arp -n -a
	atf_check -s exit:0 -o match:'permanent' rump.arp -n 10.0.1.11
	atf_check -s exit:0 -o match:'permanent' rump.arp -n 10.0.1.12
	atf_check -s exit:0 -o match:'permanent' rump.arp -n 10.0.1.13
	atf_check -s exit:0 -o match:'permanent' rump.arp -n 10.0.1.14
	atf_check -s exit:0 -o match:'permanent' rump.arp -n 10.0.1.15

	# Flush all entries
	$DEBUG && rump.arp -n -a
	atf_check -s exit:0 -o ignore rump.arp -d -a
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.11
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.12
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.13
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.14
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.15
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.1
}

cleanup()
{
	env RUMP_SERVER=$SOCKSRC rump.halt
	env RUMP_SERVER=$SOCKDST rump.halt
}

dump()
{
	export RUMP_SERVER=$SOCKSRC
	rump.netstat -nr
	rump.arp -n -a
	$HIJACKING dmesg

	export RUMP_SERVER=$SOCKDST
	rump.netstat -nr
	rump.arp -n -a
	$HIJACKING dmesg

	shmif_dumpbus -p - bus1 2>/dev/null| tcpdump -n -e -r -
}

cache_expiration_5s_cleanup()
{
	$DEBUG && dump
	cleanup
}

cache_expiration_10s_cleanup()
{
	$DEBUG && dump
	cleanup
}

command_cleanup()
{
	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{
	atf_add_test_case cache_expiration_5s
	atf_add_test_case cache_expiration_10s
	atf_add_test_case command
}
