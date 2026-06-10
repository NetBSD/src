#	$NetBSD: t_pppoe_ipcp.sh,v 1.1 2026/06/10 05:22:57 yamaguchi Exp $
#
# Copyright (c) Internet Initiative Japan Inc.
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
AUTHNAME=foobar@baz.com
SECRET=oink
BUS=bus0
TIMEOUT=3
DEBUG=${DEBUG:-false}

run_test()
{
	local auth=$1

	setup_pppoe_server_client $SERVER $CLIENT $BUS
	setup_auth_params $auth $AUTHNAME $SECRET
	setup_ipcp_addrs $SERVER_IP $CLIENT_IP

	#
	# Connect-Disconnect test
	#
	echo "Connect-Disconnect test"

	pppoe_connect
	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o ignore rump.ping -c 1 -w $TIMEOUT $SERVER_IP
	pppoe_disconnect_by_client
	export RUMP_SERVER=$CLIENT
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP

	pppoe_connect
	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o ignore rump.ping -c 1 -w $TIMEOUT $SERVER_IP
	pppoe_disconnect_by_server
	export RUMP_SERVER=$CLIENT
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping -c 1 -w $TIMEOUT $SERVER_IP

	#
	# test of invalid authname
	#
	echo "Test of invalid authname"

	export RUMP_SERVER=$CLIENT
	atf_pppoectl pppoe0 "myauthproto=$auth" \
			    "myauthname=invalid-name" \
			    "myauthsecret=$SECRET" \
			    "hisauthproto=none" \
			    "max-auth-failure=1"

	pppoe_connect "expected-failure"
	pppoe_disconnect_by_client

	setup_auth_params $auth $AUTHNAME $SECRET
	pppoe_connect

	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o ignore rump.ping -c 1 -w $TIMEOUT $SERVER_IP

	pppoe_disconnect_by_client

	#
	# test of invalid secret
	#
	echo "Test of invalid secret"

	export RUMP_SERVER=$CLIENT
	atf_pppoectl pppoe0 "myauthproto=$auth" \
			    "myauthname=$AUTHNAME" \
			    "myauthsecret=invalid-secret" \
			    "hisauthproto=none" \
			    "max-auth-failure=1"

	pppoe_connect "expected-failure"
	pppoe_disconnect_by_client

	setup_auth_params $auth $AUTHNAME $SECRET
	pppoe_connect

	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o ignore rump.ping -c 1 -w $TIMEOUT $SERVER_IP

	pppoe_disconnect_by_client
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

pppoe_passiveauthproto()
{
	local auth=$1

	setup_pppoe_server_client $SERVER $CLIENT $BUS
	setup_auth_params $auth $AUTHNAME $SECRET
	setup_ipcp_addrs   $SERVER_IP $CLIENT_IP

	export RUMP_SERVER=$CLIENT
	atf_pppoectl pppoe0 \
	    "myauthname=$AUTHNAME" "myauthsecret=$SECRET" \
	    "myauthproto=none" "hisauthproto=none" \
	    "passiveauthproto"

	pppoe_connect

	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o ignore rump.ping -c 1 -w $TIMEOUT $SERVER_IP
}

atf_test_case pppoe_passiveauthproto_pap cleanup
pppoe_passiveauthproto_pap_head()
{

	atf_set "descr" "Test for 'passiveauthproto' option with PAP"
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

	atf_set "descr" "Test for 'passiveauthproto' option with CHAP"
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

atf_init_test_cases()
{

	atf_add_test_case pppoe_pap
	atf_add_test_case pppoe_chap
	atf_add_test_case pppoe_passiveauthproto_pap
	atf_add_test_case pppoe_passiveauthproto_chap
}
