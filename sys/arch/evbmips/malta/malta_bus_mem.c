/*	$NetBSD: malta_bus_mem.c,v 1.2 2002/03/23 14:33:35 simonb Exp $	*/

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
 * Platform-specific PCI bus memory support for the MIPS Malta.
 */

#include <sys/param.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/locore.h>

#include <evbmips/malta/maltareg.h>
#include <evbmips/malta/maltavar.h>

#define	CHIP		malta
#define	CHIP_MEM	/* defined */

#define	CHIP_EX_MALLOC_SAFE(v)	(((struct malta_config *)(v))->mc_mallocsafe)
#define	CHIP_EXTENT(v)		(((struct malta_config *)(v))->mc_mem_ex)

#if 1
/*
 * There are actually 2 PCILO memory windows, but they are configured
 * as one contiguous PCI memory space.
 */

/* MEM region 1 */
#define	CHIP_W1_BUS_START(v)	0x08000000UL
#define	CHIP_W1_BUS_END(v)	MALTA_PCIMEM1_SIZE + \
				MALTA_PCIMEM2_SIZE
#define	CHIP_W1_SYS_START(v)	((u_long)MALTA_PCIMEM1_BASE)
#define	CHIP_W1_SYS_END(v)	((u_long)MALTA_PCIMEM1_BASE + \
				 CHIP_W1_BUS_END(v))
#else

/* MEM region 1 */
#define	CHIP_W1_BUS_START(v)	0x08000000UL
#define	CHIP_W1_BUS_END(v)	MALTA_PCIMEM1_SIZE
#define	CHIP_W1_SYS_START(v)	((u_long)MALTA_PCIMEM1_BASE)
#define	CHIP_W1_SYS_END(v)	((u_long)MALTA_PCIMEM1_BASE + \
				 CHIP_W1_BUS_END(v))

/* MEM region 2 */
#define	CHIP_W2_BUS_START(v)	0x10000000UL
#define	CHIP_W2_BUS_END(v)	MALTA_PCIMEM2_SIZE
#define	CHIP_W2_SYS_START(v)	((u_long)MALTA_PCIMEM2_BASE)
#define	CHIP_W2_SYS_END(v)	((u_long)MALTA_PCIMEM2_BASE + \
				 CHIP_W2_BUS_END(v))
#endif

#include <mips/mips/bus_space_alignstride_chipdep.c>
