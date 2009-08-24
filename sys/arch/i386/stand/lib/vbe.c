/* $NetBSD: vbe.c,v 1.3 2009/08/24 02:15:46 jmcneill Exp $ */

/*-
 * Copyright (c) 2009 Jared D. McNeill <jmcneill@invisible.ca>
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

/*
 * VESA BIOS Extensions routines
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#include <machine/bootinfo.h>
#include "libi386.h"
#include "vbe.h"

extern const uint8_t rasops_cmap[];

static int vbeverbose = 1;

static struct _vbestate {
	int		available;
} vbestate;

static void
vbe_dump(struct vbeinfoblock *vbe)
{
	int mem = (int)vbe->TotalMemory * 64;

	if (!vbeverbose)
		return;

	printf(">> VESA VBE Version %d.%d %d k\n",
	    vbe->VbeVersion >> 8, vbe->VbeVersion & 0xff, mem);
}

static int
vbe_mode_is_supported(struct modeinfoblock *mi)
{
	if ((mi->ModeAttributes & 0x01) == 0)
		return 0;	/* mode not supported by hardware */
	if ((mi->ModeAttributes & 0x08) == 0)
		return 0;	/* linear fb not available */
	if ((mi->ModeAttributes & 0x10) == 0)
		return 0;	/* text mode */
	if (mi->NumberOfPlanes != 1)
		return 0;	/* planar mode not supported */
	if (mi->MemoryModel != 0x04 /* Packed pixel */ &&
	    mi->MemoryModel != 0x06 /* Direct Color */)
		return 0;	/* unsupported pixel format */
	return 1;
}

void
vbe_init(void)
{
	struct vbeinfoblock vbe;

	memset(&vbe, 0, sizeof(vbe));
	memcpy(vbe.VbeSignature, "VBE2", 4);
	if (biosvbe_info(&vbe) != 0x004f)
		return;
	if (memcmp(vbe.VbeSignature, "VESA", 4) != 0) {
		printf("VESA VBE: bad signature %c%c%c%c\n",
		    vbe.VbeSignature[0], vbe.VbeSignature[1],
		    vbe.VbeSignature[2], vbe.VbeSignature[3]);
		return;
	}

	vbe_dump(&vbe);
	vbestate.available = 1;
}

int
vbe_available(void)
{
	return vbestate.available;
}

int
vbe_set_palette(const uint8_t *cmap, int slot)
{
	struct paletteentry pe;
	int ret;

	if (!vbestate.available) {
		printf("VESA BIOS extensions not available\n");
		return 1;
	}

	pe.Blue = cmap[2] >> 2;
	pe.Green = cmap[1] >> 2;
	pe.Red = cmap[0] >> 2;
	pe.Alignment = 0;

	ret = biosvbe_palette_data(0x0600, slot, &pe);

	return ret == 0x004f ? 0 : 1;
}

int
vbe_set_mode(int modenum)
{
	struct modeinfoblock mi;
	struct btinfo_framebuffer fb;
	int ret, i;

	if (!vbestate.available) {
		printf("VESA BIOS extensions not available\n");
		return 1;
	}

	ret = biosvbe_get_mode_info(modenum, &mi);
	if (ret != 0x004f) {
		printf("VESA VBE mode 0x%x is invalid.\n", modenum);
		return 1;
	}

	if (!vbe_mode_is_supported(&mi)) {
		printf("VESA VBE mode 0x%x is not supported.\n", modenum);
		return 1;
	}

	ret = biosvbe_set_mode(modenum);
	if (ret != 0x004f) {
		printf("VESA VBE mode 0x%x could not be set.\n", modenum);
		return 1;
	}

	/* Setup palette for packed pixel mode */
	if (mi.MemoryModel == 0x04)
		for (i = 0; i < 256; i++)
			vbe_set_palette(&rasops_cmap[i * 3], i);

	fb.physaddr = (uint64_t)mi.PhysBasePtr & 0xffffffff;
	fb.width = mi.XResolution;
	fb.height = mi.YResolution;
	fb.stride = mi.BytesPerScanLine;
	fb.depth = mi.BitsPerPixel;
	fb.flags = 0;
	fb.rnum = mi.RedMaskSize;
	fb.rpos = mi.RedFieldPosition;
	fb.gnum = mi.GreenMaskSize;
	fb.gpos = mi.GreenFieldPosition;
	fb.bnum = mi.BlueMaskSize;
	fb.bpos = mi.BlueFieldPosition;
	fb.vbemode = modenum;

	framebuffer_configure(&fb);

	return 0;
}

static void *
vbe_farptr(uint32_t farptr)
{
	return VBEPHYPTR((((farptr & 0xffff0000) >> 12) + (farptr & 0xffff)));
}

