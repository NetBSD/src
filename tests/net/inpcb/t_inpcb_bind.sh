#	$NetBSD: t_inpcb_bind.sh,v 1.1 2022/11/17 08:40:23 ozaki-r Exp $
#
# Copyright (c) 2022 Internet Initiative Japan Inc.
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

SOCK=unix://inpcb_bind
BUS=./bus

DEBUG=${DEBUG:-false}
NAME="inpcb_bind"

test_inpcb_bind_ipv4()
{
	local addr="10.0.0.10"
	local port=23000
	local prog="$(atf_get_srcdir)/inpcb_bind"

	rump_server_start $SOCK
	rump_server_add_iface $SOCK shmif0 $BUS

	export RUMP_SERVER=$SOCK
	atf_check -s exit:0 rump.ifconfig shmif0 $addr/24
	atf_check -s exit:0 -o ignore $HIJACKING $prog $addr $port
	atf_check -s exit:0 -o ignore $HIJACKING $prog 224.0.2.1 $port $addr
}

test_inpcb_bind_ipv6()
{
	local addr="fc00::10"
	local port=23000
	local prog="$(atf_get_srcdir)/inpcb_bind"

	rump_server_start $SOCK netinet6
	rump_server_add_iface $SOCK shmif0 $BUS

	export RUMP_SERVER=$SOCK
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $addr/64
	atf_check -s exit:0 -o ignore $HIJACKING $prog $addr $port
	atf_check -s exit:0 -o ignore $HIJACKING $prog ff1e::123 $port $addr
}

add_test()
{
	local name="${NAME}_$1"
	local desc="$2"

	atf_test_case "${name}" cleanup
	eval "${name}_head() {
			atf_set descr \"${desc}\"
			atf_set require.progs rump_server
		}
	    ${name}_body() {
			test_${name}
		}
	    ${name}_cleanup() {
			\$DEBUG && dump
			cleanup
		}"
	atf_add_test_case "${name}"
}

atf_init_test_cases()
{

	add_test ipv4	"tests for inpcb_bind (ipv4)"
	add_test ipv6	"tests for inpcb_bind (ipv6)"
}
