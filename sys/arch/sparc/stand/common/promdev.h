/*	$NetBSD: promdev.h,v 1.8 2003/02/26 17:39:08 pk Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <machine/bsd_openprom.h>

struct promdata {
	int	fd;			/* Openboot descriptor */
	struct	saioreq *si;		/* Oldmon IO request */
	int	devtype;		/* Kind of device we're booting from */
#define DT_BLOCK	1
#define DT_NET		2
#define DT_BYTE		3
	/* Hooks for netif.c */
	ssize_t	(*xmit) __P((struct promdata *, void *, size_t));
	ssize_t	(*recv) __P((struct promdata *, void *, size_t));
};

#define DDB_MAGIC0	( ('D'<<24) | ('D'<<16) | ('B'<<8) | ('0') )
#define DDB_MAGIC1	( ('D'<<24) | ('D'<<16) | ('B'<<8) | ('1') )
#define DDB_MAGIC2	( ('D'<<24) | ('D'<<16) | ('B'<<8) | ('2') )

extern time_t	getsecs __P((void));
extern char	*prom_bootdevice;
extern int	cputyp, nbpg, pgofset, pgshift;
extern int	debug;

/* Note: dvma_*() routines are for "oldmon" machines only */
extern void	dvma_init __P((void));
extern char	*dvma_mapin __P((char *, size_t));
extern char	*dvma_mapout __P((char *, size_t));
extern char	*dvma_alloc __P((int));
extern void	dvma_free __P((char *, int));

/* In net.c: */
extern int	net_open __P((struct promdata *));
extern int	net_close __P((struct promdata *));
extern int	net_mountroot __P((void));

/* In str0.S: */
extern void	sparc_noop __P((void));
extern void	*romp;
