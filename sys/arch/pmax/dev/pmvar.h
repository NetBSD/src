/*	$NetBSD: pmvar.h,v 1.7 2000/01/09 03:55:41 simonb Exp $	*/

/*
 * Initialize a Decstation 3100/2100 baseboard framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int	pminit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
int	pmattach __P((struct fbinfo *fi, int unit, int silent));

extern	struct pmax_fbtty pmfb;		/* used in dev/pm_ds.c */
extern	struct fbuaccess pmu;		/* used in dev/pm_ds.c */
