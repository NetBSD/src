#! /usr/bin/atf-sh
# $NetBSD: t_klua_pr_52864.sh,v 1.1 2018/01/08 14:17:15 martin Exp $
#
# Copyright (c) 2018 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Sevan Janiyan
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

atf_test_case luastate cleanup
luastate_head() {
	atf_set "descr" "Test that once lua(4) is loaded and a state is " \
			"created, system does not crash when listing states" \
			"(PR port-macppc/52864)"
	atf_set "require.user" "root"
}

luastate_body() {
	sysctl -q kern.lua.verbose
	if [ $? -eq 1 ]; then
	atf_check -s eq:0 modload lua
	fi
	atf_check -s eq:0 luactl -q create atfluastate
	atf_check -s eq:0 -o ignore luactl
}

luastate_cleanup() {
	modunload lua >/dev/null 2>&1
}

atf_init_test_cases() {
	atf_add_test_case luastate
}
