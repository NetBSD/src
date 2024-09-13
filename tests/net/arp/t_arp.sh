#	$NetBSD: t_arp.sh,v 1.45.6.2 2024/09/13 14:17:26 martin Exp $
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

SOCKSRC=unix://commsock1
SOCKDST=unix://commsock2
IP4SRC=10.0.1.1
IP4SRC2=10.0.1.5
IP4NET=10.0.1.0
IP4DST=10.0.1.2
IP4DST_PROXYARP1=10.0.1.3
IP4DST_PROXYARP2=10.0.1.4
IP4DST_FAIL1=10.0.1.99
IP4DST_FAIL2=10.0.99.99

DEBUG=${DEBUG:-false}
TIMEOUT=1

setup_dst_server()
{

	rump_server_add_iface $SOCKDST shmif0 bus1
	export RUMP_SERVER=$SOCKDST
	atf_check -s exit:0 rump.ifconfig shmif0 inet $IP4DST/24
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	$DEBUG && rump.ifconfig shmif0
	$DEBUG && rump.arp -n -a
	$DEBUG && rump.netstat -nr -f inet
}

setup_src_server()
{
	local keep=${1:-0}

	export RUMP_SERVER=$SOCKSRC

	# Shorten the expire time of cache entries
	if [ $keep != 0 ]; then
		# Convert to ms
		keep=$(($keep * 1000))
		atf_check -s exit:0 -o ignore \
		    rump.sysctl -w net.inet.arp.nd_reachable=$keep
	fi

	# Setup an interface
	rump_server_add_iface $SOCKSRC shmif0 bus1
	atf_check -s exit:0 rump.ifconfig shmif0 inet $IP4SRC/24
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	# Sanity check
	$DEBUG && rump.ifconfig shmif0
	$DEBUG && rump.arp -n -a
	$DEBUG && rump.netstat -nr -f inet
	atf_check -s not-exit:0 -e match:'no entry' rump.arp -n $IP4SRC
	atf_check -s not-exit:0 -e match:'no entry' rump.arp -n $IP4DST
}

get_timeout()
{
	local addr="$1"
	local timeout=$(env RUMP_SERVER=$SOCKSRC rump.arp -n $addr |grep $addr|awk '{print $7;}')
	timeout=${timeout%s}
	echo $timeout
}

test_cache_expiration()
{
	local arp_keep=7

	rump_server_start $SOCKSRC
	rump_server_start $SOCKDST

	setup_dst_server
	setup_src_server $arp_keep

	# Make a permanent cache entry to avoid sending an NS packet disturbing
	# the test
	macaddr=$(get_macaddr $SOCKSRC shmif0)
	export RUMP_SERVER=$SOCKDST
	atf_check -s exit:0 -o ignore rump.arp -s $IP4SRC $macaddr

	export RUMP_SERVER=$SOCKSRC

	#
	# Check if a cache is expired expectedly
	#
	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 $IP4DST

	$DEBUG && rump.arp -n -a
	atf_check -s not-exit:0 -o ignore -e match:'no entry' rump.arp -n $IP4SRC
	# Should be cached
	atf_check -s exit:0 -o not-match:'permanent' rump.arp -n $IP4DST

	timeout=$(get_timeout $IP4DST)

	atf_check -s exit:0 sleep $(($timeout + 1))

	$DEBUG && rump.arp -n -a
	atf_check -s not-exit:0 -o ignore -e match:'no entry' rump.arp -n $IP4SRC
	# Expired but remains until GC sweaps it (1 day)
	atf_check -s exit:0 -o match:"$ONEDAYISH" rump.arp -n $IP4DST

	rump_server_destroy_ifaces
}

check_arp_static_entry()
{
	local ip=$1
	local mac=$2
	local type=$3
	local flags=

	atf_check -s exit:0 -o match:"$mac" rump.arp -n $ip
	if [ $type = 'permanent' ]; then
		atf_check -s exit:0 -o match:'permanent' rump.arp -n $ip
		check_route $ip "$mac" UHLS shmif0
	else
		atf_check -s exit:0 -o not-match:'permanent' rump.arp -n $ip
		check_route $ip "$mac" UHL shmif0
	fi
}

