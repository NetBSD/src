# $NetBSD: t_execsnoop.sh,v 1.3 2020/07/11 09:55:26 jruoho Exp $
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
tmp="/tmp/execsnoop"

atf_test_case basic cleanup
basic_head() {
	atf_set "require.user" "root"
	atf_set "require.progs" "execsnoop"
	atf_set "descr" "Test that DTrace's execsnoop works (cf. kern/53417)"
}

basic_body() {

	n=10
	atf_check -s exit:0 -o ignore -e empty -x "execsnoop > $tmp &"
	sleep 1

	while [ $n -gt 0 ]; do
		whoami
		n=$(expr $n - 1)
	done

	sleep 5
	pkill -9 execsnoop
	sleep 1

	if [ ! $(cat $tmp | grep "whoami" | wc -l) -eq 10 ]; then
		atf_fail "execsnoop does not work"
	fi

	atf_pass
}

basic_cleanup() {

	if [ -f $tmp ]; then
		rm $tmp
	fi
}

atf_init_test_cases() {
	atf_add_test_case basic
}
