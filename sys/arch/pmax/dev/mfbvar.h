/*	$NetBSD: mfbvar.h,v 1.3.2.1 2000/11/20 20:20:18 bouyer Exp $	*/

/*
 * Initialize a Turbochannel MFB 1280x1024x1 2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int mfb_cnattach __P((paddr_t));
