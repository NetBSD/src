/*
 *	$Id: grf_ccreg.h,v 1.6 1994/05/08 05:53:06 chopps Exp $
 */
int grfcc_cnprobe __P((void));
void grfcc_iteinit __P((struct grf_softc *));
int cc_mode __P((struct grf_softc *, int, void *, int, int));
