#	$NetBSD: t_tcp_shutdown.sh,v 1.1 2022/11/04 08:01:42 ozaki-r Exp $
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

SOCK=unix://tcp_shutdown
BUS=./bus_tcp_shutdown

DEBUG=${DEBUG:-false}

setup_servers()
{

	rump_server_start $SOCK
	rump_server_add_iface $SOCK shmif0 $BUS
}

test_tcp_shutdown()
{
	local target=$1
	local prog="$(atf_get_srcdir)/tcp_shutdown"

	export RUMP_SERVER=$SOCK
	atf_check -s exit:0 $HIJACKING $prog $target
}

add_test_tcp_shutdown()
{
	local target=$1
	local name= desc=

	desc="Test TCP; call $target after shtudown"
	name="tcp_shutdown_${target}"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server
	    }
	    ${name}_body() {
	        setup_servers
	        test_tcp_shutdown $target
	        rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
	        \$DEBUG && dump
	        cleanup
	    }
	"
	atf_add_test_case ${name}
}

atf_init_test_cases()
{

	add_test_tcp_shutdown bind
	add_test_tcp_shutdown connect
	add_test_tcp_shutdown getsockname
	add_test_tcp_shutdown listen
	add_test_tcp_shutdown setsockopt
	add_test_tcp_shutdown shutdown
}
