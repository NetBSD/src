/*	$NetBSD: sfbvar.h,v 1.5 2000/02/03 04:09:18 nisimura Exp $	*/

/*
 * Initialize a Turbochannel SFB  2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int sfb_cnattach __P((paddr_t));
