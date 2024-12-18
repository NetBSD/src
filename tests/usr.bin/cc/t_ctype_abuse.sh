#	$NetBSD: t_ctype_abuse.sh,v 1.1 2024/12/18 02:47:00 riastradh Exp $
#
# Copyright (c) 2024 The NetBSD Foundation, Inc.
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

ctype_abuse_head()
{
	local ctypefn reftype desc

	ctypefn=$1
	reftype=$2

	case $reftype in
	var)	desc="variable";;
	ptr)	desc="pointer dereference";;
	array)	desc="array element";;
	funcall)
		desc="function call";;
	esac

	atf_set "descr" "Test that $ctypefn warns on $desc of type char"
	atf_set "require.progs" "cc"
}

ctype_abuse_body()
{
	local ctypefn reftype

	ctypefn=$1
	reftype=$2

	case $reftype in
	var)	decl='x'; ref='x';;
	ptr)	decl='*x'; ref='*x';;
	array)	decl='x[]'; ref='x[0]';;
	funcall)
		decl='f(void)'; ref='f()';;
	esac

	cat >test.c <<EOF
#include <ctype.h>

extern char $decl;

int
g(void)
{

	return $ctypefn($ref);
}
EOF
	case $reftype in
	var)	atf_expect_fail 'PR lib/58912: ctype(3) abuse detection' \
		    ' fails for variable references';;
	esac
	atf_check -s not-exit:0 \
	    -e match:'array subscript has type.*char.*-W.*char-subscripts' \
	    cc -c -Wall -Werror test.c
}

ctype_abuse_tests()
{
	local ctypefn reftype tc

	for ctypefn in \
		isalpha \
		isupper \
		islower \
		isdigit \
		isxdigit \
		isalnum \
		isspace \
		ispunct \
		isprint \
		isgraph \
		iscntrl \
		isblank \
		toupper \
		tolower \
		# end of ctypefn enumeration
	do
	        for reftype in var ptr array funcall; do
			tc=${ctypefn}_${reftype}
			eval "atf_test_case $tc"
			eval "${tc}_head()
			{
				ctype_abuse_head $ctypefn $reftype
			}"
			eval "${tc}_body()
			{
				ctype_abuse_body $ctypefn $reftype
			}"
			atf_add_test_case $tc
		done
	done
}

atf_init_test_cases()
{

	ctype_abuse_tests
}
