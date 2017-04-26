#	$NetBSD: t_ipsec_sysctl.sh,v 1.1.4.2 2017/04/26 02:53:34 pgoyette Exp $
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

DEBUG=${DEBUG:-false}

atf_test_case ipsec_sysctl0 cleanup
ipsec_sysctl0_head()
{

	atf_set "descr" "Tests of sysctl entries of IPsec without ipsec.so"
	atf_set "require.progs" "rump_server"
}

ipsec_sysctl0_body()
{
	local sock=unix://ipsec_sysctl

	rump_server_crypto_start $sock

	export RUMP_SERVER=$sock
	atf_check -s not-exit:0 -e match:'invalid' \
	    rump.sysctl net.inet.ipsec.enabled
	atf_check -s not-exit:0 -e match:'invalid' \
	    rump.sysctl net.inet6.ipsec6.enabled
}

ipsec_sysctl0_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case ipsec_sysctl4 cleanup
ipsec_sysctl4_head()
{

	atf_set "descr" "Tests of sysctl entries of IPsec without netinet6.so"
	atf_set "require.progs" "rump_server"
}

ipsec_sysctl4_body()
{
	local sock=unix://ipsec_sysctl

	rump_server_crypto_start $sock netipsec

	export RUMP_SERVER=$sock
	atf_check -s exit:0 -o match:'= 1' rump.sysctl net.inet.ipsec.enabled
	# net.inet6.ipsec6 entries exit regardless of netinet6
	# net.inet6.ipsec6.enabled always equals net.inet.ipsec.enabled
	atf_check -s exit:0 -o match:'= 1' rump.sysctl net.inet6.ipsec6.enabled

	atf_check -s exit:0 -o match:'= 0' rump.sysctl net.inet.ipsec.used
	# net.inet6.ipsec6.used always equals net.inet.ipsec.used
	atf_check -s exit:0 -o match:'= 0' rump.sysctl net.inet6.ipsec6.used

	# Add an SAD entry for IPv4
	atf_check -s exit:0 -o empty $HIJACKING setkey -c <<-EOF
	add 10.0.0.1 10.0.0.2 esp 9876 -E 3des-cbc "hogehogehogehogehogehoge";
	EOF
	$DEBUG && $HIJACKING setkey -D

	atf_check -s exit:0 -o match:'= 0' rump.sysctl net.inet.ipsec.used
	atf_check -s exit:0 -o match:'= 0' rump.sysctl net.inet6.ipsec6.used

	# Add an SPD entry for IPv4, which activates the IPsec function
	atf_check -s exit:0 -o empty $HIJACKING setkey -c <<-EOF
	spdadd 10.0.0.1 10.0.0.2 any -P out ipsec esp/transport//use;
	EOF
	$DEBUG && $HIJACKING setkey -D

	atf_check -s exit:0 -o match:'= 1' rump.sysctl net.inet.ipsec.used
	atf_check -s exit:0 -o match:'= 1' rump.sysctl net.inet6.ipsec6.used
}

ipsec_sysctl4_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case ipsec_sysctl6 cleanup
ipsec_sysctl6_head()
{

	atf_set "descr" "Tests of sysctl entries of IPsec"
	atf_set "require.progs" "rump_server"
}

ipsec_sysctl6_body()
{
	local sock=unix://ipsec_sysctl

	rump_server_crypto_start $sock netinet6 netipsec

	export RUMP_SERVER=$sock
	atf_check -s exit:0 -o match:'= 1' rump.sysctl net.inet.ipsec.enabled
	atf_check -s exit:0 -o match:'= 1' rump.sysctl net.inet6.ipsec6.enabled

	atf_check -s exit:0 -o match:'= 0' rump.sysctl net.inet.ipsec.used
	atf_check -s exit:0 -o match:'= 0' rump.sysctl net.inet6.ipsec6.used

	# Add an SAD entry for IPv6
	atf_check -s exit:0 -o empty $HIJACKING setkey -c <<-EOF
	add fd00::1 fd00::2 esp 9876 -E 3des-cbc "hogehogehogehogehogehoge";
	EOF
	$DEBUG && $HIJACKING setkey -D

	atf_check -s exit:0 -o match:'= 0' rump.sysctl net.inet.ipsec.used
	atf_check -s exit:0 -o match:'= 0' rump.sysctl net.inet6.ipsec6.used

	# Add an SPD entry for IPv6, which activates the IPsec function
	atf_check -s exit:0 -o empty $HIJACKING setkey -c <<-EOF
	spdadd fd00::1 fd00::2 any -P out ipsec esp/transport//use;
	EOF
	$DEBUG && $HIJACKING setkey -D

	atf_check -s exit:0 -o match:'= 1' rump.sysctl net.inet.ipsec.used
	atf_check -s exit:0 -o match:'= 1' rump.sysctl net.inet6.ipsec6.used
}

ipsec_sysctl6_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case ipsec_sysctl0
	atf_add_test_case ipsec_sysctl4
	atf_add_test_case ipsec_sysctl6
}
