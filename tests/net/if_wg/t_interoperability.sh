#	$NetBSD: t_interoperability.sh,v 1.1 2020/08/26 16:03:42 riastradh Exp $
#
# Copyright (c) 2018 Ryota Ozaki <ozaki.ryota@gmail.com>
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

BUS=bus
SOCK_LOCAL=unix://wg_local
SOCK_PEER=unix://wg_peer


atf_test_case wg_interoperability_basic cleanup
wg_interoperability_basic_head()
{

	atf_set "descr" "tests of interoperability with the WireGuard protocol"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

#
# Set ATF_NET_IF_WG_INTEROPERABILITY=yes to run the test.
# Also to run the test, the following setups are required on the host and a peer.
#
# [Host]
#   ifconfig bridge0 create
#   ifconfig tap0 create
#   brconfig bridge0 add tap0
#   brconfig bridge0 add <external-interface>
#   ifconfig tap0 up
#   ifconfig bridge0 up
#
# [Peer]
#   ip addr add 10.0.0.2/24 dev <external-interface>
#   ip link add wg0 type wireguard
#   ip addr add 10.0.1.2/24 dev wg0
#   privkey="EF9D8AOkmxjlkiRFqBnfJS+RJJHbUy02u+VkGlBr9Eo="
#   ip link set wg0 up
#   echo $privkey > /tmp/private-key
#   wg set wg0 listen-port 52428
#   wg set wg0 private-key /tmp/private-key
#   pubkey="2iWFzywbDvYu2gQW5Q7/z/g5/Cv4bDDd6L3OKXLOwxs="
#   wg set wg0 peer $pubkey endpoint 10.0.0.3:52428 allowed-ips 10.0.1.1/32
#
wg_interoperability_basic_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping -n -c 3 -w 3"
	local ping_fail="atf_check -s not-exit:0 -o ignore rump.ping -n -c 1 -w 3"
	local key_priv_local=
	local key_pub_local=
	local key_priv_peer=
	local key_pub_peer=
	local ip_local=10.0.0.3
	local ip_peer=10.0.0.2
	local ip_wg_local=10.0.1.1
	local ip_wg_peer=10.0.1.2
	local port=52428
	local outfile=./out

	if [ "$ATF_NET_IF_WG_INTEROPERABILITY" != yes ]; then
		atf_skip "set ATF_NET_IF_WG_INTEROPERABILITY=yes to run the test"
	fi

	export RUMP_SERVER=$SOCK_LOCAL
	rump_server_crypto_start $SOCK_LOCAL virtif wg netinet6
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig virt0 create
	atf_check -s exit:0 rump.ifconfig virt0 $ip_local/24
	atf_check -s exit:0 rump.ifconfig virt0 up

	$ping $ip_peer

	key_priv_local="aK3TbzUNDO4aeDRX54x8bOG+NaKuqXKt7Hwq0Uz69Wo="
	key_pub_local="2iWFzywbDvYu2gQW5Q7/z/g5/Cv4bDDd6L3OKXLOwxs="
	key_priv_peer="EF9D8AOkmxjlkiRFqBnfJS+RJJHbUy02u+VkGlBr9Eo="
	key_pub_peer="2ZM9RvDmMZS/Nuh8OaVaJrwFbO57/WJgeU+JoQ//nko="

	setup_wg_common wg0 inet $ip_wg_local 24 $port "$key_priv_local"
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/32

	$ping $ip_wg_peer

	export RUMP_SERVER=$SOCK_LOCAL
	$ifconfig wg0 destroy
}

