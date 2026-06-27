#	$NetBSD: t_basic.sh,v 1.4.6.2 2026/06/27 14:56:18 martin Exp $
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
SOCK_PEER2=unix://wg_peer2


check_ping_payload()
{
	local proto=$1
	local ip=$2
	local ping= size=

	if [ $proto = inet ]; then
		ping="atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w 1"
	else
		ping="atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 -X 1"
	fi

	for size in $(seq 1 100) $(seq 450 550) $(seq 1400 1500); do
		$ping -s $size $ip
	done
}

check_badudp()
{
	local proto=$1
	local ip=$2
	local port=51820        # XXX parametrize more clearly

	if [ $proto = inet ]; then
		atf_check -o ignore -e ignore \
		    $HIJACKING nc -4uv -w1 $ip $port </dev/null
	else
		atf_check -o ignore -e ignore \
		    $HIJACKING nc -6uv -w1 $ip $port </dev/null
	fi
}

check_badpeerkey()
{
	local proto=$1
	local ip=$2
	local ping= size=

	if [ $proto = inet ]; then
		ping="atf_check -s not-exit:0 -o ignore rump.ping -n -c 1 -w 1"
	else
		ping="atf_check -s not-exit:0 -o ignore rump.ping6 -n -c 1 -X 1"
	fi

	$ping $ip
}

check_badhandshakekey()
{
	local proto=$1
	local wg_ip=$2
	local ip=$3
	local pubkey=$4
	local port=51820        # XXX parametrize more clearly

	# For each invalid public key (representing the 32-byte
	# little-endian encoding of an x coordinate of a point of order
	# <=8 on Curve25519), we have a preassembled init message that
	# uses that public key as its ephemeral public key and
	# otherwise has all the hashes computed correctly -- generated
	# by tweaking if_wg.c to hard-code each possible invalid key,
	# and printing the resulting init message.
	#
	# (Fortunately, the set of points of order <=8 on Curve25519
	# does not change very often, so we won't have to generate this
	# list until we move on to some post-quantum key agreement that
	# has its own weird inputs!)
	#
	case $pubkey in
	"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=")
		openssl base64 -d <<EOF >initmsg
AQAAAFUFlIIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAK/jDw2scvj0v9G2UVt/+EyV
y/n+15Jq6+h2ttmPvmDSlNE9Ye6POFzitBVW30Q6jVJXmU8LmQS7c1heUmIFpA57sILsldvY9Zck
u+fbNoiZqTOtLfg/jEscBkKIAAAAAAAAAAAAAAAAAAAAAA==
EOF
		;;
	"AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=")
		openssl base64 -d <<EOF >initmsg
AQAAAFUFlIIBAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOSNjvF3Hl3dJtvCp1H6ZMS
mG4Aw86kG4/yIYjTvUsLjirhal8l7Ed/Q/Ne2naAQFz7YGufHsxUA/2JIE0mny+wq888qObeK7gz
S9e2dYZmvn15f06+aso5vV1yAAAAAAAAAAAAAAAAAAAAAA==
EOF
		;;
	"4Ot6fDtBuK4WVuP68Z/EatoJjeucMrH9hmIFFl9JuAA=")
		openssl base64 -d <<EOF >initmsg
AQAAAFUFlILg63p8O0G4rhZW4/rxn8Rq2gmN65wysf2GYgUWX0m4AONODwHdcJwRxKmw2oW4prDK
AKpg9WPA3PBgs+SYYi58hyueAzHa4Nl8wQ6qIV87jBz2nqwiqwRRzvSYCZDvB0W1obVEeJTjS71C
RT/VRq0J9xYL8UXM/69C2Pt8AAAAAAAAAAAAAAAAAAAAAA==
EOF
		;;
	"X5yVvKNQjCSx0LFVnIPvWwREXMRYHI6G2CJO3dCfEVc=")
		openssl base64 -d <<EOF >initmsg
AQAAAFUFlIJfnJW8o1CMJLHQsVWcg+9bBERcxFgcjobYIk7d0J8RV+Lelfe8koazxp5GiU6YDujl
Teck8Vcf+2Ja8YFDr5AJDmC+LSZwdO0E1U6pJO4//zuzKYZ8l/tqP5uEn+6fpIVTRUFdFh0DOMHk
78XiRuLMeo9uFYGXM7GFUQsRAAAAAAAAAAAAAAAAAAAAAA==
EOF
		;;
	"7P///////////////////////////////////////38=")
		openssl base64 -d <<EOF >initmsg
