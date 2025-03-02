#	$NetBSD: t_sort.sh,v 1.2 2025/03/02 20:00:32 riastradh Exp $
#
# Copyright (c) 2025 The NetBSD Foundation, Inc.
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

check_sort()
{
	local sortfn

	set -eu

	sortfn="$1"

	printf 'foo\nbar\nbaz\nquux' >in1
	printf '1 bar\n2 baz\n0 foo\n3 quux\n' >out1
	atf_check -s exit:0 -o file:out1 \
		"$(atf_get_srcdir)"/h_sort -n "$sortfn" <in1

	atf_check -s exit:0 -o empty sh -c 'exec shuffle -f - <in1 >in2'
	printf 'bar\nbaz\nfoo\nquux\n' >out2
	atf_check -s exit:0 -o file:out2 \
		"$(atf_get_srcdir)"/h_sort "$sortfn" <in2
}

check_stablesort()
{
	local sortfn

	set -eu

	sortfn="$1"

	printf 'foo\nfoo\nfoo\nfoo\nfoo' >in1
	printf '0 foo\n1 foo\n2 foo\n3 foo\n4 foo\n' >out1
	atf_check -s exit:0 -o file:out1 \
		"$(atf_get_srcdir)"/h_sort -n "$sortfn" <in1

	printf 'foo\nfoo\nfoo\nfoo\nfoo\nbar\nbar\nbar\nbar\nbar' >in2
	printf '5 bar\n6 bar\n7 bar\n8 bar\n9 bar\n' >out2
	printf '0 foo\n1 foo\n2 foo\n3 foo\n4 foo\n' >>out2
	atf_check -s exit:0 -o file:out2 \
		"$(atf_get_srcdir)"/h_sort -n "$sortfn" <in2

	printf 'foo\nfoo\nbar\nbaz\nquux' >in3
	printf '2 bar\n3 baz\n0 foo\n1 foo\n4 quux\n' >out3
	atf_check -s exit:0 -o file:out3 \
		"$(atf_get_srcdir)"/h_sort -n "$sortfn" <in3

	printf 'foo\nbar\nbar\nbaz\nquux' >in4
	printf '1 bar\n2 bar\n3 baz\n0 foo\n4 quux\n' >out4
	atf_check -s exit:0 -o file:out4 \
		"$(atf_get_srcdir)"/h_sort -n "$sortfn" <in4

	printf 'foo\nbar\nbaz\nbaz\nquux' >in5
	printf '1 bar\n2 baz\n3 baz\n0 foo\n4 quux\n' >out5
	atf_check -s exit:0 -o file:out5 \
		"$(atf_get_srcdir)"/h_sort -n "$sortfn" <in5

	printf 'foo\nbar\nbaz\nquux\nquux' >in6
	printf '1 bar\n2 baz\n0 foo\n3 quux\n4 quux\n' >out6
	atf_check -s exit:0 -o file:out6 \
		"$(atf_get_srcdir)"/h_sort -n "$sortfn" <in6
}

sortfn_case()
{
	local sortfn

	sortfn="$1"

	eval "${sortfn}_head() { atf_set descr \"Test ${sortfn}\"; }"
	eval "${sortfn}_body() { check_sort $sortfn; }"
	atf_add_test_case "$sortfn"
}

stablesortfn_case()
{
	local sortfn

	sortfn="$1"

	eval "${sortfn}_stable_head() { atf_set descr \"Test ${sortfn}\"; }"
	eval "${sortfn}_stable_body() { check_stablesort $sortfn; }"
	atf_add_test_case "${sortfn}_stable"
}

atf_init_test_cases()
{

	sortfn_case heapsort_r
	sortfn_case mergesort_r
	sortfn_case qsort_r
	stablesortfn_case mergesort_r
}
