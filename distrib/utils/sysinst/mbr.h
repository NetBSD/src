/*	$NetBSD: mbr.h,v 1.2 1998/06/20 13:05:49 mrg Exp $	*/

/*
 * Copyright 1997, 1988 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
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
 *      This product includes software develooped for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * mbr.h -- definitions for reading, writing and editing DOS MBRs.
 * Use by including from md.h on ports  which use MBRs (i386, powerpc, arc)
 * naming convention:  dlxxxx => NetBSD disklabel, bxxxx => bios
 */

/* constants and defines */

/* incore fdisk (mbr, bios) geometry */
EXTERN int bcyl, bhead, bsec, bsize, bcylsize;
EXTERN int bstuffset INIT(0);

/* incore copy of  MBR partitions */
enum info {ID,SIZE,START,FLAG,SET};
EXTERN int part[4][5] INIT({{0}});
EXTERN int activepart;
EXTERN int bsdpart;			/* partition in use by NetBSD */
EXTERN int usefull;			/* on install, clobber entire disk */


/* from mbr.c */
void	set_fdisk_geom __P((void));	/* edit incore BIOS geometry */
void	disp_cur_geom __P((void));
int	check_geom __P((void));		/* primitive geometry sanity-check */

int	edit_mbr __P((void));		
int 	partsoverlap __P((int, int));	/* primive partition sanity-check */

/* from fdisk.c */
void	get_fdisk_info __P((void));	/* read from disk into core */
void	set_fdisk_info __P((void));	/* write incore info into disk */
