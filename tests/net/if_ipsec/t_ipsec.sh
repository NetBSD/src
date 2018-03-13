#	$NetBSD: t_ipsec.sh,v 1.3.2.4 2018/03/13 15:34:33 martin Exp $
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

SOCK1=unix://commsock1 # for ROUTER1
SOCK2=unix://commsock2 # for ROUTER2
ROUTER1_LANIP=192.168.1.1
ROUTER1_LANNET=192.168.1.0/24
ROUTER1_WANIP=10.0.0.1
ROUTER1_IPSECIP=172.16.1.1
ROUTER1_WANIP_DUMMY=10.0.0.11
ROUTER1_IPSECIP_DUMMY=172.16.11.1
ROUTER1_IPSECIP_RECURSIVE1=172.16.101.1
ROUTER1_IPSECIP_RECURSIVE2=172.16.201.1
ROUTER2_LANIP=192.168.2.1
ROUTER2_LANNET=192.168.2.0/24
ROUTER2_WANIP=10.0.0.2
ROUTER2_IPSECIP=172.16.2.1
ROUTER2_WANIP_DUMMY=10.0.0.12
ROUTER2_IPSECIP_DUMMY=172.16.12.1
ROUTER2_IPSECIP_RECURSIVE1=172.16.102.1
ROUTER2_IPSECIP_RECURSIVE2=172.16.202.1

ROUTER1_LANIP6=fc00:1::1
ROUTER1_LANNET6=fc00:1::/64
ROUTER1_WANIP6=fc00::1
ROUTER1_IPSECIP6=fc00:3::1
ROUTER1_WANIP6_DUMMY=fc00::11
ROUTER1_IPSECIP6_DUMMY=fc00:13::1
ROUTER1_IPSECIP6_RECURSIVE1=fc00:103::1
ROUTER1_IPSECIP6_RECURSIVE2=fc00:203::1
ROUTER2_LANIP6=fc00:2::1
ROUTER2_LANNET6=fc00:2::/64
ROUTER2_WANIP6=fc00::2
ROUTER2_IPSECIP6=fc00:4::1
ROUTER2_WANIP6_DUMMY=fc00::12
ROUTER2_IPSECIP6_DUMMY=fc00:14::1
ROUTER2_IPSECIP6_RECURSIVE1=fc00:104::1
ROUTER2_IPSECIP6_RECURSIVE2=fc00:204::1

DEBUG=${DEBUG:-false}
TIMEOUT=7

atf_test_case ipsecif_create_destroy cleanup
ipsecif_create_destroy_head()
{

	atf_set "descr" "Test creating/destroying gif interfaces"
	atf_set "require.progs" "rump_server"
}

ipsecif_create_destroy_body()
{

	rump_server_start $SOCK1 ipsec

	test_create_destroy_common $SOCK1 ipsec0
}

ipsecif_create_destroy_cleanup()
{

	$DEBUG && dump
	cleanup
}

