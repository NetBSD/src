/* $NetBSD: mcpcia_bus_io.c,v 1.5.70.1 2021/08/01 22:42:02 thorpej Exp $ */

/*
 * Copyright (c) 1998 by Matthew Jacob
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(1, "$NetBSD: mcpcia_bus_io.c,v 1.5.70.1 2021/08/01 22:42:02 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <alpha/pci/mcpciareg.h>
#include <alpha/pci/mcpciavar.h>

#define	CHIP		mcpcia

#define	CHIP_EX_MALLOC_SAFE(v)	(((struct mcpcia_config *)(v))->cc_mallocsafe)
#define	CHIP_IO_EXTENT(v)	(((struct mcpcia_config *)(v))->cc_io_ex)
#define	CHIP_IO_EX_STORE(v)	(((struct mcpcia_config *)(v))->cc_io_exstorage)
#define	CHIP_IO_EX_STORE_SIZE(v)					\
	(sizeof (((struct mcpcia_config *)(v))->cc_io_exstorage))

/* IO Region 1 */
#define	CHIP_IO_W1_BUS_START(v)	0x00000000UL
#define	CHIP_IO_W1_BUS_END(v)	0x0000ffffUL
#define	CHIP_IO_W1_SYS_START(v)						\
	(((struct mcpcia_config *)(v))->cc_sysbase | MCPCIA_PCI_IOSPACE)
#define	CHIP_IO_W1_SYS_END(v)						\
	(CHIP_IO_W1_SYS_START(v) + ((CHIP_IO_W1_BUS_END(v) + 1) << 5) - 1)

/* IO Region 2 */
#define	CHIP_IO_W2_BUS_START(v)	0x00010000UL
#define	CHIP_IO_W2_BUS_END(v)	0x01ffffffUL
#define	CHIP_IO_W2_SYS_START(v)						\
	(((struct mcpcia_config *)(v))->cc_sysbase + MCPCIA_PCI_IOSPACE + \
	    (0x00010000 << 5))
#define	CHIP_IO_W2_SYS_END(v)						\
	((CHIP_IO_W1_SYS_START(v) + ((CHIP_IO_W2_BUS_END(v) + 1) << 5) - 1))

#include <alpha/pci/pci_swiz_bus_io_chipdep.c>
