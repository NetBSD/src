/* $NetBSD: sunxi_mc_mpstart.S,v 1.2 2019/02/06 13:15:59 skrll Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <arm/asm.h>
#include <arm/armreg.h>
#include "assym.h"

#define	CCI_SNOOP_CTRL		0x000
#define	 CCI_SNOOP_CTRL_DVM	__BIT(1)
#define	 CCI_SNOOP_CTRL_SNOOP	__BIT(0)

	R_TMP1		.req r8
	R_VTOPDIFF	.req r10

	.global _C_LABEL(sunxi_mc_mpstart)
_C_LABEL(sunxi_mc_mpstart):

	adr     R_TMP1, sunxi_mc_mpstart
	ldr     R_VTOPDIFF, =sunxi_mc_mpstart
	sub     R_VTOPDIFF, R_VTOPDIFF, R_TMP1

	mrc	p15, 0, r4, c0, c0, 5		// MPIDR get
	and	r4, #(MPIDR_AFF2|MPIDR_AFF1|MPIDR_AFF0)

	mov	r0, #0
	ldr	r1, =cpu_mpidr
	sub	r1, R_VTOPDIFF
1:

	ldr	r2, [r1, r0, lsl #2]		// r2 = cpu_mpidr[r0]
	cmp	r2, r4
	beq	2f				// found our mpidr

	add	r0, #1
	cmp	r0, #MAXCPUS
	bne	1b

	// Not found our mpidr in the list - use Aff0 for cpuindex
	and	r0, r4, #7
2:

	// Find our CCI-400 interface's base address by cpuindex
	ldr	r1, =sunxi_mc_cci_port
	sub	r1, R_VTOPDIFF
	ldr	r2, [r1, r0, lsl #2]		// r2 = sunxi_mc_cci_port[r0]

	cmp	r2, #0
	beq	3f

	// Enable snooping and DVM broadcast on our CCI-400 interface
	mov	r3, #(CCI_SNOOP_CTRL_DVM|CCI_SNOOP_CTRL_SNOOP)
	str	r3, [r2, #CCI_SNOOP_CTRL]

3:
	b cpu_mpstart
