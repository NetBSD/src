#	$NetBSD: t_broadcast_bind.sh,v 1.1 2022/11/17 08:42:56 ozaki-r Exp $
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

SOCK=unix://broadcast_bind
BUS=./bus

DEBUG=${DEBUG:-false}
NAME="broadcast_bind"

test_broadcast_bind_basic()
{
	#local insfx=$(jot -s '.' -r 2 0 255)
	local insfx="0.0"
	local inaddr="10.$insfx.10"
	local badaddr="10.$insfx.22"
	local bcaddr="10.$insfx.255"
	local prog="$(atf_get_srcdir)/broadcast_bind"

	rump_server_start $SOCK
	rump_server_add_iface $SOCK shmif0 $BUS

	export RUMP_SERVER=$SOCK
	atf_check -s exit:0 rump.ifconfig shmif0 $inaddr/24
	atf_check -s exit:0 $HIJACKING $prog $inaddr $badaddr $bcaddr
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

	add_test basic	"bind(2)ing with broadcast and inexistant addresses"
}
