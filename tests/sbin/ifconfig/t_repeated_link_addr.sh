# $NetBSD: t_repeated_link_addr.sh,v 1.4 2020/07/27 16:57:44 gson Exp $
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
atf_test_case repeated_link_addr
repeated_link_addr_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Check with ifconfig(8) that " \
		"setting link addresses for active interfaces " \
		"does not cause a panic (PR kern/41912)"
}

repeated_link_addr_body() {

	if ! [ $(atf_config_get "run_unsafe" "no") = "yes" ]
	then
		atf_skip "can disrupt networking; also PR port-evbarm/55521"
	fi

	fail=0
	addrs="00:11:00:00:00:00 \
	       00:11:11:00:00:00 \
	       00:11:11:11:00:00 \
	       00:11:11:11:11:00 \
	       00:00:11:00:00:00 \
	       00:00:11:11:00:00 \
	       00:00:11:11:11:00 \
	       00:00:00:11:00:00 \
	       00:00:00:11:11:00"

	pkill -9 hostapd
	pkill -9 wpa_supplicant

	for i in $(ifconfig -l); do

		state="up"
		addr=$(ifconfig $i | grep "address")
		addr=$(echo $addr | sed "s/address: //" | sed 's/ //')

		if [ -z "$addr" ]; then
			echo "Skipping $i"
			continue
		fi

		ifconfig -s $i

		if [ $? -eq 1 ]; then
			state="down"
			ifconfig $i up
			sleep 1
		fi

		for j in $addrs; do

			echo "Request: ifconfig $i link $j active"
			ifconfig $i link $j active

			if [ ! $? -eq 0 ]; then
				fail=1
			fi

			if [ -z "$(ifconfig $i | grep $j)" ]; then
				fail=1
			fi

			sleep 0.5
		done

		# From the ifconfig(8) manual page:
		#
		# "You may not delete the active address from an interface.
		#  You must activate some other address, first."
		#
		ifconfig $i link $addr active
		echo "Restored the link address of $i to $addr"
		sleep 0.5

		for j in $addrs; do
			echo "Request: ifconfig $i link $j delete"
			ifconfig $i link $j delete
			sleep 0.5
		done

		ifconfig $i $state
		echo "Restored state of $i to $state"
	done

	/bin/sh /etc/rc.d/hostapd restart >/dev/null 2>&1
	/bin/sh /etc/rc.d/wpa_supplicant restart >/dev/null 2>&1

	if [ $fail -eq 1 ]; then
		atf_fail "Failed to set link addresses"
	fi

	atf_pass
}

atf_init_test_cases() {
	atf_add_test_case repeated_link_addr
}
