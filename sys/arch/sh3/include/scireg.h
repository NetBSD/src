/* $NetBSD: scireg.h,v 1.3 2001/06/12 15:17:20 wiz Exp $ */

/*-
 * Copyright (C) 1999 SAITOH Masanobu.  All rights reserved.
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
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH3_SCIREG_H_
#define _SH3_SCIREG_H_

/*
 * Serial Communication Interface (SCI)
 */

#if !defined(SH4)

/* SH3 definitions */

#define SHREG_SCSMR	(*(volatile unsigned char *)	0xFFFFFE80)
#define SHREG_SCBRR	(*(volatile unsigned char *)	0xFFFFFE82)
#define SHREG_SCSCR	(*(volatile unsigned char *)	0xFFFFFE84)
#define SHREG_SCTDR	(*(volatile unsigned char *)	0xFFFFFE86)
#define SHREG_SCSSR	(*(volatile unsigned char *)	0xFFFFFE88)
#define SHREG_SCRDR	(*(volatile unsigned char *)	0xFFFFFE8A)

#else

/* SH4 definitions */

#define SHREG_SCSMR	(*(volatile unsigned char *)	0xffe00000)
#define SHREG_SCBRR	(*(volatile unsigned char *)	0xffe00004)
#define SHREG_SCSCR	(*(volatile unsigned char *)	0xffe00008)
#define SHREG_SCTDR	(*(volatile unsigned char *)	0xffe0000c)
#define SHREG_SCSSR	(*(volatile unsigned char *)	0xffe00010)
#define SHREG_SCRDR	(*(volatile unsigned char *)	0xffe00014)

#endif

#define SCSCR_TIE	0x80	/* Transmit Interrupt Enable */
#define SCSCR_RIE	0x40	/* Receive Interrupt Enable */
#define SCSCR_TE	0x20	/* Transmit Enable */
#define SCSCR_RE	0x10	/* Receive Enable */
#define SCSCR_MPIE	0x08	/* Multi Processor Interrupt Enable */
#define SCSCR_TEIE	0x04	/* Transmit End Interrupt Enable */
#define SCSCR_CKE1	0x02	/* ClocK Enable 1 */
#define SCSCR_CKE0	0x01	/* ClocK Enable 0 */

#define SCSSR_TDRE	0x80
#define SCSSR_RDRF	0x40
#define SCSSR_ORER	0x20
#define SCSSR_FER	0x10
#define SCSSR_PER	0x08

#endif /* !_SH3_SCIREG_ */