test_command()
{
	local arp_keep=5
	local bonus=2

	rump_server_start $SOCKSRC
	rump_server_start $SOCKDST

	setup_dst_server
	setup_src_server $arp_keep

	export RUMP_SERVER=$SOCKSRC

	# Add and delete a static entry
	$DEBUG && rump.arp -n -a
	atf_check -s exit:0 -o ignore rump.arp -s 10.0.1.10 b2:a0:20:00:00:10
	$DEBUG && rump.arp -n -a
	$DEBUG && rump.netstat -nr -f inet
	check_arp_static_entry 10.0.1.10 'b2:a0:20:00:00:10' permanent
	atf_check -s exit:0 -o ignore rump.arp -d 10.0.1.10
	$DEBUG && rump.arp -n -a
	$DEBUG && rump.netstat -nr -f inet
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.10
	check_route_no_entry 10.0.1.10

	# Add multiple entries via a file
	cat - > ./list <<-EOF
	10.0.1.11 b2:a0:20:00:00:11
	10.0.1.12 b2:a0:20:00:00:12
	10.0.1.13 b2:a0:20:00:00:13
	10.0.1.14 b2:a0:20:00:00:14
	10.0.1.15 b2:a0:20:00:00:15
	EOF
	$DEBUG && rump.arp -n -a
	$DEBUG && rump.netstat -nr -f inet
	atf_check -s exit:0 -o ignore rump.arp -f ./list
	$DEBUG && rump.arp -n -a
	$DEBUG && rump.netstat -nr -f inet
	check_arp_static_entry 10.0.1.11 'b2:a0:20:00:00:11' permanent
	check_arp_static_entry 10.0.1.12 'b2:a0:20:00:00:12' permanent
	check_arp_static_entry 10.0.1.13 'b2:a0:20:00:00:13' permanent
	check_arp_static_entry 10.0.1.14 'b2:a0:20:00:00:14' permanent
	check_arp_static_entry 10.0.1.15 'b2:a0:20:00:00:15' permanent

	# Test arp -a
	atf_check -s exit:0 -o match:'10.0.1.11' rump.arp -n -a
	atf_check -s exit:0 -o match:'10.0.1.12' rump.arp -n -a
	atf_check -s exit:0 -o match:'10.0.1.13' rump.arp -n -a
	atf_check -s exit:0 -o match:'10.0.1.14' rump.arp -n -a
	atf_check -s exit:0 -o match:'10.0.1.15' rump.arp -n -a

	# Flush all entries
	$DEBUG && rump.arp -n -a
	$DEBUG && rump.netstat -nr -f inet
	atf_check -s exit:0 -o ignore rump.arp -d -a
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.11
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.12
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.13
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.14
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.15
	atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.1
	check_route_no_entry 10.0.1.11
	check_route_no_entry 10.0.1.12
	check_route_no_entry 10.0.1.13
	check_route_no_entry 10.0.1.14
	check_route_no_entry 10.0.1.15

	# Test temp option
	$DEBUG && rump.arp -n -a
	atf_check -s exit:0 -o ignore rump.arp -s 10.0.1.10 b2:a0:20:00:00:10 temp
	$DEBUG && rump.arp -n -a
	$DEBUG && rump.netstat -nr -f inet
	check_arp_static_entry 10.0.1.10 'b2:a0:20:00:00:10' temp

	# Hm? the cache doesn't expire...
	#atf_check -s exit:0 sleep $(($arp_keep + $bonus))
	#$DEBUG && rump.arp -n -a
	#$DEBUG && rump.netstat -nr -f inet
	#atf_check -s not-exit:0 -e ignore rump.arp -n 10.0.1.10

	rump_server_destroy_ifaces
}

make_pkt_str_arpreq()
{
	local target=$1
	local sender=$2
	pkt="> ff:ff:ff:ff:ff:ff, ethertype ARP \(0x0806\), length 42:"
	pkt="$pkt Request who-has $target tell $sender, length 28"
	echo $pkt
}

