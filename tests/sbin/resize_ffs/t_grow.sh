# $NetBSD: t_grow.sh,v 1.4 2010/12/12 22:50:42 riz Exp $
#
# Copyright (c) 2010 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Jeffrey C. Rizzo.
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

grow_ffs()
{
	echo "in grow_ffs:" ${@}
	local bs=$1
	local fragsz=$2
	local osize=$3
	local nsize=$4
	local fslevel=$5
	local numdata=$6
	local swap=$7
	mkdir -p mnt
	echo "bs is ${bs} numdata is ${numdata}"
	echo "****growing fs with blocksize ${bs}"

	# we want no more than 16K/inode to allow test files to copy.
	local fpi=$((fragsz * 4))
	local i
	if [ $fpi -gt 16384 ]; then
		i="-i 16384"
	fi
	if [ x$swap != x ]; then
		newfs -B ${BYTESWAP} -O${fslevel} -b ${bs} -f ${fragsz} \
			-s ${osize} ${i} -F ${IMG}
	else
		newfs -O${fslevel} -b ${bs} -f ${fragsz} -s ${osize} ${i} \
			-F ${IMG}
	fi

	# we're specifying relative paths, so rump_ffs warns - ignore.
	atf_check -s exit:0 -e ignore rump_ffs ${IMG} mnt
	copy_multiple ${numdata}
	umount mnt
	atf_check -s exit:0 resize_ffs -y -s ${nsize} ${IMG}
	atf_check -s exit:0 -o ignore fsck_ffs -f -n -F ${IMG}
	atf_check -s exit:0 -e ignore rump_ffs ${IMG} mnt
	# checking everything because we don't delete on grow
	check_data_range 1 ${numdata}
	umount mnt
	rm -f ${IMG}	# probably unnecessary
}

# grow_ffs params as follows:
# grow_ffs blocksize fragsize fssize newfssize level numdata swap
# where 'numdata' is the number of data directories to copy - this is
# determined manually based on the maximum number that will fit in the
# created fs.  'level' is the fs-level (-O 0,1,2) passed to newfs.
# If 'swap' is included, byteswap the fs
test_case grow_16M_v1_4096 grow_ffs 4096 512 32768 131072 1 28
test_case_xfail grow_16M_v1_4096_swapped "PR bin/44203" grow_ffs 4096 512 32768 131072 1 28 swap
test_case_xfail grow_16M_v2_4096 "PR bin/44205" grow_ffs 4096 512 32768 131072 2 28
test_case_xfail grow_16M_v2_4096_swapped "PR bin/44203, PR bin/44205" grow_ffs 4096 512 32768 131072 2 28 swap
test_case grow_16M_v1_8192 grow_ffs 8192 1024 32768 131072 1 28
test_case grow_16M_v1_16384 grow_ffs 16384 2048 32768 131072 1 29
test_case grow_16M_v1_32768 grow_ffs 32768 4096 32768 131072 1 28
test_case grow_16M_v1_65536 grow_ffs 65536 8192 32768 131072 1 26
# these grow_24M grow a smaller amount; sometimes there's different issues
test_case grow_24M_v1_4096 grow_ffs 4096 512 49152 65536 1 41
test_case grow_24M_v1_8192 grow_ffs 8192 1024 49152 65536 1 42
test_case grow_24M_v1_16384 grow_ffs 16384 2048 49152 65536 1 43
test_case grow_24M_v1_32768 grow_ffs 32768 4096 49152 65536 1 42
test_case grow_24M_v1_65536 grow_ffs 65536 8192 49152 65536 1 38
test_case grow_32M_v1_4096 grow_ffs 4096 512 65536 131072 1 55
test_case grow_32M_v1_8192 grow_ffs 8192 1024 65536 131072 1 56
test_case grow_32M_v1_16384 grow_ffs 16384 2048 65536 131072 1 58
test_case grow_32M_v1_32768 grow_ffs 32768 4096 65536 131072 1 56
test_case grow_32M_v1_65536 grow_ffs 65536 8192 65536 131072 1 51
test_case grow_48M_v1_4096 grow_ffs 4096 512 98304 131072 1 82
test_case grow_48M_v1_8192 grow_ffs 8192 1024 98304 131072 1 84
test_case grow_48M_v1_16384 grow_ffs 16384 2048 98304 131072 1 87
test_case grow_48M_v1_32768 grow_ffs 32768 4096 98304 131072 1 83
test_case grow_48M_v1_65536 grow_ffs 65536 8192 98304 131072 1 76
test_case grow_64M_v1_4096 grow_ffs 4096 512 131072 262144 1 109
test_case grow_64M_v1_8192 grow_ffs 8192 1024 131072 262144 1 111
test_case grow_64M_v1_16384 grow_ffs 16384 2048 131072 262144 1 115
test_case grow_64M_v1_32768 grow_ffs 32768 4096 131072 262144 1 111
test_case grow_64M_v1_65536 grow_ffs 65536 8192 131072 262144 1 101
test_case_xfail grow_64M_v1_65536_swapped "PR bin/44203" grow_ffs 65536 8192 131072 262144 1 101 swap
test_case_xfail grow_64M_v2_65536 "PR bin/44205" grow_ffs 65536 8192 131072 262144 2 101
test_case_xfail grow_64M_v2_65536_swapped "PR bin/44203, PR bin/44205" grow_ffs 65536 8192 131072 262144 2 101 swap

atf_test_case grow_ffsv1_partial_cg
grow_ffsv1_partial_cg_head()
{
	atf_set "descr" "Checks successful ffsv1 growth by less" \
			"than a cylinder group"
}
grow_ffsv1_partial_cg_body()
{
	echo "***resize_ffs grow test"

	# resize_ffs only supports ffsv1 at the moment
	atf_check -o ignore -e ignore newfs -V1 -s 4000 -F ${IMG}

	# size to grow to is chosen to cause partial cg
	atf_check -s exit:0 resize_ffs -y -s 5760 ${IMG}
	atf_check -s exit:0 -o ignore fsck_ffs -f -n -F ${IMG}
}

atf_init_test_cases()
{
	setupvars
	atf_add_test_case grow_16M_v1_4096
	atf_add_test_case grow_16M_v1_4096_swapped
	atf_add_test_case grow_16M_v2_4096
	atf_add_test_case grow_16M_v2_4096_swapped
	atf_add_test_case grow_16M_v1_8192
	atf_add_test_case grow_16M_v1_16384
	atf_add_test_case grow_16M_v1_32768
	atf_add_test_case grow_16M_v1_65536
	atf_add_test_case grow_24M_v1_4096
	atf_add_test_case grow_24M_v1_8192
	atf_add_test_case grow_24M_v1_16384
	atf_add_test_case grow_24M_v1_32768
	atf_add_test_case grow_24M_v1_65536
	atf_add_test_case grow_32M_v1_4096
	atf_add_test_case grow_32M_v1_8192
	atf_add_test_case grow_32M_v1_16384
	atf_add_test_case grow_32M_v1_32768
	atf_add_test_case grow_32M_v1_65536
if [ "X${RESIZE_FFS_BIG_TESTS}" != "X" ]; then
	atf_add_test_case grow_48M_v1_4096
	atf_add_test_case grow_48M_v1_8192
	atf_add_test_case grow_48M_v1_16384
	atf_add_test_case grow_48M_v1_32768
	atf_add_test_case grow_48M_v1_65536
	atf_add_test_case grow_64M_v1_4096
	atf_add_test_case grow_64M_v1_8192
	atf_add_test_case grow_64M_v1_16384
	atf_add_test_case grow_64M_v1_32768
	atf_add_test_case grow_64M_v1_65536
	atf_add_test_case grow_64M_v1_65536_swapped
	atf_add_test_case grow_64M_v2_65536
	atf_add_test_case grow_64M_v2_65536_swapped
fi
	atf_add_test_case grow_ffsv1_partial_cg
}
