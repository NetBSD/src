/*	$NetBSD: sfbvar.h,v 1.2.20.1 1999/06/21 00:58:47 thorpej Exp $	*/

/*
 * Initialize a Turbochannel SFB  2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
extern int
sfbinit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
