# $NetBSD: t_random_garbage.sh,v 1.4 2020/07/27 07:36:19 jruoho Exp $
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
getrint() {
	echo $(od -An -N2 -i /dev/urandom | sed 's/ //')
}

getrstr() {
	echo $(cat /dev/urandom | head -n 1 | base64)
}

write_garbage() {
	val=$(getrint)
	echo "Test $3: write to $1 opt $2 -> $val"
	ifconfig $1 $2 $val >/dev/null 2>&1
	val=$(getrstr)
	echo "Test $3: write to $1 opt $2 -> $val"
	ifconfig $1 $2 $val >/dev/null 2>&1
}

atf_test_case random_garbage
random_garbage_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Test writing random garbage to " \
		"ifconfig(8) options (PR kern/55451)"
}

random_garbage_body() {

	# Please note:
	#
	# 1. As some drivers seem to have input validation issues, it may
	#    be possible that horrors are written directly to the hardware.
	#
	# 2. Even if the test passes, there is no easy way to restore
	#    the existing state/configuration of any given interface.
	#
	# Take care.
	#
	if ! [ $(atf_config_get "run_unsafe" "no") = "yes" ]; then
		atf_skip "The test is not safe (PR kern/55451)"
	fi

	opts="advbase advskew broadcast carpdev description \
	      media mediaopt -mediaopt mode instance metric mtu \
	      netmask state frag rts ssid nwid nwkey pass powersavesleep \
	      bssid chan tunnel session cookie pltime prefixlen linkstr \
	      vhid vlan vlanif -vlanif agrport -agrport vltime maxupd \
	      syncdev syncpeer"

	for i in $(ifconfig -l); do

		ifconfig $i up

		for o in $opts; do

			n=10

			while [ $n -gt 0 ]; do
				write_garbage $i $o $n
				n=$(expr $n - 1)
			done
		done
	done

	atf_pass
}

atf_init_test_cases() {
	atf_add_test_case random_garbage
}
