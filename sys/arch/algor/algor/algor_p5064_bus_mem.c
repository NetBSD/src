/*	$NetBSD: algor_p5064_bus_mem.c,v 1.1 2001/05/28 16:22:14 thorpej Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

/*
 * Platform-specific PCI bus memory support for the Algorithmics P-5064.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/locore.h>

#include <algor/algor/algor_p5064reg.h>
#include <algor/algor/algor_p5064var.h>

#define	CHIP		algor_p5064

#define	CHIP_EX_MALLOC_SAFE(v)	(((struct p5064_config *)(v))->ac_mallocsafe)
#define	CHIP_MEM_EXTENT(v)	(((struct p5064_config *)(v))->ac_mem_ex)

/* MEM region 1 */
#define	CHIP_MEM_W1_BUS_START(v)	0x00000000UL
#define	CHIP_MEM_W1_BUS_END(v)		0x007fffffUL
#define	CHIP_MEM_W1_SYS_START(v)	P5064_ISAMEM
#define	CHIP_MEM_W1_SYS_END(v)		(P5064_ISAMEM + CHIP_MEM_W1_BUS_END(v))

/* MEM region 2 */
#define	CHIP_MEM_W2_BUS_START(v)	0x01000000UL
#define	CHIP_MEM_W2_BUS_END(v)		0x07ffffffUL
#define	CHIP_MEM_W2_SYS_START(v)	P5064_PCIMEM
#define	CHIP_MEM_W2_SYS_END(v)		(P5064_PCIMEM + 0x06ffffffUL)

#if 0 /* XXX Should implement access to this via TLB or 64-bit KSEG */
/* MEM region 3 */
#define	CHIP_MEM_W3_BUS_START(v)	0x20000000UL
#define	CHIP_MEM_W3_BUS_END(v)		0xffffffffUL
#define	CHIP_MEM_W3_SYS_START(v)	P5064_PCIMEM_HI
#define	CHIP_MEM_W3_SYS_END(v)		(P5064_PCIMEM_HI + 0xe0000000UL)
#endif

#include <algor/pci/pci_alignstride_bus_mem_chipdep.c>
