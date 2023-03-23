# $NetBSD: t_envstat.sh,v 1.2 2023/03/23 11:32:49 martin Exp $
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

atf_test_case zerotemp
zerotemp_head() {
	atf_set "descr" "Check with envstat(8) that CPU sensors " \
		"do not show zero temperatures (PR kern/53410)"
}

zerotemp_body() {

	for dev in $( envstat -D | awk '{print $1}' )
	do

		envstat -d $dev >/dev/null 2>&1

		if [ ! $? -eq 0 ]; then
			echo "Skipping non-existent $dev"
			continue
		fi

		# extract all temperatures from $dev
		for tempf in $(envstat -d $dev | \
			awk -F: '/degC$/{print $2}' | \
			awk '{print $1}' )
		do
			tempi=$(printf "%.0f" $tempf)

			echo "$dev = $tempf =~ $tempi"

			if [ $tempi -eq 0 ]; then

				if [ $dev = "amdtemp0" ]; then
					atf_expect_fail "PR kern/53410"
				fi
				atf_fail "Zero-temperature from $dev"
			fi
		done
	done
}

atf_init_test_cases() {
	atf_add_test_case zerotemp
}
