/*	$NetBSD: apecs_lca_bus_mem.c,v 1.2 1996/04/12 06:08:06 cgd Exp $	*/

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

#if (APECS_PCI_SPARSE != LCA_PCI_SPARSE) || (APECS_PCI_DENSE != LCA_PCI_DENSE)
#error Memory addresses do not match up?
#endif

#define	CHIP		apecs_lca
#define	CHIP_D_MEM_BASE	APECS_PCI_DENSE
#define	CHIP_S_MEM_BASE	APECS_PCI_SPARSE

#include "pcs_bus_mem_common.c"
