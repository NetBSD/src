
#	$NetBSD: t_pppoe_tags.sh,v 1.1 2026/06/10 05:22:57 yamaguchi Exp $
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
DUMP_CMD="shmif_dumpbus -p - ${BUS}"
DUMP_CMD="${DUMP_CMD} | tcpdump -n -e -r -"

atf_test_case pppoe_acname cleanup

pppoe_acname_head()
{
	atf_set "descr" \
	    "Test for Access Concentrator Name tag (AC-Name tag)"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_acname_body()
{
	setup_pppoe_server_client $SERVER $CLIENT $BUS
	setup_auth_params chap $AUTHNAME $SECRET
	setup_ipcp_addrs $SERVER_IP $CLIENT_IP

	# test adding AC-Name
	export RUMP_SERVER=$CLIENT
	atf_pppoectl -e shmif0 -a ACNAME-TEST0 pppoe0

	pppoe_connect
	pppoe_disconnect_by_client
	$DEBUG && dump_bus
	atf_check -s exit:0 -o match:'\[AC-Name "ACNAME-TEST0"\]' -e ignore \
	    -x "$DUMP_CMD | grep PADI"

	# test changing AC-Name
	export RUMP_SERVER=$CLIENT
	atf_pppoectl -e shmif0 -a ACNAME-TEST1 pppoe0

	pppoe_connect
	pppoe_disconnect_by_client
	$DEBUG && dump_bus
	atf_check -s exit:0 -o match:'\[AC-Name "ACNAME-TEST1"\]' -e ignore \
	    -x "$DUMP_CMD | grep PADI"

	# test clearing AC-Name
	export RUMP_SERVER=$CLIENT
	atf_pppoectl -e shmif0 pppoe0

	cleanup_bus
	$DEBUG && dump_bus

	pppoe_connect
	pppoe_disconnect_by_client

	atf_check -s exit:0 -o not-match:'AC-Name' -e ignore \
	    -x "$DUMP_CMD | grep PADI"

	# test of 0 length AC-Name
	export RUMP_SERVER=$CLIENT
	atf_pppoectl -a \"\" -e shmif0 pppoe0

	pppoe_connect
	pppoe_disconnect_by_client

	atf_check -s exit:0 -o match:'\[AC-Name\]' -e ignore \
	    -x "$DUMP_CMD | grep PADI"
}

pppoe_acname_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case pppoe_servicename cleanup
pppoe_servicename_head()
{

	atf_set "descr" \
	    "Test for Service-Name tag"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_servicename_body()
{
	setup_pppoe_server_client $SERVER $CLIENT $BUS
	setup_auth_params chap $AUTHNAME $SECRET
	setup_ipcp_addrs $SERVER_IP $CLIENT_IP

	#
	# test adding Service-Name
	#
	echo "Test adding Service-Name"
	export RUMP_SERVER=$CLIENT
	atf_pppoectl -e shmif0 -s SNAME-TEST0 pppoe0

	pppoe_connect
	pppoe_disconnect_by_client

	$DEBUG && dump_bus
	atf_check -s exit:0 -o match:'\[Service-Name "SNAME-TEST0"\]' -e ignore \
	    -x "$DUMP_CMD | grep PADI"
	atf_check -s exit:0 -o match:'\[Service-Name "SNAME-TEST0"\]' -e ignore \
	    -x "$DUMP_CMD | grep PADR"
	atf_check -s exit:0 -o not-match:'AC-Name' -e ignore \
	    -x "$DUMP_CMD | grep PADI"

	#
	# test changing Service-Name
	#
	echo "Test changing Service-Name"
	export RUMP_SERVER=$CLIENT
	atf_pppoectl -e shmif0 -s SNAME-TEST1 pppoe0

	pppoe_connect
	pppoe_disconnect_by_client

	$DEBUG && dump_bus
	atf_check -s exit:0 -o match:'\[Service-Name "SNAME-TEST1"\]' -e ignore \
	    -x "$DUMP_CMD | grep PADI"
	atf_check -s exit:0 -o match:'\[Service-Name "SNAME-TEST1"\]' -e ignore \
	    -x "$DUMP_CMD | grep PADR"

	#
	# Test clearing Service-Name
	# 
	echo "Test clearing Service-Name"
	export RUMP_SERVER=$CLIENT
	atf_pppoectl -e shmif0 pppoe0

	cleanup_bus
	$DEBUG && dump_bus

	pppoe_connect
	pppoe_disconnect_by_client

	$DEBUG && dump_bus
	atf_check -s exit:0 -o match:'\[Service-Name\]' -e ignore \
	    -x "$DUMP_CMD | grep PADI"
	atf_check -s exit:0 -o match:'\[Service-Name\]' -e ignore \
	    -x "$DUMP_CMD | grep PADR"
	atf_check -s exit:0 -o not-match:'AC-Name' -e ignore \
	    -x "$DUMP_CMD | grep PADI"
}

pppoe_servicename_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case pppoe_tags cleanup
pppoe_tags_head()
{

	atf_set "descr" \
	    "Test with both AC-Name and Service-Name"
	atf_set "require.progs" "rump_server pppoectl"
}

pppoe_tags_body()
{
	setup_pppoe_server_client $SERVER $CLIENT $BUS
	setup_auth_params chap $AUTHNAME $SECRET
	setup_ipcp_addrs $SERVER_IP $CLIENT_IP

	#
	# test of default settings
	#
	echo "Test of default settings"
	pppoe_connect
	pppoe_disconnect_by_client

	$DEBUG && dump_bus
	atf_check -s exit:0 -o match:'\[Service-Name\]' -e ignore \
	    -x "$DUMP_CMD | grep PADI"
	atf_check -s exit:0 -o match:'\[Service-Name\]' -e ignore \
	    -x "$DUMP_CMD | grep PADR"
	atf_check -s exit:0 -o not-match:'AC-Name' -e ignore \
	    -x "$DUMP_CMD | grep PADI"

	#
	# test with both AC-Name and Service-Name configured
	#
	echo "Test with both AC-Name and Service-Name configured"
	export RUMP_SERVER=$CLIENT
	atf_pppoectl -e shmif0 -a ACNAME-TEST1 -s SNAME-TEST1 pppoe0

	pppoe_connect
	pppoe_disconnect_by_client

	$DEBUG && dump_bus
	atf_check -s exit:0 \
	    -o match:'\[Service-Name "SNAME-TEST1"\] \[AC-Name "ACNAME-TEST1"\]' \
	    -e ignore \
	    -x "$DUMP_CMD | grep PADI"
	atf_check -s exit:0 -o match:'\[Service-Name "SNAME-TEST1"\]' -e ignore \
	    -x "$DUMP_CMD | grep PADR"

	# test for change of AC-Name and clear Service-Name
	export RUMP_SERVER=$CLIENT
	atf_pppoectl -e shmif0 -a ACNAME-TEST4 pppoe0

	#
	# test clearing both AC-Name and Service-Name
	#
	echo "Test clearing both AC-Name and Service-Name"
}

pppoe_tags_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case pppoe_tags
	atf_add_test_case pppoe_acname
	atf_add_test_case pppoe_servicename
}
