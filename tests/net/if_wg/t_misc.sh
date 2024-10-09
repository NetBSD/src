#	$NetBSD: t_misc.sh,v 1.12.2.1 2024/10/09 11:15:39 martin Exp $
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


atf_test_case wg_rekey cleanup
wg_rekey_head()
{

	atf_set "descr" "tests of rekeying of wg(4)"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_rekey_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w 1"
	local ip_local=192.168.1.1
	local ip_peer=192.168.1.2
	local ip_wg_local=10.0.0.1
	local ip_wg_peer=10.0.0.2
	local port=51820
	local rekey_after_time=3
	local latest_handshake=

	setup_servers

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore \
	    rump.sysctl -w net.wg.rekey_after_time=$rekey_after_time
	$DEBUG && atf_check -s exit:0 -o ignore \
	    rump.sysctl -w net.wg.debug=-1
	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 -o ignore \
	    rump.sysctl -w net.wg.rekey_after_time=$rekey_after_time
	$DEBUG && atf_check -s exit:0 -o ignore \
	    rump.sysctl -w net.wg.debug=-1

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 inet $ip_local 24
	setup_wg_common wg0 inet $ip_wg_local 24 $port "$key_priv_local"
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER
	setup_common shmif0 inet $ip_peer 24
	setup_wg_common wg0 inet $ip_wg_peer 24 $port "$key_priv_peer"
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_LOCAL

	echo ping1time=$(date)
	$ping $ip_wg_peer

	latest_handshake=$($HIJACKING wgconfig wg0 show peer peer0 \
	    | awk -F ': ' '/latest-handshake/ {print $2;}')
	$DEBUG && echo handshake1=$latest_handshake

	sleep 1

	echo ping2time=$(date)
	$ping $ip_wg_peer

	# No reinitiation is performed
	atf_check -s exit:0 -o match:"$latest_handshake" \
	    $HIJACKING wgconfig wg0 show peer peer0

	# Wait for a reinitiation to be performed
	sleep $rekey_after_time

	echo ping3time=$(date)
	$ping $ip_wg_peer

	# A reinitiation should be performed
	atf_check -s exit:0 -o not-match:"$latest_handshake" \
	    $HIJACKING wgconfig wg0 show peer peer0

	latest_handshake=$($HIJACKING wgconfig wg0 show peer peer0 \
	    | awk -F ': ' '/latest-handshake/ {print $2;}')
	$DEBUG && echo handshake2=$latest_handshake

	# Wait for a reinitiation to be performed again
	sleep $((rekey_after_time+1))

	echo ping4time=$(date)
	$ping $ip_wg_peer

	# A reinitiation should be performed
	atf_check -s exit:0 -o not-match:"$latest_handshake" \
	    $HIJACKING wgconfig wg0 show peer peer0

	destroy_wg_interfaces
}

wg_rekey_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case wg_handshake_timeout cleanup
wg_handshake_timeout_head()
{

	atf_set "descr" "tests of handshake timeout of wg(4)"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_handshake_timeout_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w 1"
	local ip_local=192.168.1.1
	local ip_peer=192.168.1.2
	local ip_wg_local=10.0.0.1
	local ip_wg_peer=10.0.0.2
	local port=51820
	local outfile=./out
	local rekey_timeout=4
	local rekey_attempt_time=10
	local n=

	setup_servers

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore \
	    rump.sysctl -w net.wg.rekey_timeout=$rekey_timeout
	atf_check -s exit:0 -o ignore \
	    rump.sysctl -w net.wg.rekey_attempt_time=$rekey_attempt_time
	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 -o ignore \
	    rump.sysctl -w net.wg.rekey_timeout=$rekey_timeout
	atf_check -s exit:0 -o ignore \
	    rump.sysctl -w net.wg.rekey_attempt_time=$rekey_attempt_time

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 inet $ip_local 24
	setup_wg_common wg0 inet $ip_wg_local 24 $port "$key_priv_local"
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER
	setup_common shmif0 inet $ip_peer 24
	setup_wg_common wg0 inet $ip_wg_peer 24 $port "$key_priv_peer"
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32
	$ifconfig -w 10

	# Resolve arp
	export RUMP_SERVER=$SOCK_LOCAL
	$ping $ip_peer

	export RUMP_SERVER=$SOCK_PEER
	$ifconfig shmif0 down
	export RUMP_SERVER=$SOCK_LOCAL

	extract_new_packets $BUS > $outfile

	# Should fail
	atf_check -s not-exit:0 -o match:'100.0% packet loss' \
	    rump.ping -n -c 1 -w 1 $ip_wg_peer

	sleep $((rekey_attempt_time + rekey_timeout))

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	n=$(grep "$ip_local.$port > $ip_peer.$port" $outfile |wc -l)

	# Give up handshaking after three attempts
	atf_check_equal $n 3

	export RUMP_SERVER=$SOCK_PEER
	$ifconfig shmif0 up
	export RUMP_SERVER=$SOCK_LOCAL

	destroy_wg_interfaces
}

