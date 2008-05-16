/*	$NetBSD: bootpref.h,v 1.2.154.1 2008/05/16 02:22:05 yamt Exp $	*/
/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman & Leo Weppelman.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#define	PATH_NVRAM	"/dev/nvram"

#define	NVRAM_BOOTPREF	1	/* Boot preference			*/
#define	BOOTPREF_TOS	0x80
#define	BOOTPREF_SYSV	0x40
#define	BOOTPREF_NETBSD	0x20
#define	BOOTPREF_LINUX	0x10
#define BOOTPREF_MAGIC	0x08
#define BOOTPREF_NONE	0x00

#define	NVRAM_LANG	6	/* Language				*/
#define	LANG_USA	0x00
#define	LANG_D		0x01
#define	LANG_FR		0x02
#define	LANG_GB		0x03
#define	LANG_ESP	0x04
#define	LANG_I		0x05

#define	NVRAM_KBDLANG	7	/* Keyboard language			*/
#define	KBDLANG_USA	0x00
#define	KBDLANG_D	0x01
#define	KBDLANG_FR	0x02
#define	KBDLANG_GB	0x03
#define	KBDLANG_ESP	0x04
#define	KBDLANG_I	0x05
#define KBDLANG_CHF	0x07
#define KBDLANG_CHD	0x08

#define NVRAM_DATIME	8	/* TOS/GEM date & time formats		*/
#define	DATIME_24H	0x10	/* Otherwise 12H			*/
#define	DATIME_MMDDYY	0x00
#define	DATIME_DDMMYY	0x01
#define	DATIME_YYMMDD	0x02
#define	DATIME_YYDDMM	0x03

#define NVRAM_DATESEP	9	/* Date separator			*/
				/* (ASCII value of char. or 0 for '/')	*/

#define	NVRAM_BOOTDLY	10	/* Bootdelay in seconds			*/

#define	NVRAM_VID1	14	/* Video stuff				*/
#define VID1_INTERLACE	01	/* Interlace (TV) / Double line (VGA)	*/

#define	NVRAM_VID2	15	/* Video stuff				*/
#define	VID2_2COL	0x00
#define	VID2_4COL	0x01
#define	VID2_16COL	0x02
#define	VID2_256COL	0x03
#define	VID2_65535COL	0x04
#define VID2_80CLM	0x08	/* Otherwise 40 columns			*/
#define	VID2_VGA	0x10	/* Otherwise TV				*/
#define	VID2_PAL	0x20	/* Otherwise NTSC			*/
#define	VID2_OVERSCAN	0x40
#define	VID2_COMPAT	0x80

#define	NVRAM_HOSTID	16	/* Bitfield containing SCSI host-id	*/
#define HOSTID_MASK	0x7f
#define HOSTID_VALID	0x80
