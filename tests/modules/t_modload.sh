# $NetBSD: t_modload.sh,v 1.3 2008/05/02 14:20:50 ad Exp $
#
# Copyright (c) 2008 The NetBSD Foundation, Inc.
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

check_sysctl() {
	echo "${1} = ${2}" >expout
	atf_check "sysctl ${1}" 0 expout null
}

atf_test_case plain
plain_head() {
	atf_set "descr" "Test load without arguments"
	atf_set "require.user" "root"
}
plain_body() {
	cat >experr <<EOF
modload: No such file or directory
EOF
	atf_check "modload non-existent.o" 1 null experr

	atf_check "modload $(atf_get_srcdir)/k_helper.kmod" 0 null null
	check_sysctl vendor.k_helper.present 1
	check_sysctl vendor.k_helper.prop_int_ok 0
	check_sysctl vendor.k_helper.prop_str_ok 0
	atf_check "modunload k_helper" 0 null null
}
plain_cleanup() {
	modunload k_helper >/dev/null 2>&1
}

atf_test_case bflag
bflag_head() {
	atf_set "descr" "Test the -b flag"
	atf_set "require.user" "root"
}
bflag_body() {
	echo "Checking error conditions"

	atf_check "modload -b foo k_helper.kmod" 1 null stderr
	atf_check "grep 'Invalid parameter.*foo' stderr" 0 ignore null

	atf_check "modload -b foo= k_helper.kmod" 1 null stderr
	atf_check "grep 'Invalid boolean value' stderr" 0 ignore null

	atf_check "modload -b foo=bar k_helper.kmod" 1 null stderr
	atf_check "grep 'Invalid boolean value.*bar' stderr" 0 ignore null

	atf_check "modload -b foo=falsea k_helper.kmod" 1 null stderr
	atf_check "grep 'Invalid boolean value.*falsea' stderr" 0 ignore null

	atf_check "modload -b foo=truea k_helper.kmod" 1 null stderr
	atf_check "grep 'Invalid boolean value.*truea' stderr" 0 ignore null

	# TODO Once sysctl(8) supports CTLTYPE_BOOL nodes.
	#echo "Checking valid values"
}
bflag_cleanup() {
	modunload k_helper >/dev/null 2>&1
}

atf_test_case iflag
iflag_head() {
	atf_set "descr" "Test the -i flag"
	atf_set "require.user" "root"
}
iflag_body() {
	echo "Checking error conditions"

	atf_check "modload -i foo k_helper.kmod" 1 null stderr
	atf_check "grep 'Invalid parameter.*foo' stderr" 0 ignore null

	atf_check "modload -i foo= k_helper.kmod" 1 null stderr
	atf_check "grep 'Invalid integer value' stderr" 0 ignore null

	atf_check "modload -i foo=bar k_helper.kmod" 1 null stderr
	atf_check "grep 'Invalid integer value.*bar' stderr" 0 ignore null

	atf_check "modload -i foo=123a k_helper.kmod" 1 null stderr
	atf_check "grep 'Invalid integer value.*123a' stderr" 0 ignore null

	echo "Checking valid values"

	for v in 5 10; do
		atf_check "modload -i prop_int='${v}' \
			   $(atf_get_srcdir)/k_helper.kmod" 0 null null
		check_sysctl vendor.k_helper.prop_int_ok 1
		check_sysctl vendor.k_helper.prop_int_val "${v}"
		atf_check "modunload k_helper" 0 null null
	done
}
iflag_cleanup() {
	modunload k_helper >/dev/null 2>&1
}

atf_test_case sflag
sflag_head() {
	atf_set "descr" "Test the -s flag"
	atf_set "require.user" "root"
}
sflag_body() {
	echo "Checking error conditions"

	atf_check "modload -s foo k_helper.kmod" 1 null stderr
	atf_check "grep 'Invalid parameter.*foo' stderr" 0 ignore null

	echo "Checking valid values"

	for v in '1st string' '2nd string'; do
		atf_check "modload -s prop_str='${v}' \
			   $(atf_get_srcdir)/k_helper.kmod" 0 null null
		check_sysctl vendor.k_helper.prop_str_ok 1
		check_sysctl vendor.k_helper.prop_str_val "${v}"
		atf_check "modunload k_helper" 0 null null
	done
}
sflag_cleanup() {
	modunload k_helper >/dev/null 2>&1
}

atf_init_test_cases()
{
	atf_add_test_case plain
	atf_add_test_case bflag
	atf_add_test_case iflag
	atf_add_test_case sflag
}
