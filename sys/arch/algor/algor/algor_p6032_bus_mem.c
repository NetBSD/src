/*	$NetBSD: algor_p6032_bus_mem.c,v 1.2 2003/07/14 22:57:47 lukem Exp $	*/

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
 * Platform-specific PCI bus memory support for the Algorithmics P-6032.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: algor_p6032_bus_mem.c,v 1.2 2003/07/14 22:57:47 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/locore.h>

#include <algor/algor/algor_p6032reg.h>
#include <algor/algor/algor_p6032var.h>

#define	CHIP		algor_p6032

#define	CHIP_EX_MALLOC_SAFE(v)	(((struct p6032_config *)(v))->ac_mallocsafe)
#define	CHIP_MEM_EXTENT(v)	(((struct p6032_config *)(v))->ac_mem_ex)

/*
 * There are actually 3 PCILO memory windows, but PMON configures them
 * so that they map PCI memory space contiguously.
 */

/* MEM region 1 */
#define	CHIP_MEM_W1_BUS_START(v)	0x00000000UL
#define	CHIP_MEM_W1_BUS_END(v)		0x0bffffffUL
#define	CHIP_MEM_W1_SYS_START(v)	((u_long)BONITO_PCILO_BASE)
#define	CHIP_MEM_W1_SYS_END(v)		((u_long)BONITO_PCILO_BASE + \
					 0x0bffffffUL)

#if 0 /* XXX Should implement access to this via TLB or 64-bit KSEG */
/* MEM region 2 */
#define	CHIP_MEM_W2_BUS_START(v)	0x20000000UL
#define	CHIP_MEM_W2_BUS_END(v)		0xffffffffUL
#define	CHIP_MEM_W2_SYS_START(v)	((u_long)BONITO_PCIHI_BASE)
#define	CHIP_MEM_W2_SYS_END(v)		((u_long)BONITO_PCIHI_BASE + \
					 0xe0000000UL)
#endif

#include <algor/pci/pci_alignstride_bus_mem_chipdep.c>
