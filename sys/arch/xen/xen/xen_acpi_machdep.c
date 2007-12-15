/*	$NetBSD: xen_acpi_machdep.c,v 1.2 2007/12/15 19:24:17 jmcneill Exp $	*/

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

void
acpi_md_sleep_init(void)
{
	/* Not supported on xen */
	return;
}
