/*	$NetBSD: cfbvar.h,v 1.2.8.1 1999/12/27 18:33:22 wrstuden Exp $	*/

/*
 * Initialize a Turbochannel CFB  dumb 2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int cfbinit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
