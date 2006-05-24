/* $NetBSD: vreset.c,v 1.3.6.2 2006/05/24 10:57:10 yamt Exp $ */
/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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

#ifdef VGA_RESET
#include <lib/libsa/stand.h>
#include "boot.h"
#include "iso_font.h"

#define VGA_SR_PORT	0x3c4
#define VGA_CR_PORT	0x3d4
#define VGA_CR_DATA	0x3d5
#define VGA_GR_PORT	0x3ce
#define VGA_GR_DATA	0x3cf
#define SRREGS	4
#define CRREGS	24
#define GRREGS	9
#define LINES 25
#define COLS 80
#define PCI_VENDOR_S3		0x5333
#define PCI_VENDOR_CIRRUS	0x1013
#define PCI_VENDOR_DIAMOND	0x100E
#define PCI_VENDOR_MATROX	0x102B
#define PCI_VENDOR_PARADISE	0x101C

static void write_attr(u_int8_t, u_int8_t, u_int8_t);
static void set_text_regs(void);
static void set_text_clut(int);
static void load_font(u_int8_t *);
static void unlock_S3(void);
static void clear_video_memory(void);

extern char *videomem;

typedef struct vga_reg {
	u_int8_t idx;
	u_int8_t val;
} vga_reg_t;

static vga_reg_t SR_regs[SRREGS] = {
	/* idx  val */
	{ 0x1,	0x0 },	/* 01: clocking mode */
	{ 0x2,	0x3 },	/* 02: map mask */
	{ 0x3,	0x0 },	/* 03: character map select */
	{ 0x4,	0x2 }	/* 04: memory mode */
};

static vga_reg_t CR_regs[CRREGS] = {
	/* idx  val */
	{ 0x0,	0x61 }, /* 00: horizontal total */
	{ 0x1,	0x4f }, /* 01: horizontal display-enable end */
	{ 0x2,	0x50 }, /* 02: start horizontal blanking */
	{ 0x3,	0x82 }, /* 03: display skew control / end horizontal blanking */	{ 0x4,	0x55 }, /* 04: start horizontal retrace pulse */
	{ 0x5,	0x81 }, /* 05: horizontal retrace delay / end horiz. retrace */
	{ 0x6,	0xf0 }, /* 06: vertical total */
	{ 0x7,	0x1f }, /* 07: overflow register */
	{ 0x8,	0x00 }, /* 08: preset row scan */
	{ 0x9,	0x4f }, /* 09: overflow / maximum scan line */
	{ 0xa,	0x0d }, /* 0A: cursor off / cursor start */
	{ 0xb,	0x0e }, /* 0B: cursor skew / cursor end */
	{ 0xc,	0x00 }, /* 0C: start regenerative buffer address high */
	{ 0xd,	0x00 }, /* 0D: start regenerative buffer address low */
	{ 0xe,	0x00 }, /* 0E: cursor location high */
	{ 0xf,	0x00 }, /* 0F: cursor location low */
	{ 0x10, 0x9a }, /* 10: vertical retrace start */
	{ 0x11, 0x8c }, /* 11: vertical interrupt / vertical retrace end */
	{ 0x12, 0x8f }, /* 12: vertical display enable end */
	{ 0x13, 0x28 }, /* 13: logical line width */
	{ 0x14, 0x1f }, /* 14: underline location */
	{ 0x15, 0x97 }, /* 15: start vertical blanking */
	{ 0x16, 0x00 }, /* 16: end vertical blanking */
	{ 0x17, 0xa3 }, /* 17: CRT mode control */
};

static vga_reg_t GR_regs[GRREGS] = {
	/* idx  val */
	{ 0x0,	0x00 }, /* 00: set/reset map */
	{ 0x1,	0x00 }, /* 01: enable set/reset */
	{ 0x2,	0x00 }, /* 02: color compare */
	{ 0x3,	0x00 }, /* 03: data rotate */
	{ 0x4,	0x00 }, /* 04: read map select */
	{ 0x5,	0x10 }, /* 05: graphics mode */
	{ 0x6,	0x0e }, /* 06: miscellaneous */
	{ 0x7,	0x00 }, /* 07: color don't care */
	{ 0x8,	0xff }, /* 08: bit mask */
};

