/*	$NetBSD: pmvar.h,v 1.4 1999/04/24 08:01:05 simonb Exp $	*/

/*
 * Initialize a Decstation 3100/2100 baseboard framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int pminit __P((struct fbinfo *fi, int unit, int silent));
int pmattach __P((struct fbinfo *fi, int unit, int silent));
