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
tmp="/tmp/sysctl.out"

getrint() {
	echo $(od -An -N2 -i /dev/urandom | sed 's/ //')
}

getrstr() {
	echo $(cat /dev/urandom | head -n 1 | base64)
}

atf_test_case random_garbage cleanup
random_garbage_head() {
	sysctl -a > $tmp
	atf_set "require.user" "root"
	atf_set "descr" "Test writing random garbage " \
		"to sysctl nodes (PR kern/55451)"
}

random_garbage_body() {

	if ! [ $(atf_config_get "run_unsafe" "no") = "yes" ]; then
		atf_skip "The test is not safe (PR kern/55451)"
	fi

	while read line; do

		var=$(echo $line | awk '{print $1}')

		case $var in
			hw.acpi.sleep.state)
			echo "Skipping $var"
			continue
			;;

			kern.securelevel*)
			echo "Skipping $var"
			continue
			;;

			kern.veriexec.strict)
			echo "Skipping $var"
			continue
			;;

			security*)
			echo "Skipping $var"
			continue
			;;
		esac

		val=$(getrint)
		echo "Write $var -> $val"
		sysctl -w $var=$val
		val=$(getrstr)
		echo "Write $var -> $val"
		sysctl -w $var=$val

	done < $tmp
}

random_garbage_cleanup() {

	if ! [ $(atf_config_get "run_unsafe" "no") = "yes" ]; then
		atf_skip "The test is not safe (PR kern/55451)"
	fi

	while read line; do
		var=$(echo $line | awk '{print $1}')
		val=$(echo $line | awk '{print $3}')
		echo "Restoring $var -> $val"
		sysctl -w $var=$val > /dev/null 2>&1
	done < $tmp

	rm $tmp
}

atf_init_test_cases() {
	atf_add_test_case random_garbage
}