/* video DAC palette registers */
/* XXX only set up 16 colors used by internal palette in ATC regsters */
static const u_int8_t vga_dacpal[] = {
	/* R     G     B */
	0x00, 0x00, 0x00,	/* BLACK        */
	0x00, 0x00, 0x2a,	/* BLUE         */
	0x00, 0x2a, 0x00,	/* GREEN        */
	0x00, 0x2a, 0x2a,	/* CYAN         */
	0x2a, 0x00, 0x00,	/* RED          */
	0x2a, 0x00, 0x2a,	/* MAGENTA      */
	0x2a, 0x15, 0x00,	/* BROWN        */
	0x2a, 0x2a, 0x2a,	/* LIGHTGREY    */
	0x15, 0x15, 0x15,	/* DARKGREY     */
	0x15, 0x15, 0x3f,	/* LIGHTBLUE    */
	0x15, 0x3f, 0x15,	/* LIGHTGREEN   */
	0x15, 0x3f, 0x3f,	/* LIGHTCYAN    */
	0x3f, 0x15, 0x15,	/* LIGHTRED     */
	0x3f, 0x15, 0x3f,	/* LIGHTMAGENTA */
	0x3f, 0x3f, 0x15,	/* YELLOW       */
	0x3f, 0x3f, 0x3f	/* WHITE        */
};

static const u_int8_t vga_atc[] = {
	0x00,	/* 00: internal palette  0 */
	0x01,	/* 01: internal palette  1 */
	0x02,	/* 02: internal palette  2 */
	0x03,	/* 03: internal palette  3 */
	0x04,	/* 04: internal palette  4 */
	0x05,	/* 05: internal palette  5 */
	0x14,	/* 06: internal palette  6 */
	0x07,	/* 07: internal palette  7 */
	0x38,	/* 08: internal palette  8 */
	0x39,	/* 09: internal palette  9 */
	0x3a,	/* 0A: internal palette 10 */
	0x3b,	/* 0B: internal palette 11 */
	0x3c,	/* 0C: internal palette 12 */
	0x3d,	/* 0D: internal palette 13 */
	0x3e,	/* 0E: internal palette 14 */
	0x3f,	/* 0F: internal palette 15 */
	0x0c,	/* 10: attribute mode control */
	0x00,	/* 11: overscan color */
	0x0f,	/* 12: color plane enable */
	0x08,	/* 13: horizontal PEL panning */
	0x00	/* 14: color select */
};

void
vga_reset(u_char *ISA_mem)
{
	int slot;

	/* check if we are in text mode, if so, punt */
	outb(VGA_GR_PORT, 0x06);
	if ((inb(VGA_GR_DATA) & 0x01) == 0)
		return;

	/* guess not, we lose. */
	slot = -1;
	while ((slot = scan_PCI(slot)) > -1) {
		unlockVideo(slot);
		switch (PCI_vendor(slot)) {
		case PCI_VENDOR_CIRRUS:
			outw(VGA_SR_PORT, 0x0612); /* unlock ext regs */
			outw(VGA_SR_PORT, 0x0700); /* reset ext sequence mode */
			break;
		case PCI_VENDOR_PARADISE:
			outw(VGA_GR_PORT, 0x0f05); /* unlock registers */
			outw(VGA_SR_PORT, 0x0648);
			outw(VGA_CR_PORT, 0x2985);
			outw(VGA_CR_PORT, 0x34a6);
			outb(VGA_GR_PORT, 0x0b); /* disable linear addressing */
			outb(VGA_GR_DATA, inb(VGA_GR_DATA) & ~0x30);
			outw(VGA_SR_PORT, 0x1400);
			outb(VGA_GR_PORT, 0x0e); /* disable 256 color mode */
			outb(VGA_GR_DATA, inb(VGA_GR_DATA) & ~0x01);
			outb(0xd00, 0xff); /* enable auto-centering */
			if (!(inb(0xd01) & 0x03)) {
				outb(VGA_CR_PORT, 0x33);
				outb(VGA_CR_DATA, inb(VGA_CR_DATA) & ~0x90);
				outb(VGA_CR_PORT, 0x32);
				outb(VGA_CR_DATA, inb(VGA_CR_DATA) | 0x04);
				outw(VGA_CR_PORT, 0x0250);
				outw(VGA_CR_PORT, 0x07ba);
				outw(VGA_CR_PORT, 0x0900);
				outw(VGA_CR_PORT, 0x15e7);
				outw(VGA_CR_PORT, 0x2a95);
			}
			outw(VGA_CR_PORT, 0x34a0);
			break;
		case PCI_VENDOR_S3:
			unlock_S3();
			break;
		default:
			break;
		}
		outw(VGA_SR_PORT, 0x0120); /* disable video */
		set_text_regs();
		set_text_clut(0);
		load_font(ISA_mem);
		set_text_regs();
		outw(VGA_SR_PORT, 0x0100); /* re-enable video */
		clear_video_memory();

		if (PCI_vendor(slot) == PCI_VENDOR_S3)
			outb(0x3c2, 0x63);	/* ??? */
		delay(1000);
	}
	return;
}

