/* $NetBSD: smdk2800_ram_init.S,v 1.1 2003/07/30 18:54:22 bsh Exp $	*/

/*
 * Copyright (c) 2002, 2003 Fujitsu Component Limited
 * Copyright (c) 2002, 2003 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/asm.h>
#include <arm/armreg.h>
#include <arm/s3c2xx0/s3c2800reg.h>

ENTRY(smdk2800_ram_init)
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

#define REFRESH_INIT		(REFRESH_REFEN | \
				(0x0<<20) | \
				(0x3<<16) | \
				(0x0<<12) | \
				(0x1e9))
#define DMTMCON_INIT		((0x2<<16) | \
				(0x3<<10) | \
				(0x1<<8) | \
				(0x1<<6) | \
				(0x1<<4) | \
				(0x1<<2) | \
				(0x1))

#define	SMBCON_VAL(ws,st,tacs,tcoc,toch,tacc,tcah,sdw) \
	((ws) | (st) |                                 \
	 ((tacs)<<SMBCON_TACS_SHIFT) | ((tcoc)<<SMBCON_TCOS_SHIFT) |  \
	 ((toch)<<SMBCON_TOCH_SHIFT) | ((tacc)<<SMBCON_TACC_SHIFT) |  \
	 ((tcah)<<SMBCON_TCAH_SHIFT) | (sdw))
	
Lmemctl_initial_data:
	.word	S3C2800_MEMCTL_BASE + MEMCTL_SMBCON0	    /* address */
	.word	SMBCON_VAL(0,0,0,0,0,0x0a,0,SMBCON_SDW_32BIT) /* SMBCON0 */
	.word	SMBCON_VAL(0,0,0,0,0,0x0a,0,SMBCON_SDW_16BIT) /* SMBCON1 */
	.word	SMBCON_VAL(0,0,0,0,0,0x0a,0,SMBCON_SDW_32BIT) /* SMBCON2 */
	.word	SMBCON_VAL(0,0,0,0,0,0x0a,0,SMBCON_SDW_32BIT) /* SMBCON3 */
	.word	REFRESH_INIT
	.word	DMTMCON_INIT
	.word	0x3<<MRSR_CL_SHIFT			    /* MRSR */
Lmemctl_initial_data_end:
