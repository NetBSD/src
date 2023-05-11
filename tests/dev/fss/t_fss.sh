# $NetBSD: t_fss.sh,v 1.6 2023/05/11 01:56:31 gutteridge Exp $
#
# Copyright (c) 2006, 2007, 2008 The NetBSD Foundation, Inc.
# All rights reserved.
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

#
# Verify basic operation of fss(4) file system snapshot device
#

vnddev=vnd0
vnd=/dev/${vnddev}

orig_data="Original data"
repl_data="Replacement data"

atf_test_case basic cleanup
basic_head() {
	atf_set "descr" "Verify basic operation of fss(4) file system " \
		"snapshot device"
}

basic_body() {

# verify fss is available (or loadable as a module)

	fssconfig -l /dev/fss0 > /dev/null || atf_skip "FSS not available"

# create of mount-points for the file system and snapshot

	mkdir ./m1
	mkdir ./m2

# create a small 4MB file, treat it as a disk, init a file-system on it,
# and mount it

	dd if=/dev/zero of=./image bs=32k count=64
	vndconfig -c ${vnddev} ./image
	newfs -I ${vnd}
	mount ${vnd} ./m1

	echo "${orig_data}" > ./m1/text

# configure and mount a snapshot of the file system

	fssconfig -c fss0 ./m1 ./backup
	mount -o rdonly /dev/fss0 ./m2

# Modify the data on the underlying file system

	echo "${repl_data}" > ./m1/text || abort

# Verify that original data is still visible in the snapshot

	read test_data < ./m2/text
	atf_check_equal "${orig_data}" "${test_data}"
}

basic_cleanup() {
# Unmount our temporary stuff
	umount /dev/fss0	|| true
	fssconfig -u fss0	|| true
	umount ${vnd}		|| true
	vndconfig -u ${vnddev}	|| true
}

atf_init_test_cases()
{
	atf_add_test_case basic
}
