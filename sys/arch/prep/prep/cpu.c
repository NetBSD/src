/*	$NetBSD: cpu.c,v 1.1 2000/02/29 15:21:46 nonaka Exp $	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>

int cpumatch __P((struct device *, struct cfdata *, void *));
void cpuattach __P((struct device *, struct device *, void *));

struct cfattach cpu_ca = {
	sizeof(struct device), cpumatch, cpuattach
};

extern struct cfdriver cpu_cd;

int
cpumatch(parent, cfdata, aux)
	struct device *parent;
	struct cfdata *cfdata;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, cpu_cd.cd_name) != 0)
		return (0);

	return (1);
}

void
cpuattach(parent, dev, aux)
	struct device *parent;
	struct device *dev;
	void *aux;
{

	printf("\n");

	{
		/* XXX: ibm_machdep */
		u_char l2ctrl, cpuinf;

		/* system control register */
		l2ctrl = *(volatile u_char *)(PREP_BUS_SPACE_IO + 0x81c);
		/* device status register */
		cpuinf = *(volatile u_char *)(PREP_BUS_SPACE_IO + 0x80c);

		/* Enable L2 cache */
		*(volatile u_char *)(PREP_BUS_SPACE_IO + 0x81c) = l2ctrl | 0xc0;

		printf("%s: ", dev->dv_xname);
		if (((cpuinf>>1) & 1) == 0)
			printf("Upgrade CPU, ");

		printf("L2 cache ");
		if ((cpuinf & 1) == 0) {
			printf("%s ",
			    ((cpuinf>>2) & 1) ? "256KB" : "unknown size");
			printf("%s",
			    ((cpuinf>>3) & 1) ? "copy-back" : "write-through");
		} else
			printf("not present");

		printf("\n");
	}
}
