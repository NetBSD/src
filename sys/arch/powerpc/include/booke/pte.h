/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _POWERPC_BOOKE_PTE_H_
#define _POWERPC_BOOKE_PTE_H_

#ifndef _LOCORE
typedef __uint32_t pt_entry_t;
#endif

/*
 * The PTE format is software and must be translated into the various portions
 * X W R are separted by single bits so that they can map to the MAS2 bits
 * UX/UW/UR or SX/SW/SR by a mask and a shift.
 */
#define	PTE_MAS3_MASK	(MAS3_RPN|MAS3_U2)
#define	PTE_MAS2_MASK	(MAS2_WIMGE)
#define	PTE_RPN_MASK	MAS3_RPN		/* MAS3[RPN] */
#define	PTE_RWX_MASK	(PTE_xX|PTE_xW|PTE_xR)
#define	PTE_WIRED	(MAS3_U0 << 2)		/* page is wired (PTE only) */
#define	PTE_xX		(MAS3_U0 << 1)		/* MAS2[UX] | MAS2[SX] */
#define	PTE_UNSYNCED	MAS3_U0			/* page needs isync */
#define	PTE_xW		MAS3_U1			/* MAS2[UW] | MAS2[SW] */
#define	PTE_UNMODIFIED	MAS3_U2			/* page is unmodified */
#define	PTE_xR		MAS3_U3			/* MAS2[UR] | MAS2[SR] */
#define PTE_RWX_SHIFT	5
#define	PTE_WIMGE_MASK	MAS2_WIMGE
#define	PTE_W		MAS2_W			/* Write-through */
#define	PTE_I		MAS2_I			/* cache-Inhibited */
#define	PTE_M		MAS2_M			/* Memory coherence */
#define	PTE_G		MAS2_G			/* Guarded */
#define	PTE_E		MAS2_E			/* [Little] Endian */

#endif /* !_POWERPC_BOOKE_PTE_H_ */
