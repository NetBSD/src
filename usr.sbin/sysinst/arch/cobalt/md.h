/*	$NetBSD: md.h,v 1.1 2014/07/26 19:30:44 dholland Exp $	*/

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

/* md.h -- Machine specific definitions for the cobalt */


#include <machine/cpu.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* cobalt uses the mbr code. */
#include "mbr.h"

/* constants and defines */
#define EXT2FS_BOOT_SIZE	(5 * 1024 * 1024)	/* 5MB boot partition */
#define MIN_EXT2FS_BOOT		(1 * 1024 * 1024)	/* 1MB minimum */

#define PART_ROOT		PART_A
#define PART_SWAP		PART_B
#define PART_BSD		PART_C
#define PART_RAW		PART_D
#define PART_BOOT_EXT2FS	PART_E
#define PART_USR		PART_F
#define PART_FIRST_FREE		PART_G

/* We want the boot MSDOS partition mounted on /boot */
#define USE_EXT2FS
#define PART_BOOT_EXT2FS_PI_FLAGS	(PIF_NEWFS|PIF_MOUNT)
#define PART_BOOT_EXT2FS_PI_MOUNT	"/stand"

/* default partition size */
#define DEFSWAPRAM	32	/* Assume at least this RAM for swap calc */
#define DEFSWAPSIZE	128	/* Default swap size */
#define DEFROOTSIZE	64	/* Default root size, if created */
#define DEFVARSIZE	64	/* Default /var size, if created */
#define DEFUSRSIZE	256	/* Default /usr size, if created */

/* Megs required for a full X installation. */
#define XNEEDMB 100

/* have support for booting from UFS2 */
#define	HAVE_UFS2_BOOT


/*
 *  Default filesets to fetch and install during installation
 *  or upgrade. The standard sets are:
 *      base etc comp games man misc tests text xbase xcomp xetc xfont xserver
 */
#define SET_KERNEL_1_NAME	"kern-GENERIC"
#define MD_SETS_SELECTED	SET_KERNEL_1, SET_SYSTEM, SET_X11_NOSERVERS

/*
 * Machine-specific command to write a new label to a disk.
 * For example, cobalt uses "/sbin/disklabel -w -r", just like i386
 * miniroot scripts, though this may leave a bogus incore label.
 * Sun ports should probably use  DISKLABEL_CMD "/sbin/disklabel -w"
 * to get incore  to ondisk inode translation for the Sun proms.
 * If not defined, we assume the port does not support disklabels and
 * hand-edited disklabel will NOT be written by MI code.
 */
#define DISKLABEL_CMD "disklabel -w -r"
