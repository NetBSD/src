/*	$NetBSD: firepower_bus_io.c,v 1.2 2003/07/15 02:46:30 lukem Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: firepower_bus_io.c,v 1.2 2003/07/15 02:46:30 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <powerpc/pio.h>

#include <ofppc/firepower/firepowerreg.h>
#include <ofppc/firepower/firepowervar.h>

#define	CHIP		firepower

#define	CHIP_EX_MALLOC_SAFE(v)	(((struct firepower_config *)(v))->c_mallocsafe)
#define	CHIP_IO_EXTENT(v)	(((struct firepower_config *)(v))->c_io_ex)

/* IO region 1 */
#define	CHIP_IO_W1_BUS_START(v)	(0)
#define	CHIP_IO_W1_BUS_END(v)	(0 + (FP_MAP_ISAIO_SIZE - 1))
#define	CHIP_IO_W1_SYS_START(v)	(FP_MAP_ISAIO_BASE)
#define	CHIP_IO_W1_SYS_END(v)	(FP_MAP_ISAIO_BASE + (FP_MAP_ISAIO_SIZE - 1))

/* IO region 2 */
#define	CHIP_IO_W2_BUS_START(v)	(0x01000000)
#define	CHIP_IO_W2_BUS_END(v)	(0x01000000 + (FP_MAP_PCIIO_SIZE - 1))
#define	CHIP_IO_W2_SYS_START(v)	(FP_MAP_PCIIO_BASE)
#define	CHIP_IO_W2_SYS_END(v)	(FP_MAP_PCIIO_BASE + (FP_MAP_PCIIO_SIZE - 1))

#include <ofppc/pci/pci_bus_io_chipdep.c>
