#	$NetBSD: t_pr_19722.sh,v 1.2 2022/05/22 20:49:12 rillig Exp $
#
# Copyright (c) 2022 The NetBSD Foundation, Inc.
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

atf_test_case 'compress_small_file'
compress_small_file_body()
{
	# If the compressed version of a file would be larger than the
	# original file, the original file is kept.

	echo 'hello' > file

	atf_check compress file

	atf_check -o 'inline:hello\n' cat file
	atf_check test ! -f file.Z
}


atf_test_case 'compress_small_file_force'
compress_small_file_force_body()
{
	# The option '-f' forces compression to happen, even if the resulting
	# file becomes larger than the original.

	echo 'hello' > file

	atf_check compress -f file

	atf_check test ! -f file
	atf_check \
	    -o 'inline:0000000   1f  9d  90  68  ca  b0  61  f3  46  01                        \n000000a\n' \
	    od -Ax -tx1 file.Z
}


atf_test_case 'roundtrip'
roundtrip_body()
{
	# Compressing and decompressing a file must preserve every byte.

	atf_check -e 'ignore' dd if=/dev/urandom of=file bs=4k count=10
	atf_check cp file original

	atf_check compress -f file
	atf_check uncompress file.Z

	atf_check cmp file original
}


atf_test_case 'uncompress_basename'
uncompress_basename_body()
{
	# To uncompress a file, it suffices to specify the basename of the
	# file, the filename extension '.Z' is optional.

	atf_check sh -c "echo 'hello' > file"
	atf_check compress -f file

	atf_check uncompress file

	atf_check -o 'inline:hello\n' cat file
	atf_check test ! -f file.Z
}


atf_test_case 'uncompress_no_source_no_target'
uncompress_no_source_no_target_body()
{
	# PR 19722: uncompressing a missing source creates empty target

	atf_check \
	    -s 'not-exit:0' \
	    -e 'inline:uncompress: file.Z: No such file or directory\n' \
	    uncompress -f file

	# FIXME: The target file must not be created.
	atf_check cat file
	atf_check test ! -f nonexistent.Z
}


atf_test_case 'uncompress_no_source_existing_target'
uncompress_no_source_existing_target_body()
{
	# PR 19722: uncompressing a missing source truncates target

	atf_check sh -c "echo 'hello' > file"

	atf_check \
	    -s 'not-exit:0' \
	    -e 'inline:uncompress: file.Z: No such file or directory\n' \
	    uncompress -f file

	# FIXME: The file must not be truncated.
	atf_check cat file
	atf_check test ! -f file.Z
}


atf_test_case 'uncompress_broken_source_no_target'
uncompress_broken_source_no_target_body()
{
	# When trying to uncompress a broken source, the target is created
	# temporarily but deleted again, as part of the cleanup.

	echo 'broken' > file.Z

	atf_check \
	    -s 'not-exit:0' \
	    -e 'inline:uncompress: file.Z: Inappropriate file type or format\n' \
	    uncompress -f file

	atf_check test ! -f file
	atf_check test -f file.Z
}


atf_test_case 'uncompress_broken_source_existing_target'
uncompress_broken_source_existing_target_body()
{
	# PR 19722: uncompressing a broken source removes existing target

	echo 'broken' > file.Z
	echo 'before' > file

	atf_check \
	    -s 'not-exit:0' \
	    -e 'inline:uncompress: file.Z: Inappropriate file type or format\n' \
	    uncompress -f file.Z

	atf_check -o 'inline:broken\n' cat file.Z
	# FIXME: Must not be modified.
	atf_check test ! -f file
}


atf_init_test_cases()
{

	atf_add_test_case compress_small_file
	atf_add_test_case compress_small_file_force
	atf_add_test_case roundtrip
	atf_add_test_case uncompress_basename
	atf_add_test_case uncompress_no_source_no_target
	atf_add_test_case uncompress_no_source_existing_target
	atf_add_test_case uncompress_broken_source_no_target
	atf_add_test_case uncompress_broken_source_existing_target
}