AQAAAFUFlILs////////////////////////////////////////f2V4uZtAkzNU3dDcnwOD+8Qu
KIe6aFkMRYFl8KdCYAtmuwd7WzzSrM/l4YeS7VbkJAV+F6/zFykwijFporJebtDfirc5JTA055Bd
CQ4HFV7de3UCXQI4jLyzz0TwAAAAAAAAAAAAAAAAAAAAAA==
EOF
		;;
	"7f///////////////////////////////////////38=")
		openssl base64 -d <<EOF >initmsg
AQAAAFUFlILt////////////////////////////////////////f1a5i10Q+gmID8QIAcxwe5xm
OcuWZosr2bjCCP/0kNeE++Gw6AynD+OAf8uW/rWSUXE59JmiDDrP6jc4mBk2ax3vy9x5+YXAuGK5
iBXvgYPqEdlc8Fxa4jtDEXDRAAAAAAAAAAAAAAAAAAAAAA==
EOF
		;;
	"7v///////////////////////////////////////38=")
		openssl base64 -d <<EOF >initmsg
AQAAAFUFlILu////////////////////////////////////////fziZHxCJC02xxcsZh2Fm2XaG
RgfOO/oDgEgMhfZF81n/kZgmLP2rUtIWYOpNvfvOlISlJp/8Mc0OBxnO4XVpl6Ux047I/WSDZaT/
2DyOI7NnCF4UwFeH8DoL+3IPAAAAAAAAAAAAAAAAAAAAAA==
EOF
		;;
	"zet6fDtBuK4WVuP68Z/EatoJjeucMrH9hmIFFl9JuIA=")
		openssl base64 -d <<EOF >initmsg
AQAAAFUFlILN63p8O0G4rhZW4/rxn8Rq2gmN65wysf2GYgUWX0m4gDabPNBQhXIf883p655csReY
y9xTkkkDOW8fwEfFtVeALp0D0yu3C1HyEUifwnd+o2Aa8YYaeT8vkcL1wTiS3Ap1iQO1J/OTmWek
SOOopCkEWxOxF2+D9PNF27nGAAAAAAAAAAAAAAAAAAAAAA==
EOF
		;;
	"TJyVvKNQjCSx0LFVnIPvWwREXMRYHI6G2CJO3dCfEdc=")
		openssl base64 -d <<EOF >initmsg
AQAAAFUFlIJMnJW8o1CMJLHQsVWcg+9bBERcxFgcjobYIk7d0J8R1w9SPoGi2YVq3znle0dn0/5R
W4opQdY+1jkSbHuTwhYBiqgaBIbeGrNz7d88dF2lt/vkMlYfH6TGBEIHw+iIwjT/eQcjTy7sqV5h
edWqvJ00Bi97u95JKm27ogsUAAAAAAAAAAAAAAAAAAAAAA==
EOF
		;;
	"2f////////////////////////////////////////8=")
		openssl base64 -d <<EOF >initmsg
AQAAAFUFlILZ/////////////////////////////////////////wAJbt3qhxKn6Cu3UyTrkt0I
rSEtWlwOf3J7aGUVpIDdK+L68oMmM+GoCe40JJdsmdFAnKq0kcicOlPiuB+Dg+OABRsLQy/WNsEh
9UNuoKUU8GzWVjslXfJKqpSjAAAAAAAAAAAAAAAAAAAAAA==
EOF
		;;
	"2v////////////////////////////////////////8=")
		openssl base64 -d <<EOF >initmsg
AQAAAFUFlILa/////////////////////////////////////////9VCEJnMrtIplbej+1z/eoLI
3/YfsJo81t1kaJk/iTmhHvMCUxW0jFOD3DLTF6bGe9ZxqNczcRRPeAIZJnT0107QhlAjtS/EtzO8
6EnJrm75o9KcN6J5dIox963MAAAAAAAAAAAAAAAAAAAAAA==
EOF
		;;
	"2/////////////////////////////////////////8=")
		openssl base64 -d <<EOF >initmsg
