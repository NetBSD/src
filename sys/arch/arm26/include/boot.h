/* $NetBSD: boot.h,v 1.2 2001/07/28 12:53:06 bjh21 Exp $ */
/*-
 * Copyright (c) 1998 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * bootconfig.h -- Stuff to do with starting the kernel
 */

#ifndef _ARM26_BOOT_H
#define _ARM26_BOOT_H

#include <sys/types.h>

/*
 * Bootloader contract 0.2:
 *  + APCS conformant entry.
 *  + SVC mode.
 *  + Interrupts (IRQ and FIQ) disabled on CPU.
 *  + CPU caches disabled.
 *  + Physical memory below 512k is unused except for screen.
 *  + First three 32k blocks above 512k are zero page, stack and msgbuf.
 *  + From there to freebase should be preserved unless you know better.
 *  + From freebase to top of RAM is for general use.
 *  + VIDC is set up as specified (not really bootloader's job).
 */

#define BOOT_MAGIC 0x942B7DFE

struct bootconfig {
	u_int32_t magic;	/* BOOT_MAGIC */
	u_int32_t version;	/* minor version of structure */
	/* Standard NetBSD boot parameters */
	u_int32_t boothowto;	/* Reboot flags (single-user etc) */
	u_int32_t bootdev;	/* Boot device */
	u_int32_t ssym;		/* Start of debugging symbols */
	u_int32_t esym;		/* End of debugging symbols */
	/* Parameters gathered by OS_ReadMemMapInfo */
	u_int32_t nbpp;		/* Machine page size in bytes */
	u_int32_t npages;	/* Number of pages */
	/* Layout of physical memory. */
	u_int32_t txtbase;	/* Kernel text base */
	u_int32_t txtsize;	/* ... and size */
	u_int32_t database;	/* Ditto for data */
	u_int32_t datasize;	/* and bss */
	u_int32_t bssbase;	/* Note that base addresses are */
	u_int32_t bsssize;	/* "physical" ie bytes above 0x02000000 */
	u_int32_t freebase;	/* Start of free space */
	/* Framebuffer state -- used to initialise raster console */
	u_int32_t xpixels;	/* Pixels across screen (XWindLimit + 1) */
	u_int32_t ypixels;	/* Pixels down screen (YWindLimit + 1) */
	u_int32_t bpp;		/* Bits per pixel (1 << Log2BPP) */
	u_int32_t screenbase;	/* Physical base of screen */
	u_int32_t screensize;	/* Size of framebuffer (TotalScreenSize) */
	u_int32_t cpixelrow;	/* first free pixel row on screen */
	/* Version 0 ends here */
};

#ifdef _KERNEL
void start __P((struct bootconfig *));

extern struct bootconfig bootconfig;
#endif
#endif
