/*	$NetBSD: sbbrz_bus_mem.c,v 1.1.2.2 2010/01/21 08:50:00 matt Exp $	*/

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
 * Platform-specific PCI bus memory support for the BCM1250/1125.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbbrz_bus_mem.c,v 1.1.2.2 2010/01/21 08:50:00 matt Exp $");

#include <sys/param.h>

#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/pci/sbbrzvar.h>

#define	CHIP			sbbrz
#define	CHIP_MEM		/* defined */
#define	CHIP_LITTLE_ENDIAN

#define	CHIP_EX_MALLOC_SAFE(v)	true
#define	CHIP_EXTENT(v)		(((struct sbbrz_softc *)(v))->sc_mem_ex)

/* MEM region 1 */
#define	CHIP_W1_BUS_START(v)	(A_PHYS_LDTPCI_IO_MATCH_BYTES_32 + 0x01000000)
#define	CHIP_W1_BUS_END(v)	(A_PHYS_LDTPCI_IO_MATCH_BYTES_32 + 0x20000000)
#define	CHIP_W1_SYS_START(v)	CHIP_W1_BUS_START(v)
#define	CHIP_W1_SYS_END(v)	CHIP_W1_BUS_END(v)

#include <mips/mips/bus_space_alignstride_chipdep.c>
