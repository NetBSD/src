#include "sys/systm.h"
#include "sys/device.h"

#include "machine/autoconf.h"
#include "machine/obmem.h"
#include "machine/param.h"

extern void obmemattach __P((struct device *, struct device *, void *));
     
struct obmem_softc {
    struct device obmem_dev;
};

struct cfdriver obmemcd = 
{ NULL, "obmem", always_match, obmemattach, DV_DULL,
      sizeof(struct obmem_softc), 0};

void obmem_print(addr, size, level)
     caddr_t addr;
     int size;
     int level;
{
    printf(" addr 0x%x size 0x%x", addr, size);
    if (level <0)
	printf(" level %d\n", level);
}

void obmemattach(parent, self, args)
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
