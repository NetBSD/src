/*	$NetBSD: agp_machdep.c,v 1.2.2.2 2002/04/27 20:24:49 sommerfeld Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: agp_machdep.c,v 1.2.2.2 2002/04/27 20:24:49 sommerfeld Exp $");

#include <sys/types.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/agpvar.h>
#include <dev/pci/agpreg.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>

void
agp_flush_cache(void)
{
        wbinvd();
}
