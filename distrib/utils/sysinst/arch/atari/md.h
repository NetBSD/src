/*	$NetBSD: md.h,v 1.4 2002/06/19 11:44:26 leo Exp $	*/

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

/* md.h -- Machine specific definitions for the atari */


#include <machine/cpu.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* constants and defines */
#define PART_ROOT	A
#define PART_SWAP	B
#define PART_RAW	C
#define PART_USR	D	/* Can be after PART_FIRST_FREE */
#define PART_FIRST_FREE	E

#define DEFSWAPRAM	32	/* Assume at least this RAM for swap calc */
#define DEFROOTSIZE	20	/* Default root size */
#define DEFVARSIZE	32	/* Default /var size, if created */
#define DEFUSRSIZE	70	/* Default /usr size, if /home */
#define STDNEEDMB	80	/* Min space for non X install */
#define XNEEDMB		50	/* Extra megs for full X installation */

#define LIB_COUNT	0
#define LIB_MOVE	1

/*
 *  Default filesets to fetch and install during installation
 *  or upgrade. The standard sets are:
 *      base, etc, comp, games, man, misc, text,
 *      xbase, xfont, xserver, xcontrib, xcomp.
 *
 * Third entry is the last extension name in the split sets for loading
 * from floppy.
 */
EXTERN distinfo dist_list[]
#ifdef MAIN
= {
    {"kern-BOOT",	 1, "ag", "TT030/Falcon kernel : "},
    {"kern-HADES",	 1, "ag", "Hades kernel        : "},
    {"kern-MILAN-ISAIDE",1, "ag", "Milan ISAIDE kernel : "},
    {"kern-MILAN-PCIIDE",1, "ag", "Milan PCIIDE kernel : "},
    {"base",		 1, "bw", "Base                : "},
    {"etc",		 1, "aa", "System (/etc)       : "},
    {"comp",		 1, "bl", "Compiler            : "},
    {"games",		 1, "am", "Games               : "},
    {"man",		 1, "ar", "Manuals             : "},
    {"misc",		 1, "aj", "Miscellaneous       : "},
    {"text",		 1, "af", "Text tools          : "},

    /* XXX no X11 on floppies, what sets are they?*/
    {"xbase",		 1, "al", "X11 clients         : "},
    {"xfont",		 1, "az", "X11 fonts           : "},
    {"xserver",		 1, "cr", "X11 servers         : "},
    {"xcontrib",	 1, "aa", "X11 contrib         : "},
    {"xcomp",		 1, "ah", "X programming       : "},
    {"xmisc",		 1, NULL, "X11 Misc.           : "},
    {NULL, 0, NULL, NULL }
}
#endif
;

/*
 * Disk names accepted as valid targets for a from-scratch installation.
 */
EXTERN	char *disk_names[]
#ifdef MAIN
= {"wd", "sd", NULL}
#endif
;


/*
 * Legal start character for a disk for checking input. 
 * this must return 1 for a character that matches the first
 * characters of each member of disk_names.
 *
 * On atari, that means matching 's' or 'w'
 */
#define ISDISKSTART(dn)	(dn == 'w' || dn == 's')

/*
 * Machine-specific command to write a new label to a disk.
 * For example, i386  uses "/sbin/disklabel -w -r", just like i386
 * miniroot scripts, though this may leave a bogus incore label.
 * Sun ports should probably use  DISKLABEL_CMD "/sbin/disklabel -w"
 * to get incore  to ondisk inode translation for the Sun proms.
 * If not defined, we assume the port does not support disklabels and
 * hand-edited disklabel will NOT be written by MI code.
 *
 */
#define DISKLABEL_CMD "disklabel -w"


/*
 * Default fileystem type for floppy disks.
 * On atari, that is msdos.
 */
EXTERN	char *fdtype INIT("msdos");

extern struct disklist *disklist;
extern struct nativedisk_info *nativedisk;

/*
 *  prototypes for MD code.
 */
