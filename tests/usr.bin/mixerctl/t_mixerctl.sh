# $NetBSD: t_mixerctl.sh,v 1.12 2022/08/10 00:14:22 charlotte Exp $
#
# Copyright (c) 2017 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Charlotte Koch.
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

audio_setup() {
	# Open /dev/pad0 so we have a configured audio device.
	# Do it in a way that guarantees the open happens before
	# we proceed to the (</dev/mixer) below (do not expect
	# cat to be running in time to open the device.)

	# Note: it is not important that the open of pad0 succeeds,
	# if there is real audio hardware on the system, that can (will)
	# be used instead, and having pad0 open is irrelevant.
	# So, no errors reported if pad0 open fails.  If there turns
	# out to be no audio device of any kind, then the open of the
	# mixer will fail, causing the test to be skipped.

	# Saving padpid and later killing it seems to be unnecessary,
	# ATF appears to killpg() the process after the test finishes
	# which is a good thing, otherwise a test that is skipped/fails
	# would not kill the cat (doing it in a cleanup function is not
	# convenient as it is a different execution environment, no shared
	# variables, we would need to put $padpid in a file.)

	unset padpid
	( true </dev/pad0 ) >/dev/null 2>&1 &&
	    { { cat >/dev/null & } < /dev/pad0 ; } 2>/dev/null && padpid=$!

	(</dev/mixer) >/dev/null 2>&1 ||
	    atf_skip "no audio mixer available in kernel"
}

atf_test_case noargs_usage
noargs_usage_head() {
	atf_set "descr" "Ensure mixerctl(1) with no args prints a usage message"
}
noargs_usage_body() {
	audio_setup

	atf_check -s exit:1 -o empty -e not-empty \
		mixerctl

	${padpid+kill -HUP ${padpid}} 2>/dev/null || :
}

atf_test_case showvalue
showvalue_head() {
	atf_set "descr" "Ensure mixerctl(1) can print the value for all variables"
}
showvalue_body() {
	audio_setup

	for var in $(mixerctl -a | awk -F= '{print $1}'); do
		atf_check -s exit:0 -e ignore -o match:"^${var}=" \
			mixerctl ${var}
	done

	${padpid+kill -HUP ${padpid}} 2>/dev/null || :
}

atf_test_case nflag
nflag_head() {
	atf_set "descr" "Ensure 'mixerctl -n' actually suppresses some output"
}
nflag_body() {
	audio_setup

	varname="$(mixerctl -a | sed -e 's/=.*//' -e q)"

	atf_check -s exit:0 -o match:"${varname}" -e ignore \
		mixerctl ${varname}

	atf_check -s exit:0 -o not-match:"${varname}" -e ignore \
		mixerctl -n ${varname}

	${padpid+kill -HUP ${padpid}} 2>/dev/null || :
}

atf_test_case nonexistant_device
nonexistant_device_head() {
	atf_set "descr" "Ensure mixerctl(1) complains if provided a nonexistant mixer device"
}
nonexistant_device_body() {
	atf_check -s not-exit:0  -o ignore -e match:"No such file" \
		mixerctl -a -d /a/b/c/d/e
}

atf_init_test_cases() {
	atf_add_test_case noargs_usage
	atf_add_test_case showvalue
	atf_add_test_case nflag
	atf_add_test_case nonexistant_device
}