test_garp_common()
{
	local no_dad=$1
	local pkt=

	rump_server_start $SOCKSRC

	export RUMP_SERVER=$SOCKSRC

	if $no_dad; then
		atf_check -s exit:0 -o match:'3 -> 0' \
		    rump.sysctl -w net.inet.ip.dad_count=0
	fi

	# Setup an interface
	rump_server_add_iface $SOCKSRC shmif0 bus1
	atf_check -s exit:0 rump.ifconfig shmif0 inet 10.0.0.1/24
	atf_check -s exit:0 rump.ifconfig shmif0 up
	$DEBUG && rump.ifconfig shmif0

	atf_check -s exit:0 sleep 1
	extract_new_packets bus1 > ./out

	#
	# Assign an address to an interface without IFF_UP
	#
	# A GARP packet is sent for the primary address
	pkt=$(make_pkt_str_arpreq 10.0.0.1 10.0.0.1)
	atf_check -s exit:0 -o match:"$pkt" cat ./out

	atf_check -s exit:0 rump.ifconfig shmif0 down
	atf_check -s exit:0 rump.ifconfig shmif0 inet 10.0.0.2/24 alias

	atf_check -s exit:0 sleep 1
	extract_new_packets bus1 > ./out

	# A GARP packet is sent for the alias address
	pkt=$(make_pkt_str_arpreq 10.0.0.2 10.0.0.2)
	atf_check -s exit:0 -o match:"$pkt" cat ./out

	# Clean up
	atf_check -s exit:0 rump.ifconfig shmif0 inet 10.0.0.1/24 delete
	atf_check -s exit:0 rump.ifconfig shmif0 inet 10.0.0.2/24 delete

	#
	# Assign an address to an interface with IFF_UP
	#
	atf_check -s exit:0 rump.ifconfig shmif0 up

	# Primary address
	atf_check -s exit:0 rump.ifconfig shmif0 inet 10.0.0.3/24

	atf_check -s exit:0 sleep 1
	extract_new_packets bus1 > ./out

	pkt=$(make_pkt_str_arpreq 10.0.0.3 10.0.0.3)
	if $no_dad; then
		# A GARP packet is sent
		atf_check -s exit:0 -o match:"$pkt" cat ./out
	else
		# No GARP packet is sent
		atf_check -s exit:0 -o not-match:"$pkt" cat ./out
	fi

	# Alias address
	atf_check -s exit:0 rump.ifconfig shmif0 inet 10.0.0.4/24 alias

	atf_check -s exit:0 sleep 1
	extract_new_packets bus1 > ./out

	pkt=$(make_pkt_str_arpreq 10.0.0.4 10.0.0.4)
	if $no_dad; then
		# A GARP packet is sent
		atf_check -s exit:0 -o match:"$pkt" cat ./out
	else
		# No GARP packet is sent
		atf_check -s exit:0 -o not-match:"$pkt" cat ./out
	fi

	#
	# GARP on Link up
	#
	atf_check -s exit:0 rump.ifconfig shmif0 media none
	extract_new_packets bus1 > ./out
	atf_check -s exit:0 rump.ifconfig shmif0 media auto

	atf_check -s exit:0 sleep 1
	extract_new_packets bus1 > ./out

	if $no_dad; then
		# A GARP packet is sent for both primary and alias addresses.
		pkt=$(make_pkt_str_arpreq 10.0.0.3 10.0.0.3)
		atf_check -s exit:0 -o match:"$pkt" cat ./out
		pkt=$(make_pkt_str_arpreq 10.0.0.4 10.0.0.4)
		atf_check -s exit:0 -o match:"$pkt" cat ./out
	else
		# No GARP is sent.
		pkt=$(make_pkt_str_arpreq 10.0.0.3 10.0.0.3)
		atf_check -s exit:0 -o not-match:"$pkt" cat ./out
		pkt=$(make_pkt_str_arpreq 10.0.0.4 10.0.0.4)
		atf_check -s exit:0 -o not-match:"$pkt" cat ./out
		# DAD packets are sent instead.
		pkt=$(make_pkt_str_arpreq 10.0.0.3 0.0.0.0)
		atf_check -s exit:0 -o match:"$pkt" cat ./out
		pkt=$(make_pkt_str_arpreq 10.0.0.4 0.0.0.0)
		atf_check -s exit:0 -o match:"$pkt" cat ./out
	fi

	rump_server_destroy_ifaces
}

