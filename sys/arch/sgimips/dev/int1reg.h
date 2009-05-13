/*	$NetBSD: int1reg.h,v 1.2.6.2 2009/05/13 17:18:18 jym Exp $	*/

/*
 * Copyright (c) 2009 Stephen M. Rumble
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
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

#if !defined(_ARCH_SGIMIPS_DEV_INT1REG_H_)
#define	_ARCH_SGIMIPS_DEV_INT1REG_H_

/* The INT has known locations on all SGI machines */
#define	INT1_IP6_IP10		0x1f980000

/*
 * NB: The STATUS register is backwards w.r.t. INT2: a bit set implies
 *     no pending interrupt. The MASK register is like INT2: a bit
 *     set implies that the interrupt is enabled.
 */
#define INT1_LOCAL_STATUS	0x000002	/* 16-bit */
#define INT1_LOCAL_MASK		0x00000b	/*  8-bit */

/* i8254 is actually its own chip, but we can pretend to be like INT2... */
#define INT1_TIMER_0_ACK	0x0a0000	/*  8-bit */
#define INT1_TIMER_1_ACK	0x080000	/*  8-bit */
#define INT1_TIMER_0		0x1c0000	/*  8-bit */
#define	INT1_TIMER_1		0x1c0004	/*  8-bit */
#define	INT1_TIMER_2		0x1c0008	/*  8-bit */
#define INT1_TIMER_CONTROL	0x1c000c	/*  8-bit */

#endif /* _ARCH_SGIMIPS_DEV_INT1REG_H_ */
