#       $NetBSD: t_libarchive.sh,v 1.7 2020/08/28 18:46:05 christos Exp $
#
# Copyright (c) 2020 The NetBSD Foundation, Inc.
# All rights reserved.
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

atf_test_case libarchive

export TMPDIR=$PWD

libarchive_head()
{
        atf_set "descr" "test libarchive"
	atf_set	"timeout" "6000"
}

libarchive_body()
{
	local m=$(( $( sysctl -n hw.usermem64 ) / 1024 / 1024 ))
	if [ $m -lt 400 ]; then
		atf_skip "Not enough RAM"
	fi
	local ncpu=$( sysctl -n hw.ncpuonline )
	if [ $ncpu -lt 2 ]; then
		atf_skip "PR kern/55272: too dangerous to run this test"
	fi
	local d=$(atf_get_srcdir)
	atf_check -s exit:0 -o 'not-match:^Details for failing tests:.*' \
	    "$d/h_libarchive" -r "$d"
}

atf_init_test_cases()
{
	atf_add_test_case libarchive
}
