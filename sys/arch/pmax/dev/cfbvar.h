/*	$NetBSD: cfbvar.h,v 1.5 2000/02/03 04:19:59 nisimura Exp $	*/

/*
 * Initialize a Turbochannel CFB  dumb 2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int cfb_cnattach __P((paddr_t));
