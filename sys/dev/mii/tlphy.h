/*  $NetBSD: tlphy.h,v 1.1 1997/10/17 17:34:07 bouyer Exp $   */
 
/*
 * Copyright (c) 1997 Manuel Bouyer.  All rights reserved.
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
 *  This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /* control register */
#   define  CTRL_PDOWN      0x0800 /* power down */

/* Thunderland specific registers */
#define	PHY_TL_CTRL	0x11	/* Read/Write */
#	define	TL_CTRL_ILINK	0x8000 /* Ignore link */
#	define	TL_CTRL_SWPOL	0x4000 /* swap polarity */
#	define	TL_CTRL_AUISEL	0x2000 /* Select AUI */
#	define	TL_CTRL_SQEEN	0x1000 /* Enable SQE */
#	define	TL_CTRL_NFEW	0x0004 /* Not far end wrap */
#	define	TL_CTRL_INTEN	0x0002 /* Interrupts enable */
#	define	TL_CTRL_TINT	0x0001 /* Test Interrupts */

#define	PHY_TL_ST	0x12	/* Read Only */
#	define	TL_ST_MII_Int	0x8000 /* MII interrupt */
#	define	TL_ST_PHOK		0x4000 /* Power higth OK */
#	define	TL_ST_PolOK		0x2000 /* Polarity OK */
#	define	TL_ST_TPE		0x1000 /* Twisted pair energy */

