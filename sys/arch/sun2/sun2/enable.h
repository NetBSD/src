/*	$NetBSD: enable.h,v 1.1.10.2 2001/04/06 15:05:57 fredette Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Matthew Fredette.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * System Enable Register contents
 */
#define	ENA_PAR_GEN	0x01		/* enable parity generation */
#define	ENA_SOFT_INT_1	0x02		/* software interrupt on level 1 */
#define	ENA_SOFT_INT_2	0x04		/* software interrupt on level 2 */
#define	ENA_SOFT_INT_3	0x08		/* software interrupt on level 3 */
#define	ENA_PAR_CHECK	0x10		/* enable parity checking and errors */
#define	ENA_SDVMA	0x20		/* enable DVMA */
#define	ENA_INTS	0x40		/* enable interrupts */
#define	ENA_NOTBOOT	0x80		/* non-boot state */

/*
 * The Sun2 enable register is the ancestor of the Sun3 interrupt
 * register.  It can't be memory-mapped (and thus be manipulated with
 * the single-instruction (read: atomic) bit instructions) since it's
 * in control space.  SunOS seems reluctant to raise SPL (maybe it
 * doesn't help?) to manipulate it, so we won't either, but that means
 * we have to also do as it does and have these bizarre functions.
 */
#ifdef	_KERNEL
#define ENABLE_REG_SOFT_UNDEF (0)
volatile extern u_short enable_reg_soft;
u_short enable_reg_and __P((u_short));
u_short enable_reg_or __P((u_short));
#endif	/* _KERNEL */
