/* $NetBSD: pcipnp.h,v 1.1.2.2 2006/06/01 22:35:16 kardel Exp $ */
/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
 * Based on:
 * IBM Power Personal Systems Architecture: Residual Data
 * Document Number: PPS-AR-FW0001 Rev 0.5  April 3, 1996
 */

#ifndef _PCIPNP_H_
#define _PCIPNP_H_

#define MAX_PCI_INTRS	4

typedef enum _IntrTypes {
	IntrInvalid = 0,
	Intr8259 = 1,
	IntrMPIC = 2,
	IntrRS6K = 3
} _IntrTypes;

/* PCI to system conversion map */
typedef struct _IntrMap {
	uint8_t slotnum;		/* First = 1, integrated = 0 */
	uint8_t devfunc;
	uint8_t intrctrltype;		/* interrupt type */
	uint8_t intrctrlnum;		/* 8259 = 0
					 * MPIC = 1
					 * RS6K = buid ???? */
	uint16_t intr[MAX_PCI_INTRS];	/* Index 0-3 == A-D
					 * 0xFFFF == not usable
					 * 0x8nnn == edge sensitive
					 */
} IntrMap;

typedef struct _PCIInfoPack {
	uint8_t tag;			/* large tag = 0x84 */
	uint8_t count0;
	uint8_t count1;
	  /* count = number of PCI slots * sizeof(IntrMap) + 21 */
	uint8_t type;			/* == 3 PCI Bridge */
	uint8_t configbaseaddr[8];	/* Base addr of PCI configuration
					 *   system real address */
	uint8_t configbasedata[8];	/* base addr of PCI config data
					 *   system real address */
	uint8_t busnum;			/* PCI Bus Number */
	uint8_t reserved[3];		/* reserved, padded with 0 */
	IntrMap map[1];			/* Interrupt map array for each PCI
					 * slots that are pluggable.
					 * number = (count-21)/sizeof(IntrMap)*/
} PCIInfoPack;

#endif /* _PCIPNP_H_ */
