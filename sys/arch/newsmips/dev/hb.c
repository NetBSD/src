/*	$NetBSD: hb.c,v 1.3.2.2 2000/12/08 09:28:51 bouyer Exp $	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>

static int	hb_match __P((struct device *, struct cfdata *, void *));
static void	hb_attach __P((struct device *, struct device *, void *));
static int	hb_search __P((struct device *, struct cfdata *, void *));
static int	hb_print __P((void *, const char *));
void		hb_intr_dispatch __P((int));	/* XXX */

struct cfattach hb_ca = {
	sizeof(struct device), hb_match, hb_attach
};

extern struct cfdriver hb_cd;

struct intrhand {
	int (*func) __P((void *));
	void *arg;
};

#define NHBINTR	4
struct intrhand hb_intrhand[6][NHBINTR];

static int
hb_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, hb_cd.cd_name) != 0)
		return 0;

	return 1;
}

static void
hb_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct confargs *ca = aux;

	printf("\n");
	config_search(hb_search, self, ca);
}

static int
hb_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	ca->ca_addr = cf->cf_addr;
	ca->ca_name = cf->cf_driver->cd_name;

	if ((*cf->cf_attach->ca_match)(parent, cf, ca) > 0)
		config_attach(parent, cf, ca, hb_print);

	return 0;
}

/*
 * Print out the confargs.  The (parent) name is non-NULL
 * when there was no match found by config_found().
 */
static int
hb_print(args, name)
	void *args;
	const char *name;
{
	struct confargs *ca = args;

	/* Be quiet about empty HB locations. */
	if (name)
		return QUIET;

	if (ca->ca_addr != -1)
		printf(" addr 0x%x", ca->ca_addr);

	return UNCONF;
}

void *
hb_intr_establish(irq, level, func, arg)
	int irq, level;
	int (*func) __P((void *));
	void *arg;
{
	struct intrhand *ih = hb_intrhand[irq];
	int i;

	for (i = NHBINTR; i > 0; i--) {
		if (ih->func == NULL)
			goto found;
		ih++;
	}
	panic("hb_intr_establish: no room");

found:
	ih->func = func;
	ih->arg = arg;

#ifdef HB_DEBUG
	for (irq = 0; irq <= 2; irq++) {
		for (i = 0; i < NHBINTR; i++) {
			printf("%p(%p) ",
			       hb_intrhand[irq][i].func,
			       hb_intrhand[irq][i].arg);
		}
		printf("\n");
	}
#endif

	return ih;
}

void
hb_intr_dispatch(irq)
	int irq;
{
	struct intrhand *ih;
	int i;

	ih = hb_intrhand[irq];
	for (i = NHBINTR; i > 0; i--) {
		if (ih->func)
			(*ih->func)(ih->arg);
		ih++;
	}
}
