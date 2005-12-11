/*	$NetBSD: acpi_wakeup.c,v 1.3 2005/12/11 12:16:21 christos Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_wakeup.c,v 1.3 2005/12/11 12:16:21 christos Exp $");

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
