# $NetBSD: t_fstat.sh,v 1.1 2020/06/24 10:05:07 jruoho Exp $
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

atf_test_case basic
basic_head()
{
	atf_require_prog fstat
	atf_set "descr" "Check that fstat(1) works"
}

basic_body()
{
	# If there are chrooted processes running, the following
	# simple test should catch also those (cf. PR kern/55407).
	#
	pids=$(ps -A | awk '{print $1}')

	for pid in $pids; do

		if [ $pid = "PID" ]; then
			continue
		fi

		atf_check -o ignore -s exit:0 -e empty -x "fstat -p $pid"
	done
}

atf_test_case err
err_head()
{
	atf_require_prog fstat
	atf_set "descr" "Check fstat(1) with invalid parameters"
}

err_body()
{
	atf_check -o empty -s exit:1 -e not-empty -x "fstat -p -1"
	atf_check -o empty -s exit:1 -e not-empty -x "fstat -p -100"
	atf_check -o empty -s exit:1 -e not-empty -x "fstat -p abcd"
	atf_check -o empty -s exit:1 -e not-empty -x "fstat -u abcd"
	atf_check -o empty -s exit:1 -e not-empty -x "fstat -u -100"
}

atf_init_test_cases()
{
	atf_add_test_case basic
	atf_add_test_case err
}
