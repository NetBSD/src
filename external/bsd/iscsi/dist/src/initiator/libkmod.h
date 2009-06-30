/*	$NetBSD: libkmod.h,v 1.2 2009/06/30 02:44:52 agc Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
#ifndef LIBKMOD_H_
#define LIBKMOD_H_	20090619

#include <sys/module.h>

#include <stdio.h>

/* this struct describes the loaded modules in the kernel */
typedef struct kernel_t {
	size_t	 	size;	/* size of iovec array */
	size_t		c;	/* counter during "read" operations */
	struct iovec	iov;	/* iovecs from the modctl operation */
} kernel_t;

/* this struct describes a module */
typedef struct kmod_t {
	char		*name;		/* module name */
	char		*class;		/* module class */
	char		*source;	/* source of module loading */
	int		 refcnt;	/* reference count */
	unsigned	 size;		/* size of binary module */
	char		*required;	/* any pre-reqs module has */
} kmod_t;

/* low level open, read, write ops */
int openkmod(kernel_t *);
int readkmod(kernel_t *, kmod_t *);
void freekmod(kmod_t *);
int closekmod(kernel_t *);

/* high-level kmod operations */
int kmodstat(const char *, FILE *);
int kmodload(const char *);
int kmodunload(const char *);

#endif
