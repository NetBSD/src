# $NetBSD: t_diff.sh,v 1.5 2025/12/20 00:49:43 nia Exp $
#
# Copyright (c) 2011 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Jukka Ruohonen.
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

atf_test_case mallocv
mallocv_head() {
	atf_set "descr" "Test diff(1) with MALLOC_OPTIONS=V (cf. PR bin/26453)"
}

mallocv_body() {

	atf_check -s ignore \
		-e not-inline:"diff: memory exhausted\n" \
		-x "env MALLOC_OPTIONS=V diff " \
		   "$(atf_get_srcdir)/d_mallocv1.in" \
		   "$(atf_get_srcdir)/d_mallocv2.in"
}

atf_test_case nomallocv
nomallocv_head() {
	atf_set "descr" "Test diff(1) with no MALLOC_OPTIONS=V"
}

nomallocv_body() {

	atf_check -s exit:0 \
		-e inline:"" \
		-x "diff " \
		   "$(atf_get_srcdir)/d_mallocv1.in" \
		   "$(atf_get_srcdir)/d_mallocv2.in"
}

atf_test_case same
same_head() {
	atf_set "descr" "Test diff(1) with identical files"
}

same_body() {

	atf_check -s exit:0 \
		-e inline:"" \
		-x "diff $(atf_get_srcdir)/t_diff $(atf_get_srcdir)/t_diff"
}

atf_test_case simple
simple_head() {
	atf_set "descr" "Test diff(1) with simple diffs"
}

simple_body()
{
	atf_check -o file:$(atf_get_srcdir)/simple.out -s eq:1 \
		diff "$(atf_get_srcdir)/input1.in" "$(atf_get_srcdir)/input2.in"

	atf_check -o file:$(atf_get_srcdir)/simple_e.out -s eq:1 \
		diff -e "$(atf_get_srcdir)/input1.in" "$(atf_get_srcdir)/input2.in"

	atf_check -o file:$(atf_get_srcdir)/simple_u.out -s eq:1 \
		diff -u -L input1 -L input2 "$(atf_get_srcdir)/input1.in" "$(atf_get_srcdir)/input2.in"

	atf_check -o file:$(atf_get_srcdir)/simple_n.out -s eq:1 \
		diff -n "$(atf_get_srcdir)/input1.in" "$(atf_get_srcdir)/input2.in"

	atf_check -o inline:"Files $(atf_get_srcdir)/input1.in and $(atf_get_srcdir)/input2.in differ\n" -s eq:1 \
		diff -q "$(atf_get_srcdir)/input1.in" "$(atf_get_srcdir)/input2.in"

	atf_check \
		diff -q "$(atf_get_srcdir)/input1.in" "$(atf_get_srcdir)/input1.in"

	atf_check \
		diff -q -U 2 -p "$(atf_get_srcdir)/input1.in" "$(atf_get_srcdir)/input1.in"

	atf_check -o file:$(atf_get_srcdir)/simple_i.out -s eq:1 \
		diff -i "$(atf_get_srcdir)/input_c1.in" "$(atf_get_srcdir)/input_c2.in"

	atf_check -o file:$(atf_get_srcdir)/simple_w.out -s eq:1 \
		diff -w "$(atf_get_srcdir)/input_c1.in" "$(atf_get_srcdir)/input_c2.in"

	atf_check -o file:$(atf_get_srcdir)/simple_b.out -s eq:1 \
		diff -b "$(atf_get_srcdir)/input_c1.in" "$(atf_get_srcdir)/input_c2.in"

	atf_check -o file:$(atf_get_srcdir)/simple_p.out -s eq:1 \
		diff --label input_c1.in --label input_c2.in -p "$(atf_get_srcdir)/input_c1.in" "$(atf_get_srcdir)/input_c2.in"
}

atf_test_case unified
unified_head() {
	atf_set "descr" "Test diff(1) with unified diffs"
}

unified_body()
{
	atf_check -o file:$(atf_get_srcdir)/unified_p.out -s eq:1 \
		diff -up -L input_c1.in -L input_c2.in  "$(atf_get_srcdir)/input_c1.in" "$(atf_get_srcdir)/input_c2.in"
	atf_check -o file:$(atf_get_srcdir)/unified_9999.out -s eq:1 \
		diff -U 9999 -L input_c1.in -L input_c2.in "$(atf_get_srcdir)/input_c1.in" "$(atf_get_srcdir)/input_c2.in"
}

atf_test_case header
header_head() {
	atf_set "descr" "Test diff(1) modification time headers"
}

header_body()
{
	export TZ=UTC
	: > empty
	echo hello > hello
	touch -d 2015-04-03T01:02:03 empty
	touch -d 2016-12-22T11:22:33 hello
	atf_check -o "file:$(atf_get_srcdir)/header.out" -s eq:1 \
		diff -u empty hello
}

atf_test_case header_ns
header_ns_head() {
	atf_set "descr" "Test diff(1) modification time headers with nanoseconds"
}

header_ns_body()
{
	export TZ=UTC
	: > empty
	echo hello > hello
	touch -d 2015-04-03T01:02:03.123456789 empty
	touch -d 2016-12-22T11:22:33.987654321 hello
	atf_check -o "file:$(atf_get_srcdir)/header_ns.out" -s eq:1 \
		diff -u empty hello
}

atf_test_case functionname
functionname_head() {
	atf_set "descr" "Test diff(1) C function option (-p)"
}

functionname_body()
{
	atf_check -o file:$(atf_get_srcdir)/functionname_c.out -s exit:1 \
		diff -u -p -L functionname.in -L functionname_c.in \
		"$(atf_get_srcdir)/functionname.in" "$(atf_get_srcdir)/functionname_c.in"
}

atf_init_test_cases() {
	atf_add_test_case mallocv
	atf_add_test_case nomallocv
	atf_add_test_case same
	atf_add_test_case simple
	atf_add_test_case unified
	atf_add_test_case header
	atf_add_test_case header_ns
	atf_add_test_case functionname
}
