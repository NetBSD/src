# $NetBSD: t_woptions.sh,v 1.2 2020/09/08 06:11:32 mrg Exp $
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

# These tests play around with the wifi configuration on a system
# which may not be safe, destroy configuration or hang.
check_ifconfig_tests_enabled() {
	if [ "${ATF_SBIN_IFCONFIG_WIFI_ENABLE}" != "yes" ]; then
		atf_skip "Test triggers real device activity and may destroy configuration or hang."
	fi
}

atf_test_case chan
chan_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Test with ifconfig(8) that setting " \
		"802.11 channels does not panic (PR kern/55424)"
}

chan_body() {
	check_ifconfig_tests_enabled

	# This sequence covers both valid and invalid channels.
	# Different 802.11 modes are not taken into account, and
	# all interfaces are tested, including non-802.11 ones.
	#
	chans=$(seq 1 500)

	pkill -9 hostapd
	pkill -9 wpa_supplicant

	for i in $(ifconfig -l); do

		if [ ! -z "$(echo $i | grep urtwn)" ]; then
			echo "Skipping $i (PR kern/55424)"
			continue
		fi

		state="up"
		ifconfig -s $i

		if [ $? -eq 1 ]; then
			state="down"
		fi

		m=""
		mm=$(ifconfig $i | grep "chan")

		if [ ! -z "$mm" ]; then
			m=$(echo $mm | awk {'print $2'})
		fi

		for j in $chans; do
			echo "Request: $i -> 802.11 chan $j"
			ifconfig $i chan $j >/dev/null 2>&1
			echo "Request: $i -> 802.11 -chan $j"
			ifconfig $i -chan $j >/dev/null 2>&1
		done

		if [ ! -z $m ]; then
			ifconfig $i chan $m >/dev/null 2>&1
			echo "Restored the channel of $i to $m"
		fi

		ifconfig $i $state
		echo "Restored state of $i to $state"
		sleep 1
	done

	/bin/sh /etc/rc.d/hostapd restart >/dev/null 2>&1
	/bin/sh /etc/rc.d/wpa_supplicant restart >/dev/null 2>&1

	atf_pass
}

atf_test_case mediaopt
mediaopt_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Test with ifconfig(8) that setting " \
		"802.11 media options does not panic (PR kern/35045)"
}

mediaopt_body() {
	check_ifconfig_tests_enabled

	# Again, also non-802.11 interfaces are tested.
	#
	opts="adhoc monitor hostap"

	pkill -9 hostapd
	pkill -9 wpa_supplicant

	for i in $(ifconfig -l); do

		state="up"
		ifconfig -s $i

		if [ $? -eq 1 ]; then
			state="down"
		fi

		m=""
		mm=$(ifconfig $i | grep "media")

		for j in $opts; do

			match=$(echo $mm | grep $j)

			if [ ! -z "$match" ]; then
				m=$j
				break
			fi
		done

		for j in $opts; do
			echo "Request: $i -> 802.11 mediaopt $j"
			ifconfig $i mediaopt $j >/dev/null 2>&1
			echo "Request: $i -> 802.11 -mediaopt $j"
			ifconfig $i -mediaopt $j >/dev/null 2>&1
		done

		if [ ! -z $m ]; then
			ifconfig $i mode $m >/dev/null 2>&1
			echo "Restored the mediaopt of $i to $m"
		fi

		ifconfig $i $state
		echo "Restored state of $i to $state"
		sleep 1
	done

	/bin/sh /etc/rc.d/hostapd restart >/dev/null 2>&1
	/bin/sh /etc/rc.d/wpa_supplicant restart >/dev/null 2>&1

	atf_pass
}

atf_test_case modes
modes_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Test with ifconfig(8) that setting " \
		"802.11 modes does not panic (PR kern/45745)"
}

modes_body() {
	check_ifconfig_tests_enabled

	# Although 11n is not yet supported, the system
	# should not panic from invalid input parameters.
	# Therefore, the following will try to also set
	# the four 802.11 modes for non-802.11 devices.
	#
	modes="11a 11b 11g 11n"

	pkill -9 hostapd
	pkill -9 wpa_supplicant

	for i in $(ifconfig -l); do

		m=""
		mm=$(ifconfig $i | grep "media")

		for j in $modes; do

			match=$(echo $mm | grep $j)

			if [ ! -z "$match" ]; then
				m=$j
				break
			fi
		done

		for j in $modes; do
			echo "Request: $i -> 802.11 mode $j"
			ifconfig $i mode $j >/dev/null 2>&1
		done

		if [ ! -z $m ]; then
			ifconfig $i mode $m >/dev/null 2>&1
			echo "Restored the mode of $i to $m"
		fi
	done

	/bin/sh /etc/rc.d/hostapd restart >/dev/null 2>&1
	/bin/sh /etc/rc.d/wpa_supplicant restart >/dev/null 2>&1

	atf_pass
}

atf_init_test_cases() {
	atf_add_test_case chan
	atf_add_test_case mediaopt
	atf_add_test_case modes
}
