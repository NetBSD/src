/*	$NetBSD: agp_machdep.c,v 1.1 2001/09/10 10:06:54 fvdl Exp $	*/

#include <sys/types.h>
#include <sys/device.h>

#include <machine/bus.h>

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
