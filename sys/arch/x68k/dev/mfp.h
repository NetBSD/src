/*	$NetBSD: mfp.h,v 1.9 2009/11/07 19:54:17 snj Exp $	*/

/*
 * Copyright (c) 1998 NetBSD Foundation, Inc.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define MFP_ADDR	0x00e88000
#define MFP_INTR	0x40

struct mfp_softc {
	bus_space_tag_t	sc_bst;
	bus_space_handle_t	sc_bht;
	int		sc_intr;
};

/*
 * MFP registers
 */
#define MFP_GPIP	0x01
#define MFP_AER		0x03
#define MFP_DDR		0x05
#define MFP_IERA	0x07
#define MFP_IERB	0x09
#define MFP_IPRA	0x0b
#define MFP_IPRB	0x0d
#define MFP_ISRA	0x0f
#define MFP_ISRB	0x11
#define MFP_IMRA	0x13
#define MFP_IMRB	0x15
#define MFP_VR		0x17
#define MFP_TACR	0x19
#define MFP_TIMERA_STOP  0
#define MFP_TIMERA_RESET 0x10
#define MFP_TBCR	0x1b
#define MFP_TIMERB_STOP  0
#define MFP_TIMERB_RESET 0x10
#define MFP_TCDCR	0x1d
#define MFP_TADR	0x1f
#define MFP_TBDR	0x21
#define MFP_TCDR	0x23
#define MFP_TDDR	0x25
#define MFP_UCR		0x29
#define MFP_UCR_EVENP		0x02
#define MFP_UCR_PARENB		0x04
#define MFP_UCR_SYNCMODE	0x00
#define MFP_UCR_ONESB		0x08
#define MFP_UCR_1P5SB		0x10
#define MFP_UCR_TWOSB		0x18
#define MFP_UCR_RW_5		0x60
#define MFP_UCR_RW_6		0x40
#define MFP_UCR_RW_7		0x20
#define MFP_UCR_RW_8		0x00
#define MFP_UCR_CLKX16		0x80
#define MFP_RSR		0x2b
#define MFP_RSR_BF		0x80
#define MFP_RSR_OE		0x40
#define MFP_RSR_PE		0x20
#define MFP_RSR_FE		0x10
#define MFP_RSR_SS		0x02
#define MFP_RSR_RE		0x01
#define MFP_TSR		0x2d
#define MFP_TSR_BE		0x80
#define MFP_TSR_TE		0x01
#define MFP_UDR		0x2f


/*
 * machine dependent definitions
 */

/* GPIP port bitmap */
#define MFP_GPIP_HSYNC		0x80
#define MFP_GPIP_CRTC		0x40
#define MFP_GPIP_UNUSED1	0x20
#define MFP_GPIP_VDISP		0x10
#define MFP_GPIP_OPM		0x08
#define MFP_GPIP_FRONT_SWITCH	0x04
#define MFP_GPIP_EXPWON		0x02
#define MFP_GPIP_RTC_ALARM	0x01

/* interrupt A */
#define MFP_INTR_HSYNC		0x80
#define MFP_INTR_CRTC		0x40
#define MFP_INTR_TIMER_A	0x20
#define MFP_INTR_RCV_FULL	0x10
#define MFP_INTR_RCV_ERROR	0x08
#define MFP_INTR_XMIT_EMPTY	0x04
#define MFP_INTR_XMIT_ERROR	0x02
#define MFP_INTR_TIMER_B	0x01

/* interrupt B */
#define MFP_INTR_VDISP		0x40
#define MFP_INTR_TIMER_C	0x20
#define MFP_INTR_TIMER_D	0x10
#define MFP_INTR_OPM		0x08
#define MFP_INTR_FRONT_SWITCH	0x04
#define MFP_INTR_EXPWON		0x02
#define MFP_INTR_RTC_ALARM	0x01


/* XXX */
#include <arch/x68k/dev/intiovar.h>
#define mfp_base	((volatile uint8_t *)IIOV(MFP_ADDR))
#define mfp_set_aer(a) \
	mfp_base[MFP_AER] = ((u_int8_t) (a))
#define mfp_set_ddr(a) \
	mfp_base[MFP_DDR] = ((u_int8_t) (a))
#define mfp_set_iera(a) \
	mfp_base[MFP_IERA] = ((u_int8_t) (a))
#define mfp_set_ierb(a) \
	mfp_base[MFP_IERB] = ((u_int8_t) (a))
#define mfp_set_ipra(a) \
	mfp_base[MFP_IPRA] = ((u_int8_t) (a))
#define mfp_set_iprb(a) \
	mfp_base[MFP_IPRB] = ((u_int8_t) (a))
