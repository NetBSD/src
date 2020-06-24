# $NetBSD: t_patch.sh,v 1.1 2020/06/24 09:21:43 jruoho Exp $
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

patch_lines() {

	printf "%$1s" | tr " " "a" > longlines 2>/dev/null

	cat << EOF > longlines.patch
--- ./longlines.orig 2019-10-16 09:25:30.667656644 +0000
+++ ./longlines
@@ -1 +1 @@
EOF
	printf -- "-%$1s\n" | tr " " "a" >> longlines.patch 2>/dev/null
	printf -- "+%$1s" | tr " " "b" >> longlines.patch 2>/dev/null

	patch longlines < longlines.patch

	if [ ! $? -eq 0 ]; then
		atf_fail "Failed to patch long lines"
	fi
}

atf_test_case lines
lines_head()
{
	atf_set "descr" "Test patching lines"
}

lines_body()
{
	lines="1 10 100 1000 8100"

	for line in $lines; do
		patch_lines $line
	done
}

atf_test_case long_lines
long_lines_head()
{
	atf_set "descr" "Test patching long lines (PR bin/54620)"
}

long_lines_body()
{
	atf_expect_fail "PR bin/54620"
	patch_lines 10000
}

atf_init_test_cases()
{
	atf_add_test_case lines
	atf_add_test_case long_lines
}