/* write something to a VGA attribute register */
static void
write_attr(u_int8_t index, u_int8_t data, u_int8_t videoOn)
{
	u_int8_t v;

	v = inb(0x3da);		/* reset attr addr toggle */
	if (videoOn)
		outb(0x3c0, (index & 0x1F) | 0x20);
	else
		outb(0x3c0, (index & 0x1F));
	outb(0x3c0, data);
}

static void
set_text_regs(void)
{
	int i;

	for (i = 0; i < SRREGS; i++) {
		outb(VGA_SR_PORT, SR_regs[i].idx);
		outb(VGA_SR_PORT + 1, SR_regs[i].val);
	}
	for (i = 0; i < CRREGS; i++) {
		outb(VGA_CR_PORT, CR_regs[i].idx);
		outb(VGA_CR_PORT + 1, CR_regs[i].val);
	}
	for (i = 0; i < GRREGS; i++) {
		outb(VGA_GR_PORT, GR_regs[i].idx);
		outb(VGA_GR_PORT + 1, GR_regs[i].val);
	}

	outb(0x3c2, 0x67);  /* MISC */
	outb(0x3c6, 0xff);  /* MASK */

	for (i = 0; i < 0x14; i++)
		write_attr(i, vga_atc[i], 0); 
	write_attr(0x14, 0x00, 1); /* color select; video on  */
}

static void
set_text_clut(int shift)
{
	int i;

	outb(0x3C6, 0xFF);
	inb(0x3C7);
	outb(0x3C8, 0);
	inb(0x3C7);

	for (i = 0; i < (16 * 3); ) {
		outb(0x3c9, vga_dacpal[i++] << shift);
		outb(0x3c9, vga_dacpal[i++] << shift);
		outb(0x3c9, vga_dacpal[i++] << shift);
	}
}

static void
load_font(u_int8_t *ISA_mem)
{
	int i, j;
	u_int8_t *font_page = (u_int8_t *)&ISA_mem[0xA0000];

	outb(0x3C2, 0x67);
	inb(0x3DA);  /* Reset Attr toggle */

	outb(0x3C0, 0x30);
	outb(0x3C0, 0x01);	/* graphics mode */
	outw(0x3C4, 0x0001);	/* reset sequencer */
	outw(0x3C4, 0x0204);	/* write to plane 2 */
	outw(0x3C4, 0x0406);	/* enable plane graphics */
	outw(0x3C4, 0x0003);	/* reset sequencer */
	outw(0x3CE, 0x0402);	/* read plane 2 */
	outw(0x3CE, 0x0500);	/* write mode 0, read mode 0 */
	outw(0x3CE, 0x0605);	/* set graphics mode */

	for (i = 0;  i < sizeof(font);  i += 16) {
		for (j = 0;  j < 16;  j++) {
			__asm__ volatile("eieio");
			font_page[(2*i)+j] = font[i+j];
		}
	}
}

