/*	$NetBSD: bootinfo.h,v 1.2 1998/12/14 15:22:04 itohy Exp $	*/

/*
 * Copyright (c) 1998 ITOH, Yasufumi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _X68K_BOOTINFO_H_
#define _X68K_BOOTINFO_H_

/*
 *	machine dependent boot information
 *	passed from boot loader to the NetBSD kernel
 */

/*
 * NetBSD/x68k uses MAKEBOOTDEV() framework defined in <sys/reboot.h>
 */
#include <sys/reboot.h>

/*
 * for non-SCSI devices
 */
/* major */
#define X68K_MAJOR_FD	2	/* floppy disk */
#define X68K_MAJOR_MD	8	/* memory disk */

#define X68K_MAKEBOOTDEV(major, unit, part) \
	MAKEBOOTDEV(major, 0, 0, unit, part)


/*
 * for SCSI devices
 *
 * device major		-> type		(8bit)
 * type of interface	-> adaptor	(4bit)
 * unit # of interface	-> controller	(4bit)
 * target SCSI ID	-> unit		(4bit)
 * target LUN		-> partition	(3bit; bit 4:6)
 * partition		-> partition	(4bit; bit 0:3)
 *
 * bit #7 of "partition" is reserved for future extension
 */
/* major */
#define X68K_MAJOR_SD	4	/* SCSI disk */
/*	X68K_MAJOR_ST	5	XXX not yet */
#define X68K_MAJOR_CD	7	/* SCSI CD-ROM */

/* type_if */
#define X68K_BOOT_SCSIIF_OLDBOOT	0	/* old boot used this value */
#define X68K_BOOT_SCSIIF_SPC		1	/* spc */
#define X68K_BOOT_SCSIIF_MHA		2	/* mha */

#define X68K_MAKESCSIBOOTDEV(major, type_if, unit_if, scsi_id, lun, part) \
	MAKEBOOTDEV(major, type_if, unit_if, scsi_id, ((lun) << 4) | (part))

#define B_X68K_SCSI_IF(val)	B_ADAPTOR(val)
#define B_X68K_SCSI_IF_UN(val)	B_CONTROLLER(val)

#define B_X68K_SCSI_ID(val)	B_UNIT(val)
#define B_X68K_SCSI_LUN(val)	(((val) >> (B_PARTITIONSHIFT + 4)) & 07)

#define B_X68K_SCSI_PART(val)	(((val) >> B_PARTITIONSHIFT) & 017)

#if 0
/* this bit is reserved for future extension */
#define B_X68K_SCSI_EXT(val)	(((val) >> (B_PARTITIONSHIFT + 7)) & 01)
#endif


/*
 * for array initialization
 */

#define X68K_BOOT_DEV_LIST		\
	{ "fd", X68K_MAJOR_FD },	\
	{ "sd", X68K_MAJOR_SD },	\
	{ "cd", X68K_MAJOR_CD },	\
	{ "md", X68K_MAJOR_MD }

#define X68K_BOOT_DEV_STRINGS		\
	NULL, NULL, "fd", NULL, "sd", NULL, NULL, "cd"

#define X68K_BOOT_DEV_IS_SCSI(major)	\
	((major) == X68K_MAJOR_SD ||	\
	 (major) == X68K_MAJOR_CD)

#define X68K_BOOT_SCSIIF_LIST		\
	{ "spc", X68K_BOOT_SCSIIF_SPC },\
	{ "mha", X68K_BOOT_SCSIIF_MHA }

#define X68K_BOOT_SCSIIF_STRINGS	\
	NULL, "spc", "mha"

#endif /* _X68K_BOOTINFO_H_ */
