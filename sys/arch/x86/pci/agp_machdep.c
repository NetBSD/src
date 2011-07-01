/*	$NetBSD: agp_machdep.c,v 1.2 2011/07/01 18:22:08 dyoung Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: agp_machdep.c,v 1.2 2011/07/01 18:22:08 dyoung Exp $");

#include <sys/types.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <sys/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/agpvar.h>
#include <dev/pci/agpreg.h>

#include <machine/cpufunc.h>

void
agp_flush_cache(void)
{
        wbinvd();
}
