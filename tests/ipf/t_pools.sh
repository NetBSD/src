# $NetBSD: t_pools.sh,v 1.2 2010/10/19 16:36:36 jmmv Exp $
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

iptest()
{
	h_copydata $1
	mkdir input
	cp $(atf_get_srcdir)/input/ip2.data input/

	atf_check -o file:exp -e ignore ippool -f reg -nRv
}

ptest()
{
	h_copydata $1
	cp $(atf_get_srcdir)/regress/$1.pool pool 2>/dev/null
	cp $(atf_get_srcdir)/regress/$1.ipf ipf

	if [ -f pool ] ; then
		atf_check -o save:out ipftest -RD -b -P pool -r ipf -i in
	else
		atf_check -o save:out ipftest -RD -b -r ipf -i in
	fi

	echo "-------------------------------" >>out

}

test_case p1 ptest text text
test_case p2 ptest text text
test_case p3 ptest text text
test_case p5 ptest text text
test_case ip1 iptest text text
test_case ip2 iptest text text

atf_init_test_cases()
{
	atf_add_test_case p1
	atf_add_test_case p2
	atf_add_test_case p3
	atf_add_test_case p5
	atf_add_test_case ip1
	atf_add_test_case ip2
}