test_garp()
{

	test_garp_common false
}

test_garp_without_dad()
{

	test_garp_common true
}

test_cache_overwriting()
{

	rump_server_start $SOCKSRC
	rump_server_start $SOCKDST

	setup_dst_server
	setup_src_server

	export RUMP_SERVER=$SOCKSRC

	# Cannot overwrite a permanent cache
	atf_check -s exit:0 rump.arp -s $IP4DST b2:a0:20:00:00:ff
	$DEBUG && rump.arp -n -a
	atf_check -s not-exit:0 -e match:'File exists' \
	    rump.arp -s $IP4DST b2:a0:20:00:00:fe
	# cleanup
	atf_check -s exit:0 rump.arp -d $IP4DST

	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 $IP4DST
	$DEBUG && rump.arp -n -a
	# Can overwrite a dynamic cache
	atf_check -s exit:0 -o ignore rump.arp -s $IP4DST b2:a0:20:00:00:00
	$DEBUG && rump.arp -n -a
	atf_check -s exit:0 -o match:'b2:a0:20:00:00:00' rump.arp -n $IP4DST
	atf_check -s exit:0 -o match:'permanent' rump.arp -n $IP4DST

	atf_check -s exit:0 -o ignore rump.arp -s 10.0.1.10 b2:a0:20:00:00:10 temp
	$DEBUG && rump.arp -n -a
	atf_check -s exit:0 -o match:'b2:a0:20:00:00:10' rump.arp -n 10.0.1.10
	atf_check -s exit:0 -o not-match:'permanent' rump.arp -n 10.0.1.10
	# Can overwrite a temp cache
	atf_check -s exit:0 -o ignore rump.arp -s 10.0.1.10 b2:a0:20:00:00:ff
	atf_check -s exit:0 -o match:'b2:a0:20:00:00:ff' rump.arp -n 10.0.1.10
	$DEBUG && rump.arp -n -a

	rump_server_destroy_ifaces
}

make_pkt_str_arprep()
{
	local ip=$1
	local mac=$2
	pkt="ethertype ARP (0x0806), length 42: "
	pkt="Reply $ip is-at $mac, length 28"
	echo $pkt
}

make_pkt_str_garp()
{
	local ip=$1
	local mac=$2
	local pkt=
	pkt="$mac > ff:ff:ff:ff:ff:ff, ethertype ARP (0x0806),"
	pkt="$pkt length 42: Request who-has $ip tell $ip, length 28"
	echo $pkt
}

