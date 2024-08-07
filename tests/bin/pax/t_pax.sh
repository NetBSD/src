# $NetBSD: t_pax.sh,v 1.1.44.1 2024/08/07 10:52:49 martin Exp $
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
	atf_check -s eq:0 -o empty -e empty \
	    pax -w -b 512 -x ustar -f file1.tar foo bar

	# store foo into file2.tar, then append bar to file2.tar
	atf_check -s eq:0 -o empty -e empty \
	    pax -w -b 512 -x ustar -f file2.tar foo
	atf_check -s eq:0 -o empty -e empty \
	    pax -w -b 512 -x ustar -f file2.tar -a bar

	# ensure that file1.tar and file2.tar are equal
	atf_check -s eq:0 -o empty -e empty cmp file1.tar file2.tar
}

atf_test_case pr41736
pr41736_head()
{
	atf_set "descr" "Test pax exits with 0 if stdin file list is empty"
}
pr41736_body()
{
	atf_check pax -rw . </dev/null
}

atf_test_case pr44498
pr44498_head()
{
	atf_set "descr" "Ensure pax list operation works without getcwd"
	atf_set "require.user" "unprivileged"
}
pr44498_body()
{
	mkdir foo foo/bar baz
	chmod 111 foo
	touch baz/quux
	atf_check pax -w -x ustar -f baz.tar baz
	atf_check -o 'inline:baz\nbaz/quux\n' \
	    sh -c '{ cd foo/bar && exec pax; } <baz.tar'
}

atf_test_case pr44498_copy
pr44498_copy_head()
{
	atf_set "descr" \
	    "Ensure pax insecure copy operation works without getcwd"
	atf_set "require.user" "unprivileged"
}
pr44498_copy_body()
{
	mkdir foo foo/bar foo/bar/baz
	chmod 111 foo
	touch foo/bar/quux
	atf_check sh -c '{ cd foo/bar && exec pax -rw quux baz/.; }'
}

atf_test_case pr44498_insecureextract
pr44498_insecureextract_head()
{
	atf_set "descr" \
	    "Ensure pax insecure extract operation works without getcwd"
	atf_set "require.user" "unprivileged"
}
pr44498_insecureextract_body()
{
	mkdir foo foo/bar baz
	chmod 111 foo
	touch baz/quux
	atf_check pax -w -x ustar -f baz.tar baz
	atf_check sh -c '{ cd foo/bar && exec pax -r --insecure; } <baz.tar'
}

atf_test_case pr44498_listwd
pr44498_listwd_head()
{
	atf_set "descr" "Ensure pax list operation works without working dir"
	atf_set "require.user" "unprivileged"
}
pr44498_listwd_body()
{
	mkdir foo baz
	chmod 111 foo
	touch baz/quux
	atf_check pax -w -x ustar -f baz.tar baz
	atf_check -o 'inline:baz\nbaz/quux\n' \
	    sh -c '{ cd foo && exec pax; } <baz.tar'
}

atf_test_case pr44498_write
pr44498_write_head()
{
	atf_set "descr" "Ensure pax write operation works without getcwd"
	atf_set "require.user" "unprivileged"
}
pr44498_write_body()
{
	mkdir foo foo/bar
	touch foo/bar/quux
	chmod 111 foo
	atf_check sh -c '{ cd foo/bar && pax -w -x ustar .; } >bar.tar'
	atf_check -o 'inline:.\n./quux\n' pax -f bar.tar
}

atf_init_test_cases()
{
	atf_add_test_case append
	atf_add_test_case pr41736
	atf_add_test_case pr44498
	atf_add_test_case pr44498_copy
	atf_add_test_case pr44498_insecureextract
	atf_add_test_case pr44498_listwd
	atf_add_test_case pr44498_write
}
