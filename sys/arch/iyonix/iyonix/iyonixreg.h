/*	$NetBSD: iyonixreg.h,v 1.2.6.1 2006/04/22 11:37:40 simonb Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Based on code written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IYONIXREG_H_
#define	_IYONIXREG_H_

/*
 * Memory map and register definitions for the Tungsten motherboard
 */

/*
 * We allocate a page table for VA 0xfe400000 (4MB) and map the
 * PCI I/O space (64K) and i80321 memory-mapped registers (4K) there.
 */
#define	IYONIX_IOPXS_VBASE	0xfe400000UL
#define	IYONIX_IOW_VBASE	IYONIX_IOPXS_VBASE
#define	IYONIX_80321_VBASE	(IYONIX_IOW_VBASE +			\
				 VERDE_OUT_XLATE_IO_WIN_SIZE)

#define IYONIX_UART1		0x900002f8

/*
 * The on-board devices are mapped VA==PA during bootstrap.
 */
#define	IYONIX_OBIO_BASE	0x90000000UL
#define	IYONIX_OBIO_SIZE	0x00100000UL	/* 1MB */

#define	IYONIX_FLASH_BASE	0xA0000000UL
#define	IYONIX_FLASH_SIZE	0x00800000UL	/* 8MB */

#define IYONIX_MASTER_PIC	0x20
#define IYONIX_SLAVE_PIC	0xa0

#endif /* _IYONIXREG_H_ */
