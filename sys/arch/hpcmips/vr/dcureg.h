/*	$NetBSD: dcureg.h,v 1.1.1.1 1999/09/16 12:23:32 takemura Exp $	*/

/*-
 * Copyright (c) 1999 SATO Kazumi. All rights reserved.
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 *	DCU (DMA Control UNIT) Registers.
 *		start 0x0B000040 
 */
#define	DMARST_REG_W		0x000		/* DMA Reset Register */
#define		DMARST			(1)		/* DMA reset */

#define	DMAIDLE_REG_W		0x002		/* DMA Idle Register */
#define		DMAISTAT		(1)		/* DMA Idle Status */
#define		DMAIDLE			(1)		/* DMA Idle */
#define		DMABUSY			(0)		/* DMA Busy */


#define	DMASEN_REG_W		0x004		/* DMA Sequencer Enable Register */
#define		DMASENMASK		(1)		/* DMA Seq Enable */
#define		DMASEN			(1)		/* DMA Seq Enable */
#define		DMASDS			(0)		/* DMA Seq Disable */


#define	DMAMSK_REG_W		0x006		/* DMA Mask Register */
#define		DMAMSKAIN		(1<<3)	/* Audio IN  DMA Enable */
#define		DMAMSKAOUT		(1<<2)	/* Audio OUT  DMA Enable */
#define		DMAMSKFOUT		(1)	/* FIR DMA Enable  */


#define	DMAREQ_REG_W		0x008		/* DMA Request Register */
#define		DMAREQAIN		(1<<3)		/* Audio IN Request pending */
#define		DMAREQAOUT		(1<<2)		/* Audio OUT Request pending */
#define		DMAREQFOUT		(1)		/* FIR Request pending */


#define	DMATD_REG_W		0x00A		/* DMA Transfer Direction Register */
#define		DMATDMASK		(1)		/* DMA transfer direction (FIR) */
#define		DMATDIOMEM		(1)		/* I/O -> MEM */
#define		DMATDMEMIO		(0)		/* MEM -> I/O */

/* END dcureg.h */
