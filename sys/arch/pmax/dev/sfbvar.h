/*	$NetBSD: sfbvar.h,v 1.3 1999/04/24 08:01:08 simonb Exp $	*/

/*
 * Initialize a Turbochannel SFB  2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
extern int
sfbinit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