setup_router()
{
	local sock=${1}
	local lan=${2}
	local lan_mode=${3}
	local wan=${4}
	local wan_mode=${5}

	rump_server_add_iface $sock shmif0 bus0
	rump_server_add_iface $sock shmif1 bus1

	export RUMP_SERVER=${sock}
	if [ ${lan_mode} = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig shmif0 inet6 ${lan}
	else
		atf_check -s exit:0 rump.ifconfig shmif0 inet ${lan} netmask 0xffffff00
	fi
	atf_check -s exit:0 rump.ifconfig shmif0 up
	rump.ifconfig shmif0

	if [ ${wan_mode} = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig shmif1 inet6 ${wan}
	else
		atf_check -s exit:0 rump.ifconfig shmif1 inet ${wan} netmask 0xff000000
	fi
	atf_check -s exit:0 rump.ifconfig shmif1 up
	rump.ifconfig shmif1
	unset RUMP_SERVER
}

test_router()
{
	local sock=${1}
	local lan=${2}
	local lan_mode=${3}
	local wan=${4}
	local wan_mode=${5}

	export RUMP_SERVER=${sock}
	atf_check -s exit:0 -o match:shmif0 rump.ifconfig
	if [ ${lan_mode} = "ipv6" ]; then
		atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 -X $TIMEOUT ${lan}
	else
		atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w $TIMEOUT ${lan}
	fi

	atf_check -s exit:0 -o match:shmif1 rump.ifconfig
	if [ ${wan_mode} = "ipv6" ]; then
		atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 -X $TIMEOUT ${wan}
	else
		atf_check -s exit:0 -o ignore rump.ping -n -c 1 -w $TIMEOUT ${wan}
	fi
	unset RUMP_SERVER
}

setup()
{
	local inner=${1}
	local outer=${2}

	rump_server_crypto_start $SOCK1 netipsec netinet6 ipsec
	rump_server_crypto_start $SOCK2 netipsec netinet6 ipsec

	router1_lan=""
	router1_lan_mode=""
	router2_lan=""
	router2_lan_mode=""
	if [ ${inner} = "ipv6" ]; then
		router1_lan=$ROUTER1_LANIP6
		router1_lan_mode="ipv6"
		router2_lan=$ROUTER2_LANIP6
		router2_lan_mode="ipv6"
	else
		router1_lan=$ROUTER1_LANIP
		router1_lan_mode="ipv4"
		router2_lan=$ROUTER2_LANIP
		router2_lan_mode="ipv4"
	fi

	if [ ${outer} = "ipv6" ]; then
		setup_router $SOCK1 ${router1_lan} ${router1_lan_mode} \
			$ROUTER1_WANIP6 ipv6
		setup_router $SOCK2 ${router2_lan} ${router2_lan_mode} \
			$ROUTER2_WANIP6 ipv6
	else
		setup_router $SOCK1 ${router1_lan} ${router1_lan_mode} \
			$ROUTER1_WANIP ipv4
		setup_router $SOCK2 ${router2_lan} ${router2_lan_mode} \
			$ROUTER2_WANIP ipv4
	fi
}

test_setup()
{
	local inner=${1}
	local outer=${2}

	local router1_lan=""
	local router1_lan_mode=""
	local router2_lan=""
	local router2_lan_mode=""
	if [ ${inner} = "ipv6" ]; then
		router1_lan=$ROUTER1_LANIP6
		router1_lan_mode="ipv6"
		router2_lan=$ROUTER2_LANIP6
		router2_lan_mode="ipv6"
	else
		router1_lan=$ROUTER1_LANIP
		router1_lan_mode="ipv4"
		router2_lan=$ROUTER2_LANIP
		router2_lan_mode="ipv4"
	fi
	if [ ${outer} = "ipv6" ]; then
		test_router $SOCK1 ${router1_lan} ${router1_lan_mode} \
			$ROUTER1_WANIP6 ipv6
		test_router $SOCK2 ${router2_lan} ${router2_lan_mode} \
			$ROUTER2_WANIP6 ipv6
	else
		test_router $SOCK1 ${router1_lan} ${router1_lan_mode} \
			$ROUTER1_WANIP ipv4
		test_router $SOCK2 ${router2_lan} ${router2_lan_mode} \
			$ROUTER2_WANIP ipv4
	fi
}

get_if_ipsec_unique()
{
	local sock=${1}
	local src=${2}
	local proto=${3}
	local unique=""

	export RUMP_SERVER=${sock}
	unique=`$HIJACKING setkey -DP | grep -A2 "^${src}.*(${proto})$" | grep unique | sed 's/.*unique#//'`
	unset RUMP_SERVER

	echo $unique
}

setup_if_ipsec()
{
	local sock=${1}
	local addr=${2}
	local remote=${3}
	local inner=${4}
	local src=${5}
	local dst=${6}
	local peernet=${7}

	export RUMP_SERVER=${sock}
	atf_check -s exit:0 rump.ifconfig ipsec0 create
	atf_check -s exit:0 rump.ifconfig ipsec0 tunnel ${src} ${dst}
	if [ ${inner} = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig ipsec0 inet6 ${addr}/128 ${remote}
		atf_check -s exit:0 -o ignore rump.route add -inet6 ${peernet} ${addr}
	else
		atf_check -s exit:0 rump.ifconfig ipsec0 inet ${addr}/32 ${remote}
		atf_check -s exit:0 -o ignore rump.route add -inet ${peernet} ${addr}
	fi

	rump.ifconfig ipsec0
	rump.route -nL show
}

setup_if_ipsec_sa()
{
	local sock=${1}
	local src=${2}
	local dst=${3}
	local mode=${4}
	local proto=${5}
	local algo=${6}
	local dir=${7}

	local tmpfile=./tmp
	local inunique=""
	local outunique=""
	local inid=""
	local outid=""
	local algo_args="$(generate_algo_args $proto $algo)"

	inunique=`get_if_ipsec_unique ${sock} ${dst} ${mode}`
	atf_check -s exit:0 test "X$inunique" != "X"
	outunique=`get_if_ipsec_unique ${sock} ${src} ${mode}`
	atf_check -s exit:0 test "X$outunique" != "X"

	if [ ${dir} = "1to2" ] ; then
	    if [ ${mode} = "ipv6" ] ; then
		inid="10010"
		outid="10011"
	    else
		inid="10000"
		outid="10001"
	    fi
	else
	    if [ ${mode} = "ipv6" ] ; then
		inid="10011"
		outid="10010"
	    else
		inid="10001"
		outid="10000"
	    fi
	fi

	cat > $tmpfile <<-EOF
    	add $dst $src $proto $inid -u $inunique $algo_args;
    	add $src $dst $proto $outid -u $outunique $algo_args;
	EOF
	$DEBUG && cat $tmpfile
	export RUMP_SERVER=$sock
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
	$DEBUG && $HIJACKING setkey -DP
	unset RUMP_SERVER
}

setup_tunnel()
{
	local inner=${1}
	local outer=${2}
	local proto=${3}
	local algo=${4}

	local addr=""
	local remote=""
	local src=""
	local dst=""
	local peernet=""

	if [ ${inner} = "ipv6" ]; then
		addr=$ROUTER1_IPSECIP6
		remote=$ROUTER2_IPSECIP6
		peernet=$ROUTER2_LANNET6
	else
		addr=$ROUTER1_IPSECIP
		remote=$ROUTER2_IPSECIP
		peernet=$ROUTER2_LANNET
	fi
	if [ ${outer} = "ipv6" ]; then
		src=$ROUTER1_WANIP6
		dst=$ROUTER2_WANIP6
	else
		src=$ROUTER1_WANIP
		dst=$ROUTER2_WANIP
	fi
	setup_if_ipsec $SOCK1 ${addr} ${remote} ${inner} \
		     ${src} ${dst} ${peernet}

	if [ $inner = "ipv6" -a $outer = "ipv4" ]; then
	    setup_if_ipsec_sa $SOCK1 ${src} ${dst} ${outer} ${proto} ${algo} "1to2"
	fi
	setup_if_ipsec_sa $SOCK1 ${src} ${dst} ${inner} ${proto} ${algo} "1to2"

	if [ $inner = "ipv6" ]; then
		addr=$ROUTER2_IPSECIP6
		remote=$ROUTER1_IPSECIP6
		peernet=$ROUTER1_LANNET6
	else
		addr=$ROUTER2_IPSECIP
		remote=$ROUTER1_IPSECIP
		peernet=$ROUTER1_LANNET
	fi
	if [ $outer = "ipv6" ]; then
		src=$ROUTER2_WANIP6
		dst=$ROUTER1_WANIP6
	else
		src=$ROUTER2_WANIP
		dst=$ROUTER1_WANIP
	fi
	setup_if_ipsec $SOCK2 ${addr} ${remote} ${inner} \
		     ${src} ${dst} ${peernet} ${proto} ${algo}
	if [ $inner = "ipv6" -a $outer = "ipv4" ]; then
	    setup_if_ipsec_sa $SOCK2 ${src} ${dst} ${outer} ${proto} ${algo} "2to1"
	fi
	setup_if_ipsec_sa $SOCK2 ${src} ${dst} ${inner} ${proto} ${algo} "2to1"
}

test_setup_tunnel()
{
	local mode=${1}

	local peernet=""
	local opt=""
	if [ ${mode} = "ipv6" ]; then
		peernet=$ROUTER2_LANNET6
		opt="-inet6"
	else
		peernet=$ROUTER2_LANNET
		opt="-inet"
	fi
	export RUMP_SERVER=$SOCK1
	atf_check -s exit:0 -o match:ipsec0 rump.ifconfig
	atf_check -s exit:0 -o match:ipsec0 rump.route -nL get ${opt} ${peernet}

	if [ ${mode} = "ipv6" ]; then
		peernet=$ROUTER1_LANNET6
		opt="-inet6"
	else
		peernet=$ROUTER1_LANNET
		opt="-inet"
	fi
	export RUMP_SERVER=$SOCK2
	atf_check -s exit:0 -o match:ipsec0 rump.ifconfig
	atf_check -s exit:0 -o match:ipsec0 rump.route -nL get ${opt} ${peernet}
}

teardown_tunnel()
{
	export RUMP_SERVER=$SOCK1
	atf_check -s exit:0 rump.ifconfig ipsec0 deletetunnel
	atf_check -s exit:0 rump.ifconfig ipsec0 destroy
	$HIJACKING setkey -F

	export RUMP_SERVER=$SOCK2
	atf_check -s exit:0 rump.ifconfig ipsec0 deletetunnel
	atf_check -s exit:0 rump.ifconfig ipsec0 destroy
	$HIJACKING setkey -F

	unset RUMP_SERVER
}

setup_dummy_if_ipsec()
{
	local sock=${1}
	local addr=${2}
	local remote=${3}
	local inner=${4}
	local src=${5}
	local dst=${6}

	export RUMP_SERVER=${sock}
	atf_check -s exit:0 rump.ifconfig ipsec1 create
	atf_check -s exit:0 rump.ifconfig ipsec1 tunnel ${src} ${dst}
	if [ ${inner} = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig ipsec1 inet6 ${addr}/128 ${remote}
	else
		atf_check -s exit:0 rump.ifconfig ipsec1 inet ${addr}/32 ${remote}
	fi

	rump.ifconfig ipsec1
	unset RUMP_SERVER
}

setup_dummy_if_ipsec_sa()
{
	local sock=${1}
	local src=${2}
	local dst=${3}
	local mode=${4}
	local proto=${5}
	local algo=${6}
	local dir=${7}

	local tmpfile=./tmp
	local inunique=""
	local outunique=""
	local inid=""
	local outid=""
	local algo_args="$(generate_algo_args $proto $algo)"

	inunique=`get_if_ipsec_unique ${sock} ${dst} ${mode}`
	atf_check -s exit:0 test "X$inunique" != "X"
	outunique=`get_if_ipsec_unique ${sock} ${src} ${mode}`
	atf_check -s exit:0 test "X$outunique" != "X"

	if [ ${dir} = "1to2" ] ; then
	    inid="20000"
	    outid="20001"
	else
	    inid="20001"
	    outid="20000"
	fi

	cat > $tmpfile <<-EOF
    	add $dst $src $proto $inid -u $inunique $algo_args;
    	add $src $dst $proto $outid -u $outunique $algo_args;
	EOF
	$DEBUG && cat $tmpfile
	export RUMP_SERVER=$sock
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
	$DEBUG && $HIJACKING setkey -DP
	unset RUMP_SERVER
}

setup_dummy_tunnel()
{
	local inner=${1}
	local outer=${2}
	local proto=${3}
	local algo=${4}

	local addr=""
	local remote=""
	local src=""
	local dst=""

	if [ ${inner} = "ipv6" ]; then
		addr=$ROUTER1_IPSECIP6_DUMMY
		remote=$ROUTER2_IPSECIP6_DUMMY
	else
		addr=$ROUTER1_IPSECIP_DUMMY
		remote=$ROUTER2_IPSECIP_DUMMY
	fi
	if [ ${outer} = "ipv6" ]; then
		src=$ROUTER1_WANIP6_DUMMY
		dst=$ROUTER2_WANIP6_DUMMY
	else
		src=$ROUTER1_WANIP_DUMMY
		dst=$ROUTER2_WANIP_DUMMY
	fi
	setup_dummy_if_ipsec $SOCK1 ${addr} ${remote} ${inner} \
			   ${src} ${dst} ${proto} ${algo} "1to2"
	setup_dummy_if_ipsec_sa $SOCK1 ${src} ${dst} ${inner} ${proto} ${algo} "1to2"

	if [ $inner = "ipv6" ]; then
		addr=$ROUTER2_IPSECIP6_DUMMY
		remote=$ROUTER1_IPSECIP6_DUMMY
	else
		addr=$ROUTER2_IPSECIP_DUMMY
		remote=$ROUTER1_IPSECIP_DUMMY
	fi
	if [ $outer = "ipv6" ]; then
		src=$ROUTER2_WANIP6_DUMMY
		dst=$ROUTER1_WANIP6_DUMMY
	else
		src=$ROUTER2_WANIP_DUMMY
		dst=$ROUTER1_WANIP_DUMMY
	fi
	setup_dummy_if_ipsec $SOCK2 ${addr} ${remote} ${inner} \
			   ${src} ${dst} ${proto} ${algo} "2to1"
	setup_dummy_if_ipsec_sa $SOCK2 ${src} ${dst} ${inner} ${proto} ${algo} "2to1"
}

test_setup_dummy_tunnel()
{
	export RUMP_SERVER=$SOCK1
	atf_check -s exit:0 -o match:ipsec1 rump.ifconfig

	export RUMP_SERVER=$SOCK2
	atf_check -s exit:0 -o match:ipsec1 rump.ifconfig

	unset RUMP_SERVER
}

teardown_dummy_tunnel()
{
	export RUMP_SERVER=$SOCK1
	atf_check -s exit:0 rump.ifconfig ipsec1 deletetunnel
	atf_check -s exit:0 rump.ifconfig ipsec1 destroy

	export RUMP_SERVER=$SOCK2
	atf_check -s exit:0 rump.ifconfig ipsec1 deletetunnel
	atf_check -s exit:0 rump.ifconfig ipsec1 destroy

	unset RUMP_SERVER
}

setup_recursive_if_ipsec()
{
	local sock=${1}
	local ipsec=${2}
	local addr=${3}
	local remote=${4}
	local inner=${5}
	local src=${6}
	local dst=${7}
	local proto=${8}
	local algo=${9}
	local dir=${10}

	export RUMP_SERVER=${sock}
	atf_check -s exit:0 rump.ifconfig ${ipsec} create
	atf_check -s exit:0 rump.ifconfig ${ipsec} tunnel ${src} ${dst}
	if [ ${inner} = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig ${ipsec} inet6 ${addr}/128 ${remote}
	else
		atf_check -s exit:0 rump.ifconfig ${ipsec} inet ${addr}/32 ${remote}
	fi
	setup_if_ipsec_sa $sock ${src} ${dst} ${inner} ${proto} ${algo} ${dir}

	export RUMP_SERVER=${sock}
	rump.ifconfig ${ipsec}
	unset RUMP_SERVER
}

# test in ROUTER1 only
setup_recursive_tunnels()
{
	local mode=${1}
	local proto=${2}
	local algo=${3}

	local addr=""
	local remote=""
	local src=""
	local dst=""

	if [ ${mode} = "ipv6" ]; then
		addr=$ROUTER1_IPSECIP6_RECURSIVE1
		remote=$ROUTER2_IPSECIP6_RECURSIVE1
		src=$ROUTER1_IPSECIP6
		dst=$ROUTER2_IPSECIP6
	else
		addr=$ROUTER1_IPSECIP_RECURSIVE1
		remote=$ROUTER2_IPSECIP_RECURSIVE1
		src=$ROUTER1_IPSECIP
		dst=$ROUTER2_IPSECIP
	fi
	setup_recursive_if_ipsec $SOCK1 ipsec1 ${addr} ${remote} ${mode} \
		      ${src} ${dst} ${proto} ${algo} "1to2"

	if [ ${mode} = "ipv6" ]; then
		addr=$ROUTER1_IPSECIP6_RECURSIVE2
		remote=$ROUTER2_IPSECIP6_RECURSIVE2
		src=$ROUTER1_IPSECIP6_RECURSIVE1
		dst=$ROUTER2_IPSECIP6_RECURSIVE1
	else
		addr=$ROUTER1_IPSECIP_RECURSIVE2
		remote=$ROUTER2_IPSECIP_RECURSIVE2
		src=$ROUTER1_IPSECIP_RECURSIVE1
		dst=$ROUTER2_IPSECIP_RECURSIVE1
	fi
	setup_recursive_if_ipsec $SOCK1 ipsec2 ${addr} ${remote} ${mode} \
		      ${src} ${dst} ${proto} ${algo} "1to2"
}

# test in router1 only
test_recursive_check()
{
	local mode=$1

	export RUMP_SERVER=$SOCK1
	if [ ${mode} = "ipv6" ]; then
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping6 -n -X $TIMEOUT -c 1 $ROUTER2_IPSECIP6_RECURSIVE2
	else
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping -n -w $TIMEOUT -c 1 $ROUTER2_IPSECIP_RECURSIVE2
	fi

	atf_check -o match:'ipsec0: recursively called too many times' \
		-x "$HIJACKING dmesg"

	$HIJACKING dmesg

	unset RUMP_SERVER
}

teardown_recursive_tunnels()
{
	export RUMP_SERVER=$SOCK1
	atf_check -s exit:0 rump.ifconfig ipsec1 deletetunnel
	atf_check -s exit:0 rump.ifconfig ipsec1 destroy
	atf_check -s exit:0 rump.ifconfig ipsec2 deletetunnel
	atf_check -s exit:0 rump.ifconfig ipsec2 destroy
	unset RUMP_SERVER
}

test_ping_failure()
{
	local mode=$1

	export RUMP_SERVER=$SOCK1
	if [ ${mode} = "ipv6" ]; then
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping6 -n -X $TIMEOUT -c 1 -S $ROUTER1_LANIP6 \
			$ROUTER2_LANIP6
	else
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER1_LANIP \
			$ROUTER2_LANIP
	fi

	export RUMP_SERVER=$SOCK2
	if [ ${mode} = "ipv6" ]; then
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping6 -n -X $TIMEOUT -c 1 -S $ROUTER2_LANIP6 \
			$ROUTER1_LANIP6
	else
		atf_check -s not-exit:0 -o ignore -e ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER1_LANIP \
			$ROUTER2_LANIP
	fi

	unset RUMP_SERVER
}

test_ping_success()
{
	mode=$1

	export RUMP_SERVER=$SOCK1
	rump.ifconfig -v ipsec0
	if [ ${mode} = "ipv6" ]; then
		# XXX
		# rump.ping6 rarely fails with the message that
		# "failed to get receiving hop limit".
		# This is a known issue being analyzed.
		atf_check -s exit:0 -o ignore \
			rump.ping6 -n -X $TIMEOUT -c 1 -S $ROUTER1_LANIP6 \
			$ROUTER2_LANIP6
	else
		atf_check -s exit:0 -o ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER1_LANIP \
			$ROUTER2_LANIP
	fi
	rump.ifconfig -v ipsec0

	export RUMP_SERVER=$SOCK2
	rump.ifconfig -v ipsec0
	if [ ${mode} = "ipv6" ]; then
		atf_check -s exit:0 -o ignore \
			rump.ping6 -n -X $TIMEOUT -c 1 -S $ROUTER2_LANIP6 \
			$ROUTER1_LANIP6
	else
		atf_check -s exit:0 -o ignore \
			rump.ping -n -w $TIMEOUT -c 1 -I $ROUTER2_LANIP \
			$ROUTER1_LANIP
	fi
	rump.ifconfig -v ipsec0

	unset RUMP_SERVER
}

test_change_tunnel_duplicate()
{
	local mode=$1

	local newsrc=""
	local newdst=""
	if [ ${mode} = "ipv6" ]; then
		newsrc=$ROUTER1_WANIP6_DUMMY
		newdst=$ROUTER2_WANIP6_DUMMY
	else
		newsrc=$ROUTER1_WANIP_DUMMY
		newdst=$ROUTER2_WANIP_DUMMY
	fi
	export RUMP_SERVER=$SOCK1
	rump.ifconfig -v ipsec0
	rump.ifconfig -v ipsec1
	atf_check -s exit:0 -e match:SIOCSLIFPHYADDR \
		rump.ifconfig ipsec0 tunnel ${newsrc} ${newdst}
	rump.ifconfig -v ipsec0
	rump.ifconfig -v ipsec1

	if [ ${mode} = "ipv6" ]; then
		newsrc=$ROUTER2_WANIP6_DUMMY
		newdst=$ROUTER1_WANIP6_DUMMY
	else
		newsrc=$ROUTER2_WANIP_DUMMY
		newdst=$ROUTER1_WANIP_DUMMY
	fi
	export RUMP_SERVER=$SOCK2
	rump.ifconfig -v ipsec0
	rump.ifconfig -v ipsec1
	atf_check -s exit:0 -e match:SIOCSLIFPHYADDR \
		rump.ifconfig ipsec0 tunnel ${newsrc} ${newdst}
	rump.ifconfig -v ipsec0
	rump.ifconfig -v ipsec1

	unset RUMP_SERVER
}

test_change_tunnel_success()
{
	local mode=$1

	local newsrc=""
	local newdst=""
	if [ ${mode} = "ipv6" ]; then
		newsrc=$ROUTER1_WANIP6_DUMMY
		newdst=$ROUTER2_WANIP6_DUMMY
	else
		newsrc=$ROUTER1_WANIP_DUMMY
		newdst=$ROUTER2_WANIP_DUMMY
	fi
	export RUMP_SERVER=$SOCK1
	rump.ifconfig -v ipsec0
	atf_check -s exit:0 \
		rump.ifconfig ipsec0 tunnel ${newsrc} ${newdst}
	rump.ifconfig -v ipsec0

	if [ ${mode} = "ipv6" ]; then
		newsrc=$ROUTER2_WANIP6_DUMMY
		newdst=$ROUTER1_WANIP6_DUMMY
	else
		newsrc=$ROUTER2_WANIP_DUMMY
		newdst=$ROUTER1_WANIP_DUMMY
	fi
	export RUMP_SERVER=$SOCK2
	rump.ifconfig -v ipsec0
	atf_check -s exit:0 \
		rump.ifconfig ipsec0 tunnel ${newsrc} ${newdst}
	rump.ifconfig -v ipsec0

	unset RUMP_SERVER
}

basic_setup()
{
	local inner=$1
	local outer=$2
	local proto=$3
	local algo=$4

	setup ${inner} ${outer}
	test_setup ${inner} ${outer}

	# Enable once PR kern/49219 is fixed
	#test_ping_failure

	setup_tunnel ${inner} ${outer} ${proto} ${algo}
	sleep 1
	test_setup_tunnel ${inner}
}

basic_test()
{
	local inner=$1
	local outer=$2 # not use

	test_ping_success ${inner}
}

basic_teardown()
{
	local inner=$1
	local outer=$2 # not use

	teardown_tunnel
	test_ping_failure ${inner}
}

ioctl_setup()
{
	local inner=$1
	local outer=$2
	local proto=$3
	local algo=$4

	setup ${inner} ${outer}
	test_setup ${inner} ${outer}

	# Enable once PR kern/49219 is fixed
	#test_ping_failure

	setup_tunnel ${inner} ${outer} ${proto} ${algo}
	setup_dummy_tunnel ${inner} ${outer} ${proto} ${algo}
	sleep 1
	test_setup_tunnel ${inner}
}

ioctl_test()
{
	local inner=$1
	local outer=$2

	test_ping_success ${inner}

	test_change_tunnel_duplicate ${outer}

	teardown_dummy_tunnel
	test_change_tunnel_success ${outer}
}

ioctl_teardown()
{
	local inner=$1
	local outer=$2 # not use

	teardown_tunnel
	test_ping_failure ${inner}
}

recursive_setup()
{
	local inner=$1
	local outer=$2
	local proto=$3
	local algo=$4

	setup ${inner} ${outer}
	test_setup ${inner} ${outer}

	# Enable once PR kern/49219 is fixed
	#test_ping_failure

	setup_tunnel ${inner} ${outer} ${proto} ${algo}
	setup_recursive_tunnels ${inner} ${proto} ${algo}
	sleep 1
	test_setup_tunnel ${inner}
}

recursive_test()
{
	local inner=$1
	local outer=$2 # not use

	test_recursive_check ${inner}
}

recursive_teardown()
{
	local inner=$1 # not use
	local outer=$2 # not use

	teardown_recursive_tunnels
	teardown_tunnel
}

add_test()
{
	local category=$1
	local desc=$2
	local inner=$3
	local outer=$4
	local proto=$5
	local algo=$6
	local _algo=$(echo $algo | sed 's/-//g')

	name="ipsecif_${category}_${inner}over${outer}_${proto}_${_algo}"
	fulldesc="Does ${inner} over ${outer} if_ipsec ${desc}"

	atf_test_case ${name} cleanup
	eval "${name}_head() {
			atf_set descr \"${fulldesc}\"
			atf_set require.progs rump_server setkey
		}
	    ${name}_body() {
			${category}_setup ${inner} ${outer} ${proto} ${algo}
			${category}_test ${inner} ${outer}
			${category}_teardown ${inner} ${outer}
			rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
			\$DEBUG && dump
			cleanup
		}"
	atf_add_test_case ${name}
}

add_test_allproto()
{
	local category=$1
	local desc=$2

	for algo in $ESP_ENCRYPTION_ALGORITHMS_MINIMUM; do
		add_test ${category} "${desc}" ipv4 ipv4 esp $algo
		add_test ${category} "${desc}" ipv4 ipv6 esp $algo
		add_test ${category} "${desc}" ipv6 ipv4 esp $algo
		add_test ${category} "${desc}" ipv6 ipv6 esp $algo
	done

	# ah does not support yet
}

atf_init_test_cases()
{

	atf_add_test_case ipsecif_create_destroy

	add_test_allproto basic "basic tests"
	add_test_allproto ioctl "ioctl tests"
	add_test_allproto recursive "recursive check tests"
}
