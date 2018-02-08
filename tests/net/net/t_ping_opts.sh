#	$NetBSD: t_ping_opts.sh,v 1.2 2018/02/08 09:56:19 maxv Exp $
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

SOCKSRC=unix://ping_opts1
SOCKFWD=unix://ping_opts2
SOCKDST=unix://ping_opts3
IPSRC=10.0.1.2
IPSRCGW=10.0.1.1
IPDSTGW=10.0.2.1
IPDST=10.0.2.2
BUS_SRCGW=bus1
BUS_DSTGW=bus2

IPSRC2=10.0.1.3
IPSRCGW2=10.0.1.254

DEBUG=${DEBUG:-false}
TIMEOUT=1
PING_OPTS="-n -c 1 -w $TIMEOUT"

#
# Utility functions
#
setup_endpoint()
{
	local sock=${1}
	local addr=${2}
	local bus=${3}
	local gw=${4}

	rump_server_add_iface $sock shmif0 $bus

	export RUMP_SERVER=${sock}
	atf_check -s exit:0 rump.ifconfig shmif0 ${addr}/24
	atf_check -s exit:0 -o ignore rump.route add default ${gw}
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	if $DEBUG; then
		rump.ifconfig shmif0
		rump.netstat -nr
	fi
}

setup_forwarder()
{

	rump_server_add_iface $SOCKFWD shmif0 $BUS_SRCGW
	rump_server_add_iface $SOCKFWD shmif1 $BUS_DSTGW

	export RUMP_SERVER=$SOCKFWD

	atf_check -s exit:0 rump.ifconfig shmif0 ${IPSRCGW}/24
	atf_check -s exit:0 rump.ifconfig shmif1 ${IPDSTGW}/24

	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig shmif1 up
	atf_check -s exit:0 rump.ifconfig -w 10

	if $DEBUG; then
		rump.netstat -nr
		rump.sysctl net.inet.ip.forwarding
	fi
}

setup_forwarding()
{

	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 -o ignore rump.sysctl -w net.inet.ip.forwarding=1
}

setup()
{

	rump_server_start $SOCKSRC
	rump_server_start $SOCKFWD
	rump_server_start $SOCKDST

	setup_endpoint $SOCKSRC $IPSRC $BUS_SRCGW $IPSRCGW
	setup_endpoint $SOCKDST $IPDST $BUS_DSTGW $IPDSTGW
	setup_forwarder
}

check_echo_request_pkt()
{
	local pkt="$1 > $2: .+ echo request"

	extract_new_packets $BUS_SRCGW > ./out
	$DEBUG && echo $pkt
	$DEBUG && cat ./out
	atf_check -s exit:0 -o match:"$pkt" cat ./out
}

check_echo_request_pkt_with_macaddr()
{
	local pkt="$1 > $2, .+ $3 > $4: .+ echo request"

	extract_new_packets $BUS_SRCGW > ./out
	$DEBUG && echo $pkt
	$DEBUG && cat ./out
	atf_check -s exit:0 -o match:"$pkt" cat ./out
}

check_echo_request_pkt_with_macaddr_and_rthdr0()
{
	local pkt=

	pkt="$1 > $2, .+ $3 > $4:"
	pkt="$pkt srcrt \\(len=2, type=0, segleft=1, \\[0\\]$5\\)"
	pkt="$pkt .+ echo request"

	extract_new_packets $BUS_SRCGW > ./out
	$DEBUG && echo $pkt
	$DEBUG && cat ./out
	atf_check -s exit:0 -o match:"$pkt" cat ./out
}

#
# Tests
#
atf_test_case ping_opts_sourceaddr cleanup
ping_opts_sourceaddr_head()
{

	atf_set "descr" "tests of ping -I option"
	atf_set "require.progs" "rump_server"
}

ping_opts_sourceaddr_body()
{

	setup
	setup_forwarding

	export RUMP_SERVER=$SOCKSRC
	atf_check -s exit:0 -o ignore rump.ping $PING_OPTS $IPDST
	check_echo_request_pkt $IPSRC $IPDST

	atf_check -s exit:0 rump.ifconfig shmif0 $IPSRC2/24 alias
	atf_check -s exit:0 rump.ifconfig -w 10

	atf_check -s exit:0 -o ignore rump.ping $PING_OPTS $IPDST
	check_echo_request_pkt $IPSRC $IPDST

	# ping -I <sourceaddr>
	atf_check -s exit:0 -o ignore rump.ping $PING_OPTS \
	    -I $IPSRC $IPDST
	check_echo_request_pkt $IPSRC $IPDST

	atf_check -s exit:0 -o ignore rump.ping $PING_OPTS \
	    -I $IPSRC2 $IPDST
	check_echo_request_pkt $IPSRC2 $IPDST

	rump_server_destroy_ifaces
}

ping_opts_sourceaddr_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case ping_opts_gateway cleanup
ping_opts_gateway_head()
{

	atf_set "descr" "tests of ping -g option (IPOPT_LSRR)"
	atf_set "require.progs" "rump_server"
}

