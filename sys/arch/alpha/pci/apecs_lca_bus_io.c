/*	$NetBSD: apecs_lca_bus_io.c,v 1.2 1996/04/12 06:08:04 cgd Exp $	*/

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

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <machine/bus.h>

#include <alpha/pci/apecsreg.h>
#include <alpha/pci/lcareg.h>

#if (APECS_PCI_SIO != LCA_PCI_SIO)
#error Sparse I/O addresses do not match up?
#endif

#define	CHIP		apecs_lca
#define	CHIP_IO_BASE	APECS_PCI_SIO

#include "pcs_bus_io_common.c"
