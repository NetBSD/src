/*	$NetBSD: mfbvar.h,v 1.5 2000/02/03 04:09:14 nisimura Exp $	*/

/*
 * Initialize a Turbochannel MFB 1280x1024x1 2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int mfb_cnattach __P((paddr_t));
