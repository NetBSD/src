# $NetBSD: t_capabilities.sh,v 1.1.6.1 2023/10/18 14:48:44 martin Exp $
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
setcap() {

	echo "Request: $1 $2 ($3)"
	ifconfig $1 $2

	if [ $4 -eq 1 ]; then

		if [ ! $? -eq 0 ]; then
			atf_fail "Failed to enable $3 for $1"
		fi

		if [ -z "$(ifconfig $1 | grep "enabled" | grep $3)" ]; then
			atf_fail "Failed to enable $3 for $1"
		fi

		echo "Request: $1 -$2 (-$3)"
		ifconfig $1 -$2

		if [ ! $? -eq 0 ]; then
			atf_fail "Failed to disable $3 for $1"
		fi

		if [ ! -z "$(ifconfig $1 | grep "enabled" | grep $3)" ]; then
			atf_fail "Failed to disable $3 for $1"
		fi
	fi
}

parsecap() {

	i=$1
	checkrv=$3
	x=$(echo $2 | sed "s/.*<//" | sed "s/>//")

	export IFS=","

	for y in $x; do

		z=""

		if [ $y = "TSO4" ]; then
			z="tso4"
		elif [ $y = "IP4CSUM_Rx" ]; then
			z="ip4csum-rx"
		elif [ $y = "IP4CSUM_Tx" ]; then
			z="ip4csum-tx"
		elif [ $y = "TCP4CSUM_Rx" ]; then
			z="tcp4csum-rx"
		elif [ $y = "TCP4CSUM_Tx" ]; then
			z="tcp4csum-tx"
		elif [ $y = "UDP4CSUM_Rx" ]; then
			z="udp4csum-rx"
		elif [ $y = "UDP4CSUM_Tx" ]; then
			z="udp4csum-tx"
		elif [ $y = "TCP6CSUM_Rx" ]; then
			z="tcp6csum-rx"
		elif [ $y = "TCP6CSUM_Tx" ]; then
			z="tcp6csum-tx"
		elif [ $y = "UDP6CSUM_Rx" ]; then
			z="udp6csum-rx"
		elif [ $y = "UDP6CSUM_Tx" ]; then
			z="udp6csum-tx"
		elif [ $y = "TSO6" ]; then
			z="tso6"
		fi

		if [ -z $z ]; then
			echo "Skipping unrecognized $y for $i"
		else
			setcap $i $z $y $checkrv
		fi
	done

	unset IFS
}

atf_test_case basic
basic_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Test setting interface capabilities"
}

basic_body() {

	if ! [ $(atf_config_get "run_unsafe" "no") = "yes" ]; then
		atf_skip "modify if_capenable for real interfaces"
	fi

	for i in $(ifconfig -l); do

		c=$(ifconfig $i | grep "capabilities")

		if [ -z "$c" ]; then
			echo "Skipping $i, no capabilities"
			continue
		fi

		if [ ! -z "$(echo $i | grep ixg)" ]; then
			echo "Skipping $i (PR kern/50933)"
			continue
		fi

		enabled=$(ifconfig $i | grep "enabled")

		for l in $c; do

			if [ ! -z "$(echo $l | grep "ec_capabilities")" ]; then
				continue
			fi

			parsecap $i $l 1
		done

		if [ ! -z "$enabled" ]; then

			echo "Restoring capabilities for $i"

			for l in $enabled; do

				ec=$(echo $l | grep "ec_enabled")

				if [ ! -z "$ec" ]; then
					continue
				fi

				parsecap $i $l 0
			done
	       fi
	done
}

atf_init_test_cases() {
	atf_add_test_case basic
}
