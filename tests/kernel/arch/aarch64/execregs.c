/*	$NetBSD: execregs.c,v 1.1 2025/02/27 00:55:31 riastradh Exp $	*/

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
__RCSID("$NetBSD: execregs.c,v 1.1 2025/02/27 00:55:31 riastradh Exp $");

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
	/* x0: used to pass exec arg0, nonnull anyway (path) */
	/* x1: used to pass exec arg1, nonnull anyway (argv) */
	/* x2: used to pass exec arg2, nonnull anyway (environ) */
	register long x3 __asm("x3") = nonnull(3);
	register long x4 __asm("x4") = nonnull(4);
	register long x5 __asm("x5") = nonnull(5);
	register long x6 __asm("x6") = nonnull(6);
	register long x7 __asm("x7") = nonnull(7);
	register long x8 __asm("x8") = nonnull(8);
	register long x9 __asm("x9") = nonnull(9);
	register long x10 __asm("x10") = nonnull(10);
	register long x11 __asm("x11") = nonnull(11);
	register long x12 __asm("x12") = nonnull(12);
	register long x13 __asm("x13") = nonnull(13);
	register long x14 __asm("x14") = nonnull(14);
	register long x15 __asm("x15") = nonnull(15);
	register long x16 __asm("x16") = nonnull(16);
	register long x17 __asm("x17") = nonnull(17);
	register long x18 __asm("x18") = nonnull(18);
	register long x19 __asm("x19") = nonnull(19);
	register long x20 __asm("x20") = nonnull(20);
	register long x21 __asm("x21") = nonnull(21);
	register long x22 __asm("x22") = nonnull(22);
	register long x23 __asm("x23") = nonnull(23);
	register long x24 __asm("x24") = nonnull(24);
	register long x25 __asm("x25") = nonnull(25);
	register long x26 __asm("x26") = nonnull(26);
	register long x27 __asm("x27") = nonnull(27);
	register long x28 __asm("x28") = nonnull(28);
	/* x29: frame pointer, nonnull anyway */
	/* x30: link register, nonnull anyway */

	char *argv[] = {path, NULL};
	char **envp = environ;

	/*
	 * Not perfect -- compiler might use some registers for
	 * stack/argument transfers, but all the arguments are nonnull
	 * so this is probably a good test anyway.
	 */
	__asm volatile("" :
	    "+r"(x3),
	    "+r"(x4),
	    "+r"(x5),
	    "+r"(x6),
	    "+r"(x7),
	    "+r"(x8),
	    "+r"(x9),
	    "+r"(x10),
	    "+r"(x11),
	    "+r"(x12),
	    "+r"(x13),
	    "+r"(x14),
	    "+r"(x15),
	    "+r"(x16),
	    "+r"(x17)
	    :: "memory");
	/* pacify gcc error: more than 30 operands in 'asm' */
	__asm volatile("" :
	    "+r"(x18),
	    "+r"(x19),
	    "+r"(x20),
	    "+r"(x21),
	    "+r"(x22),
	    "+r"(x23),
	    "+r"(x24),
	    "+r"(x25),
	    "+r"(x26),
	    "+r"(x27),
	    "+r"(x28)
	    :: "memory");

	return execve(path, argv, envp);
}

pid_t
spawnregschild(char *path, int fd)
{
	/* x0: used to pass posix_spawn arg0, nonnull anyway (&pid) */
	/* x1: used to pass posix_spawn arg1, nonnull anyway (path) */
	/* x2: used to pass posix_spawn arg2, nonnull anyway (&fileacts) */
	/* x3: used to pass posix_spawn arg3, nonnull anyway (&attr) */
	/* x4: used to pass posix_spawn arg3, nonnull anyway (argv) */
	/* x5: used to pass posix_spawn arg3, nonnull anyway (environ) */
	register long x6 __asm("x6") = nonnull(6);
	register long x7 __asm("x7") = nonnull(7);
	register long x8 __asm("x8") = nonnull(8);
	register long x9 __asm("x9") = nonnull(9);
	register long x10 __asm("x10") = nonnull(10);
	register long x11 __asm("x11") = nonnull(11);
	register long x12 __asm("x12") = nonnull(12);
	register long x13 __asm("x13") = nonnull(13);
	register long x14 __asm("x14") = nonnull(14);
	register long x15 __asm("x15") = nonnull(15);
	register long x16 __asm("x16") = nonnull(16);
	register long x17 __asm("x17") = nonnull(17);
	register long x18 __asm("x18") = nonnull(18);
	register long x19 __asm("x19") = nonnull(19);
	register long x20 __asm("x20") = nonnull(20);
	register long x21 __asm("x21") = nonnull(21);
	register long x22 __asm("x22") = nonnull(22);
	register long x23 __asm("x23") = nonnull(23);
	register long x24 __asm("x24") = nonnull(24);
	register long x25 __asm("x25") = nonnull(25);
	register long x26 __asm("x26") = nonnull(26);
	register long x27 __asm("x27") = nonnull(27);
	register long x28 __asm("x28") = nonnull(28);
	/* x29: frame pointer, nonnull anyway */
	/* x30: link register, nonnull anyway */

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
	    "+r"(x6),
	    "+r"(x7),
	    "+r"(x8),
	    "+r"(x9),
	    "+r"(x10),
	    "+r"(x11),
	    "+r"(x12),
	    "+r"(x13),
	    "+r"(x14),
	    "+r"(x15),
	    "+r"(x16),
	    "+r"(x17),
	    "+r"(x18),
	    "+r"(x19),
	    "+r"(x20)
	    :: "memory");
	/* pacify gcc error: more than 30 operands in 'asm' */
	__asm volatile("" :
	    "+r"(x21),
	    "+r"(x22),
	    "+r"(x23),
	    "+r"(x24),
	    "+r"(x25),
	    "+r"(x26),
	    "+r"(x27),
	    "+r"(x28)
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
