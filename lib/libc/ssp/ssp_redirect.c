/*	$NetBSD: ssp_redirect.c,v 1.4 2026/10/07 00:51:18 riastradh Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: ssp_redirect.c,v 1.4 2026/10/07 00:51:18 riastradh Exp $");

#include <unistd.h>

/*
 * This file provides useless wrappers for getcwd, read, and readlink,
 * for compatibility with programs built against buggy definitions of
 * the ssp wrappers.  For the gory details, see PR lib/60858:
 * fortuitous embarrassment: fortify is all kinds of busted
 * <https://gnats.NetBSD.org/60858>.
 *
 * DO NOT ADD MORE FUNCTIONS HERE.  If you are tempted to add another
 * __ssp_protected_* symbol, YOU ARE DOING SOMETHING WRONG AND YOU MUST
 * UNDERSTAND WHY IT IS WRONG.
 */

__typeof(getcwd) __ssp_protected_getcwd;
__typeof(read) __ssp_protected_read;
__typeof(readlink) __ssp_protected_readlink;

char *
__ssp_protected_getcwd(char *buf, size_t len)
{

	return getcwd(buf, len);
}

ssize_t
__ssp_protected_read(int fd, void *buf, size_t len)
{

	return read(fd, buf, len);
}

ssize_t
__ssp_protected_readlink(const char *restrict path,
    char *restrict buf, size_t bufsiz)
{

	return readlink(path, buf, bufsiz);
}

/*
 * Remember: DO NOT ADD MORE FUNCTIONS HERE.  This file was a mistake;
 * we just can't get rid of it without breaking compatibility with
 * existing binaries (which don't even use the symbols, just demand
 * they be present).
 */
