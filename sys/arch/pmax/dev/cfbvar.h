/*	$NetBSD: cfbvar.h,v 1.4 2000/01/08 01:02:35 simonb Exp $	*/

/*
 * Initialize a Turbochannel CFB  dumb 2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int	cfbinit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
