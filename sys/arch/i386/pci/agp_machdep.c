/*	$NetBSD: agp_machdep.c,v 1.1.2.4 2002/10/10 18:33:34 jdolecek Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: agp_machdep.c,v 1.1.2.4 2002/10/10 18:33:34 jdolecek Exp $");

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
