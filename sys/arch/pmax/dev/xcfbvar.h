/*	$NetBSD: xcfbvar.h,v 1.2.26.1 1999/06/21 00:58:49 thorpej Exp $	*/

/*
 * Initialize a Personal Decstation baseboard framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
extern int
xcfbinit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
