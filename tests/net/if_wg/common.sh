#	$NetBSD: common.sh,v 1.1 2020/08/26 16:03:42 riastradh Exp $
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

escape_key()
{

	echo $1 | sed 's/\+/\\+/g' | sed 's|\/|\\/|g'
}

setup_servers()
{

	rump_server_crypto_start $SOCK_LOCAL netinet6 wg
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS

	rump_server_crypto_start $SOCK_PEER netinet6 wg
	rump_server_add_iface $SOCK_PEER shmif0 $BUS
}

check_conf_port()
{
	local ifname=$1
	local port=$2

	atf_check -s exit:0 -o match:"listen-port: $port" \
	    $HIJACKING wgconfig $ifname
}

check_conf_privkey()
{
	local ifname=$1
	local key_priv="$2"

	atf_check -s exit:0 -o match:"private-key: $(escape_key $key_priv)" \
	    $HIJACKING wgconfig $ifname show private-key
}

setup_common()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ifname=$1
	local proto=$2
	local ip=$3
	local prefix=$4

	$ifconfig $ifname $proto $ip/$prefix
}

setup_wg_common()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local wgconfig="atf_check -s exit:0 $HIJACKING wgconfig"
	local ifname=$1
	local proto=$2
	local ip=$3
	local prefix=$4
	local port=$5
	local key_priv="$6"
	local tun=$7
	local privfile=./tmp

	$ifconfig $ifname create
	if [ -n "$tun" ]; then
		$ifconfig $ifname linkstr $tun
	fi
	$ifconfig $ifname $proto $ip/$prefix
	$DEBUG && rump.netstat -nr
	echo $key_priv > $privfile
	$wgconfig $ifname set private-key $privfile
	$wgconfig $ifname set listen-port $port
	rm -f $privfile
	$ifconfig $ifname up
	$DEBUG && rump.ifconfig $ifname

	check_conf_port $ifname $port
	check_conf_privkey $ifname "$key_priv"
}

check_ping()
{
	local proto=$1
	local ip=$2
	local ping=

	if [ $proto = inet ]; then
		ping="atf_check -s exit:0 -o ignore rump.ping -n -i 0.1 -c 3 -w 1"
	else
		ping="atf_check -s exit:0 -o ignore rump.ping6 -n -i 0.1 -c 3 -X 1"
	fi

	$ping $ip
}

check_ping_fail()
{
	local proto=$1
	local ip=$2
	local ping=

	if [ $proto = inet ]; then
		ping="atf_check -s not-exit:0 -o ignore rump.ping -n -c 1 -w 1"
	else
		ping="atf_check -s not-exit:0 -o ignore rump.ping6 -n -c 1 -X 1"
	fi

	$ping $ip
}

destroy_wg_interfaces()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"

	export RUMP_SERVER=$SOCK_LOCAL
	$ifconfig wg0 destroy
	export RUMP_SERVER=$SOCK_PEER
	$ifconfig wg0 destroy
}

add_peer()
{
	local wgconfig="atf_check -s exit:0 $HIJACKING wgconfig"
	local ifname=$1
	local peername=$2
	local key=$3
	local endpoint=$4
	local allowedips=$5
	local pskfile=$6
	local key_psk="$7"
	local pskopt=
	local endpoint_opts=

	if [ -n "$pskfile" ]; then
		pskopt="--preshared-key=$pskfile"
	fi

	if [ -n "$endpoint" ]; then
		endpoint_opts="--endpoint=$endpoint"
	fi

	$wgconfig $ifname add peer $peername $key $endpoint_opts \
	    --allowed-ips=$allowedips $pskopt
	atf_check -s exit:0 -o match:"allowed-ips: $allowedips" \
	    $HIJACKING wgconfig $ifname show peer $peername
	if [ -n "$key_psk" ]; then
		atf_check -s exit:0 \
		    -o match:"preshared-key: $(escape_key $key_psk)" \
		    $HIJACKING wgconfig $ifname show peer $peername \
		    --show-preshared-key
	else
		atf_check -s exit:0 -o match:"preshared-key: \(none\)" \
		    $HIJACKING wgconfig $ifname show peer $peername \
		    --show-preshared-key
	fi
}

delete_peer()
{
	local wgconfig="atf_check -s exit:0 $HIJACKING wgconfig"
	local ifname=$1
	local peername=$2

	$wgconfig $ifname delete peer $peername
	atf_check -s exit:0 -o not-match:"peer: $peername" \
	    $HIJACKING wgconfig $ifname
}

generate_keys()
{

	key_priv_local=$(wg-keygen)
	key_pub_local=$(echo $key_priv_local| wg-keygen --pub)
	key_priv_peer=$(wg-keygen)
	key_pub_peer=$(echo $key_priv_peer| wg-keygen --pub)

	export key_priv_local key_pub_local key_priv_peer key_pub_peer
}
