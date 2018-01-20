/*	$NetBSD: bus.c,v 1.46 2018/01/20 13:56:08 skrll Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * Platform-specific I/O support for the cobalt machines
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus.c,v 1.46 2018/01/20 13:56:08 skrll Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>

#define	CHIP			mainbus
#define	CHIP_MEM		/* defined */

#if 0
#define	CHIP_EX_MALLOC_SAFE(v)	(((struct p4032_config *)(v))->ac_mallocsafe)
#define	CHIP_EXTENT(v)		(((struct p4032_config *)(v))->ac_io_ex)
#endif

/* IO region 1 */
#define	CHIP_W1_BUS_START(v)	0x10000000UL
#define	CHIP_W1_BUS_END(v)	0x10000fffUL
#define	CHIP_W1_SYS_START(v)	0x10000000UL
#define	CHIP_W1_SYS_END(v)	0x10000fffUL

/* IO region 1 */
#define	CHIP_W2_BUS_START(v)	0x14000000UL
#define	CHIP_W2_BUS_END(v)	0x1fffffffUL
#define	CHIP_W2_SYS_START(v)	0x14000000UL
#define	CHIP_W2_SYS_END(v)	0x1fffffffUL

void mainbus_bus_mem_init(bus_space_tag_t, void *);

#include <mips/mips/bus_space_alignstride_chipdep.c>
