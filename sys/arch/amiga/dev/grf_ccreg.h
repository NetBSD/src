/*	$NetBSD: grf_ccreg.h,v 1.7 1994/10/26 02:03:11 cgd Exp $	*/

int grfcc_cnprobe __P((void));
void grfcc_iteinit __P((struct grf_softc *));
int cc_mode __P((struct grf_softc *, int, void *, int, int));
