/*	$NetBSD: pmvar.h,v 1.6 2000/01/08 01:02:36 simonb Exp $	*/

/*
 * Initialize a Decstation 3100/2100 baseboard framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int	pminit __P((struct fbinfo *fi, caddr_t base, int unit, int silent));
int	pmattach __P((struct fbinfo *fi, int unit, int silent));
