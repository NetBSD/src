#	$NetBSD: t_libgomp.sh,v 1.1.2.2 2019/06/10 22:10:12 christos Exp $
#
# Copyright (c) 2019 Matthew R. Green
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
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
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


atf_test_case libgomp
libgomp_head() {
	atf_set "descr" "compile and hello world with -fopenmp"
	atf_set "require.progs" "cc"
}

libgomp_body() {
	cat > hello.c << EOF
#include <stdio.h>
#include <stdlib.h>
int main(void) {printf("hello world\n");exit(0);}
EOF
	atf_check -s exit:0 -o ignore -e ignore cc -fopenmp -o hellogomp hello.c
	atf_check -s exit:0 -o inline:"hello world\n" ./hellogomp
}

atf_init_test_cases()
{

	atf_add_test_case libgomp
}
