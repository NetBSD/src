/*	$NetBSD: mbr.h,v 1.12 2003/05/07 19:02:53 dsl Exp $	*/

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
 *      This product includes software developed for the NetBSD Project by
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

#include <sys/disklabel_mbr.h>

/*      
 * XXX  
 */     
#define MBR_SECSIZE     512

#define MBR_PUT_LSCYL(c)		((c) & 0xff)
#define MBR_PUT_MSCYLANDSEC(c,s)	(((s) & 0x3f) | (((c) >> 2) & 0xc0))

/* incore fdisk (mbr, bios) geometry */
EXTERN int bcyl, bhead, bsec, bsize, bcylsize;

/* incore copy of  MBR partitions */
EXTERN struct mbr_partition *part;
EXTERN int activepart;
EXTERN int bsdpart;			/* partition in use by NetBSD */
EXTERN int usefull;			/* on install, clobber entire disk */

extern mbr_sector_t mbr;


/* from mbr.c */
void	set_fdisk_geom (void);	/* edit incore BIOS geometry */
void	disp_cur_geom (void);
int	check_geom (void);		/* primitive geometry sanity-check */

void	disp_cur_part (struct mbr_partition *, int, int);
int	edit_mbr (mbr_sector_t *);
int 	partsoverlap (struct mbr_partition *, int, int);

/* from mbr.c */
 
int     read_mbr (const char *, mbr_sector_t *, size_t);
int     write_mbr (const char *, mbr_sector_t *, size_t, int);
int     valid_mbr (mbr_sector_t *);
int	guess_biosgeom_from_mbr (mbr_sector_t *, int *, int *, int *);
int	md_bios_info (char *);
void	set_bios_geom (int, int, int);
int	otherpart (int);
int	ourpart (int);
char	*get_partname (int);
void	edit_ptn_bounds(void);
#if defined(__i386__) || defined(__x86_64__)
#define BOOTSEL
void	disp_bootsel(void);
void	edit_bootsel_entry(int);
void	edit_bootsel_timeout(void);
void	edit_bootsel_default_ptn(int);
void	edit_bootsel_default_disk(int);
#endif

