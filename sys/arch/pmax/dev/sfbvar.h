/*	$NetBSD: sfbvar.h,v 1.3.2.1 2000/11/20 20:20:21 bouyer Exp $	*/

/*
 * Initialize a Turbochannel SFB  2-d framebuffer,
 * so it can be used as a bitmapped glass-tty console device.
 */
int sfb_cnattach __P((paddr_t));
