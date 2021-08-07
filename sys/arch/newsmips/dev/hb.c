/*	$NetBSD: hb.c,v 1.23 2021/08/07 16:19:01 thorpej Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hb.c,v 1.23 2021/08/07 16:19:01 thorpej Exp $");

#define __INTR_PRIVATE
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/intr.h>

#include <machine/autoconf.h>

#include <newsmips/dev/hbvar.h>

#include "ioconf.h"

static int	hb_match(device_t, cfdata_t, void *);
static void	hb_attach(device_t, device_t, void *);
static int	hb_search(device_t, cfdata_t, const int *, void *);
static int	hb_print(void *, const char *);

CFATTACH_DECL_NEW(hb, 0,
    hb_match, hb_attach, NULL, NULL);

#define NLEVEL	4
static struct newsmips_intr hbintr_tab[NLEVEL];

static int
hb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, hb_cd.cd_name) != 0)
		return 0;

	return 1;
}

static void
hb_attach(device_t parent, device_t self, void *aux)
{
	struct hb_attach_args ha;
	struct newsmips_intr *ip;
	int i;

	aprint_normal("\n");

	memset(&ha, 0, sizeof(ha));
	for (i = 0; i < NLEVEL; i++) {
		ip = &hbintr_tab[i];
		LIST_INIT(&ip->intr_q);
	}

	config_search(self, &ha,
	    CFARGS(.search = hb_search));
}

static int
hb_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct hb_attach_args *ha = aux;

	ha->ha_name = cf->cf_name;
	ha->ha_addr = cf->cf_addr;
	ha->ha_level = cf->cf_level;

	if (config_probe(parent, cf, ha))
		config_attach(parent, cf, ha, hb_print, CFARGS_NONE);

	return 0;
}

/*
 * Print out the confargs.  The (parent) name is non-NULL
 * when there was no match found by config_found().
 */
static int
hb_print(void *args, const char *name)
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
hb_intr_establish(int level, int mask, int priority, int (*func)(void *),
    void *arg)
{
	struct newsmips_intr *ip;
	struct newsmips_intrhand *ih, *curih;

	ip = &hbintr_tab[level];

	ih = kmem_alloc(sizeof(*ih), KM_SLEEP);
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
hb_intr_dispatch(int level, int stat)
{
	struct newsmips_intr *ip;
	struct newsmips_intrhand *ih;

	ip = &hbintr_tab[level];

	LIST_FOREACH(ih, &ip->intr_q, ih_q) {
		if (ih->ih_mask & stat)
			(*ih->ih_func)(ih->ih_arg);
	}
}
