/*	$NetBSD: acpi_wakeup.c,v 1.1 2003/05/11 21:39:41 fvdl Exp $	*/

#include <sys/types.h>
#include <sys/systm.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#define ACPI_MACHDEP_PRIVATE
#include <machine/acpi_machdep.h>

int
acpi_md_sleep(int state)
{
	printf("ACPI: sleep not implemented\n");
	return -1;
}
