/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/map.h"
#include "sys/buf.h"
#include "sys/dkstat.h"
#include "sys/conf.h"
#include "sys/dmap.h"
#include "sys/reboot.h"
#include "sys/device.h"

#include "machine/autoconf.h"
#include "machine/vmparam.h"
#include "machine/cpu.h"
#include "machine/pte.h"
#include "machine/isr.h"

extern void mainbusattach __P((struct device *, struct device *, void *));
     
struct mainbus_softc {
    struct device mainbus_dev;
};

struct cfdriver mainbuscd = 
{ NULL, "mainbus", always_match, mainbusattach, DV_DULL,
      sizeof(struct mainbus_softc), 0};

void mainbusattach(parent, self, args)
     struct device *parent;
     struct device *self;
     void *args;
{
    struct cfdata *new_match;

    printf("\n");
    while (1) {
	new_match = config_search(NULL, self, NULL);
	if (!new_match) break;
	config_attach(self, new_match, NULL, NULL);
    }
}

int nmi_intr(arg)
     int arg;
{
    printf("nmi interrupt received\n");
    return 1;
}

void configure()
{
    int root_found;

    isr_init();

    root_found = config_rootfound("mainbus", NULL);
    if (!root_found)
	panic("configure: autoconfig failed, no device tree root found");
    isr_add(7, nmi_intr, 0);
    isr_cleanup();
}

int always_match(parent, cf, args)
     struct device *parent;
     struct cfdata *cf;
     void *args;
{
    return 1;
}
