#	$NetBSD: t_pppoe.sh,v 1.33 2021/06/01 05:18:33 yamaguchi Exp $
#
# Copyright (c) 2016 Internet Initiative Japan Inc.
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

SERVER=unix://pppoe_server
CLIENT=unix://pppoe_client

SERVER_IP=10.3.3.1
CLIENT_IP=10.3.3.3
SERVER_IP6=fc00::1
CLIENT_IP6=fc00::3
AUTHNAME=foobar@baz.com
SECRET=oink
BUS=bus0
TIMEOUT=3
WAITTIME=10
DEBUG=${DEBUG:-false}

atf_ifconfig()
{

	atf_check -s exit:0 rump.ifconfig $*
}

atf_pppoectl()
{

	atf_check -s exit:0 -x "$HIJACKING pppoectl $*"
}

atf_test_case pppoe_create_destroy cleanup
pppoe_create_destroy_head()
{

	atf_set "descr" "Test creating/destroying pppoe interfaces"
	atf_set "require.progs" "rump_server"
}

pppoe_create_destroy_body()
{

	rump_server_start $CLIENT netinet6 pppoe

	test_create_destroy_common $CLIENT pppoe0 true
}

pppoe_create_destroy_cleanup()
{

	$DEBUG && dump
	cleanup
}

setup_ifaces()
{

	rump_server_add_iface $SERVER shmif0 $BUS
	rump_server_add_iface $CLIENT shmif0 $BUS
	rump_server_add_iface $SERVER pppoe0
	rump_server_add_iface $CLIENT pppoe0

	export RUMP_SERVER=$SERVER
	atf_ifconfig shmif0 up
	$inet && atf_ifconfig pppoe0 \
	    inet $SERVER_IP $CLIENT_IP down
	atf_ifconfig pppoe0 link0

	$DEBUG && rump.ifconfig pppoe0 debug
	$DEBUG && rump.ifconfig
	$DEBUG && $HIJACKING pppoectl -d pppoe0
	unset RUMP_SERVER

	export RUMP_SERVER=$CLIENT
	atf_ifconfig shmif0 up

	$inet && atf_ifconfig pppoe0 \
	    inet 0.0.0.0 0.0.0.1 down

	$DEBUG && rump.ifconfig pppoe0 debug
	$DEBUG && rump.ifconfig
	$DEBUG && $HIJACKING pppoectl -d pppoe0
	unset RUMP_SERVER
}

