/*	$NetBSD: pcr.h,v 1.2 2026/04/11 19:09:34 thorpej Exp $	*/

/*-     
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _M68K_PCR_H_
#define	_M68K_PCR_H_

/* fields in the 68060 Processor Configuration Register */

#define	PCR_ESS		__BIT(0)     /* enable superscalar dispatch */
#define	PCR_DFP		__BIT(1)     /* disable floating point unit */
#define	PCR_mbz		__BITS(2,6)  /* reserved - must be zero */
#define	PCR_EDEBUG	__BIT(7)     /* enable debug features */
#define	PCR_REVMASK	__BITS(8,15) /* revision number */
#define	PCR_IDMASK	__BITS(16,31)/* identification */

#define	PCR_REVISION(x)	__SHIFTOUT((x), PCR_REVMASK)
#define	PCR_ID(x)	__SHIFTOUT((x), PCR_IDMASK)

#define	PCR_ID_MC68060   0x0430
#define	PCR_ID_MC68xC060 0x0431	/* MC68LC060 / MC60EC060 */

#ifdef _KERNEL
u_int	get_pcr(void);
void	set_pcr(u_int);
#endif

#endif /* _M68K_PCR_H_ */