test_proxy_arp()
{
	local opts= title= flags=
	local type=$1

	rump_server_start $SOCKSRC
	rump_server_fs_start $SOCKDST tap

	setup_dst_server
	setup_src_server

	export RUMP_SERVER=$SOCKDST
	atf_check -s exit:0 -o ignore rump.sysctl -w net.inet.ip.forwarding=1
	macaddr_dst=$(get_macaddr $SOCKDST shmif0)

	if [ "$type" = "pub" ]; then
		opts="pub"
	else
		opts="pub proxy"
	fi
	# Always proxy only since migrating to lltable/llentry
	title='published \(proxy only\)'

	#
	# Test#1: First setup an endpoint then create proxy arp entry
	#
	export RUMP_SERVER=$SOCKDST
	rump_server_add_iface $SOCKDST tap1
	atf_check -s exit:0 rump.ifconfig tap1 $IP4DST_PROXYARP1/24 up
	atf_check -s exit:0 rump.ifconfig -w 10

	# Try to ping (should fail w/o proxy arp)
	export RUMP_SERVER=$SOCKSRC
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping -n -w 1 -c 1 $IP4DST_PROXYARP1
	# Remove ARP entry as it may hang around in WAITDELETE a few seconds
	atf_check -s ignore rump.arp -d $IP4DST_PROXYARP1

	# Flushing
	extract_new_packets bus1 > ./out

	# Set up proxy ARP entry
	export RUMP_SERVER=$SOCKDST
	atf_check -s exit:0 -o ignore \
	    rump.arp -s $IP4DST_PROXYARP1 $macaddr_dst $opts
	atf_check -s exit:0 -o match:"$title" rump.arp -n $IP4DST_PROXYARP1

	# Try to ping
	export RUMP_SERVER=$SOCKSRC
	atf_check -s exit:0 -o ignore rump.ping -n -w 1 -c 1 $IP4DST_PROXYARP1

	extract_new_packets bus1 > ./out
	$DEBUG && cat ./out

	pkt1=$(make_pkt_str_arprep $IP4DST_PROXYARP1 $macaddr_dst)
	pkt2=$(make_pkt_str_garp $IP4DST_PROXYARP1 $macaddr_dst)
	atf_check -s exit:0 -x "cat ./out |grep -q -e '$pkt1' -e '$pkt2'"

	#
	# Test#2: Create proxy arp entry then set up an endpoint
	#
	export RUMP_SERVER=$SOCKDST
	atf_check -s exit:0 -o ignore \
	    rump.arp -s $IP4DST_PROXYARP2 $macaddr_dst $opts
	atf_check -s exit:0 -o match:"$title" rump.arp -n $IP4DST_PROXYARP2
	$DEBUG && rump.netstat -nr -f inet

	# Try to ping (should fail because no endpoint exists)
	export RUMP_SERVER=$SOCKSRC
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping -n -w 1 -c 1 $IP4DST_PROXYARP2
	# Remove ARP entry as it may hang around in WAITDELETE a few seconds
	atf_check -s ignore rump.arp -d $IP4DST_PROXYARP2

	extract_new_packets bus1 > ./out
	$DEBUG && cat ./out

	# ARP reply should be sent
	pkt=$(make_pkt_str_arprep $IP4DST_PROXYARP2 $macaddr_dst)
	atf_check -s exit:0 -x "cat ./out |grep -q '$pkt'"

	export RUMP_SERVER=$SOCKDST
	rump_server_add_iface $SOCKDST tap2
	atf_check -s exit:0 rump.ifconfig tap2 $IP4DST_PROXYARP2/24 up
	atf_check -s exit:0 rump.ifconfig -w 10

	# Try to ping
	export RUMP_SERVER=$SOCKSRC
	atf_check -s exit:0 -o ignore rump.ping -n -w 1 -c 1 $IP4DST_PROXYARP2
}

test_proxy_arp_pub()
{

	test_proxy_arp pub
	rump_server_destroy_ifaces
}

test_proxy_arp_pubproxy()
{

	test_proxy_arp pubproxy
	rump_server_destroy_ifaces
}

test_link_activation()
{

	rump_server_start $SOCKSRC
	rump_server_start $SOCKDST

	setup_dst_server
	setup_src_server

	# flush old packets
	extract_new_packets bus1 > ./out

	export RUMP_SERVER=$SOCKSRC

	atf_check -s exit:0 -o ignore rump.ifconfig shmif0 link \
	    b2:a1:00:00:00:01

	atf_check -s exit:0 sleep 1
	extract_new_packets bus1 > ./out
	$DEBUG && cat ./out

	pkt=$(make_pkt_str_arpreq $IP4SRC $IP4SRC)
	atf_check -s exit:0 -o not-match:"$pkt" cat ./out

	atf_check -s exit:0 -o ignore rump.ifconfig shmif0 link \
	    b2:a1:00:00:00:02 active

	atf_check -s exit:0 sleep 1
	extract_new_packets bus1 > ./out
	$DEBUG && cat ./out

	pkt=$(make_pkt_str_arpreq $IP4SRC $IP4SRC)
	atf_check -s exit:0 -o match:"b2:a1:00:00:00:02 $pkt" cat ./out

	rump_server_destroy_ifaces
}

test_static()
{
	local macaddr_src=

	rump_server_start $SOCKSRC
	rump_server_start $SOCKDST

	setup_dst_server
	setup_src_server

	macaddr_src=$(get_macaddr $SOCKSRC shmif0)

	# Set a (valid) static ARP entry for the src server
	export RUMP_SERVER=$SOCKDST
	$DEBUG && rump.arp -n -a
	atf_check -s exit:0 -o ignore rump.arp -s $IP4SRC $macaddr_src
	$DEBUG && rump.arp -n -a

	# Test receiving an ARP request with the static ARP entry (as spa/sha)
	export RUMP_SERVER=$SOCKSRC
	atf_check -s exit:0 -o ignore rump.ping -n -w 1 -c 1 $IP4DST

	rump_server_destroy_ifaces
}

