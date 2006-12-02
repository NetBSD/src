/* 	$NetBSD: idcr.h,v 1.1 2006/12/02 22:18:47 freza Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_virtex.h"

#ifndef	IDCR_BASE
#error "Internal DCR base address not defined"
#endif

#include <powerpc/cpu.h>

#ifndef	_VIRTEX_IDCR_H_
#define	_VIRTEX_IDCR_H_

/* Instruction OCM control */
#define	IDCR_ISINIT 		0x0000
#define	IDCR_ISFILL 		0x0001
#define	IDCR_ISARC 		0x0002
#define	IDCR_ISCNT 		0x0003

/* APU control */
#define	IDCR_UDICFG 		0x0004
#define	IDCR_APUCFG 		0x0005

/* Data OCM control */
#define	IDCR_DSARC 		0x0006
#define	IDCR_DSCNT 		0x0007

/* 8...11 reserved */

/* Temac HIF-to-GMI communication: arguments, ... */
#define	IDCR_HIF_ARG1 		0x000c
#define	IDCR_HIF_ARG0 		0x000d

/* ... control, ... */
#define	IDCR_HIF_CTRL 		0x000e
#define	HIF_CTRL_GMIADDR 	0x000003ff
#define	HIF_CTRL_SEL 		0x00000400 	/* Keep this bit zero! */
#define	HIF_CTRL_WRITE 		0x00008000 	/* Read otherwise */

/* ... status. */
#define	IDCR_HIF_STAT 		0x000f
#define	HIF_STAT_STATS 		0x00000001
#define	HIF_STAT_MIIRR 		0x00000002 	/* MII read ready */
#define	HIF_STAT_MIIWR 		0x00000004 	/* MII write ready */
#define	HIF_STAT_AFRR 		0x00000008 	/* Address filter read ready */
#define	HIF_STAT_AFWR 		0x00000010 	/* Address filter write ready */
#define	HIF_STAT_GMIRR 		0x00000020 	/* GMI read ready */
#define	HIF_STAT_GMIWR 		0x00000040 	/* GMI write ready */

#define	mtidcr(addr, val) 	mtdcr(IDCR_BASE + (addr), (val))
#define	mfidcr(addr) 		mfdcr(IDCR_BASE + (addr))

#endif /*_VIRTEX_IDCR_H_*/
