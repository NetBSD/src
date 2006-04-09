/*	$NetBSD: xen_acpi_machdep.c,v 1.1 2006/04/09 19:28:01 bouyer Exp $	*/

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