test_rtm()
{
	local macaddr_src= macaddr_dst=
	local file=./tmp
	local pid= hdr= what= addr=

	rump_server_start $SOCKSRC
	rump_server_start $SOCKDST

	setup_dst_server
	setup_src_server

	macaddr_src=$(get_macaddr $SOCKSRC shmif0)
	macaddr_dst=$(get_macaddr $SOCKDST shmif0)

	export RUMP_SERVER=$SOCKSRC

	# Test ping and a resulting routing message (RTM_ADD)
	rump.route -n monitor -c 1 > $file &
	pid=$!
	sleep 1
	atf_check -s exit:0 -o ignore rump.ping -n -w 1 -c 1 $IP4DST
	wait $pid
	$DEBUG && cat $file

	hdr="RTM_ADD.+<UP,HOST,DONE,LLINFO,CLONED>"
	what="<DST,GATEWAY>"
	addr="$IP4DST $macaddr_dst"
	atf_check -s exit:0 -o match:"$hdr" -o match:"$what" -o match:"$addr" \
		cat $file

	# Test ping and a resulting routing message (RTM_MISS) on subnet
	rump.route -n monitor -c 1 > $file &
	pid=$!
	sleep 1
	atf_check -s exit:2 -o ignore -e ignore \
		rump.ping -n -w 1 -c 1 $IP4DST_FAIL1
	wait $pid
	$DEBUG && cat $file

	hdr="RTM_MISS.+<DONE>"
	what="<DST,GATEWAY,AUTHOR>"
	addr="$IP4DST_FAIL1 link#2 $IP4SRC"
	atf_check -s exit:0 -o match:"$hdr" -o match:"$what" -o match:"$addr" \
		cat $file

	# Test ping and a resulting routing message (RTM_MISS) off subnet
	rump.route -n monitor -c 1 > $file &
	pid=$!
	sleep 1
	atf_check -s exit:2 -o ignore -e ignore \
		rump.ping -n -w 1 -c 1 $IP4DST_FAIL2
	wait $pid
	$DEBUG && cat $file

	hdr="RTM_MISS.+<DONE>"
	what="<DST>"
	addr="$IP4DST_FAIL2"
	atf_check -s exit:0 -o match:"$hdr" -o match:"$what" -o match:"$addr" \
		cat $file

	# Test arp -d and resulting routing messages (RTM_DELETE)
	rump.route -n monitor -c 1 > $file &
	pid=$!
	sleep 1
	atf_check -s exit:0 -o ignore rump.arp -d $IP4DST
	wait $pid
	$DEBUG && cat $file

	hdr="RTM_DELETE.+<HOST,DONE,LLINFO,CLONED>"
	what="<DST,GATEWAY>"
	addr="$IP4DST $macaddr_dst"
	atf_check -s exit:0 -o match:"$hdr" -o match:"$what" -o match:"$addr" \
		grep -A 3 RTM_DELETE $file

	rump_server_destroy_ifaces
}

test_purge_on_route_change()
{

	rump_server_start $SOCKSRC
	rump_server_start $SOCKDST

	setup_dst_server
	setup_src_server

	rump_server_add_iface $SOCKSRC shmif1 bus1
	export RUMP_SERVER=$SOCKSRC
	atf_check -s exit:0 rump.ifconfig shmif1 inet $IP4SRC2/24
	atf_check -s exit:0 rump.ifconfig -w 10

	$DEBUG && rump.netstat -nr -f inet
	atf_check -s exit:0 -o ignore rump.ping -n -w 1 -c 1 $IP4DST
	$DEBUG && rump.arp -na
	atf_check -s exit:0 -o ignore \
	    rump.route change -net $IP4NET -ifp shmif1
	$DEBUG && rump.netstat -nr -f inet
	$DEBUG && rump.arp -na
	# The entry was already removed on route change
	atf_check -s not-exit:0 -e match:'no entry' rump.arp -n $IP4DST

	rump_server_destroy_ifaces
}

