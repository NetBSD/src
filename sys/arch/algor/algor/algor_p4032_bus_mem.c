/*	$NetBSD: algor_p4032_bus_mem.c,v 1.3 2003/07/14 22:57:46 lukem Exp $	*/

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
 * Platform-specific PCI bus memory support for the Algorithmics P-4032.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: algor_p4032_bus_mem.c,v 1.3 2003/07/14 22:57:46 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/locore.h>

#include <algor/algor/algor_p4032reg.h>
#include <algor/algor/algor_p4032var.h>

#include <algor/pci/vtpbcvar.h>

#define	CHIP		algor_p4032

#define	CHIP_EX_MALLOC_SAFE(v)	(((struct p4032_config *)(v))->ac_mallocsafe)
#define	CHIP_MEM_EXTENT(v)	(((struct p4032_config *)(v))->ac_mem_ex)

/* MEM region 1 */
#define	CHIP_MEM_W1_BUS_START(v)	\
	(vtpbc_configuration.vt_pci_membase + 0x00000000UL)
#define	CHIP_MEM_W1_BUS_END(v)		\
	(vtpbc_configuration.vt_pci_membase + 0x007fffffUL)
#define	CHIP_MEM_W1_SYS_START(v)	P4032_ISAMEM
#define	CHIP_MEM_W1_SYS_END(v)		(P4032_ISAMEM + CHIP_MEM_W1_BUS_END(v))

/* MEM region 2 */
#define	CHIP_MEM_W2_BUS_START(v)	\
	(vtpbc_configuration.vt_pci_membase + 0x01000000UL)
#define	CHIP_MEM_W2_BUS_END(v)		\
	(vtpbc_configuration.vt_pci_membase + 0x07ffffffUL)
#define	CHIP_MEM_W2_SYS_START(v)	P4032_PCIMEM
#define	CHIP_MEM_W2_SYS_END(v)		(P4032_PCIMEM + 0x06ffffffUL)

#if 0 /* XXX Should implement access to this via TLB or 64-bit KSEG */
/* MEM region 3 */
#define	CHIP_MEM_W3_BUS_START(v)	0x20000000UL
#define	CHIP_MEM_W3_BUS_END(v)		0xffffffffUL
#define	CHIP_MEM_W3_SYS_START(v)	P4032_PCIMEM_HI
#define	CHIP_MEM_W3_SYS_END(v)		(P4032_PCIMEM_HI + 0xe0000000UL)
#endif

#include <algor/pci/pci_alignstride_bus_mem_chipdep.c>
