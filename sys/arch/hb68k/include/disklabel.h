/*	$NetBSD: disklabel.h,v 1.2 2026/08/07 01:59:42 thorpej Exp $	*/

/*
 * Copyright (c) 1994 Christopher G. Demetriou
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
 *	This product includes software developed by Christopher G. Demetriou.
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

#ifndef _HB68K_DISKLABEL_H_
#define _HB68K_DISKLABEL_H_

/* number of boot pieces , ie xxboot bootxx */
#define NUMBOOT		0

/*
 * Use the traditional hp300 layout.  Rationale:
 * - Room at the beginning of the disk to load a small primary bootstrap
 *   if that's what the ROM wants to do.  The bootstrap can encapsulate
 *   the disklabel; installboot(8) simply needs to understand that the
 *   label needs to be preserved.
 * - Room at the beginning of the disk for an MBR table to be there, if
 *   that's necessary for some reason, although we don't do much with
 *   that MBR table other than allow it to exist.
 *
 * The implementation of readdisklabel() in subr_disk_4bsd.c will check
 * this location first, and if not found, will scan the first 3 disk
 * sectors looking for it, and use that same location in writedisklabel()
 * if updating an existing label.
 */
#define	LABELUSESMBR	0			/* no MBR partitionning */
#define	LABELSECTOR	2			/* sector containing label */
#define	LABELOFFSET	0			/* offset of label in sector */
#define	MAXPARTITIONS	8			/* number of partitions */
#define	RAW_PART	2			/* raw partition: xx?c */

/* Just a dummy. */
struct cpu_disklabel {
	int	cd_dummy;			/* must have one element */
};

#endif /* _HB68K_DISKLABEL_H_ */
