/*	$NetBSD: obio.h,v 1.4 1997/04/25 18:00:49 gwr Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * This file defines addresses in Type 1 space for various devices
 * which can be on the motherboard directly.
 *
 * Supposedly these values are constant across the entire sun3 architecture.
 *
 */

/*
 * The obio or type "1" address space.
 */
#define	OBIO_MIN_ADDRESS	0x58000000
#define	OBIO_MAX_ADDRESS	0x7BFFFFFF

/*
 * Physical addresses of nonconfigurable devices.
 */
#define	OBIO_FPA_ADDR		0x5C000000

#define	OBIO_IOMMU  		0x60000000

/* Note that these six are all in the same page. */
#define	OBIO_ENABLEREG		0x61000000
#define	OBIO_BUSERRREG		0x61000400
#define	OBIO_DIAGREG		0x61000800
#define	OBIO_IDPROM1		0x61000C00 /* 3/470 only */
#define	OBIO_MEMREG 		0x61001000
#define	OBIO_INTERREG		0x61001400

#define	OBIO_ZS_KBD_MS		0x62000000
#define	OBIO_ZS_TTY_AB		0x62002000

/*
 * Note: there are two kinds of EEPROM/IDPROM/clock!
 * On the 3/80 one Mostek MK48T02 does it all.
 * The 3/470 uses the older, discrete parts.
 */
#define	OBIO_EEPROM 		0x64000000
#define	OBIO_IDPROM2		0x640007D8 /* 3/80 only (Mostek MK48T02) */
#define	OBIO_CLOCK2 		0x640007F8 /* 3/80 only (Mostek MK48T02) */

#define	OBIO_CLOCK1 		0x64002000 /* 3/470 only */

#define	OBIO_INTEL_ETHER	0x65000000
#define	OBIO_LANCE_ETHER	0x65002000

#define	OBIO_EMULEX_SCSI	0x66000000 /* 3/80 only? */

#define	OBIO_PCACHE_TAGS	0x68000000
#define	OBIO_ECCPARREG		0x6A1E0000
#define	OBIO_IOC_TAGS		0x6C000000
#define	OBIO_IOC_FLUSH		0x6D000000

#define	OBIO_FDC    		0x6e000000	/* 3/80 only */
#define	OBIO_PRINTER_PORT	0x6f00003c	/* 3/80 only */

#ifdef	_KERNEL

void	obio_init __P((void));
caddr_t	obio_find_mapping __P((int pa, int size));
caddr_t	obio_mapin __P((int, int));
caddr_t	obio_vm_alloc __P((int));

/*
 * These are some OBIO devices that need early init calls.
 */
void	intreg_init __P((void));
void	leds_init __P((void));
void	zs_init __P((void));

#endif	/* _KERNEL */
