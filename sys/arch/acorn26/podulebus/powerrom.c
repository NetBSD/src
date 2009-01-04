/*	$NetBSD: powerrom.c,v 1.5 2009/01/04 01:02:29 bjh21 Exp $	*/

/* Test driver to see if we can talk to PowerROMs */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: powerrom.c,v 1.5 2009/01/04 01:02:29 bjh21 Exp $");

#include <sys/device.h>
#include <sys/systm.h>
#include <dev/podulebus/podulebus.h>
#include <dev/podulebus/podules.h>

struct powerrom_softc {
	device_t	sc_dev;
};

int  powerrom_match(device_t, cfdata_t, void *);
void powerrom_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(powerrom, sizeof(struct powerrom_softc),
    powerrom_match, powerrom_attach, NULL, NULL);

int
powerrom_match(device_t parent, cfdata_t cf, void *aux)
{
	struct podulebus_attach_args *pa = aux;

	return (pa->pa_product == PODULE_ALSYSTEMS_SCSI);
}

void
powerrom_attach(device_t parent, device_t self, void *aux)
{
	struct podulebus_attach_args *pa = aux;

	if (podulebus_initloader(pa) == 0)
		aprint_normal_dev(self, "card id = 0x%x",
		    podloader_callloader(pa, 0, 0));
	aprint_normal("\n");
}
