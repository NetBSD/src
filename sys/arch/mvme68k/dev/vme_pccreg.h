/*	$NetBSD: vme_pccreg.h,v 1.1.12.1 1997/10/14 10:17:32 thorpej Exp $	*/

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

/*
 * Based IN PART on a 147 VME driver by Theo de Raadt.
 */

/*
 * Register map of the Type 1 VMEchip found on the MVME-147
 * Peripheral Channel Controller.
 */

struct vme_pcc {

	/*
	 * Local VME control registers.
	 */

	int8_t			pad0;
	volatile u_int8_t	vme_scon;

#define VME1_SCON_SWITCH	0x01	/* SCON jumper is set */
#define VME1_SCON_SRESET	0x02	/* assert SRESET on bus */
#define VME1_SCON_SYSFAIL	0x04	/* assert SYSFAIL on bus */
#define VME1_SCON_ROBIN		0x08	/* round robin bus requests */

	int8_t			pad1;
	volatile u_int8_t	vme_reqconf;

#define VME1_REQ_IPLMASK	0x03	/* interrupt level for requester */
#define VME1_REQ_RNEVER		0x08
#define VME1_REQ_RWD		0x10
#define VME1_REQ_DHB		0x40
#define VME1_REQ_DWB		0x80

	int8_t			pad2;
	volatile u_int8_t	vme_masconf;

#define VME1_MAS_D16		0x01	/* force d8/16 accesses only */
#define VME1_MAS_MASA24		0x02	/* send address mod for A24 access */
#define VME1_MAS_MASA16		0x04	/* send address mod for A16 access */
#define VME1_MAS_MASUAT		0x08	/* handle unaligned VME cycles */
#define VME1_MAS_CFILL		0x10	/* DO NOT USE */
#define VME1_MAS_MASWP		0x20	/* VME fast mode (DO NOT USE) */

	int8_t			pad3;
	volatile u_int8_t	vme_slconf;

#define VME1_SLAVE_SLVD16	0x01	/* DO NOT USE */
#define VME1_SLAVE_SLVWP	0x20	/* DO NOT USE */
#define VME1_SLAVE_SLVEN	0x80	/* allow access to onboard DRAM */

	int8_t			pad4;
	volatile u_int8_t	vme_timerconf;

#define VME1_TIMER_LOCAL_MASK	0x03
#define VME1_TIMER_LOCAL_T0	0x00	/* local timeout 102 microsec */
#define VME1_TIMER_LOCAL_T1	0x01	/* local timeout 205 microsec */
#define VME1_TIMER_LOCAL_T2	0x02	/* local timeout 410 microsec */
#define VME1_TIMER_LOCAL_T3	0x03	/* local timeout disabled */
#define VME1_TIMER_VMEACC_MASK	0x0c
#define VME1_TIMER_VMEACC_T0	0x00	/* VME access timeout 102 microsec */
#define VME1_TIMER_VMEACC_T1	0x04	/* VME access timeout 1.6 millisec */
#define VME1_TIMER_VMEACC_T2	0x08	/* VME access timeout 51 millisec */
#define VME1_TIMER_VMEACC_T3	0x0c	/* VME access timeout disabled */
#define VME1_TIMER_VMEGLO_MASK	0x30
#define VME1_TIMER_VMEGLO_T0	0x00	/* VME glob timeout 102 microsec */
#define VME1_TIMER_VMEGLO_T1	0x10	/* VME glob timeout 205 microsec */
#define VME1_TIMER_VMEGLO_T2	0x20	/* VME glob timeout 410 microsec */
#define VME1_TIMER_VMEGLO_T3	0x30	/* VME glob timeout disabled */
#define VME1_TIMER_ARBTO	0x40	/* enable VME arbitration timer */

	int8_t			pad5;
	volatile u_int8_t	vme_sladdrmod;

#define VME1_SLMOD_DATA		0x01
#define VME1_SLMOD_PRGRM	0x02
#define VME1_SLMOD_BLOCK	0x04
#define VME1_SLMOD_SHORT	0x08
#define VME1_SLMOD_STND		0x10
#define VME1_SLMOD_EXTED	0x20
#define VME1_SLMOD_USER		0x40
#define VME1_SLMOD_SUPER	0x80

	int8_t			pad6;
	volatile u_int8_t	vme_msaddrmod;

#define VME1_MSMOD_AM_MASK	0x3f
#define VME1_MSMOD_AMSEL	0x80

	int8_t			pad7;
	volatile u_int8_t	vme_irqen;

#define VME1_IRQ_VME(x)		(1 << (x))

	int8_t			pad8;
	volatile u_int8_t	vme_uireqen;

	int8_t			pad9;
	volatile u_int8_t	vme_uirq;

	int8_t			pad10;
	volatile u_int8_t	vme_irq;

	int8_t			pad11;
	volatile u_int8_t	vme_vmeid;

	int8_t			pad12;
	volatile u_int8_t	vme_buserr;

	int8_t			pad13;
	volatile u_int8_t	vme_gcsr;

	int8_t			pad14[4];

	/*
	 * Global Status and Control registers.
	 */

	int8_t			pad15;
	volatile u_int8_t	vme_gcsr_gr0;

	int8_t			pad16;
	volatile u_int8_t	vme_gcsr_gr1;

	int8_t			pad17;
	volatile u_int8_t	vme_gcsr_boardid;

	int8_t			pad18;
	volatile u_int8_t	vme_gcsr_gpr0;

	int8_t			pad19;
	volatile u_int8_t	vme_gcsr_gpr1;

	int8_t			pad20;
	volatile u_int8_t	vme_gcsr_gpr2;

	int8_t			pad21;
	volatile u_int8_t	vme_gcsr_gpr3;

	int8_t			pad22;
	volatile u_int8_t	vme_gcsr_gpr4;
};

/*
 * The Type 1 VMEchip decoder maps VME address space to system addresses
 * like this:
 *
 * A24/32D32:	end of RAM - 0xefffffff
 * A32D16:	0xf0000000 - 0xff7fffff
 * A16D16:	0xffff0000 - 0xffffffff
 *
 * Note that an A24D32 access on a machine with 16MB of RAM will lose,
 * since you'll be out of address bits.
 */
#define VME1_A32D32_START	(0x00000000)
#define VME1_A32D32_END		(0xefffffff)
#define VME1_A32D32_LEN		((VME1_A32D32_END - VME1_A32D32_START) + 1)

#define VME1_A32D16_START	(0xf0000000)
#define VME1_A32D16_END		(0xff7fffff)
#define VME1_A32D16_LEN		((VME1_A32D16_END - VME1_A32D16_START) + 1)

#define VME1_A16D16_START	(0xffff0000)
#define VME1_A16D16_END		(0xffffffff)
#define VME1_A16D16_LEN		((VME1_A16D16_END - VME1_A16D16_START) + 1)
