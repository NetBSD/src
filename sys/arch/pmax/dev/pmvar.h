/*	$NetBSD: pmvar.h,v 1.8 2000/02/03 04:09:15 nisimura Exp $	*/

/*
 * Initialize a DECstation 3100/2100 baseboard framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int pm_cnattach __P((void));

extern	struct pmax_fbtty pmfb;		/* used in dev/pm_ds.c */
extern	struct fbuaccess pmu;		/* used in dev/pm_ds.c */
