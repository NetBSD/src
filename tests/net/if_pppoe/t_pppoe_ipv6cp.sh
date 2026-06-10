#	$NetBSD: t_pppoe_ipv6cp.sh,v 1.1 2026/06/10 05:22:57 yamaguchi Exp $
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
SERVER_IP6=fc00::1
CLIENT_IP6=fc00::3
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
	setup_ipv6cp_addrs $SERVER_IP6 $CLIENT_IP6

	#
	# Connect-Disconnect test
	#
	echo "Connect-Disconnect test"

	pppoe_connect
	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -X $TIMEOUT $SERVER_IP6
	pppoe_disconnect_by_client
	export RUMP_SERVER=$CLIENT
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping6 -c 1 -X $TIMEOUT $SERVER_IP6

	pppoe_connect
	export RUMP_SERVER=$CLIENT
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -X $TIMEOUT $SERVER_IP6
	pppoe_disconnect_by_server
	export RUMP_SERVER=$CLIENT
	atf_check -s not-exit:0 -o ignore -e ignore \
	    rump.ping6 -c 1 -X $TIMEOUT $SERVER_IP6

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
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -X $TIMEOUT $SERVER_IP6

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
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -X $TIMEOUT $SERVER_IP6

	pppoe_disconnect_by_client
}

atf_test_case pppoe_pap6 cleanup

pppoe_pap6_head()
{
	atf_set "descr" "Does simple pap tests"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_pap6_body()
{

	run_test pap
}

pppoe_pap6_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case pppoe_chap6 cleanup

pppoe_chap6_head()
{
	atf_set "descr" "Does simple chap tests"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_chap6_body()
{
	run_test chap
}

pppoe_chap6_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case pppoe_pap6
	atf_add_test_case pppoe_chap6
}
