/*
 *	$Id: grf_cc.c,v 1.14 1994/06/15 19:05:57 chopps Exp $
 */

#include "grfcc.h"
#if NGRFCC > 0
/*
 * currently this is a backward compat hack that interface to 
 * view.c
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/device.h>
#include <vm/vm_param.h>
#include <machine/cpu.h>
#include <amiga/amiga/color.h>	/* DEBUG */
#include <amiga/amiga/device.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cia.h>
#include <amiga/dev/grfioctl.h>
#include <amiga/dev/grfvar.h>
#include <amiga/dev/grf_ccreg.h>
#include <amiga/dev/grfabs_reg.h>
#include <amiga/dev/viewioctl.h>


int grfccmatch __P((struct device *, struct cfdata *, void *));
int grfccprint __P((void *, char *));
void grfccattach __P((struct device *, struct device *, void *));
void grf_cc_on __P((struct grf_softc *));

struct cfdriver grfcccd = {
	NULL, "grfcc", grfccmatch, grfccattach, 
	DV_DULL, sizeof(struct grf_softc), NULL, 0 };

/* 
 * only used in console init
 */
static struct cfdata *cfdata;

/*
 * we make sure to only init things once.  this is somewhat
 * tricky regarding the console.
 */
int 
grfccmatch(pdp, cfp, auxp)
	struct device *pdp;
	struct cfdata *cfp;
	void *auxp;
{
	static int ccconunit = -1;
	char *mainbus_name = auxp;

	/*
	 * allow only one cc console
	 */
	if (amiga_realconfig == 0 && ccconunit != -1)
		return(0);
	if (matchname("grfcc", mainbus_name) == 0)
		return(0);
	if (amiga_realconfig == 0 || ccconunit != cfp->cf_unit) {
		if (grfcc_probe() == 0) 
			return(0);
		viewprobe();
		/*
		 * XXX nasty hack. opens view[0] and never closes.
		 */
		if (viewopen(0, 0))
			return(0);
		if (amiga_realconfig == 0) {
			ccconunit = cfp->cf_unit;
			cfdata = cfp;
		}
	}
	return(1);
}

/* 
 * attach to the grfbus (mainbus)
 */
void
grfccattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	static struct grf_softc congrf;
	static int coninited;
	struct grf_softc *gp;

	if (dp == NULL) 
		gp = &congrf;
	else
		gp = (struct grf_softc *)dp;

	if (dp != NULL && congrf.g_regkva != 0) {
		/*
		 * we inited earlier just copy the info
		 * take care not to copy the device struct though.
		 */
		bcopy(&congrf.g_display, &gp->g_display, 
		    (char *)&gp[1] - (char *)&gp->g_display);
	} else {
		gp->g_unit = GRF_CC_UNIT;
		gp->g_flags = GF_ALIVE;
		gp->g_mode = cc_mode;
		gp->g_conpri = grfcc_cnprobe();
		grfcc_iteinit(gp);
		grf_cc_on(gp);
	}
	if (dp != NULL)
		printf("\n");
	/*
	 * attach grf
	 */
	amiga_config_found(cfdata, &gp->g_device, gp, grfccprint);
}

int
grfccprint(auxp, pnp)
	void *auxp;
	char *pnp;
{
	if (pnp)
		printf("grf%d at %s", ((struct grf_softc *)auxp)->g_unit,
			pnp);
	return(UNCONF);
}

/*
 * Change the mode of the display.
 * Right now all we can do is grfon/grfoff.
 * Return a UNIX error number or 0 for success.
 */
/*ARGSUSED*/
int
cc_mode(gp, cmd, arg, a2, a3)
	struct grf_softc *gp;
	int cmd, a2, a3;
	void *arg;
{
	switch (cmd) {
	case GM_GRFON:
		grf_cc_on(gp);
		return(0);
	case GM_GRFOFF:
		viewioctl(0, VIOCREMOVE, NULL, 0, -1);
		return(0);
	case GM_GRFCONFIG:
	default:
		break;
	}
	return(EINVAL);
}

void
grf_cc_on(gp)
	struct grf_softc *gp;
{
	struct view_size vs;
	bmap_t bm;
	struct grfinfo *gi;

	gi = &gp->g_display;

	viewioctl(0, VIOCGBMAP, &bm, 0, -1);
  
	gp->g_data = (caddr_t) 0xDeadBeaf; /* not particularly clean.. */
  
	gi->gd_regaddr = (caddr_t) 0xdff000;	/* depricated */
	gi->gd_regsize = round_page(sizeof (custom));
	gi->gd_fbaddr  = bm.hardware_address;
	gi->gd_fbsize  = bm.depth*bm.bytes_per_row*bm.rows;

	if (viewioctl (0, VIOCGSIZE, &vs, 0, -1)) {
		/* fill in some default values... XXX */
		vs.width = 640;
		vs.height = 400;
		vs.depth = 2;
	}
	gi->gd_colors = 1 << vs.depth;
	gi->gd_planes = vs.depth;
  
	gi->gd_fbwidth = vs.width;
	gi->gd_fbheight = vs.height;
	gi->gd_fbx = 0;
	gi->gd_fby = 0;
	gi->gd_dwidth = vs.width;
	gi->gd_dheight = vs.height;
	gi->gd_dx = 0;
	gi->gd_dy = 0;

	gp->g_regkva = (void *)0xDeadBeaf;	/* builtin */
	gp->g_fbkva = NULL;		/* not needed, view internal */

	viewioctl(0, VIOCDISPLAY, NULL, 0, -1);
}    
#endif

