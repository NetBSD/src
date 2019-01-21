# $NetBSD: t_sleep.sh,v 1.2 2019/01/21 13:19:18 kre Exp $
#
# Copyright (c) 2012 The NetBSD Foundation, Inc.
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

atf_test_case fraction
fraction_head() {
	atf_set "descr" "Test that sleep(1) handles " \
			"fractions of a second (PR bin/3914)"
}

fraction_body() {

	atf_check -s exit:0 -o empty -e empty -x "sleep 0.1"
	atf_check -s exit:0 -o empty -e empty -x "sleep 0.2"
	atf_check -s exit:0 -o empty -e empty -x "sleep 0.3"

	# check that '.' as the radix works, even when the
	# locale is one which uses something different
	atf_check -s exit:0 -o empty -e empty -x "LC_ALL=ru_RU.UTF-8 sleep 0.2"

	# and that it is possible to use the locale's radix char (',' here)
	atf_check -s exit:0 -o empty -e empty -x "LC_ALL=ru_RU.UTF-8 sleep 0,2"
}

atf_test_case hex
hex_head() {
	atf_set "descr" "Test that sleep(1) handles hexadecimal arguments"
}

hex_body() {

	atf_check -s exit:0 -o empty -e empty -x "sleep 0x01"
	atf_check -s exit:0 -o empty -e empty -x "sleep 0x0.F"
	atf_check -s exit:0 -o empty -e empty -x "sleep 0x.B"
}

atf_test_case nonnumeric
nonnumeric_head() {
	atf_set "descr" "Test that sleep(1) errors out with " \
			"non-numeric argument (PR bin/27140)"
}

nonnumeric_body() {

	atf_check -s not-exit:0 -o empty -e not-empty -x "sleep xyz"
	atf_check -s not-exit:0 -o empty -e not-empty -x "sleep x21"
	atf_check -s not-exit:0 -o empty -e not-empty -x "sleep  /3"
	atf_check -s not-exit:0 -o empty -e not-empty -x "sleep 3+1"
	atf_check -s not-exit:0 -o empty -e not-empty -x "sleep 0xFG"

	# This includes using an invalid radix char for the locale in use
	atf_check -s not-exit:0 -o empty -e not-empty -x "LC_ALL=C sleep 3,1"

	# no arg at all (that's non-numeric, right?)
	atf_check -s not-exit:0 -o empty -e not-empty -x "sleep"

	# and giving 2 or more args is also invalid, even if they are numeric
	atf_check -s not-exit:0 -o empty -e not-empty -x "sleep 1 2"
}

atf_init_test_cases() {

	atf_add_test_case fraction
	atf_add_test_case hex
	atf_add_test_case nonnumeric
}