test_purge_on_route_delete()
{

	rump_server_start $SOCKSRC
	rump_server_start $SOCKDST

	setup_dst_server
	setup_src_server

	$DEBUG && rump.netstat -nr -f inet
	atf_check -s exit:0 -o ignore rump.ping -n -w 1 -c 1 $IP4DST
	$DEBUG && rump.arp -na

	atf_check -s exit:0 -o ignore rump.route delete -net $IP4NET
	$DEBUG && rump.netstat -nr -f inet
	$DEBUG && rump.arp -na

	# The entry was already removed on route delete
	atf_check -s not-exit:0 -e match:'no entry' rump.arp -n $IP4DST

	rump_server_destroy_ifaces
}

test_purge_on_ifdown()
{

	rump_server_start $SOCKSRC
	rump_server_start $SOCKDST

	setup_dst_server
	setup_src_server

	$DEBUG && rump.netstat -nr -f inet
	atf_check -s exit:0 -o ignore rump.ping -n -w 1 -c 1 $IP4DST
	atf_check -s exit:0 -o match:'shmif0' rump.arp -n $IP4DST

	# Shutdown the interface
	atf_check -s exit:0 rump.ifconfig shmif0 down
	$DEBUG && rump.netstat -nr -f inet
	$DEBUG && rump.arp -na

	atf_check -s not-exit:0 -e match:'no entry' rump.arp -n $IP4DST

	rump_server_destroy_ifaces
}

test_stray_entries()
{

	rump_server_start $SOCKSRC
	rump_server_start $SOCKDST

	setup_dst_server
	setup_src_server

	rump_server_add_iface $SOCKSRC shmif1 bus1

	export RUMP_SERVER=$SOCKSRC
	atf_check -s exit:0 rump.ifconfig shmif1 inet $IP4SRC2/24
	atf_check -s exit:0 rump.ifconfig -w 10

	$DEBUG && rump.netstat -nr -f inet
	atf_check -s exit:0 -o ignore rump.ping -n -w 1 -c 1 $IP4DST
	$DEBUG && rump.arp -na
	atf_check -s exit:0 -o match:'shmif0' rump.arp -n $IP4DST
	atf_check -s exit:0 -o not-match:'shmif1' rump.arp -n $IP4DST

	# Clean up
	atf_check -s exit:0 -o ignore rump.arp -da
	atf_check -s not-exit:0 -e match:'no entry' rump.arp -n $IP4DST

	# ping from a different source address
	atf_check -s exit:0 -o ignore \
	    rump.ping -n -w 1 -c 1 -I $IP4SRC2 $IP4DST
	$DEBUG && rump.arp -na
	atf_check -s exit:0 -o match:'shmif0' rump.arp -n $IP4DST
	# ARP reply goes back via shmif1, so a cache is created on shmif1
	atf_check -s exit:0 -o match:'shmif1' rump.arp -n $IP4DST

	# Clean up by arp -da
	atf_check -s exit:0 -o ignore rump.arp -da
	atf_check -s not-exit:0 -e match:'no entry' rump.arp -n $IP4DST

	# ping from a different source address again
	atf_check -s exit:0 -o ignore \
	    rump.ping -n -w 1 -c 1 -I $IP4SRC2 $IP4DST
	atf_check -s exit:0 -o match:'shmif0' rump.arp -n $IP4DST
	# ARP reply doen't come
	atf_check -s exit:0 -o not-match:'shmif1' rump.arp -n $IP4DST

	# Cleanup caches on the destination
	export RUMP_SERVER=$SOCKDST
	atf_check -s exit:0 -o ignore rump.arp -da
	export RUMP_SERVER=$SOCKSRC

	# ping from a different source address again
	atf_check -s exit:0 -o ignore \
	    rump.ping -n -w 1 -c 1 -I $IP4SRC2 $IP4DST
	atf_check -s exit:0 -o match:'shmif0' rump.arp -n $IP4DST
	# ARP reply goes back via shmif1
	atf_check -s exit:0 -o match:'shmif1' rump.arp -n $IP4DST

	# Clean up by arp -d <ip>
	atf_check -s exit:0 -o ignore rump.arp -d $IP4DST
	# Both entries should be deleted
	atf_check -s not-exit:0 -e match:'no entry' rump.arp -n $IP4DST

	rump_server_destroy_ifaces
}

