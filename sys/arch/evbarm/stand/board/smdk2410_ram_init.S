/* $NetBSD: smdk2410_ram_init.S,v 1.1 2003/09/03 03:18:31 mycroft Exp $ */

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

#include <machine/asm.h>
#include <arm/armreg.h>
#include <arm/s3c2xx0/s3c2410reg.h>

ENTRY(smdk2410_ram_init)
	/* Initialize memory controller */
	adr	r0, Lmemctl_initial_data
	ldr	r1, [r0], #4
	adr	r2, Lmemctl_initial_data_end
1:
	ldr	r3, [r0], #4
	str	r3, [r1], #4
	cmp	r0, r2
	blo	1b

	mov	pc,lr

#define	BANKCON_VAL_ROM(tacs,tcoc,toch,tacc,tcah,tacp) \
	(BANKCON_MT_ROM | \
	 ((tacs)<<BANKCON_TACS_SHIFT) | ((tcoc)<<BANKCON_TCOS_SHIFT) |  \
	 ((toch)<<BANKCON_TOCH_SHIFT) | ((tacc)<<BANKCON_TACC_SHIFT) |  \
	 ((tcah)<<BANKCON_TCAH_SHIFT) | ((tacp)<<BANKCON_TACP_SHIFT))

#define	BANKCON_VAL_DRAM(trcd,scan) \
	(BANKCON_MT_DRAM | ((trcd)<<BANKCON_TRCD_SHIFT) | scan)
	
Lmemctl_initial_data:
	.word	S3C2410_MEMCTL_BASE + MEMCTL_BWSCON	    /* address */
	.word	(BWSCON_DW_32<<28)|(BWSCON_DW_32<<24)|(BWSCON_DW_16<<20)| \
		(BWSCON_DW_16<<16)|((BWSCON_DW_16|BWSCON_WS|BWSCON_ST)<<12)| \
		((BWSCON_DW_16|BWSCON_WS|BWSCON_ST)<< 8)|(BWSCON_DW_32<< 4)
	.word	BANKCON_VAL_ROM(0,0,0,7,0,0) /* BANKCON0 */
	.word	BANKCON_VAL_ROM(0,0,0,7,0,0) /* BANKCON1 */
	.word	BANKCON_VAL_ROM(0,0,0,7,0,0) /* BANKCON2 */
	.word	BANKCON_VAL_ROM(0,0,0,7,0,0) /* BANKCON3 */
	.word	BANKCON_VAL_ROM(0,0,0,7,0,0) /* BANKCON4 */
	.word	BANKCON_VAL_ROM(0,0,0,7,0,0) /* BANKCON5 */
	.word	BANKCON_VAL_DRAM(1,1) /* BANKCON6 */
	.word	BANKCON_VAL_DRAM(1,1) /* BANKCON7 */
	.word	(1<<23)|(0<<22)|(0<<20)|(3<<18)|(2<<16)|(1113)
	.word	0x32	/* SCLK power saving mode, BANKSIZE 128M/128M */
	.word	0x30	/* MRSR6 CL=3clk */
	.word	0x30	/* MRSR7 CL=3clk */
Lmemctl_initial_data_end:
