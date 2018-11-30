# $NetBSD: t_tar.sh,v 1.2 2018/11/30 00:53:41 christos Exp $
#
# Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
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

atf_test_case append
append_head() {
	atf_set "descr" "Ensure that appending a file to an archive" \
	                "produces the same results as if the file" \
	                "had been there during the archive's creation"
}
append_body() {
	touch foo bar

	# store both foo and bar into file1.tar
	atf_check -s eq:0 -o empty -e empty tar -cf file1.tar foo bar

	# store foo into file2.tar, then append bar to file2.tar
	atf_check -s eq:0 -o empty -e empty tar -cf file2.tar foo
	atf_check -s eq:0 -o empty -e empty tar -rf file2.tar bar

	# ensure that file1.tar and file2.tar are equal
	atf_check -s eq:0 -o empty -e empty cmp file1.tar file2.tar
}

atf_test_case rd_base256_size
rd_base256_size_head() {
	atf_set "descr" "Test extracting an archive whose member size" \
	                "is encoded as base-256 number (GNU style)"
}
rd_base256_size_body() {
	# prepare random file data for comparison
	# take 0x1200CF bytes in order to test that we:
	# - handle multiple bytes of size field correctly
	# - do not fail on NUL bytes
	# - do not fail on char values > 0x80 (with signed char)
	dd if=/dev/urandom of=reference.bin bs=1179855 count=1
	# write test archive header
	# - filename
	printf 'output.bin' > test.tar
	# - pad to 100 octets
	head -c 90 /dev/zero >> test.tar
	# - mode, uid, gid
	printf '%07d\0%07d\0%07d\0' 644 177776 177775 >> test.tar
	# - size (base-256)
	printf '\x80\0\0\0\0\0\0\0\0\x12\x00\xCF' >> test.tar
	# - timestamp, checksum
	printf '%011d\0%06d\0 0' 13377546642 12460 >> test.tar
	# - pad empty linkname (100 octets)
	head -c 100 /dev/zero >> test.tar
	# - magic, user name
	printf 'ustar  \0nobody' >> test.tar
	# - pad user name field to 32 bytes
	head -c 26 /dev/zero >> test.tar
	# - group name
	printf 'nogroup' >> test.tar
	# - pad to full block
	head -c 208 /dev/zero >> test.tar
	# append file data to the test archive
	cat reference.bin >> test.tar
	# pad to full block + append two terminating null blocks
	head -c 1450 /dev/zero >> test.tar

	# test extracting the test archive
	atf_check -s eq:0 -o empty -e empty tar -xf test.tar

	# ensure that output.bin is equal to reference.bin
	atf_check -s eq:0 -o empty -e empty cmp output.bin reference.bin
}

atf_init_test_cases()
{
	atf_add_test_case append
	atf_add_test_case rd_base256_size
}
