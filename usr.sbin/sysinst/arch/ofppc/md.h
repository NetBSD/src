/*	$NetBSD: md.h,v 1.3.2.1 2019/11/17 07:04:38 martin Exp $	*/

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
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
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

/* md.h -- Machine specific definitions for the ofppc */


#include <machine/cpu.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* ofppc uses the mbr code. */
#include "mbr.h"

/* constants and defines */
#define PREP_BOOT_SIZE	1048576		/* 1MB for prep-style boot */
#define BINFO_BOOT_SIZE	15*1024		/* 12k for bootinfo.txt */
#define FAT12_BOOT_SIZE	10485760	/* 10MB boot partition */
#define MIN_FAT12_BOOT	2097152		/* 2MB absoule minimum */
#define MIN_PREP_BOOT	1048576
#define MIN_BINFO_BOOT	13312

/* we use three boot partitions: FAT12, BINFO and PREP */
#define	PART_BOOT	FAT12_BOOT_SIZE
#define	PART_BOOT_TYPE	FS_MSDOS
#define	PART_BOOT_SUBT	MBR_PTYPE_FAT12
#define	PART_BOOT_MOUNT	"/boot"

#define	PART_BOOT1	BINFO_BOOT_SIZE
#define	PART_BOOT1_TYPE	FS_OTHER

#define	PART_BOOT2	PREP_BOOT_SIZE
#define	PART_BOOT2_TYPE	FS_BOOT


#define DEFSWAPRAM	32	/* Assume at least this RAM for swap calc */
#define DEFSWAPSIZE	128
#define DEFROOTSIZE	64	/* Default root size */
#define DEFVARSIZE	32	/* Default /var size, if created */
#define DEFUSRSIZE	750	/* Default /usr size, if /home */
#define XNEEDMB 	256	/* Extra megs for full X installation */
#define DEBNEEDMB	900	/* Extra megs for debug sets */

/* allow using tmpfs for /tmp instead of mfs */
#define HAVE_TMPFS

/*
 *  Default filesets to fetch and install during installation
 *  or upgrade. The standard sets are:
 *      base etc comp games man misc rescue tests text xbase xcomp xetc xfont xserver
 *
 * i386 has the  MD set kern first, because generic kernels are  too
 * big to fit on install floppies. i386 does not yet include the x sets.
 *
 * Third entry is the last extension name in the split sets for loading
 * from floppy.
 */
#define SET_KERNEL_1_NAME	"kern-GENERIC"
#define MD_SETS_SELECTED SET_KERNEL_1, SET_SYSTEM, SET_X11

/*
 * Machine-specific command to write a new label to a disk.
 * For example, i386  uses "/sbin/disklabel -w -r", just like i386
 * miniroot scripts, though this may leave a bogus incore label.
 * Sun ports should probably use  DISKLABEL_CMD "/sbin/disklabel -w"
 * to get incore  to ondisk inode translation for the Sun proms.
 * If not defined, we assume the port does not support disklabels and
 * hand-edited disklabel will NOT be written by MI code.
 *
 * On ofppc, do what the 1.2 install scripts did.
 */
const char *md_disklabel_cmd(void);
#define DISKLABEL_CMD md_disklabel_cmd()
