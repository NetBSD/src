/* This testcase is part of GDB, the GNU debugger.

   Copyright 2020-2023 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


/* This file implements simple Linux syscall wrappers, to be used by tests that
   need to know exactly where the syscall instructions are.  */

#include <asm/unistd.h>

/* int my_execve (const char *file, char *argv[], char *envp[]);  */

.global my_execve
my_execve:

#if defined(__x86_64__)

	mov $__NR_execve, %rax
	/* rdi, rsi and rdx already contain the right arguments.  */
my_execve_syscall:
	syscall
	ret

#elif defined(__i386__)

	mov $__NR_execve, %eax
	mov 4(%esp), %ebx
	mov 8(%esp), %ecx
	mov 12(%esp), %edx
my_execve_syscall:
	int $0x80
	ret

#elif defined(__aarch64__)

	mov x8, #__NR_execve
	/* x0, x1 and x2 already contain the right arguments.  */
my_execve_syscall:
	svc #0

#else
# error "Unsupported architecture"
#endif

	.section	.note.GNU-stack,"",@progbits
