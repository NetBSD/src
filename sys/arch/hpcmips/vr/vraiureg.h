/*	$NetBSD: vraiureg.h,v 1.1 2002/03/23 09:02:02 hamajima Exp $	*/

/*
 * Copyright (c) 2001 HAMAJIMA Katsuomi. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 *	AIU (Audio Interface Unit) Registers definitions.
 */

#define	MDMADAT_REG_W	0x000	/* Mic DMA Data Register (10bit) */

#define	SDMADAT_REG_W	0x002	/* Speaker DMA Data Register (10bit) */

#define	SODATA_REG_W	0x006	/* Speaker Output Data Register (10bit) */

#define	SCNT_REG_W	0x008	/* Speaker Control Register */
#define		DAENAIU		(1<<15)	/* D/A Enable */
#define		SSTATE		(1<<3)	/* Speaker Status */
#define		SSTOPEN		(1<<1)	/* Speaker Stop End
					   (1: 1 page, 0: 2 page) */

#define	SCNVR_REG_W	0x00a	/* Speaker Converter Rate Register */
#define		SPS8000		(4)	/* 8k sps */
#define		SPS44100	(2)	/* 44.1k sps */
#define		SPS22050	(1)	/* 22.05k sps */
#define		SPS11025	(0)	/* 11.025k sps */

#define	MIDAT_REG_W	0x010	/* Mic Input Data Register (10bit) */

#define	MCNT_REG_W	0x012	/* Mic Control Register */
#define		ADENAIU		(1<<15)	/* A/D Enable */
#define		MSTATE		(1<<3)	/* Mic Status */
#define		MSTOPEN		(1<<1)	/* Mic Stop End
					   (1: 1 page, 0: 2 page) */
#define		ADREQAIU	(1)	/* A/D Request */

#define	MCNVR_REG_W	0x014	/* Mic Converter Rate Register */
/* same SCNVR_REG_W(0x00a)
#define		SPS8000		(4)
#define		SPS44100	(2)
#define		SPS22050	(1)
#define		SPS11025	(0)
*/

#define	DVALID_REG_W	0x018	/* Data Valid Register */
#define		SODATV		(1<<3)	/* SODATREG Valid */
#define		SOMAV		(1<<2)	/* SDMADATREG Valid */
#define		MIDATV		(1<<1)	/* MIDATREG Valid */
#define		MDMAV		(1)	/* MDMADATREG Valid */

#define	SEQ_REG_W	0x01a	/* Sequencer Register */
#define		AIURST		(1<<15)	/* AIU Reset */
#define		AIUMEN		(1<<4)	/* Mic Enable */
#define		AIUSEN		(1)	/* Speaker Enable */

#define	INT_REG_W	0x01c	/* Interrupt Register */
#define		MENDINTR	(1<<11)	/* Mic End Interrupt */
#define		MINTR		(1<<10)	/* Mic Interrupt */
#define		MIDLEINTR	(1<<9)	/* Mic Idle Interrupt */
#define		MSTINTR		(1<<8)	/* Mic Set Interrupt */
#define		SENDINTR	(1<<3)	/* Speaker End Interrupt */
#define		SINTR		(1<<2)	/* Speaker Interrupt */
#define		SIDLEINTR	(1<<1)	/* Speaker Idle Interrupt */
