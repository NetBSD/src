# $NetBSD: t_pax.sh,v 1.1.4.2 2008/01/09 01:59:33 matt Exp $
#
# Copyright (c) 2007 The NetBSD Foundation, Inc.
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
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
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
	atf_check 'pax -w -b 512 -x ustar -f file1.tar foo bar' 0 null null

	# store foo into file2.tar, then append bar to file2.tar
	atf_check 'pax -w -b 512 -x ustar -f file2.tar foo' 0 null null
	atf_check 'pax -w -b 512 -x ustar -f file2.tar -a bar' 0 null null

	# ensure that file1.tar and file2.tar are equal
	atf_check 'cmp file1.tar file2.tar' 0 null null
}

atf_init_test_cases()
{
	atf_add_test_case append
}
