/*	$NetBSD: sr_601.h,v 1.2 2003/01/22 21:44:55 kleink Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus J. Klein.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _MPC6XX_SR_601_H_
#define _MPC6XX_SR_601_H_

/*
 * I/O Controller Interface Address Translation segment register
 * format specific to the PowerPC 601, per PowerPC 601 RISC
 * Microprocessor User's Manual, section 6.10.1.
 *
 * This format applies to a segment register only when its T bit is set.
 */

#define	SR601_T		0x80000000	/* Selects this format */
#define	SR601_Ks	0x40000000	/* Key-supervisor */
#define	SR601_Ku	0x20000000	/* Key-user */
#define	SR601_BUID	0x1ff00000	/* Bus unit ID */
#define	SR601_BUID_SHFT	20
#define	SR601_CSI	0x000ffff0	/* Controller Specific Information */
#define	SR601_CSI_SHFT	4
#define	SR601_PACKET1	0x0000000f	/* Address bits 0:3 of packet 1 cycle */

#define	SR601_BUID_MEMFORCED	0x07f	/* Translate to memory access, taking
					   PA[0:3] from the PACKET1 field */

#define	SR601(key, buid, csi, p1)			\
	(SR601_T |					\
	 (key) |					\
	 (buid) << SR601_BUID_SHFT |			\
	 (csi) << SR601_CSI_SHFT | (p1))

#ifdef _KERNEL
void mpc601_ioseg_add(paddr_t, register_t);
#endif

#endif /* !_MPC6XX_SR_601_H_ */
