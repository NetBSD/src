/*	$NetBSD: pmvar.h,v 1.3 1998/01/05 07:03:12 perry Exp $	*/

/* 
 * Initialize a Decstation 3100/2100 baseboard framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int pminit __P((struct fbinfo *fi, int unit, int silent));
int pmattach __P((struct fbinfo *fi, int unit, int silent));
