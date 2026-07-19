/*	$NetBSD: control.h,v 1.1 2026/07/19 01:48:24 thorpej Exp $	*/

/*
 * Copyright (c) 2026 Jason R. Thorpe.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _PG68k_CONTROL_H_
#define	_PG68k_CONTROL_H_

#include <machine/fcode.h>

#define	FC_CONTROL	4

/*
 * Definitions for the Phaethon 1 Board Control Space.
 *
 * The Phaethon 1 has some low-level board control registers that
 * are used during early bootstrap and independently of the state
 * of the MMU.  These registers are accessible via FC#4.
 *
 * Board Control Space is defined as having an address like so:
 *
 * xxxx.xxxx xxxx.xxxx xxxx.000x
 *                          ^^^
 *              Board Control Space selector
 *
 * A GAL22V10 is programmed with equations to decode the following
 * addresses for these registers implemented w/ garden-variety 74HCT
 * logic ICs:
 *
 * xxxx.xxxx xxxx.xxxx 0000.0000 - Diagnostic 7-segment display (upper digit)
 * xxxx.xxxx xxxx.xxxx 0000.0001 - Diagnostic 7-segment display (lower digit)
 * xxxx.xxxx xxxx.xxxx 0001.000x - Configuration switches (16 bits)
 * xxxx.xxxx xxxx.xxxx 0010.0000 - System Enable register
 *
 * Additionally, the I/O controller has 2 registers related to software
 * interrupts:
 *
 * xxxx.xxxx xxxx.xxxx 0100.0000 - Interrupt Set (1 byte)
 * xxxx.xxxx xxxx.xxxx 0101.0000 - Interrupt Clear (1 byte)
 */

#define	CTLREG_DD7SEG_U	0x00	/* 7-segment display, upper */
#define	CTLREG_DD7SEG_L	0x01	/* 7-segment display, lower */

	/* 7-segment display can be written as a single word */
#define	CTLREG_DD7SEG	CTLREG_DD7SEG_U

#define	CTLREG_CFGSW_U	0x10	/* configuration switches, bits 15..8 */
#define	CTLREG_CFGSW_L	0x11	/* configuration switches, bits 7..0 */

	/* configuration switches can be read as a single word */
#define	CTLREG_CFGSW	CTLREG_CFGSW_U

#define	CTLREG_SYSEN	0x20	/* system enable register */

/*
 * 7-segment display registers.
 *
 *           A
 *      +---------+
 *      |         |
 *      |         |
 *    F |         | B
 *      |         |
 *      |    G    |
 *      +---------+
 *      |         |
 *      |         |
 *    E |         | C
 *      |         |
 *      |         |
 *      +---------+
 *           D
 *
 * The segments are active-low.
 */
#define	DD7SEG_A	0x01
#define	DD7SEG_B	0x02
#define	DD7SEG_C	0x04
#define	DD7SEG_D	0x08
#define	DD7SEG_E	0x10
#define	DD7SEG_F	0x20
#define	DD7SEG_G	0x40

/*
 * Configuration switches
 *
 * These allow for hands-on actual physical control of the computer's
 * behavior.  Crazy, right?
 *
 * Configuration switches are active-high.
 *
 * All bits not defined here are reserved.
 */
/*	Yah, they're all reserved right now.	*/

/*
 * System Enable Register
 *
 * This register contains bits that enable some important things,
 * namely the MMU and interrupts.
 *
 * Oh, there's also a software bit (or maybe two or three in the future?)
 * to let the operating system hand-shake reboot behavior with the
 * firmware.
 *
 * All other bits not defined here are reserved.
 */
#define	SYSEN_MMU	0x01		/* enable MMU */
#define	SYSEN_INT	0x02		/* enable interrupts */
#define	SYSEN_HOWTO	0xc0		/* boot howto mask */
#define	SYSEN_HOWTO_SHIFT 6

#ifdef _KERNEL

static inline uint8_t
control_inb(unsigned long addr)
{
	int sfc = getsfc();
	uint8_t rv;

	setsfc(FC_CONTROL);
	__asm __volatile("moves.b (%1),%0" : "=d" (rv) : "a" (addr));
	setsfc(sfc);

	return rv;
}

static inline void
control_outb(unsigned long addr, uint8_t val)
{
	int dfc = getdfc();

	setdfc(FC_CONTROL);
	__asm __volatile("moves.b %0,(%1)" :: "d" (val), "a" (addr) : "memory");
	setdfc(dfc);
}

static inline uint16_t
control_inw(unsigned long addr)
{
	int sfc = getsfc();
	uint16_t rv;

	setsfc(FC_CONTROL);
	__asm __volatile("moves.w (%1),%0" : "=d" (rv) : "a" (addr));
	setsfc(sfc);

	return rv;
}

static inline void
control_outw(unsigned long addr, uint16_t val)
{
	int dfc = getdfc();

	setdfc(FC_CONTROL);
	__asm __volatile("moves.w %0,(%1)" :: "d" (val), "a" (addr) : "memory");
	setdfc(dfc);
}

static inline uint32_t
control_inl(unsigned long addr)
{
	int sfc = getsfc();
	uint32_t rv;

	setsfc(FC_CONTROL);
	__asm __volatile("moves.l (%1),%0" : "=d" (rv) : "a" (addr));
	setsfc(sfc);

	return rv;
}

static inline void
control_outl(unsigned long addr, uint32_t val)
{
	int dfc = getdfc();

	setdfc(FC_CONTROL);
	__asm __volatile("moves.l %0,(%1)" :: "d" (val), "a" (addr) : "memory");
	setdfc(dfc);
}

#endif /* _KERNEL */

#endif /* _PG68k_CONTROL_H_ */
