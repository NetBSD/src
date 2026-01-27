#	$NetBSD: t_libm_cabs.sh,v 1.1 2026/01/27 20:01:47 mrg Exp $
#
# Copyright (c) 2026 Matthew R. Green
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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.


atf_test_case libm_cabs
libm_cabs_head() {
	atf_set "descr" "compile using __builtin_cabsl(3) which should be renamed"
	atf_set "require.progs" "cc"
}

#
# Simple program that misses including <complex.h> so that the renames
# GCC itself is supposed to be doing are also applied, which makes their
# uses in libgfortran use the correct symbols.  The use of
# "__builtin_cabsl" should become "__c99_cabsl".
#
libm_cabs_body() {
	cat > cabsl.c << EOF
long double my_foo(long double _Complex);
long double
my_foo(long double _Complex z)
{
	return __builtin_cabsl(z);
}
EOF
	atf_check -s exit:0 -o ignore -e ignore cc -O3 -c cabsl.c
	atf_check -s exit:0 -o match:__c99_cabsl -e empty objdump -dr cabsl.o
}

atf_init_test_cases()
{

	atf_add_test_case libm_cabs
}
