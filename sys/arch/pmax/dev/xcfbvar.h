/*	$NetBSD: xcfbvar.h,v 1.3 1999/04/24 08:01:08 simonb Exp $	*/

/*
 * Initialize a Personal Decstation baseboard framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
extern int
xcfbinit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
