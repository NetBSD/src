/*	$NetBSD: r5900_machdep.c,v 1.1 2001/10/16 16:31:40 uch Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

/* Toshiba R5900 specific functions */

#include <sys/param.h>
#include <sys/systm.h>
#include <mips/locore.h>
#include <mips/r5900/locore.h>
#include <mips/r5900/cpuregs.h>

mips_locore_jumpvec_t mips3_locore_vec =
{
	r5900_FlushCache,
	r5900_FlushDCache,
	r5900_FlushICache,
	r5900_HitFlushDCache,
	mips3_SetPID,
	mips3_TBIAP,
	mips3_TBIS,
	mips3_TLBUpdate,
	mips3_wbflush,
};

static void r5900_config_cache(void);

/*
 * R5900 exception vector
 *
 *	MMU supports 32bit-mode only. no XTLB miss handler.
 *	vector address is different from ordinaly MIPS3 derivative.
 */
void
r5900_init()
{
	extern char mips3_exception[], mips3_exceptionEnd[];
	extern char mips3_TLBMiss[], mips3_TLBMissEnd[];
	size_t esz = mips3_exceptionEnd - mips3_exception;
	size_t tsz = mips3_TLBMissEnd - mips3_TLBMiss;

	KDASSERT(tsz <= 0x80);

	memcpy((void *)R5900_TLB_REFIL_EXC_VEC, mips3_TLBMiss, tsz);
	memcpy((void *)R5900_COUNTER_EXC_VEC, mips3_exception, esz);
	memcpy((void *)R5900_DEBUG_EXC_VEC, mips3_exception, esz);
	memcpy((void *)R5900_COMMON_EXC_VEC, mips3_exception, esz);
	memcpy((void *)R5900_INTERRUPT_EXC_VEC, mips3_exception, esz);

	memcpy(&mips_locore_jumpvec, &mips3_locore_vec,
	    sizeof(mips_locore_jumpvec_t));

	r5900_config_cache();

	r5900_FlushCache();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS3_SR_DIAG_BEV);
}

/*
 * R5900 cache
 *	I-cache 16KB/64B 2-way assoc.	 
 *	D-cache 8KB/64B 2-way assoc.
 *	No L2-cache.
 *	
 *	and sync.p/sync.l are needed after/before cache instruction.
 *	
 *    + don't have IB, DB bit.
 */	
void
r5900_config_cache()
{
	mips_L1ICacheSize	= R5900_C_SIZE_I;
	mips_L1DCacheSize	= R5900_C_SIZE_D;
	mips_L1ICacheLSize	= R5900_C_LSIZE_I;
	mips_L1DCacheLSize	= R5900_C_LSIZE_D;
	mips_L2CachePresent	= 0;
	mips_L2CacheSize	= 0;

	mips_CacheAliasMask = (mips_L1DCacheSize - 1) & ~(NBPG - 1);
	mips_CachePreferMask = MAX(mips_L1DCacheSize,mips_L1ICacheSize) - 1;
}
