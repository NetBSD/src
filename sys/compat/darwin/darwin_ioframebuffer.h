/*	$NetBSD: darwin_ioframebuffer.h,v 1.7 2003/05/15 23:35:37 manu Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_DARWIN_IOFRAMEBUFFER_H_
#define	_DARWIN_IOFRAMEBUFFER_H_

extern struct mach_iokit_devclass darwin_ioframebuffer_devclass;

#define DARWIN_IOFRAMEBUFFER_CURSOR_MEMORY	100
#define DARWIN_IOFRAMEBUFFER_VRAM_MEMORY	110
#define DARWIN_IOFRAMEBUFFER_SYSTEM_APERTURE	0

struct darwin_ioframebuffer_shmem {
	darwin_ev_lock_data_t dis_sem;
	char dis_cursshow;
	char dis_sursobscured;
	char dis_shieldflag;
	char dis_dhielded;
	darwin_iogbounds dis_saverect;
	darwin_iogbounds dis_shieldrect;
	darwin_iogpoint dis_location;
	darwin_iogbounds dis_cursrect;
	darwin_iogbounds dis_oldcursrect;
	darwin_iogbounds dis_screen;
	int version;
	darwin_absolutetime dis_vbltime;
	darwin_absolutetime dis_vbldelta;
	unsigned int dis_reserved1[30];
	unsigned char dis_hwcurscapable;
	unsigned char dis_hwcursactive;
	unsigned char dis_hwcursshields;
	unsigned char dis_reserved2;
	darwin_iogsize dis_cursorsize[4];
	darwin_iogpoint dis_hotspot[4];
	unsigned char dis_curs[0];
};

/* I/O selectors for io_connect_method_{scalar|struct}i_{scalar|struct}o */
#define DARWIN_IOFBCREATESHAREDCURSOR 0
#define DARWIN_IOFBGETPIXELINFORMATION 1
#define DARWIN_IOFBGETCURRENTDISPLAYMODE 2
#define DARWIN_IOFBGETVRAMMAPOFFSET 8
#define DARWIN_IOFBGETATTRIBUTE 18

#define DARWIN_IOMAXPIXELBITS 64

typedef int32_t darwin_ioindex;
typedef int32_t darwin_iodisplaymodeid;
typedef uint32_t darwin_iobytecount;
typedef darwin_ioindex darwin_iopixelaperture;
typedef char darwin_iopixelencoding[DARWIN_IOMAXPIXELBITS];

/* pixeltype */
#define DARWIN_IOFB_CLUTPIXELS 0;
#define DARWIN_IOFB_FIXEDCLUTPIXELS 1;
#define DARWIN_IOFB_RGBDIRECTPIXELS 2;
#define DARWIN_IOFB_MONODIRECTPIXELS 3;
#define DARWIN_IOFB_MONOINVERSEDIRECTPIXELS 4;

typedef struct {
	darwin_iobytecount bytesperrow;
	darwin_iobytecount bytesperplane;
	uint32_t bitsperpixel;
	uint32_t pixeltype;
	uint32_t componentcount;
	uint32_t bitspercomponent;
	uint32_t componentmasks[16];
	darwin_iopixelencoding pixelformat;
	uint32_t flags;
	uint32_t activewidth;
	uint32_t activeheight;
	uint32_t reserved[2];
} darwin_iopixelinformation;

int 
darwin_ioframebuffer_connect_method_scalari_scalaro(struct mach_trap_args *);
int 
darwin_ioframebuffer_connect_method_scalari_structo(struct mach_trap_args *);
int
darwin_ioframebuffer_connect_method_structi_structo(struct mach_trap_args *);
int darwin_ioframebuffer_connect_map_memory(struct mach_trap_args *);

#endif /* _DARWIN_IOFRAMEBUFFER_H_ */
