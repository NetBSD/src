/*	$NetBSD: disklabel.h,v 1.1.28.1 2007/02/27 16:50:20 yamt Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#ifndef _EWS4800MIPS_DISKLABEL_H_
#define	_EWS4800MIPS_DISKLABEL_H_

#if HAVE_NBTOOL_CONFIG_H
#include <nbinclude/machine/pdinfo.h>
#include <nbinclude/machine/vtoc.h>
#else
#include <machine/pdinfo.h>
#include <machine/vtoc.h>
#endif

/*
 * UX reserved partition
 *	0	/
 *	1	swap
 *	2	/usr
 *	3	/stand (bfs)
 *	6	whole disk
 *	7	boot block (VTOC)
 *	8	/var
 *	9	/home
 */

#define	MAXPARTITIONS	16
/* 'p'-partition is mostly familiar with EWS-UX */
#define	RAW_PART	15

#define	LABELSECTOR	9	/* See ews4800mips/disksubr/disksubr_conv.c */
#define	LABELOFFSET	0

struct cpu_disklabel {
	/* EWS-UX native disklabel */
	struct pdinfo_sector pdinfo;
	struct vtoc_sector vtoc;
};

#if defined(_KERNEL)
/* VTOC <-> disklabel conversion ops. */
struct disklabel;
void vtoc_set_default(struct cpu_disklabel *, struct disklabel *);
void disklabel_to_vtoc(struct cpu_disklabel *, struct disklabel *);
void vtoc_to_disklabel(struct cpu_disklabel *, struct disklabel *);
void disklabel_set_default(struct disklabel *);
bool disklabel_sanity(struct disklabel *);
#endif

#endif /* _EWS4800MIPS_DISKLABEL_H_ */
