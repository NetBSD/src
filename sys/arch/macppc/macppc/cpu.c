/*	$NetBSD: cpu.c,v 1.2 1999/02/15 04:31:31 hubertf Exp $	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/ofw/openfirm.h>
#include <machine/autoconf.h>

static int cpumatch __P((struct device *, struct cfdata *, void *));
static void cpuattach __P((struct device *, struct device *, void *));

struct cfattach cpu_ca = {
	sizeof(struct device), cpumatch, cpuattach
};

extern struct cfdriver cpu_cd;

extern void *mapiodev();

int
cpumatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, cpu_cd.cd_name))
		return 0;

	return 1;
}

void
cpuattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	u_int x;
	u_int *cache_reg;
	u_int node;
	int addr = -1;

	node = OF_finddevice("/hammerhead");
	OF_getprop(node, "reg", &addr, sizeof(addr));

	if (addr == -1) {
		printf("\n");
		return;
	}

#if 0
	/* enable L2 cache */
	cache_reg = mapiodev(addr, NBPG);
	if (((cache_reg[2] >> 24) & 0x0f) >= 3) {
		x = cache_reg[4];
		if ((x & 0x10) == 0)
                	x |= 0x04000000;
		else
                	x |= 0x04000020;

		cache_reg[4] = x;
		printf(": L2 cache enabled");
	}
#endif

	printf("\n");
}
