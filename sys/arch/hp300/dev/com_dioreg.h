/*	$NetBSD: com_dioreg.h,v 1.3 2005/12/11 12:17:13 christos Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)dcareg.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _HP300_DEV_COM_DIOREG_H_
#define	_HP300_DEV_COM_DIOREG_H_

/* interface reset/id */
#define DCA_ID		0x00			/* read */
#define  DCAID0		0x02
#define  DCAREMID0	0x82
#define  DCAID1		0x42
#define  DCAREMID1	0xC2

#define DCA_RESET	0x00			/* write */

/* interrupt control */
#define DCA_IC		0x01
#define  DCAIPL(x)	((((x) >> 4) & 3) + 3)
#define  IC_IR		0x40
#define  IC_IE		0x80

#define DCA_OCBRC	0x02

#define DCA_LCSM	0x03

#define DCA_SIZE	0x20	/* XXX */
#define DCA_COM_OFFSET	0x10	/* XXX */

/* clock frequency */
#define COM_DIO_FREQ	2457600

#endif /* _HP300_DEV_COM_DIOREG_H_ */
