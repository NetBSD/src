/*	$NetBSD: pmvar.h,v 1.3.12.1 1999/06/21 00:58:42 thorpej Exp $	*/

/*
 * Initialize a Decstation 3100/2100 baseboard framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int pminit __P((struct fbinfo *fi, int unit, int silent));
int pmattach __P((struct fbinfo *fi, int unit, int silent));
