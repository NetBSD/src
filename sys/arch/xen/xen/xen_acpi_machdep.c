/*	$NetBSD: xen_acpi_machdep.c,v 1.5 2009/08/18 16:41:03 jmcneill Exp $	*/

#include "acpica.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xen_acpi_machdep.c,v 1.5 2009/08/18 16:41:03 jmcneill Exp $");

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#define ACPI_MACHDEP_PRIVATE
#include <machine/acpi_machdep.h>

int
acpi_md_sleep(int state)
{
	printf("acpi: sleep not implemented\n");
	return (-1);
}