wg_interoperability_basic_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case wg_interoperability_cookie cleanup
wg_interoperability_cookie_head()
{

	atf_set "descr" "tests of interoperability with the WireGuard protocol"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_interoperability_cookie_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping -n -c 3 -w 3"
	local ping_fail="atf_check -s not-exit:0 -o ignore rump.ping -n -c 1 -w 3"
	local key_priv_local=
	local key_pub_local=
	local key_priv_peer=
	local key_pub_peer=
	local ip_local=10.0.0.3
	local ip_peer=10.0.0.2
	local ip_wg_local=10.0.1.1
	local ip_wg_peer=10.0.1.2
	local port=52428
	local outfile=./out
	local rekey_timeout=5 # default

	if [ "$ATF_NET_IF_WG_INTEROPERABILITY" != yes ]; then
		atf_skip "set ATF_NET_IF_WG_INTEROPERABILITY=yes to run the test"
	fi

	export RUMP_SERVER=$SOCK_LOCAL
	rump_server_crypto_start $SOCK_LOCAL virtif wg netinet6
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig virt0 create
	atf_check -s exit:0 rump.ifconfig virt0 $ip_local/24
	atf_check -s exit:0 rump.ifconfig virt0 up

	$ping $ip_peer

	key_priv_local="aK3TbzUNDO4aeDRX54x8bOG+NaKuqXKt7Hwq0Uz69Wo="
	key_pub_local="2iWFzywbDvYu2gQW5Q7/z/g5/Cv4bDDd6L3OKXLOwxs="
	key_priv_peer="EF9D8AOkmxjlkiRFqBnfJS+RJJHbUy02u+VkGlBr9Eo="
	key_pub_peer="2ZM9RvDmMZS/Nuh8OaVaJrwFbO57/WJgeU+JoQ//nko="

	setup_wg_common wg0 inet $ip_wg_local 24 $port "$key_priv_local"

	# Emulate load to send back a cookie on receiving a response message
	atf_check -s exit:0 -o ignore \
	    rump.sysctl -w net.wg.force_underload=1

	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/32

	# ping fails because we don't accept a response message and send a cookie
	$ping_fail $ip_wg_peer

	# Wait for retrying an initialization that works because the peer
	# send a response message with the cookie we sent
	atf_check -s exit:0 sleep $rekey_timeout

	# So ping works
	$ping $ip_wg_peer

	export RUMP_SERVER=$SOCK_LOCAL
	$ifconfig wg0 destroy
}

wg_interoperability_cookie_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case wg_userspace_basic cleanup
wg_userspace_basic_head()
{

	atf_set "descr" "tests of userspace implementation of wg(4)"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

#
# Set ATF_NET_IF_WG_USERSPACE=yes to run the test.
# Also to run the test, the following setups are required on the host and a peer.
#
# [Host]
#   ifconfig bridge0 create
#   ifconfig tap0 create
#   brconfig bridge0 add tap0
#   brconfig bridge0 add <external-interface>
#   ifconfig tap0 up
#   ifconfig bridge0 up
#
# [Peer]
#   ip addr add 10.0.0.2/24 dev <external-interface>
#   ip link add wg0 type wireguard
#   ip addr add 10.0.4.2/24 dev wg0
#   privkey="EF9D8AOkmxjlkiRFqBnfJS+RJJHbUy02u+VkGlBr9Eo="
#   ip link set wg0 up
#   echo $privkey > /tmp/private-key
#   wg set wg0 listen-port 52428
#   wg set wg0 private-key /tmp/private-key
#   pubkey="6mQ4lUO3oq5O8FfGW52CFXNbmh5iFT1XMqPzpdrc0nE="
#   wg set wg0 peer $pubkey endpoint 10.0.0.3:52428 allowed-ips 10.0.4.1/32
#
wg_userspace_basic_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore ping -n -c 3 -w 3"
	local ping_fail="atf_check -s not-exit:0 -o ignore ping -n -c 1 -w 3"
	local key_priv_local=
	local key_pub_local=
	local key_priv_peer=
	local key_pub_peer=
	local ip_local=10.0.0.3
	local ip_peer=10.0.0.2
	local ip_wg_local=10.0.4.1
	local ip_wg_peer=10.0.4.2
	local port_local=52429
	local port_peer=52428
	local outfile=./out

	if [ "$ATF_NET_IF_WG_USERSPACE" != yes ]; then
		atf_skip "set ATF_NET_IF_WG_USERSPACE=yes to run the test"
	fi

	export RUMP_SERVER=$SOCK_LOCAL
	rump_server_crypto_start $SOCK_LOCAL virtif wg netinet6
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0

	$DEBUG && netstat -nr -f inet

	$ping $ip_peer

	key_priv_local="6B0dualfIAiEG7/jFGOIHrJMhuypq87xCER/0ieIpE4="
	key_pub_local="6mQ4lUO3oq5O8FfGW52CFXNbmh5iFT1XMqPzpdrc0nE="
	key_priv_peer="EF9D8AOkmxjlkiRFqBnfJS+RJJHbUy02u+VkGlBr9Eo="
	key_pub_peer="2ZM9RvDmMZS/Nuh8OaVaJrwFbO57/WJgeU+JoQ//nko="

	setup_wg_common wg0 inet $ip_wg_local 24 $port_local "$key_priv_local" tun0
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port_peer $ip_wg_peer/32

	$DEBUG && rump.ifconfig wg0
	$DEBUG && ifconfig tun0
	$DEBUG && netstat -nr -f inet

	$ping $ip_wg_peer

	export RUMP_SERVER=$SOCK_LOCAL
	$ifconfig wg0 destroy
}

wg_userspace_basic_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case wg_interoperability_basic
	atf_add_test_case wg_interoperability_cookie
	atf_add_test_case wg_userspace_basic
}
