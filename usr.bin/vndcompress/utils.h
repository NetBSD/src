/*	$NetBSD: utils.h,v 1.3.8.2 2014/08/20 00:05:05 tls Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef	VNDCOMPRESS_UTILS_H
#define	VNDCOMPRESS_UTILS_H

#include <sys/types.h>
#include <sys/cdefs.h>

/* XXX Seems to be missing from <stdio.h>...  */
int	snprintf_ss(char *restrict, size_t, const char *restrict, ...)
	    __printflike(3, 4);
int	vsnprintf_ss(char *restrict, size_t, const char *restrict, va_list)
	    __printflike(3, 0);

ssize_t	read_block(int, void *, size_t);
ssize_t	pread_block(int, void *, size_t, off_t);
void	err_ss(int, const char *) __dead;
void	errx_ss(int, const char *, ...) __printflike(2, 3) __dead;
void	warn_ss(const char *);
void	warnx_ss(const char *, ...) __printflike(1, 2);
void	vwarnx_ss(const char *, va_list) __printflike(1, 0);
void	block_signals(sigset_t *);
void	restore_sigmask(const sigset_t *);

#endif	/* VNDCOMPRESS_UTILS_H */
