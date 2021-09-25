/*	$NetBSD: md.h,v 1.8 2021/09/25 08:54:31 maya Exp $	*/

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

/* md.h -- Machine specific definitions for evbarm */

#include "mbr.h"

/* Constants and defines */
#define PART_BOOT		(16*MEG)
#define PART_BOOT_LARGE		(32*MEG)
#define PART_BOOT_TYPE		FS_MSDOS
#define	PART_BOOT_SUBT		MBR_PTYPE_FAT16L
#define PART_BOOT_MOUNT		"/boot"

/* Megs required for a full X installation. */
#define XNEEDMB 60

#define	DEFSWAPSIZE	(-1)

/* use UFS2 by default for ffs */
#define	DEFAULT_UFS2

#define HAVE_UFS2_BOOT

/* allow using tmpfs for /tmp instead of mfs */
#define HAVE_TMPFS

/*
 *  Default filesets to fetch and install during installation
 *  or upgrade. The standard sets are:
 *      base etc comp games gpufw man misc rescue tests text xbase xcomp xetc xfont xserver
 */
#if 0	/* XXX */
#define SET_KERNEL_1_NAME	"kern-ADI_BRH"
#define SET_KERNEL_2_NAME	"kern-INTEGRATOR"
#define SET_KERNEL_3_NAME	"kern-IQ80310"
#define SET_KERNEL_4_NAME	"kern-IQ80321"
#define SET_KERNEL_5_NAME	"kern-MINI2440"
#define SET_KERNEL_6_NAME	"kern-TEAMASA_NPWR"
#define SET_KERNEL_7_NAME	"kern-TS7200"
#define SET_KERNEL_8_NAME	"kern-RPI"
#define SET_KERNEL_9_NAME	"kern-KUROBOX_PRO"
#endif

#ifdef _LP64
#define SET_KERNEL_1_NAME	"kern-GENERIC64"
#else
#define SET_KERNEL_1_NAME	"kern-GENERIC"
#endif

#define MD_SETS_SELECTED SET_SYSTEM

/*
 * Machine-specific command to write a new label to a disk.
 * For example, shark uses "/sbin/disklabel -w -r".
 * Sun ports should probably use  DISKLABEL_CMD "/sbin/disklabel -w"
 * to get incore to ondisk inode translation for the Sun proms.
 * If not defined, we assume the port does not support disklabels and
 * hand-edited disklabel will NOT be written by MI code.
 */
#define DISKLABEL_CMD "disklabel -w -r"

/* Special board type routines need a switch */
#define BOARD_TYPE_NORMAL	0	/* assume u-boot */
#define BOARD_TYPE_RPI		1	/* RPi firmware booted us */
#define	BOARD_TYPE_ACPI		2	/* generic SBSA machine */
extern int boardtype;

/*
 * Size limit for the initial GPT part, in bytes. This allows us to
 * dd u-boot at 8k into the image for allwinner SoCs.
 */
#define	MD_GPT_INITIAL_SIZE		(8*1024)


#define	HAVE_GPT_BOOT		/* yes, u-boot can boot us from GPT */
#define	NO_DISKLABEL_BOOT	/* ... but not directly from disklabel */

#define MD_MAY_SWAP_TO(DISK)	may_swap_if_not_sdmmc(DISK)

int evbarm_extract_finalize(int);
#define	MD_SET_EXTRACT_FINALIZE(UP)	evbarm_extract_finalize(UP)

void evbarm_part_defaults(struct pm_devs*, struct part_usage_info*,
            size_t num_usage_infos);

#define MD_PART_DEFAULTS(A,B,C)	evbarm_part_defaults(A,B,C)

#define	HAVE_EFI_BOOT		1	/* we support EFI boot partitions */

