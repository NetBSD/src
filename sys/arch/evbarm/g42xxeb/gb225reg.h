/*	$NetBSD: gb225reg.h,v 1.2 2005/12/11 12:17:08 christos Exp $ */

/*
 * Copyright (c) 2002  Genetec corp.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec corp.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec corp. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _EVBARM_GB225_REG_H
#define _EVBARM_GB225_REG_H

#include <arm/xscale/pxa2x0reg.h>

#define GB225_PLDREG_BASE	PXA2X0_CS4_START
#define GB225_PLDREG_SIZE 	0x0a

#define GB225_CFDET 		0	/* CompactFlash slot card detect */
#define GB225_PCMCIADET 	2	/* PCMCIA slot card detect */
#define  CARDDET_DET   0x03
#define  CARDDET_BVD1  (1<<2)
#define  CARDDET_BVD2  (1<<3)
#define  CARDDET_NVS1  (1<<4)
#define  CARDDET_NVS2  (1<<5)
#define  CARDDET_NOCARD  0x3f
#define GB225_CARDPOW   	4	/* card power supply control */
#define  CARDPOW_PCMCIA_SHIFT	0
#define  CARDPOW_CF_SHIFT	4
#define  CARDPOW_VPP0	0x00
#define  CARDPOW_VPP12  0x01
#define  CARDPOW_VPPVCC 0x02
#define  CARDPOW_VPPHIZ 0x03
#define  CARDPOW_VCC0	0x00
#define  CARDPOW_VCC33	0x04	/* 3.3V */
#define  CARDPOW_VCC5   0x08
#define  CARDPOW_MASK 	0x0f
#define  CARDPOW_VPPMASK 0x03
#define  CARDPOW_VCCMASK 0x0c

#define GB225_IRDA_CONT  	6	/* IrDA control */
#define  IRDACONT_SIRFULL 	0
#define  IRDACONT_SHUTDOWN 	1
#define  IRDACONT_SIR2 		2	/* 2/3 */
#define  IRDACONT_SIR1    	3	/* 1/3 */
#define  IRDACONT_FIRFULL 	4
#define  IRDACONT_FIR2 		6	/* 2/3 */
#define  IRDACONT_FIR1    	7	/* 1/3 */

#define GB225_DEVERROR 	8
#define  DEVERROR_CARD	(1<<0)		/* CF/PCMCIA power failure */
#define  DEVERROR_USB	(1<<1)		/* USB power failure */

#endif /* _EVBARM_GB225_REG_H */
