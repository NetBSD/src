#	$NetBSD: t_pr_19722.sh,v 1.1 2022/05/22 17:55:08 rillig Exp $
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

# https://gnats.netbsd.org/19722
#
# Before TODO:rev from TODO:date, trying to uncompress a nonexistent or
# malformed source file resulted in a truncated target file.


atf_test_case 'no_source_no_target'
no_source_no_target_body()
{

	atf_check \
	    -s 'not-exit:0' \
	    -e 'inline:uncompress: file.Z: No such file or directory\n' \
	    uncompress -f 'file'

	# FIXME: The target file must not be created.
	atf_check cat 'file'
	atf_check test ! -f 'nonexistent.Z'
}


atf_test_case 'no_source_existing_target'
no_source_existing_target_body()
{

	echo 'before' > 'file'

	atf_check \
	    -s 'not-exit:0' \
	    -e 'inline:uncompress: file.Z: No such file or directory\n' \
	    uncompress -f 'file'

	# FIXME: The target file must not be truncated.
	atf_check cat 'file'
}


atf_test_case 'broken_source_existing_target'
broken_source_existing_target_body()
{
	# If the source file is not compressed, preserve the target file.

	echo 'broken' > 'file.Z'
	echo 'before' > 'file'

	atf_check \
	    -s 'not-exit:0' \
	    -e 'inline:uncompress: file.Z: Inappropriate file type or format\n' \
	    uncompress -f 'file.Z'

	# FIXME: Must not be removed, must not be truncated.
	atf_check test ! -f 'file'
}


atf_init_test_cases()
{

	atf_add_test_case no_source_no_target
	atf_add_test_case no_source_existing_target
	atf_add_test_case broken_source_existing_target
}
