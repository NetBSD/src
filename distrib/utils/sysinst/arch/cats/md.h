/*	$NetBSD: md.h,v 1.5 2003/05/30 11:56:25 dsl Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.
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

/* md.h -- Machine specific definitions for the cats */

/* Constants and defines */

/* Megs required for a full X installation. */
#define XNEEDMB 60

/*
 *  Default filesets to fetch and install during installation
 *  or upgrade. The standard sets are:
 *      base, etc, comp, games, man, misc, text,
 *      xbase, xfont, xserver, xcontrib, xcomp.
 *
 * cats has the MD set kern first, because generic kernels are too
 * big to fit on install floppies. 
 *
 * Third entry is the last extension name in the split sets for loading
 * from floppy.
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

    {"xbase",		1, "X11 clients  : "},
    {"xfont",		1, "X11 fonts    : "},
    {"xserver",		1, "X11 servers  : "},
    {"xcontrib",	1, "X11 contrib  : "},
    {"xcomp",		1, "X programming: "},
    {"xmisc",		1, "X11 Misc.    : "},
    { NULL,		0, NULL }
}
#endif
;

/*
 * Machine-specific command to write a new label to a disk.
 * For example, cats uses "/sbin/disklabel -w -r", just like cats
 * miniroot scripts, though this may leave a bogus incore label.
 * Sun ports should probably use  DISKLABEL_CMD "/sbin/disklabel -w"
 * to get incore to ondisk inode translation for the Sun proms.
 * If not defined, we assume the port does not support disklabels and
 * hand-edited disklabel will NOT be written by MI code.
 */
#define DISKLABEL_CMD "disklabel -w -r"

/*
 * Default fileystem type for floppy disks.
 * On cats, that is msdos.
 */
EXTERN char *fdtype INIT("msdos");

#define LIB_COUNT       0
#define LIB_MOVE        1

/*
 *  prototypes for MD code.
 */