#define mfp_set_isra(a) \
	mfp_base[MFP_ISRA] = ((u_int8_t) (a))
#define mfp_set_isrb(a) \
	mfp_base[MFP_ISRB] = ((u_int8_t) (a))
#define mfp_set_imra(a) \
	mfp_base[MFP_IMRA] = ((u_int8_t) (a))
#define mfp_set_imrb(a) \
	mfp_base[MFP_IMRB] = ((u_int8_t) (a))
#define mfp_set_vr(a) \
	mfp_base[MFP_VR] = ((u_int8_t) (a))
#define mfp_set_tacr(a) \
	mfp_base[MFP_TACR] = ((u_int8_t) (a))
#define mfp_set_tbcr(a) \
	mfp_base[MFP_TBCR] = ((u_int8_t) (a))
#define mfp_set_tcdcr(a) \
	mfp_base[MFP_TCDCR] = ((u_int8_t) (a))
#define mfp_set_tadr(a) \
	mfp_base[MFP_TADR] = ((u_int8_t) (a))
#define mfp_set_tbdr(a) \
	mfp_base[MFP_TBDR] = ((u_int8_t) (a))
#define mfp_set_tcdr(a) \
	mfp_base[MFP_TCDR] = ((u_int8_t) (a))
#define mfp_set_tddr(a) \
	mfp_base[MFP_TDDR] = ((u_int8_t) (a))
#define mfp_set_ucr(a) \
	mfp_base[MFP_UCR] = ((u_int8_t) (a))
#define mfp_set_rsr(a) \
	mfp_base[MFP_RSR] = ((u_int8_t) (a))
#define mfp_set_tsr(a) \
	mfp_base[MFP_TSR] = ((u_int8_t) (a))
#define mfp_set_udr(a) \
	mfp_base[MFP_UDR] = ((u_int8_t) (a))

#define mfp_get_gpip() (mfp_base[MFP_GPIP])
#define mfp_get_aer() (mfp_base[MFP_AER])
#define mfp_get_ddr() (mfp_base[MFP_DDR])
#define mfp_get_iera() (mfp_base[MFP_IERA])
#define mfp_get_ierb() (mfp_base[MFP_IERB])
#define mfp_get_ipra() (mfp_base[MFP_IPRA])
#define mfp_get_iprb() (mfp_base[MFP_IPRB])
#define mfp_get_isra() (mfp_base[MFP_ISRA])
#define mfp_get_isrb() (mfp_base[MFP_ISRB])
#define mfp_get_imra() (mfp_base[MFP_IMRA])
#define mfp_get_imrb() (mfp_base[MFP_IMRB])
#define mfp_get_vr() (mfp_base[MFP_VR])
#define mfp_get_tacr() (mfp_base[MFP_TACR])
#define mfp_get_tbcr() (mfp_base[MFP_TBCR])
#define mfp_get_tcdcr() (mfp_base[MFP_TCDCR])
#define mfp_get_tadr() (mfp_base[MFP_TADR])
#define mfp_get_tbdr() (mfp_base[MFP_TBDR])
#define mfp_get_tcdr() (mfp_base[MFP_TCDR])
#define mfp_get_tddr() (mfp_base[MFP_TDDR])
#define mfp_get_ucr() (mfp_base[MFP_UCR])
#define mfp_get_rsr() (mfp_base[MFP_RSR])
#define mfp_get_tsr() (mfp_base[MFP_TSR])
#define mfp_get_udr() (mfp_base[MFP_UDR])

#define mfp_bit_set(reg,bits) (mfp_base[(reg)] |= (bits))
#define mfp_bit_clear(reg,bits) (mfp_base[(reg)] &= (~(bits)))

#define mfp_bit_set_gpip(bits) mfp_bit_set(MFP_GPIP, (bits))
#define mfp_bit_clear_gpip(bits) mfp_bit_clear(MFP_GPIP, (bits))
#define mfp_bit_set_aer(bits) mfp_bit_set(MFP_AER, (bits))
#define mfp_bit_clear_aer(bits) mfp_bit_clear(MFP_AER, (bits))
#define mfp_bit_set_iera(bits) mfp_bit_set(MFP_IERA, (bits))
#define mfp_bit_clear_iera(bits) mfp_bit_clear(MFP_IERA, (bits))
#define mfp_bit_set_ierb(bits) mfp_bit_set(MFP_IERB, (bits))
#define mfp_bit_clear_ierb(bits) mfp_bit_clear(MFP_IERB, (bits))

void mfp_wait_for_hsync(void);
int mfp_send_usart(int);
int mfp_receive_usart(void);
