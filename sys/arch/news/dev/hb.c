
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/adrsmap.h>

static int	hb_match __P((struct device *, struct cfdata *, void *));
static void	hb_attach __P((struct device *, struct device *, void *));
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

static char *hbdevs[] = {
	"clock",
	"fb",
	"zsc",
	"kb",
	"ms",
	"le",
	"sc",
	NULL
};

static void
hb_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct confargs ca;
	char **p = hbdevs;

	printf("\n");
	bzero(&ca, sizeof(ca));

	while (*p) {
		ca.ca_name = *p;
		config_found(self, &ca, hb_print);
		p++;
	}
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
#if 0
	struct confargs *ca = args;
#endif

	/* Be quiet about empty HB locations. */
	if (name)
		return(QUIET);

#if 0
	if (ca->ca_addr != -1)
		printf(" addr 0x%x", ca->ca_addr);
#endif

	return(UNCONF);
}

