#include "sys/systm.h"
#include "sys/device.h"

#include "machine/autoconf.h"
#include "machine/obctl.h"
#include "machine/param.h"

extern void obctlattach __P((struct device *, struct device *, void *));
     
struct obctl_softc {
    struct device obctl_dev;
};

struct cfdriver obctlcd = 
{ NULL, "obctl", always_match, obctlattach, DV_DULL,
      sizeof(struct obctl_softc), 0};

void obctl_print(addr, size)
     caddr_t addr;
     int size;
{
    printf(" addr 0x%x size 0x%x", addr, size);
}

void obctlattach(parent, self, args)
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
