/*	$NetBSD: mfbvar.h,v 1.4 2000/01/08 01:02:35 simonb Exp $	*/

/*
 * Initialize a Turbochannel MFB 1280x1024x1 2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int	mfbinit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
