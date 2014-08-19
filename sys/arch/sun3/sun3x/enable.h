/*	$NetBSD: enable.h,v 1.2.44.1 2014/08/20 00:03:26 tls Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeremy Cooper.
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

/*
 * System Enable Register
 * The Sun3x System Enable Register controls the function of a few
 * on-board devices and general system operation.  It is cleared when
 * the system is reset.
 *
 * 15                                                               0
 *  +---+---+---+---+---+---+---+---+---+---+---+---+---+---.---.---+
 *  |BT |FPP|DMA| 0 |VID|RES|FPA|DIA| 0 |CCH|IOC|LBK|DCH|  UNUSED   |
 *  +---+---+---+---+---+---+---+---+---+---+---+---+---+---.---.---+
 *
 *
 * Bits in the Enable Register defined.
 */
#define	ENA_DBGCACHE	0x0008	/* Debug mode for system cache              */
#define	ENA_LOOPBACK	0x0010	/* VME loopback mode                        */
#define	ENA_IOCACHE	0x0020	/* Enable I/O cache                             */
#define	ENA_CACHE	0x0040	/* Enable system cache                          */
#define	ENA_DIAG	0x0100	/* Diagnostic switch                            */
#define	ENA_FPA		0x0200	/* Enable floating point acc.                   */
#define	ENA_RES		0x0400	/* Video display resolution (0 => hi, 1 => low) */
#define	ENA_VIDEO	0x0800	/* Enable video display                         */
#define	ENA_SDVMA	0x2000	/* Enable system DVMA                           */
#define	ENA_FPP		0x4000	/* Enable floating point coprocessor            */
#define	ENA_NOTBOOT	0x8000	/* Non-boot state (0 => boot, 1 => normal)      */

#ifdef	_KERNEL
extern volatile short *enable_reg;
#endif

