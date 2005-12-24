/* $NetBSD: jensenioreg.h,v 1.2 2005/12/24 20:06:46 perry Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * System register description for the DECpc AXP 150 ("Jensen").
 */

#define	REGVAL(r)	(*(volatile u_int64_t *)ALPHA_PHYS_TO_K0SEG(r))

/*
 * EISA Interrupt Acknowledge:			1.0000.0000
 */
#define	JENSEN_EISA_INTA	0x100000000UL


/*
 * FEPROM 0 (256K):				1.8000.0000
 */
#define	JENSEN_FEPROM0		0x180000000UL


/*
 * FEPROM 1 (1M):				1.a000.0000
 */
#define	JENSEN_FEPROM1		0x1a0000000UL


/*
 * VLSI VL82C106 junk I/O chip:			1.C000.0000
 */
#define	JENSEN_VL82C106		0x1c0000000UL


/*
 * Host Address Extension Register:		1.D000.0000
 *
 *	8-bit register that contains the upper bigs of address
 *	destined for the EISA bus.
 */
#define	JENSEN_HAE		0x1d0000000UL
#define	HAE_MASK		0x7		/* EISA <31:25> */
#define	HAE_SHIFT		25


/*
 * System Control Register:			1.E000.0000
 *
 *	8-bit register that contains memory configuration information
 *	and the LED display code bits.
 *
 *	Memory configuration:
 *
 *		0		4M x 36 SIMMs
 *		1		4M x 36 x 2 SIMMs
 *		2		16M x 36 SIMMs
 *		3		16M x 36 x 2 SIMMs
 */
#define	JENSEN_SYSCTL		0x1e0000000UL
#define	SYSCTL_LEDMASK		0x0f		/* LED codes */
#define	SYSCTL_BANK0_CFG	0x30		/* Bank 0 config */
#define	SYSCTL_BANK0_CFG_SHIFT	4
#define	SYSCTL_BANK1_CFG	0xc0		/* Bank 1 config */
#define	SYSCTL_BANK1_CFG_SHIFT	6


/*
 * Spare Register:				1.f000.0000
 */
#define	JENSEN_SPARE		0x1f0000000UL


/*
 * EISA Memory Space:				2.0000.0000
 */
#define	JENSEN_EISA_MEM		0x200000000UL


/*
 * EISA I/O Space:				3.0000.0000
 */
#define	JENSEN_EISA_IO		0x300000000UL
