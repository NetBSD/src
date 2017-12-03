/* $NetBSD: pci_machdep.h,v 1.2.166.1 2017/12/03 11:36:09 jdolecek Exp $ */

#ifdef _KERNEL_OPT
#include "opt_cputype.h"
#endif

/* SB1 PCI isn't finished */
#ifndef MIPS64_SB1
/* Before including <mips/pci_machdep.h> */
#define	__HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH
#endif

#include <mips/pci_machdep.h>