ping_opts_gateway_body()
{
	local my_macaddr=
	local gw_shmif0_macaddr=
	local gw_shmif2_macaddr=

	setup
	setup_forwarding

	my_macaddr=$(get_macaddr ${SOCKSRC} shmif0)
	gw_shmif0_macaddr=$(get_macaddr ${SOCKFWD} shmif0)

	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.allowsrcrt=1
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwsrcrt=1

	export RUMP_SERVER=$SOCKSRC
	atf_check -s exit:0 -o ignore rump.ping $PING_OPTS $IPDST
	check_echo_request_pkt_with_macaddr \
	    $my_macaddr $gw_shmif0_macaddr $IPSRC $IPDST

	rump_server_add_iface $SOCKFWD shmif2 $BUS_SRCGW
	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 rump.ifconfig shmif2 $IPSRCGW2/24
	atf_check -s exit:0 rump.ifconfig -w 10
	gw_shmif2_macaddr=$(get_macaddr ${SOCKFWD} shmif2)

	export RUMP_SERVER=$SOCKSRC
	atf_check -s exit:0 -o ignore rump.ping $PING_OPTS $IPDST
	check_echo_request_pkt_with_macaddr \
	    $my_macaddr $gw_shmif0_macaddr $IPSRC $IPDST

	# ping -g <gateway>
	atf_check -s exit:0 -o ignore rump.ping $PING_OPTS \
	    -g $IPSRCGW $IPDST
	# The destination address is the specified gateway and
	# the final destination address is stored in the IP options
	check_echo_request_pkt_with_macaddr \
	    $my_macaddr $gw_shmif0_macaddr $IPSRC $IPSRCGW

	atf_check -s exit:0 -o ignore rump.ping $PING_OPTS \
	    -g $IPSRCGW2 $IPDST
	check_echo_request_pkt_with_macaddr \
	    $my_macaddr $gw_shmif2_macaddr $IPSRC $IPSRCGW2

	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.allowsrcrt=0
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwsrcrt=0

	rump_server_destroy_ifaces
}

ping_opts_gateway_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case ping_opts_recordroute cleanup
ping_opts_recordroute_head()
{

	atf_set "descr" "tests of ping -R option (IPOPT_RR)"
	atf_set "require.progs" "rump_server"
}

check_recorded_routes()
{
	local outfile=$1
	local out=./__file1 cmp=./__file2

	$DEBUG && cat $outfile
	grep -A 3 'RR: ' $outfile> $out
	cat <<EOF > $cmp
RR: 	10.0.2.1
	10.0.2.2
	10.0.1.1
	10.0.1.2
EOF
	atf_check -s exit:0 diff $out $cmp

	rm -f $fixed $cmp
}

ping_opts_recordroute_body()
{
	local my_macaddr=
	local gw_shmif0_macaddr=
	local gw_shmif2_macaddr=
	local out=./file1

	setup
	setup_forwarding

	my_macaddr=$(get_macaddr ${SOCKSRC} shmif0)
	gw_shmif0_macaddr=$(get_macaddr ${SOCKFWD} shmif0)

	export RUMP_SERVER=$SOCKSRC
	atf_check -s exit:0 -o save:$out rump.ping $PING_OPTS -R $IPDST
	check_echo_request_pkt_with_macaddr \
	    $my_macaddr $gw_shmif0_macaddr $IPSRC $IPDST
	check_recorded_routes $out

	rump_server_add_iface $SOCKFWD shmif2 $BUS_SRCGW
	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 rump.ifconfig shmif2 $IPSRCGW2/24
	atf_check -s exit:0 rump.ifconfig -w 10
	gw_shmif2_macaddr=$(get_macaddr ${SOCKFWD} shmif2)

	export RUMP_SERVER=$SOCKSRC
	atf_check -s exit:0 -o save:$out rump.ping $PING_OPTS -R $IPDST
	check_echo_request_pkt_with_macaddr \
	    $my_macaddr $gw_shmif0_macaddr $IPSRC $IPDST
	check_recorded_routes $out

	# ping -R -g <gateway>
	atf_check -s exit:0 -o save:$out rump.ping $PING_OPTS \
	    -R -g $IPSRCGW $IPDST
	# The destination address is the specified gateway and
	# the final destination address is stored in the IP options
	check_echo_request_pkt_with_macaddr \
	    $my_macaddr $gw_shmif0_macaddr $IPSRC $IPSRCGW
	check_recorded_routes $out

	atf_check -s exit:0 -o save:$out rump.ping $PING_OPTS \
	    -R -g $IPSRCGW2 $IPDST
	check_echo_request_pkt_with_macaddr \
	    $my_macaddr $gw_shmif2_macaddr $IPSRC $IPSRCGW2
	check_recorded_routes $out

	rm -f $out

	rump_server_destroy_ifaces
}

ping_opts_recordroute_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case ping_opts_sourceaddr
	atf_add_test_case ping_opts_gateway
	atf_add_test_case ping_opts_recordroute
}