wg_handshake_timeout_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case wg_cookie cleanup
wg_cookie_head()
{

	atf_set "descr" "tests of cookie messages of the wg(4) protocol"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_cookie_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping -n -i 0.1 -c 3 -w 1"
	local ping_fail="atf_check -s not-exit:0 -o ignore rump.ping -n -c 1 -w 1"
	local ip_local=192.168.1.1
	local ip_peer=192.168.1.2
	local ip_wg_local=10.0.0.1
	local ip_wg_peer=10.0.0.2
	local port=51820
	local outfile=./out
	local rekey_timeout=5

	setup_servers

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 inet $ip_local 24
	setup_wg_common wg0 inet $ip_wg_local 24 $port "$key_priv_local"
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER
	setup_common shmif0 inet $ip_peer 24
	setup_wg_common wg0 inet $ip_wg_peer 24 $port "$key_priv_peer"
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER
	# Emulate load on the peer
	atf_check -s exit:0 -o ignore \
	    rump.sysctl -w net.wg.force_underload=1

	export RUMP_SERVER=$SOCK_LOCAL

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	# The peer doesn't return a response message but a cookie message
	# and a session doesn't start
	$ping_fail $ip_wg_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile
	# XXX length 64 indicates the message is a cookie message
	atf_check -s exit:0 \
	    -o match:"$ip_peer.$port > $ip_local.$port: UDP, length 64" \
	    cat $outfile

	$DEBUG && $HIJACKING wgconfig wg0 show all
	atf_check -s exit:0 -o match:"latest-handshake: \(never\)" \
	    $HIJACKING wgconfig wg0

	# Wait for restarting a session
	sleep $rekey_timeout

	# The second attempt should be success because the init message has
	# a valid cookie.
	$ping $ip_wg_peer

	$DEBUG && $HIJACKING wgconfig wg0 show all
	atf_check -s exit:0 -o not-match:"latest-handshake: \(never\)" \
	    $HIJACKING wgconfig wg0

	destroy_wg_interfaces
}

