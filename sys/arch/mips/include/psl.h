/*	$NetBSD: psl.h,v 1.8 1997/06/22 07:42:52 jonathan Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)psl.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Define PSL_LOWIPL, PSL_USERSET, PSL_USERCLR, USERMODE, BASEPRI
 * for MI code, for MIPS1, MIPS3, or both, depending on the
 * configured CPU types.
 */

#include <mips/cpuregs.h>

/*
 * mips3-specific  definitions
 */
#define	MIPS3_PSL_LOWIPL	(MIPS3_INT_MASK | MIPS_SR_INT_IE)

#define	MIPS3_PSL_USERSET 	\
	(MIPS3_SR_KSU_USER |	\
	 MIPS_SR_INT_IE |	\
	 MIPS3_SR_EXL |		\
	 MIPS3_INT_MASK)

#define	MIPS3_PSL_USERCLR 	\
	(MIPS_SR_COP_USABILITY |\
	 MIPS_SR_BOOT_EXC_VEC |	\
	 MIPS_SR_TLB_SHUTDOWN |	\
	 MIPS_SR_PARITY_ERR |	\
	 MIPS_SR_CACHE_MISS |	\
	 MIPS_SR_PARITY_ZERO |	\
	 MIPS_SR_SWAP_CACHES |	\
	 MIPS_SR_ISOL_CACHES |	\
	 MIPS_SR_KU_CUR |	\
	 MIPS_SR_INT_IE |	\
	 MIPS_SR_MBZ)

#define	MIPS3_USERMODE(ps) \
	(((ps) & MIPS3_SR_KSU_MASK) == MIPS3_SR_KSU_USER)

#define	MIPS3_BASEPRI(ps) \
	(((ps) & (MIPS3_INT_MASK | MIPS_SR_INT_ENA_PREV)) \
			== (MIPS3_INT_MASK | MIPS_SR_INT_ENA_PREV))


/*
 * mips1-specific definitions
 */
#define	MIPS1_PSL_LOWIPL	(MIPS_INT_MASK | MIPS_SR_INT_IE)

#define	MIPS1_PSL_USERSET \
	(MIPS1_SR_KU_OLD |	\
	 MIPS1_SR_INT_ENA_OLD |	\
	 MIPS1_SR_KU_PREV |	\
	 MIPS1_SR_INT_ENA_PREV |\
	 MIPS_INT_MASK)

#define	MIPS1_PSL_USERCLR \
	(MIPS_SR_COP_USABILITY |\
	 MIPS_SR_BOOT_EXC_VEC |	\
	 MIPS_SR_TLB_SHUTDOWN |	\
	 MIPS_SR_PARITY_ERR |	\
	 MIPS_SR_CACHE_MISS |	\
	 MIPS_SR_PARITY_ZERO |	\
	 MIPS_SR_SWAP_CACHES |	\
	 MIPS_SR_ISOL_CACHES |	\
	 MIPS_SR_KU_CUR |	\
	 MIPS_SR_INT_IE |	\
	 MIPS_SR_MBZ)

#define	MIPS1_USERMODE(ps) \
	((ps) & MIPS1_SR_KU_PREV)

#define	MIPS1_BASEPRI(ps) \
		(((ps) & (MIPS_INT_MASK | MIPS1_SR_INT_ENA_PREV)) == \
		 (MIPS_INT_MASK | MIPS1_SR_INT_ENA_PREV))


/*
 * Choose mips3-only, mips1-only, or runtime-selected values.
 */

#if defined(MIPS3) && !defined(MIPS1) /* mips3 only */
# define  PSL_LOWIPL	MIPS3_PSL_LOWIPL
# define  PSL_USERSET	MIPS3_PSL_USERSET
# define  PSL_USERCLR	MIPS3_PSL_USERCLR
# define  USERMODE(ps)	MIPS3_USERMODE(ps)
# define  BASEPRI(ps)	MIPS3_BASEPRI(ps)
#endif /* mips3 only */


#if !defined(MIPS3) && defined(MIPS1) /* mips1 only */
# define  PSL_LOWIPL	MIPS1_PSL_LOWIPL
# define  PSL_USERSET	MIPS1_PSL_USERSET
# define  PSL_USERCLR	MIPS1_PSL_USERCLR
# define  USERMODE(ps)	MIPS1_USERMODE(ps)
# define  BASEPRI(ps)	MIPS1_BASEPRI(ps)
#endif /* mips1 only */


#if  MIPS3 +  MIPS1 > 1
# define PSL_LOWIPL \
	((cpu_arch == 3) ? MIPS3_PSL_LOWIPL : MIPS1_PSL_LOWIPL)
#define PSL_USERSET \
	((cpu_arch == 3) ? MIPS3_PSL_USERSET : MIPS1_PSL_USERSET)
# define PSL_USRCLR \
	((cpu_arch == 3) ? MIPS3_PSL_USRCLR : MIPS1_PSL_USRCLR)
# define USERMODE(ps) \
	((cpu_arch == 3) ? MIPS3_USERMODE(ps) : MIPS1_USERMODE(ps))
# define  BASEPRI(ps)	\
	((cpu_arch == 3) ? MIPS3_BASEPRI(ps) : MIPS1_BASEPRI(ps))
#endif
