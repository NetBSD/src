/*	$NetBSD: vidmodes.c,v 1.2 2003/07/14 22:48:23 lukem Exp $	*/
/*
 * XFree86 modes are:
 * Modeline "name" dotclock hdisp hsyncstart hsyncend htotal \
 *                          vdisp vsyncstart vsyncend vtotal flags
 *
 * hswr = hsyncend - hsyncstart
 * hdsr = htotal - hsyncstart
 * hder = hdsr + hdisp
 * hcr  = htotal
 * Same for vertical.  XFree doesn't do borders.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vidmodes.c,v 1.2 2003/07/14 22:48:23 lukem Exp $");

/* RISC OS Mode 0 etc (I think) 640x256 @ 50Hz, 15.6kHz hsync */
struct arcvideo_timings timing_std640x256 = {
	16000000,
	72, 217, 265, 905, 953, 1024,
	3, 21, 39, 295, 312, 312
};

/* 
 * # 640x400 @ 70 Hz, 31.5 kHz hsync
 * Modeline "640x400"     25.175 640  664  760  800   400  409  411  450
 */
struct arcvideo_timings timing_vga640x400 = {
	25175000,
	96, 136, 136, 776, 776, 800,
	2, 41, 41, 441, 441, 450
};
	
/*
 * # 640x480 @ 60 Hz, 31.5 kHz hsync
 * Modeline "640x480"     25.175 640  664  760  800   480  491  493  525
 */
struct arcvideo_timings timing_vga640x480 = {
	25175000,
	96, 136, 136, 776, 776, 800,
	2, 34, 34, 514, 514, 525
};

/*
 * # 800x600 @ 56 Hz, 35.15 kHz hsync
 * ModeLine "800x600"     36     800  824  896 1024   600  601  603  625
 */
struct arcvideo_timings timing_svga800x600 = {
	36000000,
	72, 200, 200, 1000, 1000, 1024,
	2, 24, 24, 624, 624, 625
};
