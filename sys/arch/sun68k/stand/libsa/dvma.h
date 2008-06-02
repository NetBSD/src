/*	$NetBSD: dvma.h,v 1.3.74.1 2008/06/02 13:22:47 mjf Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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

extern char *	(*dvma_alloc_p)(int);
#define dvma_alloc	(*dvma_alloc_p)

extern void	(*dvma_free_p)(char *, int);
#define dvma_free	(*dvma_free_p)

extern char *	(*dvma_mapin_p)(char *, int);
#define dvma_mapin	(*dvma_mapin_p)

extern void	(*dvma_mapout_p)(char *, int);
#define dvma_mapout	(*dvma_mapout_p)

/*
 * This stuff is not really DVMA-related,
 * but is mapping related.  Oh well...
 */
extern char *	(*dev_mapin_p)(int, u_long, int);
#define dev_mapin	(*dev_mapin_p)

/*
 * Called from SRT1.c to set the pointers declared above.
 */
void	sun2_init(void);
void	sun3_init(void);
void	sun3x_init(void);
