/*	$NetBSD: agp_machdep.c,v 1.1.2.2 2001/09/13 01:13:50 thorpej Exp $	*/

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
