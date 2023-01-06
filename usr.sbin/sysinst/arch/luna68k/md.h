/*	$NetBSD: md.h,v 1.7 2023/01/06 18:14:56 martin Exp $	*/

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

/* md.h -- Machine specific definitions for the luna68k */

/* Constants and defines */

/*
 * Symbolic names for disk partitions
 */

#define PART_BOOT	(8 * MEG)	/* for a.out kernel and boot */
#define PART_BOOT_TYPE	FS_BSDFFS
#define PART_BOOT_SUBT	1		/* old 4.3BSD UFS */

#define DEFROOTSIZE	40	/* Default root size */
#define DEFSWAPSIZE	32	/* Default swap size */
#define DEFVARSIZE	32	/* Default /var size, if created */
#define DEFUSRSIZE	700	/* Default /usr size, if created */
#define XNEEDMB		250	/* Extra megs for full X installation */
#define DEBNEEDMB	800	/* Extra megs for debug sets */

/*
 * Default filesets to fetch and install during installation
 * or upgrade.
 */
#define SET_KERNEL_1_NAME	"kern-GENERIC"

/*
 * Machine-specific command to write a new label to a disk.
 * If not defined, we assume the port does not support disklabels and
 * the hand-edited disklabel will NOT be written by MI code.
 */
#define DISKLABEL_CMD	"disklabel -w -r"

/*
 * Our boot partition is FFSv1, so it will be reported as PT_root
 * and might take up disklabel slot 0 (partition 'a'), which we would
 * like to avoid and make it use 'd' instead.
 * Only allow PT_root partitions for slots before RAW_PART if they
 * start past the boot partition size.
 */
#define	MD_DISKLABEL_PART_INDEX_CHECK(DL,NDX,INFO)	\
	(((INFO)->nat_type->generic_ptype != PT_root)	\
	|| (NDX > RAW_PART)				\
	|| ((INFO)->start >= (PART_BOOT/512)))
