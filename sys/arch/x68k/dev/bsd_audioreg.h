/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)bsd_audioreg.h	8.1 (Berkeley) 6/11/93
 *
 * from: Header: bsd_audioreg.h,v 1.3 92/06/07 21:12:50 mccanne Exp  (LBL)
 * $NetBSD: bsd_audioreg.h,v 1.1.1.1.12.1 1997/10/14 10:20:06 thorpej Exp $
 */

/*
 * Bit encodings for chip commands from "Microprocessor Access Guide for
 * Indirect Registers", p.19 Am79C30A/32A Advanced Micro Devices spec 
 * sheet (preliminary).
 *
 * Indirect register numbers (the value written into cr to select a given
 * chip registers) have the form AMDR_*.  Register fields look like AMD_*.
 */

#define ADPCM_CMD_STOP	0x01
#define ADPCM_CMD_PLAY	0x02
#define ADPCM_CMD_REC	0x04

#define ADPCM_CLOCK_4MHZ	0x80
#define ADPCM_CLOCK_8MHZ	0x00

#define ADPCM_RATE_512	0x08
#define ADPCM_RATE_768	0x04
#define ADPCM_RATE_1024	0x00

#define ADPCM_PAN_LEFT_OFF	0x01
#define ADPCM_PAN_RIGHT_OFF	0x02


