/*	$NetBSD: mfbvar.h,v 1.3 1999/04/24 08:01:05 simonb Exp $	*/

/*
 * Initialize a Turbochannel MFB 1280x1024x1 2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
extern int
mfbinit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
