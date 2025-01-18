# $NetBSD: t_gcov.sh,v 1.1 2025/01/18 22:31:22 rillig Exp $
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
#

after_exec_head()
{
}
after_exec_body()
{
	atf_require_prog cat
	atf_require_prog gcc
	atf_require_prog gcov
	atf_require_prog grep

	cat <<EOF >prog.c
#include <unistd.h>

int
main(void)
{
	pid_t pid = vfork();
	switch (pid) {
	case 0:
		execl("/bin/sh", "sh", "-c", ":", (const char *)0);
		/* FALLTHROUGH */
	case -1:
		write(2, "error\n", 6);
		_exit(1);
	}

	write(1, "reached\n", 8);
	return 0;
}
EOF

	cat <<EOF >prog.c.gcov.expected
        -:    0:Source:prog.c
        -:    0:Graph:prog.gcno
        -:    0:Data:prog.gcda
        -:    0:Runs:1
        -:    1:#include <unistd.h>
        -:    2:
        -:    3:int
        1:    4:main(void)
        -:    5:{
        1:    6:	pid_t pid = vfork();
        1:    7:	switch (pid) {
        1:    8:	case 0:
        1:    9:		execl("/bin/sh", "sh", "-c", ":", (const char *)0);
        -:   10:		/* FALLTHROUGH */
    #####:   11:	case -1:
    #####:   12:		write(2, "error\n", 6);
    #####:   13:		_exit(1);
        -:   14:	}
        -:   15:
    #####:   16:	write(1, "reached\n", 8);
    #####:   17:	return 0;
        -:   18:}
EOF

	atf_check \
	    gcc --coverage -c prog.c
	atf_check \
	    gcc --coverage -o prog prog.o
	atf_check -o inline:'reached\n' \
	    ./prog
	atf_check -o ignore \
	    gcov prog.c

	atf_check -o file:prog.c.gcov.expected \
	    cat prog.c.gcov

	# FIXME: The code was reached once but is reported as unreached.
	atf_check -o ignore \
	    grep "#####.*reached" prog.c.gcov
}

atf_init_test_cases()
{
	atf_add_test_case after_exec
}