AQAAAFUFlILb//////////////////////////////////////////H9a+a+mgjUU5agdYuUlcIC
MM72yinuThd0eNVAJ5HzvxFZiVvycJuWGB8GsY5uAWWHmZydf19tH8OKWr4ZAn5cE0uJYJibUUlf
zzfeXl7dhyg51yP62ZIFMgw3AAAAAAAAAAAAAAAAAAAAAA==
EOF
		;;
	*)	atf_fail "They're the wrong trousers, and they've gone wrong!"
		;;
	esac

	if [ $proto = inet ]; then
		atf_check -o ignore -e ignore \
		    $HIJACKING nc -4u -w0 $ip $port <initmsg
		ping="atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w 1"
	else
		atf_check -o ignore -e ignore \
		    $HIJACKING nc -6u -w0 $ip $port <initmsg
		ping="atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 -X 1"
	fi

	$ping $wg_ip
}

test_common()
{
	local type=$1
	local outer_proto=$2
	local inner_proto=$3
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local port=51820
	local ip_local= ip_peer=
	local ip_wg_local= ip_wg_peer=
	local outer_prefix= outer_prefixall=
	local inner_prefix= inner_prefixall=
	local handshake_key=

	if [ $outer_proto = inet ]; then
		ip_local=192.168.1.1
		ip_peer=192.168.1.2
		outer_prefix=24
		outer_prefixall=32
	else
		ip_local=fc00::1
		ip_peer=fc00::2
		outer_prefix=64
		outer_prefixall=128
	fi

	if [ $inner_proto = inet ]; then
		ip_wg_local=10.0.0.1
		ip_wg_peer=10.0.0.2
		inner_prefix=24
		inner_prefixall=32
	else
		ip_wg_local=fd00::1
		ip_wg_peer=fd00::2
		inner_prefix=64
		inner_prefixall=128
	fi

	setup_servers

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	case $type in
	badhandshakekey)
		generate_fixed_test_keys
		handshake_key=$4
		;;
	*)	generate_keys
		;;
	esac

	case $type in
	badpeerkey)
		key_pub_peer=$4
		;;
	esac

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 $outer_proto $ip_local $outer_prefix
	setup_wg_common wg0 $inner_proto $ip_wg_local $inner_prefix $port "$key_priv_local"
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/$inner_prefixall
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER
	setup_common shmif0 $outer_proto $ip_peer $outer_prefix
	setup_wg_common wg0 $inner_proto $ip_wg_peer $inner_prefix $port "$key_priv_peer"
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/$inner_prefixall
	$ifconfig -w 10

	if [ $type = basic ]; then
		export RUMP_SERVER=$SOCK_LOCAL
		check_ping $inner_proto $ip_wg_peer
	elif [ $type = payload ]; then
		export RUMP_SERVER=$SOCK_LOCAL
		check_ping_payload $inner_proto $ip_wg_peer
	elif [ $type = badudp ]; then
		export RUMP_SERVER=$SOCK_LOCAL
		check_badudp $outer_proto $ip_peer
	elif [ $type = badpeerkey ]; then
		export RUMP_SERVER=$SOCK_LOCAL
		check_badpeerkey $outer_proto $ip_wg_peer
	elif [ $type = badhandshakekey ]; then
		export RUMP_SERVER=$SOCK_LOCAL
		check_badhandshakekey $outer_proto $ip_wg_peer $ip_peer \
		    $handshake_key
	fi

	destroy_wg_interfaces
}

