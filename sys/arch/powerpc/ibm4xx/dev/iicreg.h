/*	$NetBSD: iicreg.h,v 1.1 2002/08/13 04:57:49 simonb Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge and Eduardo Horvath for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IBM4XX_IICREG_H_
#define	_IBM4XX_IICREG_H_

/* IIC Registers */
#define	IIC_MDBUF		0x00	/* Master Data Buffer */
#define	IIC_SDBUF		0x02	/* Slave Data Buffer */
#define	IIC_LMADR		0x04	/* Low Master Address */
#define	IIC_HMADR		0x05	/* High Master Address */
#define	IIC_CNTL		0x06	/* Control */
#define	IIC_MDCNTL		0x07	/* Mode Control */
#define	IIC_STS			0x08	/* Status */
#define	IIC_EXTSTS		0x09	/* Extended Status */
#define	IIC_LSADR		0x0a	/* Low Slave Address */
#define	IIC_HSADR		0x0b	/* High Slave Address */
#define	IIC_CLKDIV		0x0c	/* Clock Divide */
#define	IIC_INTRMSK		0x0d	/* Interrupt Mask */
#define	IIC_XFRCNT		0x0e	/* Transfer Count */
#define	IIC_XTCNTLSS		0x0f	/* Extended Control and Slave Status */
#define	IIC_DIRECTCNTL		0x10	/* Direct Control */
#endif	/* _IBM4XX_IICREG_H_ */
