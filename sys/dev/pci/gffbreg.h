/*	$NetBSD: gffbreg.h,v 1.1 2013/09/18 14:30:45 macallan Exp $	*/

/*
 * Copyright (c) 2007, 2012 Michael Lorenz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * A console driver for nvidia geforce graphics controllers
 * tested on macppc only so far
 * register definitions are mostly from the xf86-video-nv driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gffbreg.h,v 1.1 2013/09/18 14:30:45 macallan Exp $");

#ifndef GFFBREG_H
#define GFFBREG_H

#define GFFB_RAMDAC0	0x00680000
#define GFFB_RAMDAC1	0x00682000

#define GFFB_PCIO0	0x00601000
#define GFFB_PCIO1	0x00603000

/* VGA registers live here, one set for each head */
#define GFFB_PDIO0	0x00681000
#define GFFB_PDIO1	0x00683000

#define GFFB_CRTC0	0x00600000
#define GFFB_CRTC1	0x00602000

/* CRTC registers */
#define GFFB_DISPLAYSTART	0x800

/* VGA registers */
#define GFFB_PEL_MASK	0x3c6
#define GFFB_PEL_IR	0x3c7
#define GFFB_PEL_IW	0x3c8
#define GFFB_PEL_D	0x3c9

#endif /* GFFBREG_H */