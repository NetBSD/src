/*	$NetBSD: romcall.h,v 1.1 1999/12/09 14:53:13 tsutsui Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define SYS_reboot	0
#define SYS_exit	1
#define SYS_read	3
#define SYS_write	4
#define SYS_open	5
#define SYS_close	6
#define SYS_lseek	19
#define SYS_ioctl	54

#define BOOTDEV_MAG(x)	(((x) >> 28) & 0x0f)	/* MAGIC (5) */
#if 0
#define BOOTDEV_BUS(x)	(((x) >> 24) & 0x0f)	/* bus number */
#define BOOTDEV_CTLR(x) (((x) >> 20) & 0x0f)
#else
#define BOOTDEV_CTLR(x) (((x) >> 24) & 0x0f)
#endif
#define BOOTDEV_UNIT(x) (((x) >> 16) & 0x0f)
#define BOOTDEV_HOST(x) (((x) >> 12) & 0x0f)
#define BOOTDEV_PART(x) (((x) >> 8) & 0x0f)
#define BOOTDEV_TYPE(x) ((x) & 0xff)

#define BOOTDEV_SD	0	/* SCSI disk */
#define BOOTDEV_FH	1	/* 1.4M floppy */
#define BOOTDEV_FD	2	/* 800k floppy */
#define BOOTDEV_RD	5	/* remote(?) disk */
#define BOOTDEV_ST	6	/* SCSI TAPE */

#ifndef _LOCORE
int rom_open(const char * ,int);
int rom_close(int);
int rom_read(int, void *, int);
int rom_write(int, void *, int);
int rom_lseek(int, int, int);
#endif
