# $NetBSD: t_patch.sh,v 1.4 2021/09/11 20:28:06 andvar Exp $
#
# Copyright (c) 2020, 2021 The NetBSD Foundation, Inc.
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
	patch_lines 10000
}

atf_test_case backup_simple
backup_simple_head()
{
	atf_set "descr" "Test backup type 'simple' (-V simple)"
}

backup_simple_body()
{
	# Create the backup file so it's overwritten.
	touch to_patch.orig
	printf '%s\n' 'original file' > to_patch

	cat << EOF > test_diff.patch
--- original_file       2021-02-20 09:21:07.100869692 +0100
+++ new_file    2021-02-20 09:21:10.912906887 +0100
@@ -1 +1 @@
-original text
+new text
EOF
	cksum=$(sha256 -n to_patch | cut -d' ' -f1)
	patch -V simple to_patch < test_diff.patch
	atf_check [ -f to_patch.orig ]
	origfile_cksum=$(sha256 -n to_patch.orig | cut -d' ' -f1)
	atf_check_equal "$cksum" "$origfile_cksum"
	atf_check grep -q -m 1 '^new text$' to_patch
}

atf_test_case backup_none
backup_none_head()
{
	atf_set "descr" "Test backup type 'none' (-V none)"
}

backup_none_body()
{
	printf '%s\n' 'original file' > to_patch

	cat << EOF > test_diff.patch
--- original_file       2021-02-20 09:21:07.100869692 +0100
+++ new_file    2021-02-20 09:21:10.912906887 +0100
@@ -1 +1 @@
-original text
+new text
EOF
	# Patch would mistakenly create 'simple' backup files when unwanted:
	# http://mail-index.netbsd.org/tech-userlevel/2021/02/19/msg012901.html
	patch -V none to_patch < test_diff.patch
	atf_check [ ! -f to_patch.orig ]
	atf_check grep -q -m 1 '^new text$' to_patch

	# Environment variables should be overridden.
	printf '%s\n' 'original file' > to_patch
	VERSION_CONTROL=existing patch -V none to_patch \
	    < test_diff.patch
	atf_check [ ! -f to_patch.orig ]
	atf_check grep -q -m 1 '^new text$' to_patch

	# --posix should imply -V none.
	printf '%s\n' 'original file' > to_patch
	patch --posix to_patch < test_diff.patch
	atf_check [ ! -f to_patch.orig ]
	atf_check grep -q -m 1 '^new text$' to_patch
}

atf_test_case backup_numbered
backup_numbered_head()
{
	atf_set "descr" "Test backup type 'numbered' (-V numbered)"
}

backup_numbered_body()
{
	printf '%s\n' 'original file' > to_patch

	cat << EOF > test_diff.patch
--- original_file       2021-02-20 09:21:07.100869692 +0100
+++ new_file    2021-02-20 09:21:10.912906887 +0100
@@ -1 +1 @@
-original text
+new text
EOF
	cksum1=$(sha256 -n to_patch | cut -d' ' -f1)
	patch -V numbered to_patch < test_diff.patch
	atf_check grep -q -m 1 '^new text$' to_patch

	cat << EOF > test_diff2.patch
--- new_file	2021-02-20 09:44:52.363230019 +0100
+++ newer_file	2021-02-20 09:43:54.592863401 +0100
@@ -1 +1 @@
-new text
+newer text
EOF
	cksum2=$(sha256 -n to_patch | cut -d' ' -f1)
	patch -V numbered to_patch < test_diff2.patch
	atf_check grep -q -m 1 '^newer text$' to_patch

	# Make sure the backup files match the original files.
	origfile_cksum1=$(sha256 -n to_patch.~1~ | cut -d' ' -f1)
	origfile_cksum2=$(sha256 -n to_patch.~2~ | cut -d' ' -f1)
	atf_check [ -f to_patch.~1~ ]
	atf_check_equal "$cksum1" "$origfile_cksum1"
	atf_check [ -f to_patch.~2~ ]
	atf_check_equal "$cksum2" "$origfile_cksum2"
}

atf_init_test_cases()
{
	atf_add_test_case lines
	atf_add_test_case long_lines
	atf_add_test_case backup_simple
	atf_add_test_case backup_none
	atf_add_test_case backup_numbered
}
