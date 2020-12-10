# $NetBSD: t_repeated_mtu.sh,v 1.2 2020/12/10 08:16:59 mrg Exp $
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

atf_test_case repeated_mtu
repeated_mtu_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Check with ifconfig(8) that setting MTUs works"
}

repeated_mtu_body() {

	if ! [ $(atf_config_get "run_unsafe" "no") = "yes" ]
	then
		atf_skip "can disrupt networking; also PR port-evbarm/55521"
	fi

	# This sequence covers both valid and invalid MTUs; we are
	# only interested in testing that the system does not hang.
	#
	mtus=$(seq -10000 1000 90000)

	for i in $(ifconfig -l); do

		mtu=$(ifconfig $i | grep "mtu" | sed "s/.*mtu //")

		if [ -z $mtu ]; then
			echo "Skipping $i"
			continue
		fi

		for j in $mtus; do
			echo "Request: ifconfig $i mtu $j"
			ifconfig $i mtu $j >/dev/null 2>&1
		done

		ifconfig $i mtu $mtu
		echo "Restored the MTU of $i to $mtu"
	done
}

atf_init_test_cases() {
	atf_add_test_case repeated_mtu
}
