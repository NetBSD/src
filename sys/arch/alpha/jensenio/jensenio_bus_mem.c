/* $NetBSD: jensenio_bus_mem.c,v 1.1.2.2 2000/07/12 20:59:12 thorpej Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(1, "$NetBSD: jensenio_bus_mem.c,v 1.1.2.2 2000/07/12 20:59:12 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/eisa/eisavar.h>

#include <dev/isa/isavar.h>

#include <alpha/jensenio/jensenioreg.h>
#include <alpha/jensenio/jenseniovar.h>

#define	CHIP		jensenio

#define	CHIP_V(v)		((struct jensenio_config *)(v))

#define	CHIP_EX_MALLOC_SAFE(v)	(CHIP_V(v)->jc_mallocsafe)
#define	CHIP_S_MEM_EXTENT(v)	(CHIP_V(v)->jc_s_mem_ex)

#define	CHIP_ADDR_SHIFT		7
#define	CHIP_SIZE_SHIFT		5

/* Sparse region 1 */
#define	CHIP_S_MEM_W1_BUS_START(v)	0UL
#define	CHIP_S_MEM_W1_BUS_END(v)	0x00ffffffUL
#define	CHIP_S_MEM_W1_SYS_START(v)	JENSEN_EISA_MEM
#define	CHIP_S_MEM_W1_SYS_END(v)	(JENSEN_EISA_MEM + 0x100000000UL - 1)

/*
 * We're not really PCI, but these routines work for us just as well.
 */
#include <alpha/pci/pci_swiz_bus_mem_chipdep.c>
