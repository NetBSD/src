/*	$NetBSD: cpu_ofbus.c,v 1.1.2.2 2002/02/28 04:11:54 nathanw Exp $	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>

#include <dev/ofw/openfirm.h>

/*
 * int cpu_ofbus_match(struct device *parent, struct cfdata *cf, void *aux)
 *
 * Probe for the main cpu. Currently all this does is return 1 to
 * indicate that the cpu was found.
 */ 
 
static int
cpu_ofbus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ofbus_attach_args *aa = aux;
	char buf[32];

	if (OF_getprop(aa->oba_phandle, "device_type", buf, sizeof(buf)) < 0)
		return (0);
	if (strcmp("cpu", buf))
		return (0);
	return(1);
}

/*
 * void cpu_ofbus_attach(struct device *parent, struct device *dev, void *aux)
 *
 * Attach the main cpu
 */
  
static void
cpu_ofbus_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	cpu_attach(self);
}

struct cfattach cpu_ofbus_ca = {
	sizeof(struct device), cpu_ofbus_match, cpu_ofbus_attach
};
