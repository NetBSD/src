/* $NetBSD: pci_machdep.h,v 1.2.186.1 2017/08/28 17:51:36 skrll Exp $ */

#ifdef _KERNEL_OPT
#include "opt_cputype.h"
#endif

/* SB1 PCI isn't finished */
#ifndef MIPS64_SB1
/* Before including <mips/pci_machdep.h> */
#define	__HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH
#endif

#include <mips/pci_machdep.h>
