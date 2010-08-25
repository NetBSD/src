/*	$NetBSD: promdev.h,v 1.15 2010/08/25 18:11:54 christos Exp $ */

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
	ssize_t	(*xmit)(struct promdata *, void *, size_t);
	ssize_t	(*recv)(struct promdata *, void *, size_t);
};

#define DDB_MAGIC0	( ('D'<<24) | ('D'<<16) | ('B'<<8) | ('0') )
#define DDB_MAGIC1	( ('D'<<24) | ('D'<<16) | ('B'<<8) | ('1') )
#define DDB_MAGIC2	( ('D'<<24) | ('D'<<16) | ('B'<<8) | ('2') )

#define	MAX_PROM_PATH	128

extern char	prom_bootdevice[MAX_PROM_PATH];
extern int	cputyp, nbpg, pgofset, pgshift;
extern int	debug;

/* Note: dvma_*() routines are for "oldmon" machines only */
extern void	dvma_init(void);
extern char	*dvma_mapin(char *, size_t);
extern char	*dvma_mapout(char *, size_t);
extern char	*dvma_alloc(int);
extern void	dvma_free(char *, int);

/* In net.c: */
extern int	net_open(struct promdata *);
extern int	net_close(struct promdata *);
extern int	net_mountroot(void);

/* In mmu.c */
extern int	mmu_init(void);
extern int	(*pmap_map)(vaddr_t, paddr_t, psize_t);
extern int	(*pmap_extract)(vaddr_t, paddr_t *);

/* In promdev.c */
extern int	bootdev_isfloppy(void);

/* In str0.S: */
extern void	sparc_noop(void);
extern void	*romp;
