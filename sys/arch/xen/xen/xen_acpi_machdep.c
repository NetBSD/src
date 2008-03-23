/*	$NetBSD: xen_acpi_machdep.c,v 1.1.24.1 2008/03/23 10:26:05 jdc Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xen_acpi_machdep.c,v 1.1.24.1 2008/03/23 10:26:05 jdc Exp $");

#include "acpi.h"

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