wg_cookie_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case wg_mobility cleanup
wg_mobility_head()
{

	atf_set "descr" "tests of the mobility of wg(4)"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_mobility_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping -n -i 0.1 -c 3 -w 1"
	local ping_fail="atf_check -s not-exit:0 -o ignore rump.ping -n -c 1 -w 1"
	local ip_local=192.168.1.1
	local ip_peer=192.168.1.2
	local ip_peer_new=192.168.1.3
	local ip_wg_local=10.0.0.1
	local ip_wg_peer=10.0.0.2
	local port=51820
	local outfile=./out

	setup_servers

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 inet $ip_local 24
	setup_wg_common wg0 inet $ip_wg_local 24 $port "$key_priv_local"
	# Initially, the local doesn't know the endpoint of the peer
	add_peer wg0 peer0 $key_pub_peer "" $ip_wg_peer/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER
	setup_common shmif0 inet $ip_peer 24
	setup_wg_common wg0 inet $ip_wg_peer 24 $port "$key_priv_peer"
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32
	$ifconfig -w 10

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	# Ping from the local to the peer doesn't work because the local
	# doesn't know the endpoint of the peer
	export RUMP_SERVER=$SOCK_LOCAL
	$ping_fail $ip_wg_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	export RUMP_SERVER=$SOCK_PEER
	$ping $ip_wg_local

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	atf_check -s exit:0 -o match:"$ip_local.$port > $ip_peer.$port" cat $outfile

	# Change the IP address of the peer
	setup_common shmif0 inet $ip_peer_new 24
	$ifconfig -w 10

	# Ping from the local to the peer doesn't work because the local
	# doesn't know the change of the IP address of the peer
	export RUMP_SERVER=$SOCK_LOCAL
	$ping_fail $ip_wg_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	atf_check -s exit:0 -o match:"$ip_local.$port > $ip_peer.$port" cat $outfile

	# Ping from the peer to the local works because the local notices
	# the change and updates the IP address of the peer
	export RUMP_SERVER=$SOCK_PEER
	$ping $ip_wg_local

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	atf_check -s exit:0 -o match:"$ip_local.$port > $ip_peer_new.$port" cat $outfile
	atf_check -s exit:0 -o match:"$ip_peer_new.$port > $ip_local.$port" cat $outfile
	atf_check -s exit:0 -o not-match:"$ip_local.$port > $ip_peer.$port" cat $outfile

	destroy_wg_interfaces
}

wg_mobility_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case wg_keepalive cleanup
wg_keepalive_head()
{

	atf_set "descr" "tests keepalive messages"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_keepalive_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping -n -i 0.1 -c 3 -w 1"
	local ping_fail="atf_check -s not-exit:0 -o ignore rump.ping -n -c 1 -w 1"
	local ip_local=192.168.1.1
	local ip_peer=192.168.1.2
	local ip_peer_new=192.168.1.3
	local ip_wg_local=10.0.0.1
	local ip_wg_peer=10.0.0.2
	local port=51820
	local outfile=./out
	local keepalive_timeout=3

	setup_servers

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 inet $ip_local 24
	setup_wg_common wg0 inet $ip_wg_local 24 $port "$key_priv_local"
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER
	setup_common shmif0 inet $ip_peer 24
	setup_wg_common wg0 inet $ip_wg_peer 24 $port "$key_priv_peer"
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32
	$ifconfig -w 10

	# Shorten keepalive_timeout of the peer
	atf_check -s exit:0 -o ignore \
	    rump.sysctl -w net.wg.keepalive_timeout=$keepalive_timeout

	export RUMP_SERVER=$SOCK_LOCAL

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	$ping $ip_wg_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	sleep $((keepalive_timeout + 1))

	$ping $ip_wg_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	# XXX length 32 indicates the message is a keepalive (empty) message
	atf_check -s exit:0 -o match:"$ip_peer.$port > $ip_local.$port: UDP, length 32" \
	    cat $outfile

	destroy_wg_interfaces
}

