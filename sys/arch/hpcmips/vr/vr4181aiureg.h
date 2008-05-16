/* $NetBSD: vr4181aiureg.h,v 1.1.108.1 2008/05/16 02:22:30 yamt Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Naoto Shimazaki of YOKOGAWA Electric Corporation.
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
 *	VR4181 AIU (Audio Interface Unit) Registers definitions.
 */

#define VR4181AIU_DCU1_BASE	0x0a000020
#define VR4181AIU_DCU1_SIZE	0x28
#define VR4181AIU_DCU2_BASE	0x0a000650
#define VR4181AIU_DCU2_SIZE	0x18
#define VR4181AIU_AIU_BASE	0x0b000160
#define VR4181AIU_AIU_SIZE	0x20


#define	VR4181AIU_SDMADAT_REG_W	0x00	/* speaker DMA data (10bit) */

#define	VR4181AIU_MDMADAT_REG_W	0x02	/* microphone DMA data (10bit) */

#define	VR4181AIU_DAVREF_SETUP_REG_W	0x004	/* D/A Vref setup */

#define	VR4181AIU_SODATA_REG_W	0x06	/* speaker output data (10bit) */

#define	VR4181AIU_SCNT_REG_W	0x08	/* speaker control */
#define	 VR4181AIU_DAENAIU	0x8000	/* D/A enable */
#define	 VR4181AIU_SSTATE	0x0008	/* speaker status */
#define	 VR4181AIU_SSTOPEN	0x0002	/* speaker stop end
					   (1: 1 page, 0: 2 page) */

#define	VR4181AIU_SCNVC_END	0x0e	/* speaker convert rate */

#define	VR4181AIU_MIDAT_REG_W	0x10	/* microphone input data (10bit) */

#define	VR4181AIU_MCNT_REG_W	0x12	/* microphone control */
#define	 VR4181AIU_ADENAIU	0x8000	/* A/D enable */
#define	 VR4181AIU_MSTATE	0x0008	/* microphone status */
#define	 VR4181AIU_MSTOPEN	0x0002	/* microphone stop end
					   (1: 1 page, 0: 2 page) */
#define  VR4181AIU_ADREQAIU	0x0001	/* A/D Request */

#define	VR4181AIU_DVALID_REG_W	0x18	/* data valid */
#define	 VR4181AIU_SODATV	0x0008	/* SODATREG valid */
#define  VR4181AIU_SOMAV	0x0004	/* SDMADATREG valid */
#define  VR4181AIU_MIDATV	0x0002	/* MIDATREG valid */
#define  VR4181AIU_MDMAV	0x0001	/* MDMADATREG valid */

#define	VR4181AIU_SEQ_REG_W	0x1a	/* sequencer */
#define	 VR4181AIU_AIURST	0x8000	/* AIU reset */
#define  VR4181AIU_AIUMEN	0x0010	/* microphone enable */
#define	 VR4181AIU_AIUSEN	0x0001	/* speaker enable */

#define	VR4181AIU_INT_REG_W	0x1c	/* interrupt */
#define  VR4181AIU_MIDLEINTR	0x0200	/* microphone idle interrupt */
#define  VR4181AIU_MSTINTR	0x0100	/* microphone set interrupt */
#define  VR4181AIU_SIDLEINTR	0x0002	/* speaker idle interrupt */

#define	VR4181AIU_MCNVC_END	0x1e	/* microphone convert rate */
