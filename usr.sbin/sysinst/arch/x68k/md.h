/*	$NetBSD: md.h,v 1.1.2.2 2014/08/10 07:00:28 tls Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.  Modified by Minoura Makoto for x68k.
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

/* md.h -- Machine specific definitions for the x68k */


#include <machine/cpu.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


/* constants and defines */

/*
 * Symbolic names for disk partitions.
 */
#define PART_ROOT	PART_A
#define PART_SWAP	PART_B
#define PART_RAW	PART_C
#define PART_USR	PART_D	/* Can be after PART_FIRST_FREE */
#define PART_FIRST_FREE	PART_E

#define DEFSWAPRAM	32	/* Assume at least this RAM for swap calc */
#define DEFROOTSIZE	20	/* Default root size */
#define DEFVARSIZE	32	/* Default /var size, if created */
#define DEFUSRSIZE	70	/* Default /usr size, if /home */
#define XNEEDMB		35	/* Extra megs for full X installation */

/* have support for booting from UFS2 */
#define HAVE_UFS2_BOOT

/*
 *  Default filesets to fetch and install during installation
 *  or upgrade. The standard sets are:
 *      base etc comp games man misc tests text xbase xcomp xetc xfont xserver
 *
 * x68k has the  MD set kern first, because generic kernels are  too
 * big to fit on install floppies. i386 does not yet include the x sets.
 *
 * Third entry is the last extension name in the split sets for loading
 * from floppy.
 */
#define SET_KERNEL_1_NAME	"kern-GENERIC"

/*
 * Machine-specific command to write a new label to a disk.
 * For example, i386  uses "/sbin/disklabel -w -r", just like i386
 * miniroot scripts, though this may leave a bogus incore label.
 * Sun ports should probably use  DISKLABEL_CMD "/sbin/disklabel -w"
 * to get incore  to ondisk inode translation for the Sun proms.
 * If not defined, we assume the port does not support disklabels and
 * hand-edited disklabel will NOT be written by MI code.
 *
 * On x68k, do what the 1.2 install scripts did.
 */
#define DISKLABEL_CMD "disklabel -w"
