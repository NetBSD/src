/*	$NetBSD: sfbvar.h,v 1.4 2000/01/08 01:02:36 simonb Exp $	*/

/*
 * Initialize a Turbochannel SFB  2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int	sfbinit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