setup()
{
	inet=true

	if [ $# -ne 0 ]; then
		eval $@
	fi

	rump_server_start $SERVER netinet6 pppoe
	rump_server_start $CLIENT netinet6 pppoe

	setup_ifaces

	export RUMP_SERVER=$SERVER
	atf_pppoectl -e shmif0 pppoe0
	unset RUMP_SERVER

	export RUMP_SERVER=$CLIENT
	atf_pppoectl -e shmif0 pppoe0
	unset RUMP_SERVER
}

wait_for_opened()
{
	local cp=$1
	local dontfail=$2
	local n=$WAITTIME

	for i in $(seq $n); do
		$HIJACKING pppoectl -dd pppoe0 | grep -q "$cp state: opened"
		if [ $? = 0 ]; then
			rump.ifconfig -w 10
			return
		fi
		sleep 1
	done

	if [ "$dontfail" != "dontfail" ]; then
		atf_fail "Couldn't connect to the server for $n seconds."
	fi
}

wait_for_disconnected()
{
	local dontfail=$1
	local n=$WAITTIME

	for i in $(seq $n); do
		# If PPPoE client is disconnected by PPPoE server, then
		# the LCP state will of the client is in a starting to send PADI.
		$HIJACKING pppoectl -dd pppoe0 | grep -q \
		    -e "LCP state: initial" -e "LCP state: starting"
		[ $? = 0 ] && return

		sleep 1
	done

	if [ "$dontfail" != "dontfail" ]; then
		atf_fail "Couldn't disconnect for $n seconds."
	fi
}

run_test()
{
	local auth=$1
	local cp="IPCP"
	setup

	# As pppoe client doesn't support rechallenge yet.
	local server_optparam=""
	if [ $auth = "chap" ]; then
		server_optparam="norechallenge"
	fi

	export RUMP_SERVER=$SERVER
	atf_pppoectl pppoe0 "hisauthproto=$auth" \
	    "hisauthname=$AUTHNAME" "hisauthsecret=$SECRET" \
	    "myauthproto=none" $server_optparam
	atf_ifconfig pppoe0 up
	unset RUMP_SERVER

	export RUMP_SERVER=$CLIENT
	atf_pppoectl pppoe0 \
	    "myauthname=$AUTHNAME" "myauthsecret=$SECRET" \
	    "myauthproto=$auth" "hisauthproto=none"
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	atf_check -s exit:0 -o ignore rump.ping -c 1 -w $TIMEOUT $SERVER_IP
	unset RUMP_SERVER

	# test for disconnection from server
	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	export RUMP_SERVER=$CLIENT
	wait_for_disconnected
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP
	atf_check -s exit:0 -o match:'(PADI sent)|(initial)' \
	    -x "$HIJACKING pppoectl -d pppoe0"
	unset RUMP_SERVER

	# test for reconnecting
	atf_check -s exit:0 -x "env RUMP_SERVER=$SERVER rump.ifconfig pppoe0 up"
	export RUMP_SERVER=$CLIENT
	wait_for_opened $cp
	atf_check -s exit:0 -o ignore rump.ping -c 1 -w $TIMEOUT $SERVER_IP
	unset RUMP_SERVER

	# test for disconnection from client
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	export RUMP_SERVER=$SERVER
	wait_for_disconnected
	$DEBUG && $HIJACKING pppoectl -d pppoe0
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping -c 1 -w $TIMEOUT $CLIENT_IP
	atf_check -s exit:0 -o match:'initial' -x "$HIJACKING pppoectl -d pppoe0"
	unset RUMP_SERVER

	# test for reconnecting
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 up
	wait_for_opened $cp
	$DEBUG && rump.ifconfig pppoe0
	$DEBUG && $HIJACKING pppoectl -d pppoe0
	unset RUMP_SERVER

	export RUMP_SERVER=$SERVER
	atf_ifconfig -w 10
	atf_check -s exit:0 -o ignore rump.ping -c 1 -w $TIMEOUT $CLIENT_IP
	atf_check -s exit:0 -o match:'session' -x "$HIJACKING pppoectl -d pppoe0"
	$DEBUG && $HIJACKING pppoectl -d pppoe0
	unset RUMP_SERVER

	# test for invalid password
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	atf_pppoectl pppoe0 "myauthproto=$auth" \
			    "myauthname=$AUTHNAME" \
			    "myauthsecret=invalidsecret" \
			    "hisauthproto=none" \
			    "max-auth-failure=1"
	atf_ifconfig pppoe0 up
	wait_for_opened $cp dontfail
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP
	atf_check -s exit:0 -o match:'DETACHED' rump.ifconfig pppoe0
	unset RUMP_SERVER
}

atf_test_case pppoe_pap cleanup

pppoe_pap_head()
{
	atf_set "descr" "Does simple pap tests"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_pap_body()
{
	run_test pap
}

pppoe_pap_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case pppoe_chap cleanup

pppoe_chap_head()
{
	atf_set "descr" "Does simple chap tests"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_chap_body()
{
	run_test chap
}

pppoe_chap_cleanup()
{

	$DEBUG && dump
	cleanup
}

run_test6()
{
	local auth=$1
	local cp="IPv6CP"
	setup "inet=false"

	# As pppoe client doesn't support rechallenge yet.
	local server_optparam=""
	if [ $auth = "chap" ]; then
		server_optparam="norechallenge"
	fi

	export RUMP_SERVER=$SERVER
	atf_pppoectl pppoe0 \
	    "hisauthname=$AUTHNAME" "hisauthsecret=$SECRET" \
	    "hisauthproto=$auth" "myauthproto=none" \
	    $server_optparam
	atf_ifconfig pppoe0 inet6 $SERVER_IP6/64 down
	atf_ifconfig pppoe0 up
	unset RUMP_SERVER

	export RUMP_SERVER=$CLIENT
	atf_pppoectl pppoe0 \
	    "myauthname=$AUTHNAME" "myauthsecret=$SECRET" \
	    "myauthproto=$auth" "hisauthproto=none"
	atf_ifconfig pppoe0 inet6 $CLIENT_IP6/64 down
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	atf_ifconfig -w 10
	export RUMP_SERVER=$SERVER
	rump.ifconfig -w 10
	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -X $TIMEOUT $SERVER_IP6
	unset RUMP_SERVER

	# test for disconnection from server
	export RUMP_SERVER=$SERVER
	session_id=`$HIJACKING pppoectl -d pppoe0 | grep state`
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	export RUMP_SERVER=$CLIENT
	wait_for_disconnected
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping6 -c 1 -X $TIMEOUT $SERVER_IP6
	atf_check -s exit:0 -o not-match:"$session_id" -x "$HIJACKING pppoectl -d pppoe0"
	unset RUMP_SERVER

	# test for reconnecting
	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 up
	wait_for_opened $cp
	atf_ifconfig -w 10
	$DEBUG && $HIJACKING pppoectl -d pppoe0
	$DEBUG && rump.ifconfig pppoe0
	export RUMP_SERVER=$CLIENT
	atf_ifconfig -w 10
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -X $TIMEOUT $SERVER_IP6
	unset RUMP_SERVER

	# test for disconnection from client
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected

	export RUMP_SERVER=$SERVER
	wait_for_disconnected
	$DEBUG && $HIJACKING pppoectl -d pppoe0
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping6 -c 1 -X $TIMEOUT $CLIENT_IP6
	atf_check -s exit:0 -o match:'initial' -x "$HIJACKING pppoectl -d pppoe0"
	unset RUMP_SERVER

	# test for reconnecting
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 up
	wait_for_opened $cp
	atf_ifconfig -w 10

	$DEBUG && rump.ifconfig pppoe0
	$DEBUG && $HIJACKING pppoectl -d pppoe0
	unset RUMP_SERVER

	export RUMP_SERVER=$SERVER
	atf_ifconfig -w 10
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -X $TIMEOUT $CLIENT_IP6
	atf_check -s exit:0 -o match:'session' -x "$HIJACKING pppoectl -d pppoe0"
	$DEBUG && HIJACKING pppoectl -d pppoe0
	unset RUMP_SERVER

	# test for invalid password
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	atf_pppoectl pppoe0 \
	    "myauthname=$AUTHNAME" "myauthsecret=invalidsecret" \
	    "myauthproto=$auth" "hisauthproto=none" \
	    "max-auth-failure=1"
	atf_ifconfig pppoe0 up
	wait_for_opened $cp dontfail
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping6 -c 1 -X $TIMEOUT $SERVER_IP6
	atf_check -s exit:0 -o match:'DETACHED' rump.ifconfig pppoe0
	unset RUMP_SERVER
}

atf_test_case pppoe6_pap cleanup

pppoe6_pap_head()
{
	atf_set "descr" "Does simple pap using IPv6 tests"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe6_pap_body()
{
	run_test6 pap
}

pppoe6_pap_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case pppoe6_chap cleanup

pppoe6_chap_head()
{
	atf_set "descr" "Does simple chap using IPv6 tests"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe6_chap_body()
{
	run_test6 chap
}

pppoe6_chap_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case pppoe_params cleanup

dump_bus()
{

	shmif_dumpbus -p - ${BUS} | tcpdump -n -e -r -
}

setup_auth_conf()
{
	local auth=chap
	local server_optparam="norechallenge"

	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 link0
	atf_pppoectl pppoe0 \
	    "hisauthname=$AUTHNAME" "hisauthsecret=$SECRET" \
	    "hisauthproto=$auth"  "myauthproto=none" \
	    $server_optparam
	unset RUMP_SERVER

	export RUMP_SERVER=$CLIENT
	$inet && atf_ifconfig pppoe0 \
	    inet 0.0.0.0 0.0.0.1 down
	atf_pppoectl pppoe0 \
	    "myauthname=$AUTHNAME" "myauthsecret=$SECRET" \
	    "myauthproto=$auth" "hisauthproto=none"

	$DEBUG && rump.ifconfig
	unset RUMP_SERVER
}

pppoe_params_head()
{
	atf_set "descr" "Set and clear access concentrator name and service name"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_params_body()
{
	local dumpcmd
	local cp="LCP"

	dumpcmd="shmif_dumpbus -p - ${BUS}"
	dumpcmd="${dumpcmd} | tcpdump -n -e -r -"

	rump_server_start $SERVER netinet6 pppoe
	rump_server_start $CLIENT netinet6 pppoe

	setup_ifaces
	setup_auth_conf

	export RUMP_SERVER=$SERVER
	atf_pppoectl -e shmif0 pppoe0
	atf_ifconfig pppoe0 up
	unset RUMP_SERVER

	export RUMP_SERVER=$CLIENT
	atf_pppoectl -e shmif0 pppoe0
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	unset RUMP_SERVER

	$DEBUG && dump_bus
	atf_check -s exit:0 -o match:'\[Service-Name\]' -e ignore \
	    -x "${dumpcmd} | grep PADI"
	atf_check -s exit:0 -o match:'\[Service-Name\]' -e ignore \
	    -x "${dumpcmd} | grep PADR"
	atf_check -s exit:0 -o not-match:'AC-Name' -e ignore \
	    -x "${dumpcmd} | grep PADI"

	# set Remote access concentrator name (AC-NAME, -a option)
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	atf_pppoectl -e shmif0 -a ACNAME-TEST0 pppoe0
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	unset RUMP_SERVER

	$DEBUG && dump_bus
	atf_check -s exit:0 -o match:'\[AC-Name "ACNAME-TEST0"\]' -e ignore \
	    -x "${dumpcmd} | grep PADI"

	# change AC-NAME
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	atf_pppoectl -e shmif0 -a ACNAME-TEST1 pppoe0
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	unset RUMP_SERVER

	$DEBUG && dump_bus
	atf_check -s exit:0 -o match:'\[AC-Name "ACNAME-TEST1"\]' -e ignore \
	    -x "${dumpcmd} | grep PADI"

	# clear AC-NAME
	rump_server_destroy_ifaces
	rm ${BUS} 2> /dev/null
	setup_ifaces
	setup_auth_conf

	export RUMP_SERVER=$SERVER
	atf_pppoectl -e shmif0 pppoe0
	atf_ifconfig pppoe0 up
	unset RUMP_SERVER

	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	atf_pppoectl -a ACNAME-TEST2 -e shmif0 pppoe0
	atf_pppoectl -e shmif0 pppoe0
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	unset RUMP_SERVER

	$DEBUG && dump_bus
	atf_check -s exit:0 -o match:'\[Service-Name\]' -e ignore \
	    -x "${dumpcmd} | grep PADI"
	atf_check -s exit:0 -o match:'\[Service-Name\]' -e ignore \
	    -x "${dumpcmd} | grep PADR"
	atf_check -s exit:0 -o not-match:'AC-Name' -e ignore \
	    -x "${dumpcmd} | grep PADI"

	# store 0 length string in AC-NAME
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	atf_pppoectl -a \"\" -e shmif0 pppoe0
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	unset RUMP_SERVER

	atf_check -s exit:0 -o match:'\[AC-Name\]' -e ignore \
	    -x "${dumpcmd} | grep PADI"

	# set Service Name (Service-Name, -s option)
	rump_server_destroy_ifaces
	rm ${BUS} 2> /dev/null
	setup_ifaces
	setup_auth_conf

	export RUMP_SERVER=$SERVER
	atf_pppoectl -e shmif0 pppoe0
	atf_ifconfig pppoe0 up
	unset RUMP_SERVER

	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	atf_pppoectl -e shmif0 -s SNAME-TEST0 pppoe0
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	unset RUMP_SERVER

	$DEBUG && dump_bus
	atf_check -s exit:0 -o match:'\[Service-Name "SNAME-TEST0"\]' -e ignore \
	    -x "${dumpcmd} | grep PADI"
	atf_check -s exit:0 -o match:'\[Service-Name "SNAME-TEST0"\]' -e ignore \
	    -x "${dumpcmd} | grep PADR"
	atf_check -s exit:0 -o not-match:'AC-Name' -e ignore \
	    -x "${dumpcmd} | grep PADI"

	# change Service-Name
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	atf_pppoectl -e shmif0 -s SNAME-TEST1 pppoe0
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	unset RUMP_SERVER

	$DEBUG && dump_bus
	atf_check -s exit:0 -o match:'\[Service-Name "SNAME-TEST1"\]' -e ignore \
	    -x "${dumpcmd} | grep PADI"
	atf_check -s exit:0 -o match:'\[Service-Name "SNAME-TEST1"\]' -e ignore \
	    -x "${dumpcmd} | grep PADR"

	# clear Service-Name
	rump_server_destroy_ifaces
	rm ${BUS} 2> /dev/null
	setup_ifaces
	setup_auth_conf

	export RUMP_SERVER=$SERVER
	atf_pppoectl -e shmif0 pppoe0
	atf_ifconfig pppoe0 up
	unset RUMP_SERVER

	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	atf_pppoectl -s SNAME-TEST2 -e shmif0 pppoe0
	atf_pppoectl -e shmif0 pppoe0
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	unset RUMP_SERVER

	$DEBUG && dump_bus
	atf_check -s exit:0 -o match:'\[Service-Name\]' -e ignore \
	    -x "${dumpcmd} | grep PADI"
	atf_check -s exit:0 -o match:'\[Service-Name\]' -e ignore \
	    -x "${dumpcmd} | grep PADR"
	atf_check -s exit:0 -o not-match:'AC-Name' -e ignore \
	    -x "${dumpcmd} | grep PADI"

	# set AC-NAME and Service-Name
	rump_server_destroy_ifaces
	rm ${BUS} 2> /dev/null
	setup_ifaces
	setup_auth_conf

	export RUMP_SERVER=$SERVER
	atf_pppoectl -e shmif0 pppoe0
	atf_ifconfig pppoe0 up
	unset RUMP_SERVER

	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	atf_pppoectl -e shmif0 -a ACNAME-TEST3 -s SNAME-TEST3 pppoe0
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	unset RUMP_SERVER

	$DEBUG && dump_bus
	atf_check -s exit:0 \
	    -o match:'\[Service-Name "SNAME-TEST3"\] \[AC-Name "ACNAME-TEST3"\]' \
	    -e ignore \
	    -x "${dumpcmd} | grep PADI"
	atf_check -s exit:0 -o match:'\[Service-Name "SNAME-TEST3"\]' -e ignore \
	    -x "${dumpcmd} | grep PADR"

	# change AC-NAME
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	atf_pppoectl -e shmif0 -a ACNAME-TEST4 pppoe0
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	unset RUMP_SERVER

	$DEBUG && dump_bus
	atf_check -s exit:0 \
	    -o match:'\[Service-Name\] \[AC-Name "ACNAME-TEST4"\]' \
	    -e ignore \
	    -x "${dumpcmd} | grep PADI"
	atf_check -s exit:0 -o match:'\[Service-Name\]' -e ignore \
	    -x "${dumpcmd} | grep PADR"

	# change Service-Name
	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	wait_for_disconnected
	atf_pppoectl -e shmif0 -a ACNAME-TEST5 -s SNAME-TEST5 pppoe0
	atf_pppoectl -e shmif0 -s SNAME-TEST6 pppoe0
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	unset RUMP_SERVER

	$DEBUG && dump_bus
	atf_check -s exit:0 \
	    -o match:'\[Service-Name "SNAME-TEST6"\]' \
	    -e ignore \
	    -x "${dumpcmd} | grep PADI"
	atf_check -s exit:0 -o match:'\[Service-Name "SNAME-TEST6"\]' -e ignore \
	    -x "${dumpcmd} | grep PADR"
	atf_check -s exit:0 -o not-match:'\[AC-Name "ACNAME-TEST5\]"' -e ignore \
	    -x "${dumpcmd} | grep PADI"

	export RUMP_SERVER=$CLIENT
	atf_ifconfig pppoe0 down
	export RUMP_SERVER=$SERVER
	wait_for_disconnected

	# ipcp & ipv6cp are enabled by default
	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o match:'ipcp: enable' \
	    -x "$HIJACKING pppoectl pppoe0"
	atf_check -s exit:0 -o match:'ipv6cp: enable' \
	    -x "$HIJACKING pppoectl pppoe0"

	# ipcp enable & ipv6cp disable
	atf_pppoectl pppoe0 noipv6cp
	atf_ifconfig pppoe0 up
	wait_for_opened "IPCP"
	atf_check -s exit:0 -o match:'IPv6CP state: closed' \
	    -x "$HIJACKING pppoectl -dd pppoe0"

	atf_ifconfig pppoe0 down
	export RUMP_SERVER=$SERVER
	wait_for_disconnected

	# ipcp disable & ipv6cp enable
	export RUMP_SERVER=$CLIENT
	atf_pppoectl pppoe0 noipcp ipv6cp
	atf_ifconfig pppoe0 up
	wait_for_opened "IPv6CP"
	atf_check -s exit:0 -o match:'IPCP state: closed' \
	    -x "$HIJACKING pppoectl -dd pppoe0"
}

pppoe_params_cleanup()
{

	$DEBUG && dump
	cleanup
}

pppoe_passiveauthproto()
{
	local auth=$1
	local cp="IPCP"
	setup

	local server_optparam=""
	if [ $auth = "chap" ]; then
		server_optparam="norechallenge"
	fi

	export RUMP_SERVER=$SERVER
	atf_pppoectl pppoe0 \
	    "hisauthname=$AUTHNAME" "hisauthsecret=$SECRET" \
	    "hisauthproto=$auth" "myauthproto=none" \
	    $server_optparam
	atf_ifconfig pppoe0 up

	export RUMP_SERVER=$CLIENT
	atf_pppoectl pppoe0 \
	    "myauthname=$AUTHNAME" "myauthsecret=$SECRET" \
	    "myauthproto=none" "hisauthproto=none" \
	    "passiveauthproto"
	atf_ifconfig pppoe0 up
	$DEBUG && rump.ifconfig
	wait_for_opened $cp
	atf_ifconfig -w 10
	atf_check -s exit:0 -o ignore rump.ping -c 1 -w $TIMEOUT $SERVER_IP
}

atf_test_case pppoe_passiveauthproto_pap cleanup
pppoe_passiveauthproto_pap_head()
{

	atf_set "descr" "Test for passiveauthproto option on PAP"
	atf_set "require.progs" "rump_server"
}

pppoe_passiveauthproto_pap_body()
{

	pppoe_passiveauthproto "pap"
}

pppoe_passiveauthproto_pap_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case pppoe_passiveauthproto_chap cleanup
pppoe_passiveauthproto_chap_head()
{

	atf_set "descr" "Test for passiveauthproto option on chap"
	atf_set "require.progs" "rump_server"
}

pppoe_passiveauthproto_chap_body()
{

	pppoe_passiveauthproto "chap"
}

pppoe_passiveauthproto_chap_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case pppoe_mtu cleanup
pppoe_mtu_head()
{

	atf_set "descr" "Test for mtu"
	atf_set "require.progs" "rump_server"
}

pppoe_mtu_body()
{
	local auth=chap
	local cp="IPCP"
	setup

	export RUMP_SERVER=$SERVER
	atf_pppoectl pppoe0 \
	    "hisauthname=$AUTHNAME" "hisauthsecret=$SECRET" \
	    "hisauthproto=$auth" "myauthproto=none" \
	    norechallenge
	atf_ifconfig pppoe0 mtu 1400
	atf_ifconfig pppoe0 up

	export RUMP_SERVER=$CLIENT
	atf_pppoectl pppoe0 \
	    "myauthname=$AUTHNAME" "myauthsecret=$SECRET" \
	    "myauthproto=$auth" "hisauthproto=none"
	atf_ifconfig pppoe0 mtu 1450
	atf_ifconfig pppoe0 up

	wait_for_opened $cp
	atf_ifconfig -w 10

	export RUMP_SERVER=$SERVER
	atf_check -s exit:0 -o match:'mtu 1400' rump.ifconfig pppoe0

	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o match:'mtu 1400' rump.ifconfig pppoe0

	# mtu can set to 1460 but it is not applied.
	atf_ifconfig pppoe0 mtu 1460
	atf_check -s exit:0 -o match:'mtu 1400' rump.ifconfig pppoe0

	export RUMP_SERVER=$SERVER
	atf_ifconfig pppoe0 mtu 1470
	atf_ifconfig pppoe0 down
	atf_ifconfig pppoe0 up
	wait_for_opened $cp
	atf_ifconfig -w 10

	# mtu 1460 is applied after LCP negotiation
	atf_check -s exit:0 -o match:'mtu 1460' rump.ifconfig pppoe0

	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o match:'mtu 1460' rump.ifconfig pppoe0

	rump.ifconfig pppoe0 mtu 1500
	atf_check -s exit:0 -o ignore \
	    -e match:'SIOCSIFMTU: Invalid argument' \
	    rump.ifconfig pppoe0 mtu 1501
}

pppoe_mtu_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case pppoe_create_destroy
	atf_add_test_case pppoe_params
	atf_add_test_case pppoe_pap
	atf_add_test_case pppoe_chap
	atf_add_test_case pppoe6_pap
	atf_add_test_case pppoe6_chap
	atf_add_test_case pppoe_passiveauthproto_pap
	atf_add_test_case pppoe_passiveauthproto_chap
	atf_add_test_case pppoe_mtu
}
