/*	$NetBSD: xen_acpi_machdep.c,v 1.1.20.2 2006/09/09 02:45:05 rpaulo Exp $	*/

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
