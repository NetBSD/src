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
	register long edi __asm("edi") = nonnull('d');
	register long esi __asm("esi") = nonnull('s');
	/* ebp: frame pointer, can't touch that here, but it'll be nonnull */
	/* ebx: ps_strings, passed to child */
	register long edx __asm("edx") = nonnull('x');
	register long ecx __asm("ecx") = nonnull('c');
	register long eax __asm("eax") = nonnull('a');

	char *argv[] = {path, NULL};
	char **envp = environ;

	/*
	 * Not perfect -- compiler might use some registers for
	 * stack/argument transfers, but all the arguments are nonnull
	 * so this is probably a good test anyway.
	 */
	__asm volatile("" :
	    "+r"(edi),
	    "+r"(esi),
	    "+r"(edx),
	    "+r"(ecx),
	    "+r"(eax)
	    :: "memory");

	return execve(path, argv, envp);
}

pid_t
spawnregschild(char *path, int fd)
{
	register long edi __asm("edi") = nonnull('d');
	register long esi __asm("esi") = nonnull('s');
	/* ebp: frame pointer, can't touch that here, but it'll be nonnull */
	/* ebx: ps_strings, passed to child */
	register long edx __asm("edx") = nonnull('x');
	register long ecx __asm("ecx") = nonnull('c');
	register long eax __asm("eax") = nonnull('a');

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
	    "+r"(edi),
	    "+r"(esi),
	    "+r"(edx),
	    "+r"(ecx),
	    "+r"(eax)
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
