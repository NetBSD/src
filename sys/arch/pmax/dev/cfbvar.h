/*	$NetBSD: cfbvar.h,v 1.2 1999/04/24 08:01:02 simonb Exp $	*/

/*
 * Initialize a Turbochannel CFB  dumb 2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
extern int
cfbinit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