wg_keepalive_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case wg_psk cleanup
wg_psk_head()
{

	atf_set "descr" "tests preshared-key"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

test_psk_common()
{
}

wg_psk_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping -n -i 0.1 -c 3 -w 1"
	local ping_fail="atf_check -s not-exit:0 -o ignore rump.ping -n -c 1 -w 1"
	local ip_local=192.168.1.1
	local ip_peer=192.168.1.2
	local ip_peer_new=192.168.1.3
	local ip_wg_local=10.0.0.1
	local ip_wg_peer=10.0.0.2
	local port=51820
	local outfile=./out
	local pskfile=./psk
	local rekey_after_time=3

	setup_servers

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore \
	    rump.sysctl -w net.wg.rekey_after_time=$rekey_after_time
	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 -o ignore \
	    rump.sysctl -w net.wg.rekey_after_time=$rekey_after_time

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys
	key_psk=$(wg-keygen --psk)
	$DEBUG && echo $key_psk

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 inet $ip_local 24
	setup_wg_common wg0 inet $ip_wg_local 24 $port "$key_priv_local"

	export RUMP_SERVER=$SOCK_PEER
	setup_common shmif0 inet $ip_peer 24
	setup_wg_common wg0 inet $ip_wg_peer 24 $port "$key_priv_peer"

	echo "$key_psk" > $pskfile

	export RUMP_SERVER=$SOCK_LOCAL

	# The local always has the preshared key
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/32 \
	    $pskfile "$key_psk"
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER

	# First, try the peer without the preshared key
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_LOCAL

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	$ping_fail $ip_wg_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	# Next, try with the preshared key
	export RUMP_SERVER=$SOCK_PEER
	delete_peer wg0 peer0
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32 \
	    $pskfile "$key_psk"
	$ifconfig -w 10

	# Need a rekey
	atf_check -s exit:0 sleep $((rekey_after_time + 1))

	export RUMP_SERVER=$SOCK_LOCAL

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	$ping $ip_wg_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	# Then, try again without the preshared key just in case
	export RUMP_SERVER=$SOCK_PEER
	delete_peer wg0 peer0
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32
	$ifconfig -w 10

	# Need a rekey
	atf_check -s exit:0 sleep $((rekey_after_time + 1))

	export RUMP_SERVER=$SOCK_LOCAL
	$ping_fail $ip_wg_peer

	rm -f $pskfile

	destroy_wg_interfaces
}

wg_psk_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case wg_malformed cleanup
wg_malformed_head()
{

	atf_set "descr" "tests malformed packet headers"
	atf_set "require.progs" "nc" "rump_server" "wgconfig" "wg-keygen"
	atf_set "timeout" "100"
}

wg_malformed_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w 1"
	local ip_local=192.168.1.1
	local ip_peer=192.168.1.2
	local ip_wg_local=10.0.0.1
	local ip_wg_peer=10.0.0.2
	local port=51820
	setup_servers

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 inet $ip_local 24
	setup_wg_common wg0 inet $ip_wg_local 24 $port "$key_priv_local"
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER
	setup_common shmif0 inet $ip_peer 24
	setup_wg_common wg0 inet $ip_wg_peer 24 $port "$key_priv_peer"
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_LOCAL

	$ping $ip_wg_peer

	printf 'send malformed packets\n'

	$HIJACKING ping -c 1 -n $ip_peer

	printf 'x' | $HIJACKING nc -Nu -w 0 $ip_peer $port
	printf 'xy' | $HIJACKING nc -Nu -w 0 $ip_peer $port
	printf 'xyz' | $HIJACKING nc -Nu -w 0 $ip_peer $port
	printf 'xyzw' | $HIJACKING nc -Nu -w 0 $ip_peer $port
	printf '\x00\x00\x00\x00' | $HIJACKING nc -Nu -w 0 $ip_peer $port
	printf '\x00\x00\x00\x00z' | $HIJACKING nc -Nu -w 0 $ip_peer $port
	printf '\x01\x00\x00\x00' | $HIJACKING nc -Nu -w 0 $ip_peer $port
	printf '\x01\x00\x00\x00z' | $HIJACKING nc -Nu -w 0 $ip_peer $port
	printf '\x02\x00\x00\x00' | $HIJACKING nc -Nu -w 0 $ip_peer $port
	printf '\x02\x00\x00\x00z' | $HIJACKING nc -Nu -w 0 $ip_peer $port
	printf '\x03\x00\x00\x00' | $HIJACKING nc -Nu -w 0 $ip_peer $port
	printf '\x03\x00\x00\x00z' | $HIJACKING nc -Nu -w 0 $ip_peer $port
	printf '\x04\x00\x00\x00' | $HIJACKING nc -Nu -w 0 $ip_peer $port
	printf '\x04\x00\x00\x00z' | $HIJACKING nc -Nu -w 0 $ip_peer $port

	printf 'done sending malformed packets\n'

	$ping $ip_wg_peer
}

wg_malformed_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case wg_rekey
	atf_add_test_case wg_handshake_timeout
	atf_add_test_case wg_cookie
	atf_add_test_case wg_mobility
	atf_add_test_case wg_keepalive
	atf_add_test_case wg_psk
	atf_add_test_case wg_malformed
}
