/*	$NetBSD: dmaaureg.h,v 1.2 2001/01/19 11:42:21 sato Exp $	*/

/*-
 * Copyright (c) 1999,2000,2001 SATO Kazumi. All rights reserved.
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
 *	DMAAU (DMA Address UNIT) Registers.
 *		start 0x0B000020 
 */
#define	AIUIBAL_REG_W		0x000		/* AIU IN DMA Base Low */
#define	AIUIBAH_REG_W		0x002		/* AIU IN DMA Base High */


#define	AIUIAL_REG_W		0x004		/* AIU IN DMA Address Low */
#define	AIUIAH_REG_W		0x006		/* AIU IN DMA Address High */


#define	AIUOBAL_REG_W		0x008		/* AIU OUT DMA Base Low */
#define	AIUOBAH_REG_W		0x00A		/* AIU OUT DMA Base High */


#define	AIUOAL_REG_W		0x00C		/* AIU OUT DMA Address Low */
#define	AIUOAH_REG_W		0x00E		/* AIU OUT DMA Address High */


#define	FIRBAL_REG_W		0x010		/* FIU DMA Base Low */
#define	FIRBAH_REG_W		0x012		/* FIU DMA Base High */


#define	FIRAL_REG_W		0x014		/* FIU DMA Address Low */
#define	FIRAH_REG_W		0x016		/* FIU DMA Address High */

/* END dmaaureg.h */
