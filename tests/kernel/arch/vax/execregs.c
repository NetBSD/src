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
	return x;
}

int
execregschild(char *path)
{
	/* fp: frame pointer, nonnull */
	/* ap: argument pointer, on user stack, nonnull */
	/* sp: stack pointer, nonnull */
	register long r0 __asm("r0") = nonnull(0x10);
	register long r1 __asm("r1") = nonnull(1);
	register long r2 __asm("r2") = nonnull(2);
	register long r3 __asm("r3") = nonnull(3);
	register long r4 __asm("r4") = nonnull(4);
	register long r5 __asm("r5") = nonnull(5);
	register long r6 __asm("r6") = nonnull(6);
	register long r7 __asm("r7") = nonnull(7);
	register long r8 __asm("r8") = nonnull(8);
	register long r9 __asm("r9") = nonnull(9);
	register long r10 __asm("r10") = nonnull(10);
	register long r11 __asm("r11") = nonnull(11);
	/* pc: user PC, will be nonnull */
	/* psl: processor status longword, will be nonnull */

	char *argv[] = {path, NULL};
	char **envp = environ;

	/*
	 * Not perfect -- compiler might use some registers for
	 * stack/argument transfers, but all the arguments are nonnull
	 * so this is probably a good test anyway.
	 */
	__asm volatile("" :
	    "+r"(r0),
	    "+r"(r1),
	    "+r"(r2),
	    "+r"(r3),
	    "+r"(r4),
	    "+r"(r5),
	    "+r"(r6),
	    "+r"(r7),
	    "+r"(r8),
	    "+r"(r9),
	    "+r"(r10),
	    "+r"(r11)
	    :: "memory");

	return execve(path, argv, envp);
}

pid_t
spawnregschild(char *path, int fd)
{
	/* fp: frame pointer, nonnull */
	/* ap: argument pointer, on user stack, nonnull */
	/* sp: stack pointer, nonnull */
	register long r0 __asm("r0") = nonnull(0x10);
	register long r1 __asm("r1") = nonnull(1);
	register long r2 __asm("r2") = nonnull(2);
	register long r3 __asm("r3") = nonnull(3);
	register long r4 __asm("r4") = nonnull(4);
	register long r5 __asm("r5") = nonnull(5);
	register long r6 __asm("r6") = nonnull(6);
	register long r7 __asm("r7") = nonnull(7);
	register long r8 __asm("r8") = nonnull(8);
	register long r9 __asm("r9") = nonnull(9);
	register long r10 __asm("r10") = nonnull(10);
	register long r11 __asm("r11") = nonnull(11);
	/* pc: user PC, will be nonnull */
	/* psl: processor status longword, will be nonnull */

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
	    "+r"(r0),
	    "+r"(r1),
	    "+r"(r2),
	    "+r"(r3),
	    "+r"(r4),
	    "+r"(r5),
	    "+r"(r6),
	    "+r"(r7),
	    "+r"(r8),
	    "+r"(r9),
	    "+r"(r10),
	    "+r"(r11)
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