static int
vbe_parse_mode_str(char *str, int *x, int *y, int *bpp)
{
	char *p;

	p = str;
	*x = strtoul(p, NULL, 0);
	if (*x == 0)
		return 0;
	p = strchr(p, 'x');
	if (!p)
		return 0;
	++p;
	*y = strtoul(p, NULL, 0);
	if (*y == 0)
		return 0;
	p = strchr(p, 'x');
	if (!p)
		*bpp = 8;
	else {
		++p;
		*bpp = strtoul(p, NULL, 0);
		if (*bpp == 0)
			return 0;
	}

	return 1;
}

static int
vbe_find_mode(char *str)
{
	struct vbeinfoblock vbe;
	struct modeinfoblock mi;
	uint32_t farptr;
	uint16_t mode;
	int x, y, bpp;
	int safety = 0;

	if (!vbe_parse_mode_str(str, &x, &y, &bpp))
		return 0;

	memset(&vbe, 0, sizeof(vbe));
	memcpy(vbe.VbeSignature, "VBE2", 4);
	if (biosvbe_info(&vbe) != 0x004f)
		return 0;
	if (memcmp(vbe.VbeSignature, "VESA", 4) != 0)
		return 0;
	farptr = vbe.VideoModePtr;
	if (farptr == 0)
		return 0;

	while ((mode = *(uint16_t *)vbe_farptr(farptr)) != 0xffff) {
		safety++;
		farptr += 2;
		if (safety == 100)
			return 0;
		if (biosvbe_get_mode_info(mode, &mi) != 0x004f)
			continue;
		/* we only care about linear modes here */
		if (vbe_mode_is_supported(&mi) == 0)
			continue;
		safety = 0;
		if (mi.XResolution == x &&
		    mi.YResolution == y &&
		    mi.BitsPerPixel == bpp)
			return mode;
	}

	printf("VESA VBE BIOS does not support %s\n", str);
	return 0;
}

static void
vbe_dump_mode(int modenum, struct modeinfoblock *mi)
{
	printf("0x%x=%dx%dx%d", modenum,
	    mi->XResolution, mi->YResolution, mi->BitsPerPixel);
}

void
vbe_modelist(void)
{
	struct vbeinfoblock vbe;
	struct modeinfoblock mi;
	uint32_t farptr;
	uint16_t mode;
	int nmodes = 0, safety = 0;

	if (!vbestate.available) {
		printf("VESA BIOS extensions not available\n");
		return;
	}

	printf("Modes: ");
	memset(&vbe, 0, sizeof(vbe));
	memcpy(vbe.VbeSignature, "VBE2", 4);
	if (biosvbe_info(&vbe) != 0x004f)
		goto done;
	if (memcmp(vbe.VbeSignature, "VESA", 4) != 0)
		goto done;
	farptr = vbe.VideoModePtr;
	if (farptr == 0)
		goto done;

	while ((mode = *(uint16_t *)vbe_farptr(farptr)) != 0xffff) {
		safety++;
		farptr += 2;
		if (safety == 100) {
			printf("[garbage] ");
			break;
		}
		if (biosvbe_get_mode_info(mode, &mi) != 0x004f)
			continue;
		/* we only care about linear modes here */
		if (vbe_mode_is_supported(&mi) == 0)
			continue;
		safety = 0;
		if (nmodes % 4 == 0)
			printf("\n");
		else
			printf("  ");
		vbe_dump_mode(mode, &mi);
		nmodes++;
	}

done:
	if (nmodes == 0)
		printf("none found");
	printf("\n");
}

void
command_vesa(char *cmd)
{
	char arg[20];
	int modenum;

	if (!vbe_available()) {
		printf("VESA VBE not available\n");
		return;
	}

	strlcpy(arg, cmd, sizeof(arg));

	if (strcmp(arg, "list") == 0) {
		vbe_modelist();
		return;
	}

	if (strcmp(arg, "disabled") == 0 || strcmp(arg, "off") == 0) {
		framebuffer_configure(NULL);
		biosvideomode();
		return;
	}

	if (strcmp(arg, "enabled") == 0 || strcmp(arg, "on") == 0)
		modenum = VBE_DEFAULT_MODE;
	else if (strncmp(arg, "0x", 2) == 0)
		modenum = strtoul(arg, NULL, 0);
	else if (strchr(arg, 'x') != NULL) {
		modenum = vbe_find_mode(arg);
		if (modenum == 0)
			return;
	} else
		modenum = 0;

	if (modenum >= 0x100) {
		vbe_set_mode(modenum);
		return;
	}

	printf("invalid flag, must be 'enabled', 'disabled', "
	    "a display mode, or a valid VESA VBE mode number.\n");
}
