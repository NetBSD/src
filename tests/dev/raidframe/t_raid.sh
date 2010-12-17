#	$NetBSD: t_raid.sh,v 1.2 2010/12/17 14:51:27 pooka Exp $
#
# Copyright (c) 2010 The NetBSD Foundation, Inc.
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

makecfg()
{
	level=${1}
	ncol=${2}

	printf "START array\n1 ${ncol} 0\nSTART disks\n" > raid.conf
	diskn=0
	while [ ${ncol} -gt ${diskn} ] ; do
		echo "/disk${diskn}" >> raid.conf
		diskn=$((diskn+1))
	done

	printf "START layout\n32 1 1 ${level}\nSTART queue\nfifo 100\n" \
	    >> raid.conf
}

atf_test_case smalldisk cleanup
smalldisk_head()
{

	atf_set "descr" "Checks the raidframe works on small disks"
}

smalldisk_body()
{

	makecfg 1 2
	export RUMP_SERVER=unix://sock
	atf_check -s exit:0 rump_allserver			\
	    -d key=/disk0,hostpath=disk0.img,size=1m		\
	    -d key=/disk1,hostpath=disk1.img,size=1m		\
	    ${RUMP_SERVER}

	atf_expect_fail "PR kern/44239" # ADJUST CLEANUP WHEN THIS IS FIXED!
	atf_check -s exit:0 rump.raidctl -C raid.conf raid0
}

smalldisk_cleanup()
{

	export RUMP_SERVER=unix://sock
	rump.halt
	: server dumps currently, so reset error.  remove this line when fixed
}

atf_init_test_cases()
{

	atf_add_test_case smalldisk
}
