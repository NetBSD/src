/* $NetBSD: dwlpx_bus_io.c,v 1.3 1997/04/07 05:19:39 cgd Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <machine/bus.h>

#include <alpha/pci/dwlpxreg.h>
#include <alpha/pci/dwlpxvar.h>

#define	CHIP		dwlpx

#define	CHIP_EX_MALLOC_SAFE(v)	(1)
#define	CHIP_IO_EXTENT(v)	(((struct dwlpx_config *)(v))->cc_io_ex)
#define	CHIP_IO_EX_STORE(v)	(((struct dwlpx_config *)(v))->cc_io_exstorage)
#define	CHIP_IO_EX_STORE_SIZE(v)					\
	(sizeof (((struct dwlpx_config *)(v))->cc_io_exstorage))

/* IO Region 1 */
#define	CHIP_IO_W1_BUS_START(v)	0x00000000UL
#define	CHIP_IO_W1_BUS_END(v)	0x00ffffffUL
#define	CHIP_IO_W1_SYS_START(v)						\
	(((struct dwlpx_config *)(v))->cc_sysbase | DWLPX_PCI_IOSPACE)
#define	CHIP_IO_W1_SYS_END(v)						\
	(CHIP_IO_W1_SYS_START(v) + ((CHIP_IO_W1_BUS_END(v) + 1) << 5) - 1)

/* IO Region 2 */
#define	CHIP_IO_W2_BUS_START(v)						\
	(((0x0 << 27) & 0xf8000000) + 0x01000000UL)
#define	CHIP_IO_W2_BUS_END(v)						\
	(CHIP_IO_W2_BUS_START(v) + 0x07ffffffUL)
#define	CHIP_IO_W2_SYS_START(v)						\
	(((struct dwlpx_config *)(v))->cc_sysbase + DWLPX_PCI_IOSPACE +	\
	    (0x01000000UL << 5))
#define	CHIP_IO_W2_SYS_END(v)						\
	(((struct dwlpx_config *)(v))->cc_sysbase + DWLPX_PCI_IOSPACE +	\
	    (0x08000000UL << 5) - 1)

#include "pcs_bus_io_common.c"
