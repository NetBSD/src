/*	$NetBSD: pmvar.h,v 1.5.2.1 2000/11/20 20:20:19 bouyer Exp $	*/

/*
 * Initialize a DECstation 3100/2100 baseboard framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int pm_cnattach __P((void));

extern	struct pmax_fbtty pmfb;		/* used in dev/pm_ds.c */
extern	struct fbuaccess pmu;		/* used in dev/pm_ds.c */
