/*	$NetBSD: acpi_wakeup.c,v 1.1.2.1 2004/08/03 10:31:30 skrll Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_wakeup.c,v 1.1.2.1 2004/08/03 10:31:30 skrll Exp $");

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
