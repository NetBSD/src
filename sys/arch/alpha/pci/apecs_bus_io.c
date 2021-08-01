/* $NetBSD: apecs_bus_io.c,v 1.11.70.1 2021/08/01 22:42:02 thorpej Exp $ */

/*
 * Copyright (c) 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(1, "$NetBSD: apecs_bus_io.c,v 1.11.70.1 2021/08/01 22:42:02 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <alpha/pci/apecsreg.h>
#include <alpha/pci/apecsvar.h>

#define	CHIP		apecs

#define	CHIP_EX_MALLOC_SAFE(v)	(((struct apecs_config *)(v))->ac_mallocsafe)
#define	CHIP_IO_EXTENT(v)	(((struct apecs_config *)(v))->ac_io_ex)

/* IO region 1 */
#define	CHIP_IO_W1_BUS_START(v)	0x00000000UL
#define	CHIP_IO_W1_BUS_END(v)	0x0003ffffUL
#define	CHIP_IO_W1_SYS_START(v)	APECS_PCI_SIO
#define	CHIP_IO_W1_SYS_END(v)	(APECS_PCI_SIO + (0x00040000UL << 5) - 1)

/* IO region 2 */
#define	CHIP_IO_W2_BUS_START(v)						\
    ((((struct apecs_config *)(v))->ac_haxr2 & EPIC_HAXR2_EADDR) +	\
      0x00040000UL)
#define	CHIP_IO_W2_BUS_END(v)						\
    ((((struct apecs_config *)(v))->ac_haxr2 & EPIC_HAXR2_EADDR) +	\
      0x00ffffffUL)
#define	CHIP_IO_W2_SYS_START(v)	(APECS_PCI_SIO + (0x00040000UL << 5))
#define	CHIP_IO_W2_SYS_END(v)	(APECS_PCI_SIO + (0x01000000UL << 5) - 1)

#include <alpha/pci/pci_swiz_bus_io_chipdep.c>
