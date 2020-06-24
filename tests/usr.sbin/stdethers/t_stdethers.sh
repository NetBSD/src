# $NetBSD: t_stdethers.sh,v 1.1 2020/06/24 09:47:18 jruoho Exp $
#
# Copyright (c) 2020 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Jukka Ruohonen.
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
tmp="/tmp/stdethers"

check() {

	stdethers $1 1>/dev/null 2>$tmp

	if [ ! $? -eq 0 ]; then
		atf_fail "failed to parse $1"
	fi

	if [ -s $tmp ]; then
		strerror=$(cat $tmp)
		atf_fail "$strerror"
	fi
}

clean() {

	if [ -f $tmp ]; then
		rm $tmp
	fi
}

atf_test_case default cleanup
default_head() {
	atf_require_prog stdethers
	atf_set "descr" "Test the system ethers(3) with stdethers(8)"
}

default_body() {

	if [ -f "/etc/ethers" ]; then
		check "/etc/ethers"
	fi
}

default_cleanup() {
	clean
}

atf_test_case valid cleanup
valid_head() {
	atf_require_prog stdethers
	atf_set "descr" "Test valid addresses with stdethers(8)"
}

valid_body() {
	check "$(atf_get_srcdir)/d_valid.in"
}

valid_cleanup() {
	clean
}

atf_init_test_cases() {
	atf_add_test_case default
	atf_add_test_case valid
}
