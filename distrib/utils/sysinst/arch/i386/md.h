/*	$NetBSD: md.h,v 1.3 1997/10/17 22:18:06 phil Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
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

/* md.h -- Machine specific definitions for the i386 */

/* constants and defines */

/* Megs required for a full X installation. */
#define XNEEDMB 50

/* fdisk partition information dlxxxx => NetBSD disklabel, bxxxx => bios */
EXTERN int bcyl, bhead, bsec, bsize, bcylsize;
EXTERN int bstuffset INIT(0);

enum info {ID,SIZE,START,FLAG};
EXTERN int part[4][4] INIT({{0}});
EXTERN int activepart;
EXTERN int bsdpart;
EXTERN int usefull;

/* Definition of files to retreive from ftp. */
EXTERN char ftp_prefix[STRSIZE] INIT("/binary/Tarfiles");
EXTERN char dist_postfix[STRSIZE] INIT(".tar.gz");
EXTERN char *dist_list[]
#ifdef MAIN
= { "kern%s%s", "base%s%s", "comp%s%s", "etc%s%s", "games%s%s", "man%s%s",
    "misc%s%s", "text%s%s", "CKSUMS", NULL }
#endif
;
EXTERN char *fd_last[]
#ifdef MAIN
= { "ab", "ab", "ab", "ab", "ab", "ab", "ab", "ab", NULL}
#endif
;

EXTERN char *fdtype INIT("msdos");

/* prototypes */

/* from fdisk.c */
void	set_fdisk_geom (void);
void	disp_cur_geom (void);
int	check_geom (void);
int 	partsoverlap (int, int);
void	get_fdisk_info (void);
