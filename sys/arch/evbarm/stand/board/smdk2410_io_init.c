/* $NetBSD: smdk2410_io_init.c,v 1.1 2003/09/03 03:18:31 mycroft Exp $ */

/*
 * Copyright (c) 2003 By Noon Software, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <arm/s3c2xx0/s3c2410reg.h>

#define IOW(a, d)	(*(volatile unsigned int *)(a) = (d))
#define IOR(a)		(*(volatile unsigned int *)(a))

void smdk2410_io_init(void);

void
smdk2410_io_init(void)
{
#define	O	PCON_OUTPUT
#define	I	PCON_INPUT
#define	A	PCON_ALTFUN
#define	_	0       
#define	_C(b15,b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1,b0) \
	((b15<<30)|(b14<<28)|(b13<<26)|(b12<<24)|(b11<<22)|(b10<<20)|(b9<<18)|(b8<<16)|(b7<<14)|(b6<<12)|(b5<<10)|(b4<<8)|(b3<<6)|(b2<<4)|(b1<<2)|(b0<<0))

	/* GPIO port */
	IOW(S3C2410_GPIO_BASE+GPIO_PACON, 0x7fffff);
	IOW(S3C2410_GPIO_BASE+GPIO_PBCON, _C(_,_,_,_,_,I,O,I,O,I,O,O,O,O,O,O));
	IOW(S3C2410_GPIO_BASE+GPIO_PBUP, 0x07ff);
	IOW(S3C2410_GPIO_BASE+GPIO_PCCON, _C(A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A));
	IOW(S3C2410_GPIO_BASE+GPIO_PCUP, 0xffff);
	IOW(S3C2410_GPIO_BASE+GPIO_PDCON, _C(A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A));
	IOW(S3C2410_GPIO_BASE+GPIO_PDUP, 0xffff);
	IOW(S3C2410_GPIO_BASE+GPIO_PECON, _C(A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A));
	IOW(S3C2410_GPIO_BASE+GPIO_PEUP, 0xffff);
	IOW(S3C2410_GPIO_BASE+GPIO_PFCON, _C(_,_,_,_,_,_,_,_,O,O,O,O,A,A,A,A));
	IOW(S3C2410_GPIO_BASE+GPIO_PFUP, 0x00ff);
	IOW(S3C2410_GPIO_BASE+GPIO_PGCON, _C(3,3,3,3,A,O,O,O,3,3,3,3,A,3,A,A));
	IOW(S3C2410_GPIO_BASE+GPIO_PGUP, 0xffff);
	IOW(S3C2410_GPIO_BASE+GPIO_PHCON, _C(_,_,_,_,_,A,A,A,3,3,A,A,A,A,A,A));
	IOW(S3C2410_GPIO_BASE+GPIO_PHUP, 0x07ff);
	IOW(S3C2410_GPIO_BASE+GPIO_EXTINT(0), 0x22222222);
	IOW(S3C2410_GPIO_BASE+GPIO_EXTINT(1), 0x22222222);
	IOW(S3C2410_GPIO_BASE+GPIO_EXTINT(2), 0x22222222);

#undef	O
#undef	I
#undef	A
#undef	_
#undef 	_C

	/* Interrupt controller */
	IOW(S3C2410_INTCTL_BASE+INTCTL_INTMOD, 0);
	IOW(S3C2410_INTCTL_BASE+INTCTL_INTMSK, 0);
}
