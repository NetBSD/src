/*	$NetBSD: dmacreg.h,v 1.2.30.1 2007/04/10 13:23:14 ad Exp $ */

/*
 * Copyright (c) 2004 Valeriy E. Ushakov
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _SH3_DMACREG_H_
#define _SH3_DMACREG_H_

#include <sh3/devreg.h>


#define SH3_DMAC_SAR0		0xa4000020 /* Source Address */
#define SH3_DMAC_DAR0		0xa4000024 /* Destination Address */
#define SH3_DMAC_DMATCR0	0xa4000028 /* Transfer Counter */
#define SH3_DMAC_CHCR0		0xa400002c /* Channel Control */

#define SH3_DMAC_SAR1		0xa4000030 /* ditto for channel 1 */
#define SH3_DMAC_DAR1		0xa4000034
#define SH3_DMAC_DMATCR1	0xa4000038
#define SH3_DMAC_CHCR1		0xa400003c

#define SH3_DMAC_SAR2		0xa4000040 /* ditto for channel 2 */
#define SH3_DMAC_DAR2		0xa4000044
#define SH3_DMAC_DMATCR2	0xa4000048
#define SH3_DMAC_CHCR2		0xa400004c

#define SH3_DMAC_SAR3		0xa4000050 /* ditto for channel 3 */
#define SH3_DMAC_DAR3		0xa4000054
#define SH3_DMAC_DMATCR3	0xa4000058
#define SH3_DMAC_CHCR3		0xa400005c

#define SH3_DMAC_DMAOR		0xa4000060 /* DMA Operation Register */

#define SH3_DMAC_CMT_CMSTR	0xa4000070 /* CMT Start */
#define SH3_DMAC_CMT_CMCSR	0xa4000072 /* CMT Control/Status */
#define SH3_DMAC_CMT_CMCNT	0xa4000074 /* CMT Counter */
#define SH3_DMAC_CMT_CMCOR	0xa4000076 /* CMT Constant */


/**
 * Only bits 0..23 of DMATCR registers are valid.
 * Writing 0 to these registers means count of SH3_DMAC_DMATCR_MAX.
 */
#define SH3_DMAC_DMATCR_MAX		0x01000000


/**
 * Channel Control Register bits.
 */

/* Direct (0) or Indirect (1) mode. */
#define SH3_DMAC_CHCR_DI		0x00100000

/* Source address reload.
   Only valid for channel 2. */
#define SH3_DMAC_CHCR_RO		0x00080000

/* Request check level (0 - low, 1 - high).
   Only valid for channels 0 and 1. */
#define SH3_DMAC_CHCR_RL		0x00040000

/* Acknowledge mode (0 - read, 1 - write).
   Only valid for channels 0 and 1. */
#define SH3_DMAC_CHCR_AM		0x00020000

/* Acknowledge level (0 - low, 1 - high).
   Only valid for channels 0 and 1. */
#define SH3_DMAC_CHCR_AL		0x00010000

/* Destination address mode. */
#define SH3_DMAC_CHCR_DM_MASK		0x0000c000
#define SH3_DMAC_CHCR_DM_FIXED		0x00000000
#define SH3_DMAC_CHCR_DM_INC		0x00004000
#define SH3_DMAC_CHCR_DM_DEC		0x00008000

/* Source address mode. */
#define SH3_DMAC_CHCR_SM_MASK		0x00003000
#define SH3_DMAC_CHCR_SM_FIXED		0x00000000
#define SH3_DMAC_CHCR_SM_INC		0x00001000
#define SH3_DMAC_CHCR_SM_DEC		0x00002000

/* Resource select */
#define SH3_DMAC_CHCR_RS_MASK		0x00000f00
#define SH3_DMAC_CHCR_RS_EXT_DUAL	0x00000000
#define SH3_DMAC_CHCR_RS_EXT_OUT	0x00000200
#define SH3_DMAC_CHCR_RS_EXT_IN		0x00000300
#define SH3_DMAC_CHCR_RS_AUTO		0x00000400
#define SH3_DMAC_CHCR_RS_IRDA_TX	0x00000a00
#define SH3_DMAC_CHCR_RS_IRDA_RX	0x00000b00
#define SH3_DMAC_CHCR_RS_SCIF_TX	0x00000c00
#define SH3_DMAC_CHCR_RS_SCIF_RX	0x00000d00
#define SH3_DMAC_CHCR_RS_ADC		0x00000e00
#define SH3_DMAC_CHCR_RS_CMT		0x00000f00

