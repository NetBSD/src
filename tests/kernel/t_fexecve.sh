# $NetBSD: t_fexecve.sh,v 1.2 2023/05/04 00:02:10 gutteridge Exp $
#
# Copyright (c) 2019 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Christos Zoulas.
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

HELPER=$(atf_get_srcdir)/h_fexecve

atf_test_case fexecve_elf
fexecve_elf_head()
{
	atf_set "descr" "Test fexecve with ELF executables"
}

fexecve_elf_body()
{
	cat > hello.c << EOF
#include <stdio.h>
int main(void) {
        printf("hello world\n");
        return 0;
}
EOF
	atf_check -s exit:0 -o ignore -e ignore cc -o hello hello.c
	atf_check -o inline:"hello world\n" ${HELPER} ./hello
}

atf_test_case fexecve_script
fexecve_script_head()
{
	atf_set "descr" "Test fexecve with a shell script"
}

fexecve_script_body()
{
	cat > goodbye << EOF
#!/bin/sh
echo goodbye
EOF
	atf_check -s exit:0 -o ignore -e ignore chmod +x goodbye
	atf_check -s exit:0 -o inline:"goodbye\n" ${HELPER} ./goodbye
}

atf_init_test_cases()
{
	atf_add_test_case fexecve_elf
	atf_add_test_case fexecve_script
}
