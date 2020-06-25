# $NetBSD: t_tap.sh,v 1.1 2020/06/25 14:24:46 jruoho Exp $
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
taps="/tmp/taps"

atf_test_case manytaps cleanup
manytaps_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Test creating many, many tap(4)'s (PR kern/55417)"
}

manytaps_body() {

	atf_skip "The test causes a panic (PR kern/55417)"
	seq 65535 64000 > $taps

	while read tap; do

		ifconfig "tap$tap"

		if [ $? -eq 0 ]; then
			echo "Skipping existing tap$tap"
			continue
		fi

		ifconfig "tap$tap" create
		echo "Created $tap"

	done < $taps
}

manytaps_cleanup() {

	if [ -f $taps ]; then

		while read tap; do

			ifconfig "tap$tap"

			if [ $? -eq 0 ]; then
				ifconfig "tap$tap" destroy
				echo "Destroyed tap$tap"
			fi

		done < $taps

		rm $taps
	fi
}

atf_test_case overflow cleanup
overflow_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Test creating a tap(4) with a " \
		"negative device number (PR kern/53546)"
}

overflow_body() {
	atf_skip "The test causes a panic (PR kern/53546)"
	ifconfig tap65537 create
}

overflow_cleanup() {

	ifconfig tap65537

	if [ $? -eq 0 ]; then
		ifconfig tap65537 destroy
	fi
}

atf_init_test_cases() {
	atf_add_test_case manytaps
	atf_add_test_case overflow
}
