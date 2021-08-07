# $NetBSD: t_lint2.sh,v 1.2 2021/08/07 21:24:14 rillig Exp $
#
# Copyright (c) 2021 The NetBSD Foundation, Inc.
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

lint2=/usr/libexec/lint2

std_head()
{
	atf_set 'require.progs' "$lint2"
}

std_body()
{
	# shellcheck disable=SC2155
	local srcdir="$(atf_get_srcdir)"

	# remove comments and empty lines from the .ln file
	sed -e '/^#/d' -e '/^$/d' -e 's,[[:space:]]*#.*,,' \
	    < "$srcdir/$1.ln" \
	    > "$1.ln"

	atf_check -o "file:$srcdir/$1.exp" \
	    "$lint2" -h -p -x "$1.ln"
}

atf_init_test_cases()
{
	for i in $(printf 'msg_%03d\n' $(seq 0 18)) "read"; do
		eval "${i}_head() { std_head; }"
		eval "${i}_body() { std_body '$i'; }"
		atf_add_test_case "$i"
	done
}
