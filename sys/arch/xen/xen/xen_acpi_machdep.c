/*	$NetBSD: xen_acpi_machdep.c,v 1.1.14.3 2008/02/27 08:36:30 yamt Exp $	*/

#include "acpi.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xen_acpi_machdep.c,v 1.1.14.3 2008/02/27 08:36:30 yamt Exp $");

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
