#       $NetBSD: t_ipsec_spflags.sh,v 1.1 2022/10/11 09:55:21 knakahara Exp $
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

SOCK_LOCAL=unix://ipsec_local

DEBUG=${DEBUG:-false}

test_flag_if_ipsec_sp_common()
{
	local ip_gwlo_tun=20.0.0.1
	local ip_gwre_tun=20.0.0.2

	rump_server_crypto_start $SOCK_LOCAL netipsec ipsec
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.ifconfig ipsec0 create
	atf_check -s exit:0 rump.ifconfig ipsec0 \
	    tunnel $ip_gwlo_tun $ip_gwre_tun
	atf_check -s exit:0 -o match:'sadb_x_policy\{ type=2 dir=2 flags=0x80' $HIJACKING setkey -DPv
}

test_flag_userland_sp_common()
{
	local ip_gwlo_tun=20.0.0.1
	local ip_gwre_tun=20.0.0.2
	local tmpfile=./tmp

	name="ipsec_spflag_userland_sp"
	desc="Tests of IPsec SPD flags at userland"

	atf_test_case ${name} cleanup

	rump_server_crypto_start $SOCK_LOCAL netipsec ipsec

	export RUMP_SERVER=$SOCK_LOCAL

	cat > $tmpfile <<-EOF
	spdadd $ip_gwlo_tun $ip_gwre_tun ipv4 -P in ipsec esp/transport//require ;
	spdadd $ip_gwre_tun $ip_gwlo_tun ipv4 -P out ipsec esp/transport//require ;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	atf_check -s exit:0 -o match:'sadb_x_policy\{ type=2 dir=2 flags=0x00' $HIJACKING setkey -DPv
}

add_test_spflag()
{
	local name=$1
	local desc=$2

	atf_test_case ${name} cleanup
	eval "								\
	    ${name}_head() {						\
	        atf_set \"descr\" \"$desc\";				\
	        atf_set \"require.progs\" \"rump_server\" \"setkey\";	\
	    };								\
	    ${name}_body() {						\
	        test_${name}_common;					\
	    };        							\
	    ${name}_cleanup() {						\
	        $DEBUG && dump;						\
	        cleanup;						\
	    }								\
	"
	atf_add_test_case ${name}

}

atf_init_test_cases()
{

	add_test_spflag "flag_if_ipsec_sp" "Tests of IPsec SPD flags at IPsec interface"
	add_test_spflag "flag_userland_sp" "Tests of IPsec SPD flags at userland"
}
