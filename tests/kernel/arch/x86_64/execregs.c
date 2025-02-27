/*	$NetBSD: execregs.c,v 1.1 2025/02/27 00:55:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: execregs.c,v 1.1 2025/02/27 00:55:32 riastradh Exp $");

#include "execregs.h"

#include <errno.h>
#include <spawn.h>
#include <stddef.h>
#include <unistd.h>

extern char **environ;

static unsigned long
nonnull(unsigned long x)
{

	x |= x << 8;
	x |= x << 16;
	x |= x << 32;
	return x;
}

int
execregschild(char *path)
{
	/* rdi: used to pass exec arg0, nonnull anyway (path) */
	/* rsi: used to pass exec arg1, nonnull anyway (argv) */
	/* rdx: used to pass exec arg2, nonnull anyway (environ) */
	register long r10 __asm("r10") = nonnull(10);
	register long r8 __asm("r8") = nonnull(8);
	register long r9 __asm("r9") = nonnull(9);
	register long rcx __asm("rcx") = nonnull('c');
	register long r11 __asm("r11") = nonnull(11);
	register long r12 __asm("r12") = nonnull(12);
	register long r13 __asm("r13") = nonnull(13);
	register long r14 __asm("r14") = nonnull(14);
	register long r15 __asm("r15") = nonnull(15);
	/* rbp: frame pointer, can't touch that here, but it'll be nonnull */
	/* rbx: ps_strings, passed to child */
	register long rax __asm("rax") = nonnull('a');

	char *argv[] = {path, NULL};
	char **envp = environ;

	/*
	 * Not perfect -- compiler might use some registers for
	 * stack/argument transfers, but all the arguments are nonnull
	 * so this is probably a good test anyway.
	 */
	__asm volatile("" :
	    "+r"(r10),
	    "+r"(r8),
	    "+r"(r9),
	    "+r"(rcx),
	    "+r"(r11),
	    "+r"(r12),
	    "+r"(r13),
	    "+r"(r14),
	    "+r"(r15),
	    "+r"(rax)
	    :: "memory");

	return execve(path, argv, envp);
}

pid_t
spawnregschild(char *path, int fd)
{
	/* rdi: used to pass posix_spawn arg0, nonnull anyway (&pid) */
	/* rsi: used to pass posix_spawn arg1, nonnull anyway (path) */
	/* rdx: used to pass posix_spawn arg2, nonnull anyway (&fileacts) */
	register long r10 __asm("r10") = nonnull(10);
	/* r8: used to pass posix_spawn arg4, nonnull anyway (argv) */
	/* r9: used to pass posix_spawn arg5, nonnull anyway (environ) */
	/* rcx: used to pass posix_spawn arg3, nonnull anyway (&attr) */
	register long r11 __asm("r11") = nonnull(11);
	register long r12 __asm("r12") = nonnull(12);
	register long r13 __asm("r13") = nonnull(13);
	register long r14 __asm("r14") = nonnull(14);
	register long r15 __asm("r15") = nonnull(15);
	/* rbp: frame pointer, can't touch that here, but it'll be nonnull */
	/* rbx: ps_strings, passed to child */
	register long rax __asm("rax") = nonnull('a');

	char *argv[] = {path, NULL};
	char **envp = environ;
	posix_spawn_file_actions_t fileacts;
	posix_spawnattr_t attr;
	pid_t pid;
	int error;

	error = posix_spawn_file_actions_init(&fileacts);
	if (error)
		goto out;
	error = posix_spawn_file_actions_adddup2(&fileacts, fd, STDOUT_FILENO);
	if (error)
		goto out;
	error = posix_spawnattr_init(&attr);
	if (error)
		goto out;

	/*
	 * Not perfect -- compiler might use some registers for
	 * stack/argument transfers, but all the arguments are nonnull
	 * so this is probably a good test anyway.
	 */
	__asm volatile("" :
	    "+r"(r10),
	    "+r"(r11),
	    "+r"(r12),
	    "+r"(r13),
	    "+r"(r14),
	    "+r"(r15),
	    "+r"(rax)
	    :: "memory");

	error = posix_spawn(&pid, path, &fileacts, &attr, argv, envp);
	if (error)
		goto out;

out:	posix_spawnattr_destroy(&attr);
	posix_spawn_file_actions_destroy(&fileacts);
	if (error) {
		errno = error;
		return -1;
	}
	return 0;
}
