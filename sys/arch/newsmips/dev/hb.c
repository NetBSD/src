/*	$NetBSD: hb.c,v 1.12 2003/05/10 09:46:25 tsutsui Exp $	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/intr.h>

#include <newsmips/dev/hbvar.h>

static int	hb_match __P((struct device *, struct cfdata *, void *));
static void	hb_attach __P((struct device *, struct device *, void *));
static int	hb_search __P((struct device *, struct cfdata *, void *));
static int	hb_print __P((void *, const char *));

CFATTACH_DECL(hb, sizeof(struct device),
    hb_match, hb_attach, NULL, NULL);

extern struct cfdriver hb_cd;

#define NLEVEL	4
static struct newsmips_intr hbintr_tab[NLEVEL];

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
	struct hb_attach_args ha;
	struct newsmips_intr *ip;
	int i;

	printf("\n");

	memset(&ha, 0, sizeof(ha));
	for (i = 0; i < NLEVEL; i++) {
		ip = &hbintr_tab[i];
		LIST_INIT(&ip->intr_q);
	}

	config_search(hb_search, self, &ha);
}

static int
hb_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct hb_attach_args *ha = aux;

	ha->ha_name = cf->cf_name;
	ha->ha_addr = cf->cf_addr;
	ha->ha_level = cf->cf_level;

	if (config_match(parent, cf, ha) > 0)
		config_attach(parent, cf, ha, hb_print);

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
	struct hb_attach_args *ha = args;

	/* Be quiet about empty HB locations. */
	if (name)
		return QUIET;

	if (ha->ha_addr != -1)
		aprint_normal(" addr 0x%x", ha->ha_addr);

	return UNCONF;
}

void *
hb_intr_establish(level, mask, priority, func, arg)
	int level, mask, priority;
	int (*func) __P((void *));
	void *arg;
{
	struct newsmips_intr *ip;
	struct newsmips_intrhand *ih, *curih;

	ip = &hbintr_tab[level];

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		panic("hb_intr_establish: malloc failed");

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_level = level;
	ih->ih_mask = mask;
	ih->ih_priority = priority;

	if (LIST_EMPTY(&ip->intr_q)) {
		LIST_INSERT_HEAD(&ip->intr_q, ih, ih_q);
		goto done;
	}

	for (curih = LIST_FIRST(&ip->intr_q);
	    LIST_NEXT(curih, ih_q) != NULL;
	    curih = LIST_NEXT(curih, ih_q)) {
		if (ih->ih_priority > curih->ih_priority) {
			LIST_INSERT_BEFORE(curih, ih, ih_q);
			goto done;
		}
	}

	LIST_INSERT_AFTER(curih, ih, ih_q);

 done:
	return ih;
}

void
hb_intr_dispatch(level, stat)
	int level;
	int stat;
{
	struct newsmips_intr *ip;
	struct newsmips_intrhand *ih;

	ip = &hbintr_tab[level];

	LIST_FOREACH(ih, &ip->intr_q, ih_q) {
		if (ih->ih_mask & stat)
			(*ih->ih_func)(ih->ih_arg);
	}
}