atf_test_case wg_create_destroy cleanup
wg_create_destroy_head()
{

	atf_set "descr" "tests to create/destroy wg(4) interfaces"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_create_destroy_body()
{

	rump_server_crypto_start $SOCK_LOCAL netinet6 wg

	test_create_destroy_common $SOCK_LOCAL wg0 true
}

wg_create_destroy_cleanup()
{

	$DEBUG && dump
	cleanup
}

wg_create_destroy_peers_common()
{
	local proto=$1
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local port=51820
	local ip_local= ip_peer=
	local ip_wg_local= ip_wg_peer=
	local outer_prefix= outer_prefixall=
	local inner_prefix= inner_prefixall=

	if [ $proto = inet ]; then
		ip_local=192.168.1.1
		ip_peer=192.168.1.2
		outer_prefix=24
		outer_prefixall=32
		ip_wg_local=10.0.0.1
		ip_wg_peer=10.0.0.2
		inner_prefix=24
		inner_prefixall=32
	else
		ip_local=fc00::1
		ip_peer=fc00::2
		outer_prefix=64
		outer_prefixall=128
		ip_wg_local=fd00::1
		ip_wg_peer=fd00::2
		inner_prefix=64
		inner_prefixall=128
	fi

	rump_server_crypto_start $SOCK_LOCAL netinet6 wg
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 $proto $ip_local $outer_prefix
	setup_wg_common wg0 $proto $ip_wg_local $inner_prefix $port "$key_priv_local"

	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/$inner_prefixall

	delete_peer wg0 peer0
}

atf_test_case wg_create_destroy_peers_ipv4 cleanup
wg_create_destroy_peers_ipv4_head()
{

	atf_set "descr" "tests to create/destroy peers (IPv4)"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_create_destroy_peers_ipv4_body()
{

	wg_create_destroy_peers_common inet
}

wg_create_destroy_peers_ipv4_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case wg_create_destroy_peers_ipv6 cleanup
wg_create_destroy_peers_ipv6_head()
{

	atf_set "descr" "tests to create/destroy peers (IPv6)"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_create_destroy_peers_ipv6_body()
{

	wg_create_destroy_peers_common inet6
}

wg_create_destroy_peers_ipv6_cleanup()
{

	$DEBUG && dump
	cleanup
}

add_basic_test()
{
	local inner=$1
	local outer=$2
	local ipv4=inet
	local ipv6=inet6

	name="wg_basic_${inner}_over_${outer}"
	fulldesc="Test wg(4) with ${inner} over ${outer}"

	eval inner=\$$inner
	eval outer=\$$outer

	atf_test_case ${name} cleanup
	eval "
		${name}_head() {
			atf_set descr \"${fulldesc}\"
			atf_set require.progs rump_server wgconfig wg-keygen
		}
		${name}_body() {
			test_common basic $outer $inner
			rump_server_destroy_ifaces
		}
		${name}_cleanup() {
			\$DEBUG && dump
			cleanup
		}"
	atf_add_test_case ${name}
}

add_payload_sizes_test()
{
	local inner=$1
	local outer=$2
	local ipv4=inet
	local ipv6=inet6

	name="wg_payload_sizes_${inner}_over_${outer}"
	fulldesc="Test wg(4) with ${inner} over ${outer} with various payload sizes"

	eval inner=\$$inner
	eval outer=\$$outer

	atf_test_case ${name} cleanup
	eval "
		${name}_head() {
			atf_set descr \"${fulldesc}\"
			atf_set require.progs rump_server wgconfig wg-keygen
		}
		${name}_body() {
			test_common payload $outer $inner
			rump_server_destroy_ifaces
		}
		${name}_cleanup() {
			\$DEBUG && dump
			cleanup
		}"
	atf_add_test_case ${name}
}

add_badudp_test()
{
	local inner=$1
	local outer=$2
	local ipv4=inet
	local ipv6=inet6

	name="wg_badudp_${inner}_over_${outer}"
	fulldesc="Test wg(4) with ${inner} over ${outer} with bad UDP packets"

	eval inner=\$$inner
	eval outer=\$$outer

	atf_test_case ${name} cleanup
	eval "
		${name}_head() {
			atf_set descr \"${fulldesc}\"
			atf_set require.progs rump_server wgconfig wg-keygen nc
		}
		${name}_body() {
			test_common badudp $outer $inner
			rump_server_destroy_ifaces
		}
		${name}_cleanup() {
			\$DEBUG && dump
			cleanup
		}"
	atf_add_test_case ${name}
}

add_badpeerkey_test()
{
	local inner=$1
	local outer=$2
	local testno=$3
	local pubkey=$4
	local ipv4=inet
	local ipv6=inet6

	name="wg_badpeerkey_${inner}_over_${outer}_test_${testno}"
	fulldesc="Test wg(4) with ${inner} over ${outer} with bad peer key"

	eval inner=\$$inner
	eval outer=\$$outer

	atf_test_case ${name} cleanup
	eval "
		${name}_head() {
			atf_set descr \"${fulldesc}\"
			atf_set require.progs rump_server wgconfig wg-keygen nc
		}
		${name}_body() {
			test_common badpeerkey $outer $inner $pubkey
			rump_server_destroy_ifaces
		}
		${name}_cleanup() {
			\$DEBUG && dump
			cleanup
		}"
	atf_add_test_case ${name}
}

add_badhandshakekey_test()
{
	local inner=$1
	local outer=$2
	local testno=$3
	local pubkey=$4
	local ipv4=inet
	local ipv6=inet6

	name="wg_badhanddshakekey_${inner}_over_${outer}_test_${testno}"
	fulldesc="Test wg(4) with ${inner} over ${outer} with bad handshake key"

	eval inner=\$$inner
	eval outer=\$$outer

	atf_test_case ${name} cleanup
	eval "
		${name}_head() {
			atf_set descr \"${fulldesc}\"
			atf_set require.progs rump_server wgconfig wg-keygen nc
		}
		${name}_body() {
			test_common badhandshakekey $outer $inner $pubkey
			rump_server_destroy_ifaces
		}
		${name}_cleanup() {
			\$DEBUG && dump
			cleanup
		}"
	atf_add_test_case ${name}
}

atf_test_case wg_multiple_interfaces cleanup
wg_multiple_interfaces_head()
{

	atf_set "descr" "tests multiple wg(4) interfaces"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_multiple_interfaces_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping -n -i 0.1 -c 3 -w 1"
	local ping_fail="atf_check -s not-exit:0 -o ignore rump.ping -n -c 1 -w 1"
	local key_priv_peer2=
	local key_pub_peer2=
	local ip_local=192.168.1.1
	local ip_local2=192.168.2.1
	local ip_peer=192.168.1.2
	local ip_peer2=192.168.2.2
	local ip_wg_local=10.0.0.1
	local ip_wg_local2=10.0.1.1
	local ip_wg_peer=10.0.0.2
	local ip_wg_peer2=10.0.1.2
	local port=51820
	local port2=51821
	local outfile=./out

	setup_servers
	rump_server_add_iface $SOCK_LOCAL shmif1 $BUS

	rump_server_crypto_start $SOCK_PEER2 netinet6 wg
	rump_server_add_iface $SOCK_PEER2 shmif0 $BUS

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys
	key_priv_peer2=$(wg-keygen)
	key_pub_peer2=$(echo $key_priv_peer2| wg-keygen --pub)

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 inet $ip_local 24
	setup_common shmif1 inet $ip_local2 24
	setup_wg_common wg0 inet $ip_wg_local 24 $port "$key_priv_local"
	setup_wg_common wg1 inet $ip_wg_local2 24 $port2 "$key_priv_local"
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/32
	add_peer wg1 peer0 $key_pub_peer2 $ip_peer2:$port2 $ip_wg_peer2/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER
	setup_common shmif0 inet $ip_peer 24
	setup_wg_common wg0 inet $ip_wg_peer 24 $port "$key_priv_peer"
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER2
	setup_common shmif0 inet $ip_peer2 24
	setup_wg_common wg0 inet $ip_wg_peer2 24 $port2 "$key_priv_peer2"
	add_peer wg0 peer0 $key_pub_local $ip_local2:$port2 $ip_wg_local2/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_LOCAL

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	$ping $ip_wg_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	$ping $ip_wg_peer2

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	$ifconfig wg0 destroy
	$ifconfig wg1 destroy
	export RUMP_SERVER=$SOCK_PEER
	$ifconfig wg0 destroy
	export RUMP_SERVER=$SOCK_PEER2
	$ifconfig wg0 destroy
}

wg_multiple_interfaces_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case wg_multiple_peers cleanup
wg_multiple_peers_head()
{

	atf_set "descr" "tests multiple wg(4) peers"
	atf_set "require.progs" "rump_server" "wgconfig" "wg-keygen"
}

wg_multiple_peers_body()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping -n -i 0.1 -c 3 -w 1"
	local ping_fail="atf_check -s not-exit:0 -o ignore rump.ping -n -c 1 -w 1"
	local key_priv_peer2=
	local key_pub_peer2=
	local ip_local=192.168.1.1
	local ip_peer=192.168.1.2
	local ip_peer2=192.168.1.3
	local ip_wg_local=10.0.0.1
	local ip_wg_peer=10.0.0.2
	local ip_wg_peer2=10.0.0.3
	local port=51820
	local outfile=./out

	setup_servers
	rump_server_add_iface $SOCK_LOCAL shmif1 $BUS

	rump_server_crypto_start $SOCK_PEER2 netinet6 wg
	rump_server_add_iface $SOCK_PEER2 shmif0 $BUS

	# It sets key_priv_local key_pub_local key_priv_peer key_pub_peer
	generate_keys
	key_priv_peer2=$(wg-keygen)
	key_pub_peer2=$(echo $key_priv_peer2| wg-keygen --pub)

	export RUMP_SERVER=$SOCK_LOCAL
	setup_common shmif0 inet $ip_local 24
	setup_wg_common wg0 inet $ip_wg_local 24 $port "$key_priv_local"
	add_peer wg0 peer0 $key_pub_peer $ip_peer:$port $ip_wg_peer/32
	add_peer wg0 peer1 $key_pub_peer2 $ip_peer2:$port $ip_wg_peer2/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER
	setup_common shmif0 inet $ip_peer 24
	setup_wg_common wg0 inet $ip_wg_peer 24 $port "$key_priv_peer"
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_PEER2
	setup_common shmif0 inet $ip_peer2 24
	setup_wg_common wg0 inet $ip_wg_peer2 24 $port "$key_priv_peer2"
	add_peer wg0 peer0 $key_pub_local $ip_local:$port $ip_wg_local/32
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_LOCAL

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	$ping $ip_wg_peer

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	$ping $ip_wg_peer2

	extract_new_packets $BUS > $outfile
	$DEBUG && cat $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	$ifconfig wg0 destroy
	export RUMP_SERVER=$SOCK_PEER
	$ifconfig wg0 destroy
	export RUMP_SERVER=$SOCK_PEER2
	$ifconfig wg0 destroy
}

wg_multiple_peers_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{
	local testno badkey

	add_badudp_test ipv4 ipv4
	add_badudp_test ipv4 ipv6
	add_badudp_test ipv6 ipv4
	add_badudp_test ipv6 ipv6

	add_basic_test ipv4 ipv4
	add_basic_test ipv4 ipv6
	add_basic_test ipv6 ipv4
	add_basic_test ipv6 ipv6

	add_payload_sizes_test ipv4 ipv4
	add_payload_sizes_test ipv4 ipv6
	add_payload_sizes_test ipv6 ipv4
	add_payload_sizes_test ipv6 ipv6

	atf_add_test_case wg_create_destroy
	atf_add_test_case wg_create_destroy_peers_ipv4
	atf_add_test_case wg_create_destroy_peers_ipv6
	atf_add_test_case wg_multiple_interfaces
	atf_add_test_case wg_multiple_peers

	# These are all possible little-endian x coordinates of points
	# of order <=8 on Curve25519.  See
	# <https://web.archive.org/web/20260613191208/https://cr.yp.to/ecdh.html#validate>
	# for details.
	while read testno badkey; do
		add_badpeerkey_test ipv4 ipv4 "$testno" "$badkey"
		add_badhandshakekey_test ipv4 ipv4 "$testno" "$badkey"
	done <<EOF
0 AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=
1 AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=
2 4Ot6fDtBuK4WVuP68Z/EatoJjeucMrH9hmIFFl9JuAA=
3 X5yVvKNQjCSx0LFVnIPvWwREXMRYHI6G2CJO3dCfEVc=
4 7P///////////////////////////////////////38=
5 7f///////////////////////////////////////38=
6 7v///////////////////////////////////////38=
7 zet6fDtBuK4WVuP68Z/EatoJjeucMrH9hmIFFl9JuIA=
8 TJyVvKNQjCSx0LFVnIPvWwREXMRYHI6G2CJO3dCfEdc=
9 2f////////////////////////////////////////8=
10 2v////////////////////////////////////////8=
11 2/////////////////////////////////////////8=
EOF
}
