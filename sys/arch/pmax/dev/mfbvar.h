/*	$NetBSD: mfbvar.h,v 1.2.20.1 1999/06/21 00:58:41 thorpej Exp $	*/

/*
 * Initialize a Turbochannel MFB 1280x1024x1 2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
extern int
mfbinit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
