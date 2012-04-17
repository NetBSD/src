# $NetBSD: t_nat_exec.sh,v 1.3.6.1 2012/04/17 00:09:04 yamt Exp $
#
# Copyright (c) 2008, 2010 The NetBSD Foundation, Inc.
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
#
# (C)opyright 1993-1996 by Darren Reed.
#
# See the IPFILTER.LICENCE file for details on licencing.
#

nattest()
{
	h_copydata $1

	if [ $3 = hex ] ; then
		format="-xF $2"
	else
		format="-F $2"
	fi

	format="$4 $5 $format"

	{ while read rule; do
		atf_check -o save:save -x \
		    "echo \"$rule\" | ipftest $format -RbN - -i in"
		cat save >>out
		echo "-------------------------------" >>out
	done; } <reg

	diff -u exp out || atf_fail "results differ"
}

#broken_test_case n1 nattest text text
#broken_test_case n2 nattest text text
broken_test_case n3 nattest text text
#broken_test_case n4 nattest text text
#broken_test_case n5 nattest text text
#broken_test_case n6 nattest text text
broken_test_case n7 nattest text text
broken_test_case n8 nattest hex hex -T fr_update_ipid=0
broken_test_case n9 nattest hex hex -T fr_update_ipid=0
broken_test_case n10 nattest hex hex -T fr_update_ipid=0
#broken_test_case n11 nattest text text
broken_test_case n12 nattest hex hex -T fr_update_ipid=0
broken_test_case n13 nattest text text
broken_test_case n14 nattest text text
test_case n16 nattest hex hex -D
test_case n17 nattest hex hex -D

atf_init_test_cases()
{
	atf_add_test_case n3
	atf_add_test_case n7
	atf_add_test_case n8
	atf_add_test_case n9
	atf_add_test_case n10
	atf_add_test_case n12
	atf_add_test_case n13
	atf_add_test_case n14
	atf_add_test_case n16
	atf_add_test_case n17

	#atf_add_test_case n1
	#atf_add_test_case n2
	#atf_add_test_case n4
	#atf_add_test_case n5
	#atf_add_test_case n6
	#atf_add_test_case n11
}
