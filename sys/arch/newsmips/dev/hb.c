/*	$NetBSD: hb.c,v 1.3 1999/07/11 12:44:04 tsubai Exp $	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/adrsmap.h>

static int	hb_match __P((struct device *, struct cfdata *, void *));
static void	hb_attach __P((struct device *, struct device *, void *));
static int	hb_search __P((struct device *, struct cfdata *, void *));
static int	hb_print __P((void *, const char *));

struct cfattach hb_ca = {
	sizeof(struct device), hb_match, hb_attach
};

extern struct cfdriver hb_cd;

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
