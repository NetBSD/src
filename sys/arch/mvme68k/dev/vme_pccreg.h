/*	$NetBSD: vme_pccreg.h,v 1.2.18.2 2000/12/08 09:28:32 bouyer Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
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

#ifndef _MVME68K_VME_PCCREG_H
#define _MVME68K_VME_PCCREG_H

/*
 * Register map of the Type 1 VMEchip found on the MVME-147
 * Peripheral Channel Controller.
 */

/*
 * Local control registers...
 */
#define VME1REG_SCON		0x01
#define  VME1_SCON_SWITCH	0x01	/* SCON jumper is set */
#define  VME1_SCON_SRESET	0x02	/* assert SRESET on bus */
#define  VME1_SCON_SYSFAIL	0x04	/* assert SYSFAIL on bus */
#define  VME1_SCON_ROBIN	0x08	/* round robin bus requests */

#define VME1REG_REQCONF		0x03
#define  VME1_REQ_IPLMASK	0x03	/* interrupt level for requester */
#define  VME1_REQ_RNEVER	0x08
#define  VME1_REQ_RWD		0x10
#define  VME1_REQ_DHB		0x40
#define  VME1_REQ_DWB		0x80

#define VME1REG_MASCONF		0x05
#define  VME1_MAS_D16		0x01	/* force d8/16 accesses only */
#define  VME1_MAS_MASA24	0x02	/* send address mod for A24 access */
#define  VME1_MAS_MASA16	0x04	/* send address mod for A16 access */
#define  VME1_MAS_MASUAT	0x08	/* handle unaligned VME cycles */
#define  VME1_MAS_CFILL		0x10	/* DO NOT USE */
#define  VME1_MAS_MASWP		0x20	/* VME fast mode (DO NOT USE) */

#define VME1REG_SLCONF		0x07
#define  VME1_SLAVE_SLVD16	0x01	/* DO NOT USE */
#define  VME1_SLAVE_SLVWP	0x20	/* DO NOT USE */
#define  VME1_SLAVE_SLVEN	0x80	/* allow access to onboard DRAM */

#define VME1REG_TIMERCONF	0x09
#define  VME1_TIMER_LOCAL_MASK	0x03
#define  VME1_TIMER_LOCAL_T0	0x00	/* local timeout 102 microsec */
#define  VME1_TIMER_LOCAL_T1	0x01	/* local timeout 205 microsec */
#define  VME1_TIMER_LOCAL_T2	0x02	/* local timeout 410 microsec */
#define  VME1_TIMER_LOCAL_T3	0x03	/* local timeout disabled */
#define  VME1_TIMER_VMEACC_MASK	0x0c
#define  VME1_TIMER_VMEACC_T0	0x00	/* VME access timeout 102 microsec */
#define  VME1_TIMER_VMEACC_T1	0x04	/* VME access timeout 1.6 millisec */
#define  VME1_TIMER_VMEACC_T2	0x08	/* VME access timeout 51 millisec */
#define  VME1_TIMER_VMEACC_T3	0x0c	/* VME access timeout disabled */
#define  VME1_TIMER_VMEGLO_MASK	0x30
#define  VME1_TIMER_VMEGLO_T0	0x00	/* VME glob timeout 102 microsec */
#define  VME1_TIMER_VMEGLO_T1	0x10	/* VME glob timeout 205 microsec */
#define  VME1_TIMER_VMEGLO_T2	0x20	/* VME glob timeout 410 microsec */
#define  VME1_TIMER_VMEGLO_T3	0x30	/* VME glob timeout disabled */
#define  VME1_TIMER_ARBTO	0x40	/* enable VME arbitration timer */

#define VME1REG_SLADDRMOD	0x0b
#define  VME1_SLMOD_DATA	0x01
#define  VME1_SLMOD_PRGRM	0x02
#define  VME1_SLMOD_BLOCK	0x04
#define  VME1_SLMOD_SHORT	0x08
#define  VME1_SLMOD_STND	0x10
#define  VME1_SLMOD_EXTED	0x20
#define  VME1_SLMOD_USER	0x40
#define  VME1_SLMOD_SUPER	0x80

#define VME1REG_MSADDRMOD	0x0d
#define  VME1_MSMOD_AM_MASK	0x3f
#define  VME1_MSMOD_AMSEL	0x80

#define VME1REG_IRQEN		0x0f
#define  VME1_IRQ_VME(x)	(1 << (x))

#define VME1REG_UIREQEN		0x11
#define VME1REG_UIRQ		0x13
#define VME1REG_IRQ		0x15
#define VME1REG_VMEID		0x17
#define VME1REG_BUSERR		0x19
#define VME1REG_GCSR		0x1b


/*
 * Global Status and Control registers.
 */
#define VME1REG_GCSR_GR0	0x21
#define VME1REG_GCSR_GR1	0x23
#define VME1REG_GCSR_BOARDID	0x25
#define VME1REG_GCSR_GPR0	0x27
#define VME1REG_GCSR_GPR1	0x29
#define VME1REG_GCSR_GPR2	0x2b
#define VME1REG_GCSR_GPR3	0x2d
#define VME1REG_GCSR_GPR4	0x2f

/*
 * Length of the VME chip's register mapping
 */
#define VME1REG_SIZE		0x30


/*
 * Convenience macros for reading and writing the registers
 */
#define	vme1_reg_read(sc,r)	\
		bus_space_read_1((sc)->sc_mvmebus.sc_bust, (sc)->sc_bush, (r))
#define	vme1_reg_write(sc,r,v)	\
		bus_space_write_1((sc)->sc_mvmebus.sc_bust, (sc)->sc_bush, (r), (v))

/*
 * The Type 1 VMEchip decoder maps VME address space to system addresses
 * like this:
 *
 * A24D32:	0x00000000 - 0x00ffffff
 * A32D32:	0x01000000 - 0xefffffff
 * A24D16:	0xf0000000 - 0xf0ffffff
 * A32D16:	0xf1000000 - 0xff7fffff
 * A16D16:	0xffff0000 - 0xffffffff
 *
 * Note that the first A24D32 range is overlayed with onboard RAM. Thus
 * an 8Mb board will only allow A24D32:0x00800000 - 0x00ffffff. When
 * onboard RAM is >= 16Mb, the A24D32 range is unavailable and the RAM
 * starts encroaching on the A32D32 range.
 */
#define VME1_A24D32_LOC_START	(0x00000000u)
#define VME1_A24D32_START	(0x00000000u)
#define VME1_A24D32_END		(0x00ffffffu)

#define VME1_A32D32_LOC_START	(0x00000000u)
#define VME1_A32D32_START	(0x01000000u)
#define VME1_A32D32_END		(0xefffffffu)

#define VME1_A24D16_LOC_START	(0xf0000000u)
#define VME1_A24D16_START	(0x00000000u)
#define VME1_A24D16_END		(0x00ffffffu)

#define VME1_A32D16_LOC_START	(0x00000000u)
#define VME1_A32D16_START	(0xf1000000u)
#define VME1_A32D16_END		(0xff7fffffu)

#define VME1_A16D16_LOC_START	(0xffff0000u)
#define VME1_A16D16_START	(0x00000000u)
#define VME1_A16D16_END		(0x0000ffffu)

#define VME1_A32_MASK		(0xffffffffu)
#define VME1_A24_MASK		(0x00ffffffu)
#define VME1_A16_MASK		(0x0000ffffu)

#endif /* _MVME68K_VME_PCCREG_H */
