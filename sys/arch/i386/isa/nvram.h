/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)rtc.h	7.1 (Berkeley) 5/12/91
 *	$Id: nvram.h,v 1.2 1993/09/24 08:49:21 mycroft Exp $
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
  Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

		All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
 * RTC/CMOS Register locations
 */

/*
 * Register A definitions
 */
#define CLOCK_RATE		0x0a	/* register A address */
#define CLOCK_RATE_UIP		0x80	/* Update in progress bit */
#define CLOCK_RATE_DIV0		0x00	/* Time base of 4.194304 MHz */
#define CLOCK_RATE_DIV1		0x10	/* Time base of 1.048576 MHz */
#define CLOCK_RATE_DIV2		0x20	/* Time base of 32.768 KHz */
#define CLOCK_RATE_6		0x06	/* interrupt rate of 976.562 */

/*
 * Register B definitions
 */
#define CLOCK_MODE		0x0b	/* register B address */
#define CLOCK_MODE_SET		0x80	/* stop updates for time set */
#define CLOCK_MODE_PIE		0x40	/* Periodic interrupt enable */
#define CLOCK_MODE_AIE		0x20	/* Alarm interrupt enable */
#define CLOCK_MODE_UIE		0x10	/* Update ended interrupt enable */
#define CLOCK_MODE_SQWE		0x08	/* Square wave enable */
#define CLOCK_MODE_DM		0x04	/* Date mode, 1 = binary, 0 = BCD */
#define CLOCK_MODE_HM		0x02	/* hour mode, 1 = 24 hour, 0 = 12 hour */
#define CLOCK_MODE_DSE		0x01	/* Daylight savings enable */

/* 
 * Register C definitions
 */
#define CLOCK_INTR		0x0c	/* register C address */
#define CLOCK_INTR_IRQF		0x80	/* IRQ flag */
#define CLOCK_INTR_PF		0x40	/* PF flag bit */
#define CLOCK_INTR_AF		0x20	/* AF flag bit */
#define CLOCK_INTR_UF		0x10	/* UF flag bit */

/*
 * Register D definitions
 */
#define NVRAM_VALID		0x0d	/* register D address */
#define NVRAM_VALID_VRT		0x80	/* Valid RAM and time bit */

#define CLOCK_NREG		0x0a	/* number of RTC registers */

/*
 * These are generic CMOS locations, but we call then RTC anyway...
 */
#define NVRAM_DIAG		0x0e	/* status register E - bios diagnostic */
#define NVRAM_DIAG_BITS		"\020\010clock_battery\007ROM_cksum\006config_unit"\
				"\005memory_size\004fixed_disk\003invalid_time"

#define NVRAM_RESET		0x0f	/* status register F - reset code byte */
#define	NVRAM_RESET_RST		0x00	/* normal reset */
#define	NVRAM_RESET_LOAD	0x04	/* load system */

#define NVRAM_DISKETTE		0x10	/* diskette drive type in upper/lower nibble */
#define	NVRAM_DISKETTE_NONE	0x00	/* none present */
#define	NVRAM_DISKETTE_360K	0x10	/* 360K */
#define	NVRAM_DISKETTE_12M	0x20	/* 1.2M */
#define	NVRAM_DISKETTE_720K	0x30	/* 720K */
#define	NVRAM_DISKETTE_144M	0x40	/* 1.44M */

#define NVRAM_BASEMEM_LO	0x15	/* low byte of basemem size */
#define NVRAM_BASEMEM_HI	0x16	/* high byte of basemem size */
#define NVRAM_EXTMEM_LO		0x17	/* low byte of extended mem size */
#define NVRAM_EXTMEM_HI		0x18	/* low byte of extended mem size */

#define CLOCK_CENTURY		0x32	/* current century - please increment in Dec99 */

#define	CLOCK_NPORTS		16


u_char nvram __P((u_char pos));
