/*	$NetBSD: cfbvar.h,v 1.2.2.1 2000/11/20 20:20:16 bouyer Exp $	*/

/*
 * Initialize a Turbochannel CFB  dumb 2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int cfb_cnattach __P((paddr_t));