/* ~DREQ select (0 - low, 1 - high).
   Only valid for channels 0 and 1. */
#define SH3_DMAC_CHCR_DS		0x00000040

/* Transmit mode (0 - cycle steal, 1 - burst). */
#define SH3_DMAC_CHCR_TM		0x00000020

/* Transmit size */
#define SH3_DMAC_CHCR_TS_MASK		0x00000018
#define SH3_DMAC_CHCR_TS_1		0x00000000
#define SH3_DMAC_CHCR_TS_2		0x00000008
#define SH3_DMAC_CHCR_TS_4		0x00000010
#define SH3_DMAC_CHCR_TS_16		0x00000018

/* Interrupt enable. */
#define SH3_DMAC_CHCR_IE		0x00000004

/* Transfer end. */
#define SH3_DMAC_CHCR_TE		0x00000002

/* DMAC enable. */
#define SH3_DMAC_CHCR_DE		0x00000001

#define SH3_DMAC_CHCR_BITS "\177\20"					\
	"b\24DI\0" "b\23RO\0" "b\22RL\0" "b\21AM\0" "b\20AL\0"		\
	"f\16\2DM\0" ":\0(FIXED)\0" ":\1(INC)\0" ":\2(DEC)\0"		\
	"f\14\2SM\0" ":\0(FIXED)\0" ":\1(INC)\0" ":\2(DEC)\0"		\
	"f\10\4RS\0" ":\0(EXT_DUAL)\0" ":\2(EXT_OUT)\0" ":\3(EXT_IN)\0"	\
		":\4(AUTO)\0"						\
		":\12(IRTX)\0" ":\13(IRRX)\0" ":\14(SCTX)\0"		\
		":\15(SCRX)\0" ":\16(ADC)\0" ":\17(CMT)\0"		\
	"b\6DS\0" "b\5TM\0"						\
	"f\3\2TS\0" ":\0(1)\0" ":\1(2)\0" ":\2(4)\0" ":\3(16)\0"	\
	"b\2IE\0" "b\1TE\0" "b\0DE\0"


/**
 * DMA Operation Register bits
 */

/* Priority mode. */
#define SH3_DMAC_DMAOR_PR_MASK		0x0300
#define SH3_DMAC_DMAOR_PR_0123		0x0000 /* 0 > 1 > 2 > 3 */
#define SH3_DMAC_DMAOR_PR_0231		0x0100 /* 0 > 2 > 3 > 1 */
#define SH3_DMAC_DMAOR_PR_2013		0x0200 /* 2 > 0 > 1 > 3 */
#define SH3_DMAC_DMAOR_PR_RR		0x0300 /* round robbin */

/* Address error flag. */
#define SH3_DMAC_DMAOR_AE		0x0004

/* NMI flag. */
#define SH3_DMAC_DMAOR_NMIF		0x0002

/* DMA master enable. */
#define SH3_DMAC_DMAOR_DME		0x0001

#define SH3_DMAC_DMAOR_BITS "\177\20"					   \
	"f\10\2PR\0" ":\0(0123)\0" ":\1(0231)\0" ":\2(2013)\0" ":\3(RR)\0" \
	"b\2AE\0" "b\1NMIF\0" "b\0DME\0"


/**
 * Compare Match Timer.
 */

/* Start the CMT. */
#define SH3_DMAC_CMT_CMSTR_STR		0x0001

/* Compare Match Flag. */
#define SH3_DMAC_CMT_CMCSR_CMF		0x0080

/* Clock select (PCLOCK/x). */
#define SH3_DMAC_CMT_CMCSR_CKS_MASK	0x0003
#define SH3_DMAC_CMT_CMCSR_CKS_4	0x0000
#define SH3_DMAC_CMT_CMCSR_CKS_8	0x0001
#define SH3_DMAC_CMT_CMCSR_CKS_16	0x0002
#define SH3_DMAC_CMT_CMCSR_CKS_64	0x0003

#define SH3_DMAC_CMT_CMCSR_BITS "\177\20"				\
	"b\7CMF\0"							\
	"f\0\2CKS\0" ":\0(1/4)\0" ":\1(1/8)\0" ":\2(1/16)\0" ":\3(1/64)\0"

#endif /* _SH3_DMACREG_H_ */
