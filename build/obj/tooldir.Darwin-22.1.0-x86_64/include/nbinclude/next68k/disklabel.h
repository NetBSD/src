/*	$NetBSD: disklabel.h,v 1.7 2011/08/30 12:39:57 bouyer Exp $	*/
/*
 * Copyright (c) 1994 Rolf Grossmann
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _MACHINE_DISKLABEL_H_
#define _MACHINE_DISKLABEL_H_

#if HAVE_NBTOOL_CONFIG_H
#include <nbinclude/sys/bootblock.h>
#else
#include <sys/bootblock.h>
#endif /* HAVE_NBTOOL_CONFIG_H */

#if 0 /* XXX the following doesn't work - still need to find a proper place for the NetBSD disklabel */
/*
 * The NetBSD disklabel is located in the last sector of the next68k label
 * area, so that it can coexists with a next68k v3 label
 */
#define LABELUSESMBR	0	/* no MBR partitionning */
#define	LABELSECTOR	15	/* sector containing label */
#define	LABELOFFSET	0	/* offset of label in sector */
#else
#define LABELUSESMBR	0	/* no MBR partitionning */
#define LABELSECTOR 0
#define LABELOFFSET 0
#endif /* 0 */
#define	MAXPARTITIONS	8	/* number of partitions */
#define	RAW_PART	2	/* raw partition: xx?c */

#define IS_DISKLABEL(l) ((l)->cd_version == NEXT68K_LABEL_CD_V1 || \
			 (l)->cd_version == NEXT68K_LABEL_CD_V2 || \
                         (l)->cd_version == NEXT68K_LABEL_CD_V3)

#define CD_UNINIT	0x80000000 /* label is uninitialized */

struct cpu_disklabel {
	int od_version;
};

#endif /* _MACHINE_DISKLABEL_H_ */
