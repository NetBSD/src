/*	$NetBSD: md.h,v 1.5 2003/05/07 03:34:23 shin Exp $	*/

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

/* md.h -- Machine specific definitions for the playstation2 */


#include <machine/cpu.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "mbr.h"

#define	PART_ROOT	A
#define	PART_SWAP	B
#define	PART_RAW	C
#define	PART_USR	D
#define	PART_FIRST_FREE	D

#define	DEFSWAPRAM	64	/* Assume at least this RAM for swap calc */
#define	DEFROOTSIZE	32	/* Default root size */
#define	DEFVARSIZE	0	/* Default /var size, if created */
#define	DEFUSRSIZE	0	/* Default /usr size, if created */
#define	STDNEEDMB	0	/* Min space for non X install */
#define	XNEEDMB		0	/* Extra megs for full X installation(not yet)*/

/*
 *  Default filesets to fetch and install during installation
 *  or upgrade. The standard sets are:
 *      base, etc, comp, games, man, misc, text
 */
EXTERN distinfo dist_list[]
#ifdef MAIN
= {
    {"kern-GENERIC",	1, "Kernel       : "},
    {"base",		1, "Base         : "},
    {"etc",		1, "System (/etc): "},
    {"comp",		1, "Compiler     : "},
    {"games",		1, "Games        : "},
    {"man",		1, "Manuals      : "},
    {"misc",		1, "Miscellaneous: "},
    {"text",		1, "Text tools   : "},
    { NULL,		0, NULL }
}
#endif
;

/*
 * Disk names accepted as valid targets for a from-scratch installation.
 *
 * On playstation2, we allow "wd" for IDE disk.
 */
EXTERN	char *disk_names[]
#ifdef MAIN
= {"wd", NULL}
#endif
;

/*
 * Legal start character for a disk for checking input.
 * this must return 1 for a character that matches the first
 * characters of each member of disk_names.
 *
 * On playstation2, that means matching 'w' for IDE disk.
 */
#define	ISDISKSTART(dn)	(dn == 'w')

/*
 * Machine-specific command to write a new label to a disk.
 */
#define	DISKLABEL_CMD "disklabel -w -r"

/*
 * Default fileystem type for IDE disk.
 * On playstation2, that is msdos.
 */
EXTERN	char *fdtype INIT("msdos");
