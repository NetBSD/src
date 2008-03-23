/*	xen_acpi_machdep.c,v 1.1 2006/04/09 19:28:01 bouyer Exp	*/

#include "acpi.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xen_acpi_machdep.c,v 1.1.50.1 2008/03/23 02:04:30 matt Exp $");

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