test_cache_creation_common()
{
	local no_dad=$1

	rump_server_start $SOCKSRC
	rump_server_start $SOCKDST

	if $no_dad; then
		export RUMP_SERVER=$SOCKSRC
		atf_check -s exit:0 -o match:'3 -> 0' \
		    rump.sysctl -w net.inet.ip.dad_count=0
		export RUMP_SERVER=$SOCKDST
		atf_check -s exit:0 -o match:'3 -> 0' \
		    rump.sysctl -w net.inet.ip.dad_count=0
	fi

	setup_dst_server
	setup_src_server

	macaddr_src=$(get_macaddr $SOCKSRC shmif0)
	macaddr_dst=$(get_macaddr $SOCKDST shmif0)

	# ARP cache entries are not created for DAD/GARP packets.
	export RUMP_SERVER=$SOCKSRC
	atf_check -s exit:0 -o empty rump.arp -n -a
	export RUMP_SERVER=$SOCKDST
	atf_check -s exit:0 -o empty rump.arp -n -a

	export RUMP_SERVER=$SOCKSRC

	extract_new_packets bus1 > ./out

	atf_check -s exit:0 -o ignore rump.ping -n -w $TIMEOUT -c 1 $IP4DST
	$DEBUG && rump.arp -n -a

	extract_new_packets bus1 > ./out

	atf_check -s exit:0 -o match:"\? \(10.0.1.2\) at $macaddr_dst on shmif0 [0-9]+s R" \
	    rump.arp -n -a

	export RUMP_SERVER=$SOCKDST

	# An entry was first created as stale then sending an ARP reply made it delay.
	atf_check -s exit:0 -o match:"\? \(10.0.1.1\) at $macaddr_src on shmif0 [0-9]+s D" \
	    rump.arp -n -a

	# The sender resolves the receiver's address.
	pkt=$(make_pkt_str_arpreq 10.0.1.2 10.0.1.1)
	atf_check -s exit:0 -o match:"$pkt" cat ./out

	# The receiver doesn't resolv the sender's address because the ARP request
	# from the sender has let make an entry already.
	pkt=$(make_pkt_str_arpreq 10.0.1.1 10.0.1.2)
	atf_check -s exit:0 -o not-match:"$pkt" cat ./out

	rump_server_destroy_ifaces
}

test_cache_creation()
{

	test_cache_creation_common false
}

test_cache_creation_nodad()
{

	test_cache_creation_common true
}

add_test()
{
	local name=$1
	local desc="$2"

	atf_test_case "arp_${name}" cleanup
	eval "arp_${name}_head() {
			atf_set descr \"${desc}\"
			atf_set require.progs rump_server
		}
	    arp_${name}_body() {
			test_${name}
		}
	    arp_${name}_cleanup() {
			\$DEBUG && dump
			cleanup
		}"
	atf_add_test_case "arp_${name}"
}

atf_init_test_cases()
{

	add_test cache_expiration      "Tests for ARP cache expiration"
	add_test command               "Tests for arp_commands of arp(8)"
	add_test garp                  "Tests for GARP"
	add_test garp_without_dad      "Tests for GARP with DAD disabled"
	add_test cache_overwriting     "Tests for behavior of overwriting ARP caches"
	add_test proxy_arp_pub         "Tests for Proxy ARP (pub)"
	add_test proxy_arp_pubproxy    "Tests for Proxy ARP (pub proxy)"
	add_test link_activation       "Tests for activating a new MAC address"
	add_test static                "Tests for static ARP entries"
	add_test rtm                   "Tests for routing messages on operations of ARP entries"
	add_test purge_on_route_change "Tests if ARP entries are removed on route change"
	add_test purge_on_route_delete "Tests if ARP entries are removed on route delete"
	add_test purge_on_ifdown       "Tests if ARP entries are removed on interface down"
	add_test stray_entries         "Tests if ARP entries are removed on route change"
	add_test cache_creation        "Tests for ARP cache creation"
	add_test cache_creation_nodad  "Tests for ARP cache creation without DAD"
}