static void
unlock_S3(void)
{
	int s3_devid;

	outw(VGA_CR_PORT, 0x3848);
	outw(VGA_CR_PORT, 0x39a5);
	outb(VGA_CR_PORT, 0x2d);
	s3_devid = inb(VGA_CR_DATA) << 8;
	outb(VGA_CR_PORT, 0x2e);
	s3_devid |= inb(VGA_CR_DATA);

	if (s3_devid != 0x8812) {
		/* from the S3 manual */
		outb(0x46E8, 0x10);  /* Put into setup mode */
		outb(0x3C3, 0x10);
		outb(0x102, 0x01);   /* Enable registers */
		outb(0x46E8, 0x08);  /* Enable video */
		outb(0x3C3, 0x08);
		outb(0x4AE8, 0x00);
                outb(VGA_CR_PORT, 0x38);  /* Unlock all registers */
		outb(VGA_CR_DATA, 0x48);
		outb(VGA_CR_PORT, 0x39);
		outb(VGA_CR_DATA, 0xA5);
		outb(VGA_CR_PORT, 0x40);
		outb(VGA_CR_DATA, inb(0x3D5)|0x01);
		outb(VGA_CR_PORT, 0x33);
		outb(VGA_CR_DATA, inb(0x3D5)&~0x52);
		outb(VGA_CR_PORT, 0x35);
		outb(VGA_CR_DATA, inb(0x3D5)&~0x30);
		outb(VGA_CR_PORT, 0x3A);
		outb(VGA_CR_DATA, 0x00);
		outb(VGA_CR_PORT, 0x53);
		outb(VGA_CR_DATA, 0x00);
		outb(VGA_CR_PORT, 0x31);
		outb(VGA_CR_DATA, inb(0x3D5)&~0x4B);
		outb(VGA_CR_PORT, 0x58);

		outb(VGA_CR_DATA, 0);

		outb(VGA_CR_PORT, 0x54);
		outb(VGA_CR_DATA, 0x38);
		outb(VGA_CR_PORT, 0x60);
		outb(VGA_CR_DATA, 0x07);
		outb(VGA_CR_PORT, 0x61);
		outb(VGA_CR_DATA, 0x80);
		outb(VGA_CR_PORT, 0x62);
		outb(VGA_CR_DATA, 0xA1);
		outb(VGA_CR_PORT, 0x69);  /* High order bits for cursor address */
		outb(VGA_CR_DATA, 0);

		outb(VGA_CR_PORT, 0x32);
		outb(VGA_CR_DATA, inb(0x3D5)&~0x10);
	} else {
		/* IBM Portable 860 */
		outw(VGA_SR_PORT, 0x0806);
		outw(VGA_SR_PORT, 0x1041);
		outw(VGA_SR_PORT, 0x1128);
		outw(VGA_CR_PORT, 0x4000);
		outw(VGA_CR_PORT, 0x3100);
		outw(VGA_CR_PORT, 0x3a05);
		outw(VGA_CR_PORT, 0x6688);
		outw(VGA_CR_PORT, 0x5800); /* disable linear addressing */
		outw(VGA_CR_PORT, 0x4500); /* disable H/W cursor */
		outw(VGA_SR_PORT, 0x5410); /* enable auto-centering */
		outw(VGA_SR_PORT, 0x561f);
		outw(VGA_SR_PORT, 0x1b80); /* lock DCLK selection */
		outw(VGA_CR_PORT, 0x3900); /* lock S3 registers */
		outw(VGA_CR_PORT, 0x3800);
	}
}

static void
clear_video_memory(void)
{
	int i, j;

	for (i = 0;  i < LINES; i++) {
		for (j = 0; j < COLS; j++) {
			videomem[((i * COLS)+j) * 2] = 0x20; /* space */
			videomem[((i * COLS)+j) * 2 + 1] = 0x07; /* fg/bg */
		}
	}
}

#endif /* VGA_RESET */
