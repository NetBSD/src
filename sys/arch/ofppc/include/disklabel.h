/*	$NetBSD: disklabel.h,v 1.10.6.1 2012/02/18 07:32:50 mrg Exp $	*/

/*-
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _MACHINE_DISKLABEL_H_
#define _MACHINE_DISKLABEL_H_

#define LABELUSESMBR	0			/* no MBR partitionning */
#define	LABELSECTOR	1			/* sector containing label */
#define	LABELOFFSET	0			/* offset of label in sector */
#define	MAXPARTITIONS	16			/* number of partitions */
#define	RAW_PART	2			/* raw partition: xx?c */

#if HAVE_NBTOOL_CONFIG_H
#include <nbinclude/sys/bootblock.h>		/* MBR partition definitions */
#include <nbinclude/sys/disklabel_rdb.h>	/* RDB partition definitions */
#else
#include <sys/bootblock.h>			/* MBR partition definitions */
#include <sys/disklabel_rdb.h>			/* RDB partition definitions */
#endif /* HAVE_NBTOOL_CONFIG_H */

struct cpu_disklabel {
	daddr_t cd_start;	/* Offset to NetBSD partition in blocks */
	daddr_t cd_labelsector; /* label sector offset from cd_start */   
	int cd_labeloffset;     /* label byte offset within label sector */

	u_long rdblock;			/* may be RDBNULL which invalidates */
	u_long pblist[MAXPARTITIONS];	/* partblock number (RDB list order) */
	int pbindex[MAXPARTITIONS];	/* index of pblock (partition order) */
	int valid;			/* essential that this is valid */
};

#endif /* _MACHINE_DISKLABEL_H_ */
