/*	$NetBSD: int2reg.h,v 1.1 2004/01/10 05:16:57 sekiya Exp $	*/

/*
 * Copyright (c) 2004 Christopher SEKIYA
 * All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if !defined(_ARCH_SGIMIPS_DEV_INT2_H_)
#define	_ARCH_SGIMIPS_DEV_INT2_H_

#define INT2_LOCAL0_STATUS	0x00
#define INT2_LOCAL0_MASK	0x04
#define INT2_LOCAL1_STATUS	0x08
#define INT2_LOCAL1_MASK	0x0c
#define INT2_MAP_STATUS		0x10
#define INT2_MAP_MASK0		0x14
#define INT2_MAP_MASK1		0x18
#define INT2_MAP_POL		0x1c
#define INT2_TIMER_CLEAR	0x20
#define INT2_ERROR_STATUS	0x24
#define INT2_TIMER_0		0x30
#define	INT2_TIMER_1		0x34
#define	INT2_TIMER_2		0x38
#define INT2_TIMER_CONTROL	0x3c

#endif /* _ARCH_SGIMIPS_DEV_INT2_H_ */

