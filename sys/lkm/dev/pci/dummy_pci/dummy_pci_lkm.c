/* $NetBSD: dummy_pci_lkm.c,v 1.1.2.3 2004/09/18 14:54:09 skrll Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/lkm.h>

CFDRIVER_DECL(mist, DV_DULL, NULL);
extern struct cfattach mist_ca;

static int pciloc[] = { -1, -1 }; /* device, function */
static struct cfparent pciparent = {
	"pci", "pci", DVUNIT_ANY
};
static struct cfdata mist_cfdata[] = {
	{"mist", "mist", 0, FSTATE_STAR, pciloc, 0, &pciparent, 0},
	{ 0 }
};

static struct cfdriver *mist_cfdrivers[] = {
	&mist_cd,
	NULL
};
static struct cfattach *mist_cfattachs[] = {
	&mist_ca,
	NULL
};
static const struct cfattachlkminit mist_cfattachinit[] = {
	{ "mist", mist_cfattachs },
	{ NULL }
};

int dummy_pci_lkmentry(struct lkm_table *, int, int);
MOD_DRV("dummy_pci", mist_cfdrivers, mist_cfattachinit,
	mist_cfdata);

int
dummy_pci_lkmentry(struct lkm_table *lkmtp, int cmd, int ver)
{

	DISPATCH(lkmtp, cmd, ver, lkm_nofunc, lkm_nofunc, lkm_nofunc);
}
