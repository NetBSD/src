/*	$NetBSD: cfbvar.h,v 1.1.26.1 1999/06/21 00:58:36 thorpej Exp $	*/

/*
 * Initialize a Turbochannel CFB  dumb 2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
extern int
cfbinit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
