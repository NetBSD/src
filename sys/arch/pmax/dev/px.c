/* 	$NetBSD: px.c,v 1.24 2000/01/05 18:44:27 ad Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andy Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include "px.h"

#if NPX > 1
#error Multiboard px support not in-tree right now.
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: px.c,v 1.24 2000/01/05 18:44:27 ad Exp $");

/*
 * px.c: driver for the DEC TURBOchannel 2D and 3D accelerated framebuffers
 * with PixelStamp blitter asics (and i860 accelerators, on the higher end
 * cards).
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/mman.h>
#include <sys/resourcevar.h>

#include <vm/vm.h>
#include <miscfs/specfs/specdev.h>

#include <dev/cons.h>
#include <dev/tc/tcvar.h>
#include <dev/ic/bt459reg.h>
#include <dev/rcons/rcons.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>

#include <mips/cpuregs.h>
#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/pmioctl.h>
#include <machine/fbio.h>
#include <machine/fbvar.h>

#include <pmax/dev/fbreg.h>
#include <pmax/dev/pxreg.h>
#include <pmax/dev/pxvar.h>
#include <pmax/dev/rconsvar.h>
#include <pmax/dev/qvssvar.h>

struct px_softc {
	struct	device px_dv;
	struct	px_info *px_info;
};

int	px_match __P((struct device *, struct cfdata *, void *));
void	px_attach __P((struct device *, struct device *, void *));
int	px_intr __P((void *xxx_sc));

int	pxopen __P((dev_t, int, int, struct proc *));
int	pxclose __P((dev_t, int, int, struct proc *));
int	pxioctl __P((dev_t, u_long, caddr_t, int, struct proc *));
int	pxpoll __P((dev_t, int, struct proc *));
int	pxmmap __P((dev_t, int, int));

static int32_t *px_alloc_pbuf __P((struct px_info *));
static int	px_send_packet __P((struct px_info *, int *buf));
static void	px_init_stic __P((struct px_info *, int));
static void	px_bt459_init __P((struct px_info *));
static int	px_probe_planes __P((struct px_info *, int));
static void	px_load_cursor __P((struct px_info *));
static void	px_conv_cursor __P((struct px_info *, u_short *));
static void	px_load_cmap __P((struct px_info *, int, int));
static void	px_load_cursor_data __P((struct px_info *, int, int));
static void	px_make_cursor __P((struct px_info *));
static int	px_rect __P((struct px_info *, int, int, int, int, int));
static void	px_qvss_init __P((struct px_info *));
static int	px_mmap_info  __P((struct proc *, dev_t, vm_offset_t *));
static void	px_cursor_hack __P((struct fbinfo *, int, int));
static int	px_probe_sram __P((struct px_info *));
static void	px_bt459_flush __P((struct px_info *));

struct cfattach px_ca = {
	sizeof(struct px_softc),
	px_match,
	px_attach,
};

/* The different types of card that we support, for px_match(). */
static const char *px_types[] = {
	"PMAG-CA ",
	"PMAG-DA ",
	"PMAG-FA ",	/* XXX um, does this ever get reported? */
};

#define NUM_PX_TYPES (sizeof(px_types) / sizeof(px_types[0]))

/* wscons emulator operations */
static void	px_cursor __P((void *, int, int, int));
static void	px_putchar __P((void *, int, int, u_int, long));
static void	px_copycols __P((void *, int, int, int, int));
static void	px_copyrows __P((void *, int, int, int num));
static void	px_erasecols __P((void *, int, int, int num, long));
static void	px_eraserows __P((void *, int, int, long));
static int	px_alloc_attr __P((void *, int, int, int, long *));
static int	px_mapchar __P((void *, int, unsigned int *));

static struct wsdisplay_emulops px_emulops = {
	px_cursor,
	px_mapchar,
	px_putchar,
	px_copycols,
	px_erasecols,
	px_copyrows,
	px_eraserows,
	px_alloc_attr
};

/* Colormap for wscons, matching WSCOL_*. Upper 8 are high-intensity */
static const u_char px_cmap[16*3] = {
	0x00, 0x00, 0x00, /* black */
	0x7f, 0x00, 0x00, /* red */
	0x00, 0x7f, 0x00, /* green */
	0x7f, 0x7f, 0x00, /* brown */
	0x00, 0x00, 0x7f, /* blue */
	0x7f, 0x00, 0x7f, /* magenta */
	0x00, 0x7f, 0x7f, /* cyan */
	0xc7, 0xc7, 0xc7, /* white - XXX too dim? */

	0x7f, 0x7f, 0x7f, /* black */
	0xff, 0x00, 0x00, /* red */
	0x00, 0xff, 0x00, /* green */
	0xff, 0xff, 0x00, /* brown */
	0x00, 0x00, 0xff, /* blue */
	0xff, 0x00, 0xff, /* magenta */
	0x00, 0xff, 0xff, /* cyan */
	0xff, 0xff, 0xff, /* white */
};

/*
 * Compose 2 bit/pixel cursor image.  Bit order will be reversed.
 *   M M M M I I I I		M I M I M I M I
 *	[ before ]		   [ after ]
 *   3 2 1 0 3 2 1 0		0 0 1 1 2 2 3 3
 *   7 6 5 4 7 6 5 4		4 4 5 5 6 6 7 7
 *
 * XXX duplicated in sfb.c, cfb.c, mfb.c
 */
static const u_char px_shuffle[256] = {
	0x00, 0x40, 0x10, 0x50, 0x04, 0x44, 0x14, 0x54,
	0x01, 0x41, 0x11, 0x51, 0x05, 0x45, 0x15, 0x55,
	0x80, 0xc0, 0x90, 0xd0, 0x84, 0xc4, 0x94, 0xd4,
	0x81, 0xc1, 0x91, 0xd1, 0x85, 0xc5, 0x95, 0xd5,
	0x20, 0x60, 0x30, 0x70, 0x24, 0x64, 0x34, 0x74,
	0x21, 0x61, 0x31, 0x71, 0x25, 0x65, 0x35, 0x75,
	0xa0, 0xe0, 0xb0, 0xf0, 0xa4, 0xe4, 0xb4, 0xf4,
	0xa1, 0xe1, 0xb1, 0xf1, 0xa5, 0xe5, 0xb5, 0xf5,
	0x08, 0x48, 0x18, 0x58, 0x0c, 0x4c, 0x1c, 0x5c,
	0x09, 0x49, 0x19, 0x59, 0x0d, 0x4d, 0x1d, 0x5d,
	0x88, 0xc8, 0x98, 0xd8, 0x8c, 0xcc, 0x9c, 0xdc,
	0x89, 0xc9, 0x99, 0xd9, 0x8d, 0xcd, 0x9d, 0xdd,
	0x28, 0x68, 0x38, 0x78, 0x2c, 0x6c, 0x3c, 0x7c,
	0x29, 0x69, 0x39, 0x79, 0x2d, 0x6d, 0x3d, 0x7d,
	0xa8, 0xe8, 0xb8, 0xf8, 0xac, 0xec, 0xbc, 0xfc,
	0xa9, 0xe9, 0xb9, 0xf9, 0xad, 0xed, 0xbd, 0xfd,
	0x02, 0x42, 0x12, 0x52, 0x06, 0x46, 0x16, 0x56,
	0x03, 0x43, 0x13, 0x53, 0x07, 0x47, 0x17, 0x57,
	0x82, 0xc2, 0x92, 0xd2, 0x86, 0xc6, 0x96, 0xd6,
	0x83, 0xc3, 0x93, 0xd3, 0x87, 0xc7, 0x97, 0xd7,
	0x22, 0x62, 0x32, 0x72, 0x26, 0x66, 0x36, 0x76,
	0x23, 0x63, 0x33, 0x73, 0x27, 0x67, 0x37, 0x77,
	0xa2, 0xe2, 0xb2, 0xf2, 0xa6, 0xe6, 0xb6, 0xf6,
	0xa3, 0xe3, 0xb3, 0xf3, 0xa7, 0xe7, 0xb7, 0xf7,
	0x0a, 0x4a, 0x1a, 0x5a, 0x0e, 0x4e, 0x1e, 0x5e,
	0x0b, 0x4b, 0x1b, 0x5b, 0x0f, 0x4f, 0x1f, 0x5f,
	0x8a, 0xca, 0x9a, 0xda, 0x8e, 0xce, 0x9e, 0xde,
	0x8b, 0xcb, 0x9b, 0xdb, 0x8f, 0xcf, 0x9f, 0xdf,
	0x2a, 0x6a, 0x3a, 0x7a, 0x2e, 0x6e, 0x3e, 0x7e,
	0x2b, 0x6b, 0x3b, 0x7b, 0x2f, 0x6f, 0x3f, 0x7f,
	0xaa, 0xea, 0xba, 0xfa, 0xae, 0xee, 0xbe, 0xfe,
	0xab, 0xeb, 0xbb, 0xfb, 0xaf, 0xef, 0xbf, 0xff,
};

#define PXMAP_INFO_SIZE	(NBPG)
#define PXMAP_RBUF_SIZE	(4096 * 16 + 8192 * 2)

/* Need alignment to 8KB here... */
static u_char	px_cons_rbuf[PXMAP_INFO_SIZE + PXMAP_RBUF_SIZE + 8192];
static u_int	px_cons_rbuf_use;
struct px_info *px_cons_info;
struct px_info *px_unit[NPX];

/* bt459 gunk. XXX should be done with driver independant interface */
struct bt459_regs {
        volatile int32_t lo;
        volatile int32_t hi;
        volatile int32_t reg;
        volatile int32_t cmap;
};

#define	BT459_SELECT(vdac, regno) do {		\
	(vdac)->lo = DUPBYTE0(regno);		\
	(vdac)->hi = DUPBYTE1(regno);		\
	tc_wmb();				\
   } while(0);

#define	BT459_WRITE_REG(vdac, data) \
	((vdac)->reg = (data)), tc_wmb();

#define	BT459_WRITE_CMAP(vdac, data) \
	((vdac)->cmap = (data)), tc_wmb();

#define BT459_READ_REG(vdac)	((vdac)->reg)

/* We have to support 3 VDACs on the 24-bit cards */
#define DUPBYTE0(x) ((((x)&0xff)<<16) | (((x)&0xff)<<8) | ((x)&0xff))
#define DUPBYTE1(x) ((((x)<<8)&0xff0000) | ((x)&0xff00) | (((x)>>8)&0xff))
#define DUPBYTE2(x) (((x)&0xff0000) | (((x)>>8)&0xff00) | (((x)>>16)&0xff))

#define PACK_WORD(p, o) ((p)[(o)] | ((p)[(o)+1] << 16))

/*
 * Match a supported board.
 */
int
px_match(parent, match, aux)
	struct	device *parent;
	struct	cfdata *match;
	void	*aux;
{
	struct tc_attach_args *ta = (struct tc_attach_args *)aux;
	int i;

	for (i = 0; i < NUM_PX_TYPES; i++)
		if (strncmp(px_types[i], ta->ta_modname, TC_ROM_LLEN) == 0)
			return (1);

	return (0);
}


/*
 * Attach the graphics board.
 */
void
px_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct px_softc *sc;
	struct px_info *pxi;
	struct tc_attach_args *ta;
	caddr_t slotbase;
	char *p;
	int i;

	sc = (struct px_softc *)self;
	ta = (struct tc_attach_args *)aux;
	slotbase = (caddr_t)TC_PHYS_TO_UNCACHED(ta->ta_addr);

	/* Init the card only if it hasn't been done before... */
	if (!px_cons_info || slotbase != (caddr_t)px_cons_info->pxi_slotbase)
		px_init((struct fbinfo *)1, slotbase, sc->px_dv.dv_unit, 0);

	/* px_init() fills in px_unit[#] */
	pxi = px_unit[sc->px_dv.dv_unit];
	sc->px_info = pxi;

	/* Now grab the interrupt */
	tc_intr_establish(parent, ta->ta_cookie, TC_IPL_TTY, px_intr, sc);

	for (i = 0, p = ta->ta_modname; *p != ' ' && i < TC_ROM_LLEN; i++)
		pxi->pxi_type[i] = *p++;

	pxi->pxi_type[i] = '\0';

	/* Set ISR driven packet-buffer polling addresses */
	for (i = 0; i < 16; i++) {
		caddr_t addr = (caddr_t)pxi->pxi_rbuf + (i << 12);
		pxi->pxi_qpoll[i] = px_poll_addr(slotbase, addr);
	}

	/* The following values are filled in by px_init_stic. */
	printf(": %cD, %dx%d stamp, %d plane", (pxi->pxi_option ? '3' : '2'),
	    pxi->pxi_stampw, pxi->pxi_stamph, pxi->pxi_nplanes);
	if (pxi->pxi_option)
		printf(", %dKB SRAM", px_probe_sram(pxi) >> 10);
	printf("\n");
}


/*
 * Initialize the graphics board. This can be called from tc_findcons and
 * the pmax console trickery, in which case ``fi'' will be null.
 *
 * XXX use magic number to make sure fi isn't a real struct fbinfo?
 */
int
px_init(fi, slotbase, unit, console)
	struct fbinfo *fi;
	caddr_t slotbase;
	int unit, console;
{
	struct px_info *pxi;
	u_long bufpa;
	int i;
	
#if NPX > 1
	if (px_cons_rbuf_use)
		/* XXX allocate buffers */;
	else
#endif
	{
		bufpa = MIPS_KSEG0_TO_PHYS(px_cons_rbuf);
		px_cons_rbuf_use = 1;
	}

	/* Align to 8KB. px_info struct gets the first 4KB */
	bufpa = (bufpa + 8191) & ~8191;
	pxi = (struct px_info *)MIPS_PHYS_TO_KSEG0(bufpa);
	px_unit[unit] = pxi;
	bufpa += PXMAP_INFO_SIZE;

	if (bufpa + PXMAP_RBUF_SIZE > 8192*1024) {
		printf("px%d: ring buffer outside first 8MB of RAM\n", unit);
		return 0;
	}

	pxi->pxi_slotbase = TC_PHYS_TO_UNCACHED(slotbase);
	pxi->pxi_unit = unit;
	pxi->pxi_stamp = (caddr_t) (pxi->pxi_slotbase + PX_STAMP_OFFSET);
	pxi->pxi_poll = (int32_t *) (pxi->pxi_slotbase + PX_STIC_POLL_OFFSET);
	pxi->pxi_stic = (struct stic_regs *) (pxi->pxi_slotbase + PX_STIC_OFFSET);
	pxi->pxi_rbuf = (int *)MIPS_PHYS_TO_KSEG1(bufpa);
	pxi->pxi_rbuf_phys = bufpa;
	pxi->pxi_rbuf_size = PXMAP_RBUF_SIZE;
	pxi->pxi_pbuf_select = 0;
	pxi->pxi_flg = PX_ENABLE;

	/* We need to do this ASAP so we can disable the co-processor */
	px_init_stic(pxi, 1);
	
	/* 
	 * If this is a PXG, use the SRAM and not kernel bss.
	 * XXX this is a big fat waste of memory.
	 */
	if (pxi->pxi_option) {
		bufpa = MIPS_KSEG0_TO_PHYS(slotbase + PXG_SRAM_OFFSET);
		pxi->pxi_rbuf = (int *)MIPS_PHYS_TO_KSEG0(bufpa);
		pxi->pxi_rbuf_phys = bufpa;
		pxi->pxi_rbuf_size = px_probe_sram(pxi);
	}

	/* Get a font and lock. If we're not the console, we don't care */
	if (console) {
		wsfont_init();

		if ((i = wsfont_find(NULL, 0, 0, 2)) <= 0)
			panic("px_init: unable to get font");
		
		if (wsfont_lock(i, &pxi->pxi_font, WSDISPLAY_FONTORDER_R2L, 
		    WSDISPLAY_FONTORDER_L2R) <= 0)
			panic("px_init: unable to lock font");
			
		pxi->pxi_wsfcookie = i;
	} else 
		pxi->pxi_wsfcookie = -1;

	/* Only now can we init the bt459... */
	px_bt459_init(pxi);

	/* Clear ringbuffer and then the screen */
	bzero(pxi->pxi_rbuf, pxi->pxi_rbuf_size);
	px_rect(pxi, 0, 0, 1280, 1024, 0);

	/* Connect to rcons if this is the console device */
	if (console) {
		pxi->pxi_fontscale = pxi->pxi_font->fontheight * 
		    pxi->pxi_font->stride;
		px_cons_info = pxi;
		
		/* XXX no multiscreen X support yet */
		px_qvss_init(pxi);

		rcons_connect_native(&px_emulops, pxi, 1280, 1024,
		  1280 / pxi->pxi_font->fontwidth,
		  1024 / pxi->pxi_font->fontheight);
	}

	return 1;
}


/*
 * Initalize our DISGUSTING little hack so we can use the qvss event-buffer
 * stuff for the X server.
 */
static void
px_qvss_init(pxi)
	struct px_info *pxi;
{
	struct fbinfo *fi;
	static struct pmax_fbtty pxfb;
	static struct fbdriver fbdriver = {
		NULL, NULL, NULL, NULL,
		NULL, px_cursor_hack, NULL, NULL
	};

	fi = &pxi->pxi_fbinfo;
	fi->fi_driver = &fbdriver;
	fi->fi_base = (caddr_t)pxi;
	fi->fi_fbu = &pxi->pxi_fbuaccess;
	fi->fi_type.fb_width = 1280;
	fi->fi_type.fb_height = 1024;
	fi->fi_type.fb_boardtype = PMAX_FBTYPE_PX;
	fi->fi_type.fb_cmsize = 256;
	fi->fi_type.fb_depth = 8;
	fi->fi_fbu->scrInfo.max_row = 1024 / 8;
	fi->fi_fbu->scrInfo.max_col = 80;
	fi->fi_glasstty = &pxfb;

	tb_kbdmouseconfig(fi);
	init_pmaxfbu(fi);
}


/*
 * Initialize the Brooktree 459 VDAC.
 */
static void
px_bt459_init(pxi)
	struct px_info *pxi;
{
	struct bt459_regs *vdac = pxi->pxi_vdac;
	int i;

	/* Hit it... */
	BT459_SELECT(vdac, BT459_IREG_COMMAND_0);
	BT459_WRITE_REG(vdac, 0xc0c0c0);

	/* Now reset the VDAC */
	if (pxi->pxi_option)
		*(int32_t *)(pxi->pxi_slotbase + PXG_VDAC_RESET_OFFSET) = 0;
	else
		*(int32_t *)(pxi->pxi_slotbase + PX_VDAC_RESET_OFFSET) = 0;

	tc_wmb();

	/* Finish the initalization */
	BT459_SELECT(vdac, BT459_IREG_COMMAND_1);
	BT459_WRITE_REG(vdac, 0x000000);
	BT459_WRITE_REG(vdac, 0xc2c2c2);
	BT459_WRITE_REG(vdac, 0xffffff);

	for (i = 0; i < 7; i++)
		BT459_WRITE_REG(vdac, 0);

	/* Set cursor colormap */
	BT459_SELECT(vdac, BT459_IREG_CCOLOR_1);
	BT459_WRITE_REG(vdac, 0xffffff);
	BT459_WRITE_REG(vdac, 0xffffff);
	BT459_WRITE_REG(vdac, 0xffffff);

	BT459_WRITE_REG(vdac, 0x00);
	BT459_WRITE_REG(vdac, 0x00);
	BT459_WRITE_REG(vdac, 0x00);

	BT459_WRITE_REG(vdac, 0xffffff);
	BT459_WRITE_REG(vdac, 0xffffff);
	BT459_WRITE_REG(vdac, 0xffffff);

	/* Build and load a sane colormap */
	bzero(pxi->pxi_cmap, sizeof(pxi->pxi_cmap));
	memcpy(pxi->pxi_cmap, px_cmap, 16 * 3);
	pxi->pxi_cmap[255*3+0] = 0xff;
	pxi->pxi_cmap[255*3+1] = 0xff;
	pxi->pxi_cmap[255*3+2] = 0xff;
	px_load_cmap(pxi, 0, 256);

	/* Make a sane cursor and load it */
	if (pxi->pxi_font != NULL) {
		px_make_cursor(pxi);
		px_load_cursor(pxi);

		/* Enable cursor */
		BT459_SELECT(vdac, BT459_IREG_CCR);
		BT459_WRITE_REG(vdac, 0x1c1c1c1);
		pxi->pxi_flg |= PX_CURSOR_ENABLE;
	} else {
		BT459_SELECT(vdac, BT459_IREG_CCR);
		BT459_WRITE_REG(vdac, 0);
	}
}


/*
 * Determine the number of planes supported by the given buffer.
 * XXX should know this from module name
 */
static int
px_probe_planes(pxi, buf)
	struct px_info *pxi;
	int buf;
{
	int i;
	
	if (buf == 0) {
		/*
		 * For the real framebuffer (# 0), we can cheat and use the
		 * VDAC ID. One color is active at level 0x4a for 8 bits, all
		 * colors are active at 0x4a on the 24 bit cards.
		 */
		BT459_SELECT(pxi->pxi_vdac, BT459_IREG_ID);
		i = pxi->pxi_vdac->reg & 0x00ffffff;

		/* 3 VDACs */
		if (i == 0x4a4a4a)
			return 24;

		/* 1 VDAC */
		if ((i & 0xff0000) == 0x4a0000 ||
		    (i & 0x00ff00) == 0x004a00 ||
		    (i & 0x0000ff) == 0x00004a)
			return 8;

		/* Whoops... Assume 8 planes? */
		printf("\n");
		printf("px%d: invalid VDAC ID 0x%08x", pxi->pxi_unit, i);
		return 8;
	}

	/* Don't give a damn about Z-buffers... */
	panic("px_probe_planes: (buf != 0) was un-implemented (bloat)");
}
	

/*
 * Figure out how much SRAM the PXG has: 128KB or 256KB.
 */
static int
px_probe_sram(pxi)
	struct px_info *pxi;
{
	volatile int32_t *a, *b;
	
	a = (int32_t *)(pxi->pxi_slotbase + PXG_SRAM_OFFSET);
	b = a + (0x20000 >> 1);
	
	*a = 4321;
	*b = 1234;
	tc_wmb();

	return ((*a == *b) ? 0x20000 : 0x40000);
}

/*
 * Initialize the STIC (STamp Interface Chip) and stamp
 */
void
px_init_stic(pxi, probe)
	struct px_info *pxi;
	int probe;
{
	int modtype, xconfig, yconfig, config;
	struct stic_regs *stic;
	volatile int32_t *slot;
	caddr_t stamp;
	int i;

	stic = pxi->pxi_stic;
	stamp = pxi->pxi_stamp;
	
	/* If this is a 3D board, disable the i860 co-processor. */
	if (((stic->modcl >> 12) & 3) != 0) {
		slot = (volatile int32_t *)pxi->pxi_slotbase;

		slot[PXG_N10_RESET_OFFSET >> 2] = 0;
		tc_wmb();
		slot[PXG_HOST_INTR_OFFSET >> 2] = 0;
		tc_wmb();
		DELAY(40000); /* paranoia */
	}	

	/*
	 * Initialize STIC interface chip registers. Magic sequence from
	 * logic analyser.
	 */
	stic->sticsr = 0x00000030;	/* Get the STIC's attention. */
	tc_wmb();
	DELAY(4000);			/* wait 4ms for STIC to respond. */
	stic->sticsr = 0x00000000;	/* Hit the STIC's csr again... */
	tc_wmb();
	stic->buscsr = 0xffffffff;	/* and bash its bus-acess csr. */
	tc_wmb();			/* Blam! */
	DELAY(20000);			/* wait until the stic recovers... */

	/* Initialize Stamp config register for model 0. */
	modtype = stic->modcl;
	xconfig = (modtype & 0x800) >> 11;
	yconfig = (modtype & 0x600) >> 9;
	config = (yconfig << 1) | xconfig;

	*(int32_t *) (stamp + __PXS(0x000b0)) = config;
	*(int32_t *) (stamp + __PXS(0x000b4)) = 0x0;

	if (yconfig > 0) {
		/* pixelstamp 1 configuration */
		*(int32_t *) (stamp + __PXS(0x100b0)) = config | 8;
		*(int32_t *) (stamp + __PXS(0x100b4)) = 0x0;
	}
	/*
	 * Remember the size of the stamp, and card revision. Also
	 * figure out the number of planes.
	 */
	if (probe) {
		pxi->pxi_stampw = (xconfig ? 5 : 4);
		pxi->pxi_stamph = (1 << yconfig);
		pxi->pxi_revision = (char)(modtype >> 24);
		pxi->pxi_option = (char)((modtype >> 12) & 3);

		/* PXG upwards has it's VDAC at a different location */
		i = (pxi->pxi_option ? PXG_VDAC_OFFSET : PX_VDAC_OFFSET);
		pxi->pxi_vdac = (struct bt459_regs *) (pxi->pxi_slotbase + i);
	
		if (pxi->pxi_option == 0) {
			/* 2D board */
			pxi->pxi_nplanes = 8;
			pxi->pxi_planemask = 0xff;
		} else {
			if (pxi->pxi_stamph > 1) {
				/* high-end 3D */
				pxi->pxi_nplanes = 24;
				pxi->pxi_planemask = 0xffffff;
			} else {
				/* low/mid 3D */
				pxi->pxi_nplanes = px_probe_planes(pxi, 0);

				/* XXX is this right? */
				if (pxi->pxi_nplanes == 8)
					pxi->pxi_planemask = 0xff0000;
				else
					pxi->pxi_planemask = 0xffffff;
			}
		}
	}

	/*
	 * Initialize STIC video registers. (if we knew what we were doing,
	 * we might be able to frob this to work at different montior
	 * frequencies.)
	 */
	stic->vblank = (1024 << 16) | 1063;
	stic->vsync = (1027 << 16) | 1030;
	stic->hblank = (255 << 16) | 340;
	stic->hsync2 = 245;
	stic->hsync = (261 << 16) | 293;
	stic->ipdvint = STIC_INT_CLR | STIC_INT_WE;
	stic->sticsr = 0x00000008;
	tc_wmb();

#ifdef notdef	
	/* Now enable the i860 and STIC interrupts (PXG only) */
	if (pxi->pxi_option) {
		slot = (volatile int32_t *)pxi->pxi_slotbase;

		slot[PXG_N10_START_OFFSET >> 2] = 1;
		tc_wmb();
		DELAY(2000);
		stic->sticsr = STIC_INT_WE | STIC_INT_CLR;
		tc_wmb();
	}
#endif
}


/*
 * Make a cursor matching the current font dimensions.
 */
static void
px_make_cursor(pxi)
	struct px_info *pxi;
{
	u_int8_t *ip, *mp;
	int r, c, o, b;

	ip = pxi->pxi_cursor;
	mp = pxi->pxi_cursor + (sizeof(pxi->pxi_cursor) >> 1);
	bzero(pxi->pxi_cursor, sizeof(pxi->pxi_cursor));

	for (r = 0; r < pxi->pxi_font->fontheight; r++) {
		for (c = 0; c < pxi->pxi_font->fontwidth; c++) {
			o = c >> 3;
			b = 1 << (c & 7);
			ip[o] |= b;
			mp[o] |= b;
		}

		ip += 8;
		mp += 8;
	}
}


/*
 * Convert a 16x16 cursor from the Xserver.
 */
static void
px_conv_cursor(pxi, sip)
	struct px_info *pxi;
	u_short *sip;
{
	u_short *ip, *mp;
	int r;

	ip = (u_short *)pxi->pxi_cursor;
	mp = (u_short *)(pxi->pxi_cursor + (sizeof(pxi->pxi_cursor) >> 1));
	bzero(pxi->pxi_cursor, sizeof(pxi->pxi_cursor));

	for (r = 0; r < 16; r++) {
		*ip = sip[0];
		*mp = sip[16];
		sip++;
		ip += 4;
		mp += 4;
	}
}


/*
 * Load a single byte into the cursor map.
 */
static void
px_load_cursor_data(pxi, pos, val)
	struct px_info *pxi;
	int pos, val;
{
	struct bt459_regs *vdac;
	int cnt;

	vdac = pxi->pxi_vdac;
	val = DUPBYTE0(val);
		
	for (cnt = 10; cnt; cnt--) {
		BT459_SELECT(vdac, BT459_IREG_CRAM_BASE + pos);
		BT459_WRITE_REG(vdac, val);

		if ((BT459_READ_REG(vdac) & pxi->pxi_planemask) == val)
			break;
	}
}


/*
 * Load the cursor image.
 */
static void
px_load_cursor(pxi)
	struct px_info *pxi;
{
	struct bt459_regs *vdac;
	u_int8_t *ip, *mp, img, msk, u;
	int bcnt;

	vdac = pxi->pxi_vdac;
	ip = pxi->pxi_cursor;
	mp = pxi->pxi_cursor + (sizeof(pxi->pxi_cursor) >> 1);

	bcnt = 0;
	BT459_SELECT(vdac, BT459_IREG_CRAM_BASE + 0);

	/* 64 pixel scan line is made with 8 bytes of cursor RAM */
	while (bcnt < sizeof(pxi->pxi_cursor)) {
		img = *ip++;
		msk = *mp++;
		img &= msk;	/* cookie off image */
		u = (msk & 0x0f) << 4 | (img & 0x0f);
		px_load_cursor_data(pxi, bcnt++, px_shuffle[u]);
		u = (msk & 0xf0) | (img & 0xf0) >> 4;
		px_load_cursor_data(pxi, bcnt++, px_shuffle[u]);
	}
}


/*
 * Load the colormap.
 */
static void
px_load_cmap(pxi, index, num)
	struct px_info *pxi;
	int index, num;
{
	struct bt459_regs *vdac;
	u_char *p;

	if (index < 0 || num <= 0 || index + num > 256)
		return;

	vdac = pxi->pxi_vdac;
	num = (num << 1) + num; /* multiply by 3 */
	p = pxi->pxi_cmap + (index << 1) + index;

	BT459_SELECT(vdac, index);
	BT459_SELECT(vdac, index);
	DELAY(20);

	for (; num--; p++)
		BT459_WRITE_CMAP(vdac, DUPBYTE0(*p));
}


/*
 * Flush any pending updates to the bt459. This gets called during vblank
 * on the PX to prevent shearing/snow. The PXG always has to flush.
 */
static void
px_bt459_flush(pxi)
	struct px_info *pxi;
{
	struct bt459_regs *vdac;
	u_char *cp;
	int i;
	
	vdac = pxi->pxi_vdac;

	if (pxi->pxi_dirty & PX_DIRTY_CURSOR_POS) {
		BT459_SELECT(vdac, BT459_IREG_CURSOR_X_LOW);
		BT459_WRITE_REG(vdac, DUPBYTE0(pxi->pxi_curx));
		BT459_WRITE_REG(vdac, DUPBYTE1(pxi->pxi_curx));
		BT459_WRITE_REG(vdac, DUPBYTE0(pxi->pxi_cury));
		BT459_WRITE_REG(vdac, DUPBYTE1(pxi->pxi_cury));
	}

	if (pxi->pxi_dirty & PX_DIRTY_CURSOR)
		px_load_cursor(pxi);

	if (pxi->pxi_dirty & PX_DIRTY_CURSOR_CMAP) {
		cp = pxi->pxi_curcmap;

		BT459_SELECT(vdac, BT459_IREG_CCOLOR_1);
		BT459_WRITE_REG(vdac, DUPBYTE0(cp[3]));
		BT459_WRITE_REG(vdac, DUPBYTE0(cp[4]));
		BT459_WRITE_REG(vdac, DUPBYTE0(cp[5]));
		BT459_WRITE_REG(vdac, DUPBYTE0(cp[0]));
		BT459_WRITE_REG(vdac, DUPBYTE0(cp[1]));
		BT459_WRITE_REG(vdac, DUPBYTE0(cp[2]));
		BT459_WRITE_REG(vdac, DUPBYTE0(cp[3]));
		BT459_WRITE_REG(vdac, DUPBYTE0(cp[4]));
		BT459_WRITE_REG(vdac, DUPBYTE0(cp[5]));
	}

	if (pxi->pxi_dirty & PX_DIRTY_ENABLE) {
		if (pxi->pxi_flg & PX_ENABLE) {
			BT459_SELECT(vdac, BT459_IREG_PRM);
			BT459_WRITE_REG(vdac, 0xffffff);
			px_load_cmap(pxi, 0, 1);
			pxi->pxi_dirty |= PX_DIRTY_CURSOR_ENABLE;
		} else {
			BT459_SELECT(vdac, BT459_IREG_PRM);
			BT459_WRITE_REG(vdac, 0);

			BT459_SELECT(vdac, 0);
			BT459_WRITE_CMAP(vdac, 0);
			BT459_WRITE_CMAP(vdac, 0);
			BT459_WRITE_CMAP(vdac, 0);

			BT459_SELECT(vdac, BT459_IREG_CCR);
			BT459_WRITE_REG(vdac, 0);
		}
	}

	if (pxi->pxi_flg & PX_ENABLE) {
		if (pxi->pxi_dirty & PX_DIRTY_CMAP)
			px_load_cmap(pxi, pxi->pxi_cmap_idx,
			    pxi->pxi_cmap_cnt);

		if (pxi->pxi_dirty & PX_DIRTY_CURSOR_ENABLE) {
			if (pxi->pxi_flg & PX_CURSOR_ENABLE) {
				/* No flashing cursor for X */
				if (pxi->pxi_flg & PX_OPEN)
					i = 0x00c0c0c0;
				else
					i = 0x01c1c1c1;
			} else
				i = 0;

			BT459_SELECT(vdac, BT459_IREG_CCR);
			BT459_WRITE_REG(vdac, i);
		}
	}

	pxi->pxi_dirty = 0;
}


/*
 * PixelStamp board interrupt handler. We can get more than one interrupt
 * at a time (i.e. packet+vertical retrace) so we don't return after
 * handling each case.
 */
int
px_intr(xxx_sc)
	void	*xxx_sc;
{
	struct px_cliplist *cl;
	struct stic_regs *stic;
	struct px_info *pxi;
	int caught, state;

	pxi = (struct px_info *)
	   MIPS_PHYS_TO_KSEG1(((struct px_softc *)xxx_sc)->px_info);

	stic = pxi->pxi_stic;
	caught = 0;
	state = stic->ipdvint;
	
#ifdef notdef
	/* Getting this from the i860? */
	if (pxi->pxi_option) {
		hi = (int32_t *)pxi->pxi_slotbase + (PXG_HOST_INTR_OFFSET>>2);
		
		/* Clear the interrupt condition */
		i = hi[0] & 15;
		hi[0] = 0;
		tc_wmb();
		hi[2] = 0;
		tc_wmb();

		if (i == 3) /* 3 == vblank */
			state |= STIC_INT_V;
	}
#endif

	/* Vertical retrace interrupt. */
	if (state & STIC_INT_V) {
		stic->ipdvint = STIC_INT_V_WE | (stic->ipdvint & STIC_INT_V_EN);
		tc_wmb();
		caught = 1;

		if (pxi->pxi_dirty)
			px_bt459_flush(pxi);
	}

	/* Packet interrupt. Clear packet done flag. */
	if (state & STIC_INT_P) {
		stic->ipdvint = STIC_INT_P_WE | (stic->ipdvint & STIC_INT_P_EN);
		tc_wmb();
		caught = 1;
	}

#ifdef notyet
	/* Error/stray interrupt */
	if ((stic->ipdvint & STIC_INT_E) || !caught) {
		printf("px%d: %s intr, %x %x %x %x %x", pxi->pxi_unit,
		    (caught ? "error" : "stray"), stic->ipdvint, stic->sticsr,
		    stic->buscsr, stic->busadr, stic->busdat);
	}
#endif
	/* Abort if no packets are in the queue */
	if (pxi->pxi_lpr == pxi->pxi_lpw) {
		pxi->pxi_flg &= ~PX_ISR_ACTIVE;
		return (0);
	}

	/* Abort if we're awaiting reload of cliplist */
	if (pxi->pxi_flg & PX_ISR_LOAD_CLIP)
		return (0);

	cl = &pxi->pxi_cliplist;

	/* Does this new packet need to be clipped? */
	if (cl->cl_loaded != 0 && (pxi->pxi_flg & PX_ISR_PASS_CLIP) == 0) {
		int32_t *buf;

		buf = (int32_t *)pxi->pxi_rbuf + (pxi->pxi_lpr & 15) * 1024;

		if (*buf & STAMP_CLIPRECT) {
			pxi->pxi_flg |= PX_ISR_PASS_CLIP;

			/* Figure out where in the packet fixup should occur */
			cl->cl_fixup = buf + 4;

			/* XXX this computation is wrong... */
			if (*buf & STAMP_XY_PERPACKET)
				cl->cl_fixup++;

			if (*buf & STAMP_LW_PERPACKET)
				cl->cl_fixup++;

			cl->cl_cur = 0;
		}
	}

	/* Need to fix up packet with next cliprect? */
	if (pxi->pxi_flg & PX_ISR_PASS_CLIP) {
		cl->cl_fixup[0] = cl->cl_minval[cl->cl_cur];
		cl->cl_fixup[1] = cl->cl_maxval[cl->cl_cur];

		/* Need more cliprects/finished? */
		if ((cl->cl_cur + 1) >= cl->cl_loaded) {
			if (cl->cl_notloaded != 0)
				pxi->pxi_flg |= PX_ISR_LOAD_CLIP;
			else
				pxi->pxi_flg &= ~PX_ISR_PASS_CLIP;
		}
	}

	/* Fire off the packet and move to next cliprect OR next packet */
	if (*pxi->pxi_qpoll[pxi->pxi_lpr & 15] == STAMP_OK) {
		if (pxi->pxi_flg & PX_ISR_PASS_CLIP)
			cl->cl_cur++;
		else
			pxi->pxi_lpr++;
	}

	return (0);
}


/*
 * Allocate a PixelStamp packet buffer.
 */
static inline int32_t *
px_alloc_pbuf(pxi)
	struct px_info *pxi;
{
	volatile int32_t *poll;
	int32_t *buf;
	int i, j;

	/* Use queueing if the ISR is enabled */
	if (pxi->pxi_flg & PX_ISR_ENABLE) {
 		/* Wait until we have a free buffer */
		for (i = STAMP_RETRIES; i; i--) {
			if (pxi->pxi_lpw - pxi->pxi_lpr < 15)
				break;

			DELAY(STAMP_DELAY);
		}

		/*
		 * Dequeue stalled packets manually. This should only
		 * ever happen during prelonged periods at splhigh().
		 * Autoconfiguration time is an example.
		*/
		if (i == 0) {
			for (i = pxi->pxi_lpr; i < pxi->pxi_lpw; i++) {
				poll = pxi->pxi_qpoll[i & 15];

				for (j = STAMP_RETRIES; j; j--) {
					if (*poll == STAMP_OK)
						break;

					DELAY(STAMP_DELAY);
				}
			}

			pxi->pxi_lpr = pxi->pxi_lpw;
		}

		return (int32_t *)((caddr_t)pxi->pxi_rbuf +
		    ((pxi->pxi_lpw & 15) << 12));
	}
	
#ifdef notdef
	/* If this is a PXG, ask the damn i860 which buffer to use */
	if (pxi->pxi_option) {
		poll = (volatile int32_t *)pxi->pxi_slotbase;
		poll += PXG_COPROC_INTR_OFFSET >> 2;
		
		/* 
		 * XXX these should be defined as constants. 0x30 is
		 * "pause coprocessor and interrupt."
		 */
		*poll = 0x30;
		tc_wmb();
	
		for (i = 1000000; i; i--) {
			DELAY(4);

			switch(j = *poll) {
				case 2:
					pxi->pxi_pbuf_select = 4096;
					break;
				case 1:
					pxi->pxi_pbuf_select = 0;
					break;
				default:	
					if (j == 0x30)
						continue;
					break;
			}
			
			break;
		}
		
		if (j != 1 || j != 2) {
			/* i860 has gone mad, punish it */
			px_init_stic(pxi, 0);
			pxi->pxi_pbuf_select = 0;
		}
	}
#endif

	buf = (int32_t *)((u_long)pxi->pxi_rbuf + pxi->pxi_pbuf_select);
	pxi->pxi_pbuf_select ^= 4096;
	return buf;
}


/*
 * Send a PixelStamp packet.
 */
static int
px_send_packet(pxi, buf)
	struct px_info *pxi;
	int32_t *buf;
{
	volatile int32_t *poll;
	int c;

	if (pxi->pxi_flg & PX_ISR_ENABLE) {
		struct stic_regs *stic;

		pxi->pxi_lpw++;
		stic = pxi->pxi_stic;

		/* Generate a packet-done interrupt */
		c = stic->ipdvint;
		c |= (STIC_INT_P_EN | STIC_INT_P | STIC_INT_P_WE);
		c &= ~(STIC_INT_E_WE | STIC_INT_V_WE);
		stic->ipdvint = c;
		tc_wmb();
		return (0);
	}

	/* Convert buffer address to i860 physical address for PXG */ 
	if (pxi->pxi_option)
		buf = (int32_t *)(((long)buf-(long)pxi->pxi_rbuf) & ~0x40000);

	/* Get address of poll register for this buffer */
	poll = px_poll_addr((caddr_t)pxi->pxi_slotbase, buf);

	/*
	 * Now check the poll register and make sure the stamp wants to
	 * accept our packet. This read will initiate the DMA. Don't
	 * wait for ever, just in case something's wrong.
	 */
	tc_wmb();

	for (c = STAMP_RETRIES; c; c--) {
		if (*poll == STAMP_OK) {
#ifdef notdef
			/* Tell the i860 we are done */
			if (pxi->pxi_option) {
				poll = (volatile int32_t*)pxi->pxi_slotbase + 
				    (PXG_HOST_INTR_OFFSET >> 2);
				
				poll[0] = 0;
				tc_wmb();
				poll[2] = 0;
				tc_wmb();
			}
#endif
			return (0);
		}
		
		DELAY(STAMP_DELAY);
	}

	/*
	 * Oops... The STIC seems to have died. Reinitialize it and hope
	 * for the best. This is a non destructive operation anyhow, except
	 * for the packet that we just dropped on the floor.
	 */
	px_init_stic(pxi, 0);
	tc_wmb();
	return (-*poll);
}


/*
 * Draw a 'flat' rectangle
 */
static int
px_rect(pxi, x, y, w, h, color)
	struct px_info *pxi;
	int x, y, w, h, color;
{
	int *pb, linewidth;

	pb = px_alloc_pbuf(pxi);

	linewidth = (h << 2) - 1;
	y = (y << 3) + linewidth;

	pb[0] = STAMP_CMD_LINES | STAMP_RGB_CONST | STAMP_LW_PERPACKET;
	pb[1] = 0x01ffffff;
	pb[2] = 0;
	pb[3] = STAMP_UPDATE_ENABLE | STAMP_METHOD_COPY;
	pb[4] = linewidth;
	pb[5] = color;
	pb[6] = (x << 19) | y;
	pb[7] = ((((x + w) << 3) - 1) << 16) | y;

	return px_send_packet(pxi, pb);
}


/*
 * Allocate attribute. We just pack these into an integer.
 */
static int
px_alloc_attr(cookie, fg, bg, flags, attr)
	void *cookie;
	int fg, bg, flags;
	long *attr;
{
	int swap;

	if (flags & (WSATTR_BLINK | WSATTR_UNDERLINE))
		return (EINVAL);

	if (flags & WSATTR_HILIT)
		fg += 8;

	if (flags & WSATTR_REVERSE) {
		swap = fg;
		fg = bg;
		bg = swap;
	}

	*attr = fg | (bg << 8) | (flags << 16);
	return 0;
}


/*
 * Erase columns
 */
static void
px_erasecols(cookie, row, col, num, attr)
	void *cookie;
	int row, col, num;
	long attr;
{
	struct px_info *pxi = (struct px_info *)cookie;
	int32_t *pb;
	u_int linewidth;

	col = (col * pxi->pxi_font->fontwidth) << 19;
	num = (num * pxi->pxi_font->fontwidth) << 19;
	row = row * pxi->pxi_font->fontheight;

	pb = px_alloc_pbuf(pxi);

	linewidth = (pxi->pxi_font->fontheight << 2) - 1;
	row = (row << 3) + linewidth;

	pb[0] = STAMP_CMD_LINES | STAMP_RGB_CONST | STAMP_LW_PERPACKET;
	pb[1] = 0x01ffffff;
	pb[2] = 0;
	pb[3] = STAMP_UPDATE_ENABLE | STAMP_METHOD_COPY;
	pb[4] = linewidth;
	pb[5] = DUPBYTE1(attr);
	pb[6] = col | row;
	pb[7] = (col + num) | row;

	px_send_packet(pxi, pb);
}


/*
 * Erase rows
 */
static void
px_eraserows(cookie, row, num, attr)
	void *cookie;
	int row, num;
	long attr;
{
	struct px_info *pxi = (struct px_info *)cookie;
	int32_t *pb;
	int linewidth;

	row *= pxi->pxi_font->fontheight;
	num *= pxi->pxi_font->fontheight;

	pb = px_alloc_pbuf(pxi);

	linewidth = (num << 2) - 1;
	row = (row << 3) + linewidth;

	pb[0] = STAMP_CMD_LINES | STAMP_RGB_CONST | STAMP_LW_PERPACKET;
	pb[1] = 0x01ffffff;
	pb[2] = 0;
	pb[3] = STAMP_UPDATE_ENABLE | STAMP_METHOD_COPY;
	pb[4] = linewidth;
	pb[5] = DUPBYTE1(attr);
	pb[6] = row;
	pb[7] = (1280 << 19) | row;

	px_send_packet(pxi, pb);
}


/*
 * Copy rows.
 */
static void
px_copyrows(cookie, src, dst, height)
	void *cookie;
	int src, dst, height;
{
	struct px_info *pxi;
	int32_t *pb, *pbs;
	int num, inc, adj;

	pxi = (struct px_info *)cookie;

	/*
	 * We need to do this in reverse if the destination row is below
	 * the source.
	 */
	if (dst > src) {
		src += height;
		dst += height;
		inc = -8;
		adj = -1;
	} else {
		inc = 8;
		adj = 0;
	}

	src = (src * pxi->pxi_font->fontheight + adj) << 3;
	dst = (dst * pxi->pxi_font->fontheight + adj) << 3;
	height *= pxi->pxi_font->fontheight;

	while (height > 0) {
		num = (height < 255 ? height : 255);
		height -= num;

		pb = pbs = px_alloc_pbuf(pxi);

		/*
		 * We can use COPYSPAN_ALIGNED here to improve performance
		 * since the source and destination X co-ordinates are
		 * identical.
		 */
		*pb++ = STAMP_CMD_COPYSPANS | STAMP_LW_PERPACKET;
		*pb++ = (num << 24) | 0xffffff;
		*pb++ = 0x0;
		*pb++ = STAMP_UPDATE_ENABLE |
			STAMP_METHOD_COPY |
			STAMP_SPAN |
			STAMP_COPYSPAN_ALIGNED;
		*pb++ = 1; /* linewidth */

		for ( ; num > 0; num--, src += inc, dst += inc) {
			*pb++ = 1280 << 3;
			*pb++ = src;
			*pb++ = dst;
		}

	    	px_send_packet(pxi, pbs);
	}
}


/*
 * Copy columns.
 */
static void
px_copycols(cookie, row, src, dst, num)
	void *cookie;
	int row, src, dst, num;
{
	struct px_info *pxi = (struct px_info *)cookie;
	int32_t *pb, *pbs;
	int height, updword;

	/*
	 * Due the fact that the stamp reads and writes left->right only,
	 * we need to "half-buffer" if the source and destination regions
	 * overlap, and the source is left of the destination. This is
	 * slower than just a simple copy.
	 */
	updword = STAMP_UPDATE_ENABLE | STAMP_METHOD_COPY | STAMP_SPAN;

	if (src < dst && src + num > dst)
		updword |= STAMP_HALF_BUFF;

	row = (row * pxi->pxi_font->fontheight) << 3;
	num = (num * pxi->pxi_font->fontwidth) << 3;
	src = row | ((src * pxi->pxi_font->fontwidth) << 19);
	dst = row | ((dst * pxi->pxi_font->fontwidth) << 19);
	height = pxi->pxi_font->fontheight;

	pb = pbs = px_alloc_pbuf(pxi);

	*pb++ = STAMP_CMD_COPYSPANS | STAMP_LW_PERPACKET;
	*pb++ = (height << 24) | 0xffffff;
	*pb++ = 0x0;
	*pb++ = updword;
	*pb++ = 1; /* linewidth */

	for ( ; height; height--, src += 8, dst += 8) {
		*pb++ = num;
		*pb++ = src;
		*pb++ = dst;
	}

	px_send_packet(pxi, pbs);
}


/*
 * Blit a character at the specified co-ordinates.
 */
static void
px_putchar(cookie, r, c, uc, attr)
	void *cookie;
	int r, c;
	u_int uc;
	long attr;
{
	struct wsdisplay_font *font;
	struct px_info *pxi;
	int i, bgcolor, fgcolor;
	unsigned short *fr;	/* font row data */
	int	       *pb;	/* packet buffer ptr */
	int		v1;	/* mask co-ords */
	int		v2;	/* mask co-ords */
	int		xya;	/* saved XY position */

	/* Don't bother blitting the space character */
	if (uc == ' ') {
		px_erasecols(cookie, r, c, 1, attr);
		return;
	}

	pxi = (struct px_info *)cookie;
	font = pxi->pxi_font;
	pb = px_alloc_pbuf(pxi);

	pb[0] = STAMP_CMD_LINES |
		STAMP_RGB_FLAT |
		STAMP_XY_PERPRIMATIVE |
		STAMP_LW_PERPRIMATIVE;
	pb[2] = 0x0;
	pb[3] = STAMP_UPDATE_ENABLE | STAMP_WE_XYMASK | STAMP_METHOD_COPY;

	if (font->fontheight > 16)
		pb[1] = 0x04ffffff;
	else
		pb[1] = 0x02ffffff;

	r *= font->fontheight;
	c *= font->fontwidth;
	uc = (uc - font->firstchar) * pxi->pxi_fontscale;
	fr = (u_short *)((caddr_t)font->data + uc);
	bgcolor = DUPBYTE1(attr);
	fgcolor = DUPBYTE0(attr);
	
	i = (16 << 2) - 1;
	v1 = (c << 19) | ((r << 3) + i);
	v2 = ((c + font->fontwidth) << 19) | (v1 & 0xffff);
	xya = XYMASKADDR(c, r, 0, 0);

	pb[4] = PACK_WORD(fr, 0);
	pb[5] = PACK_WORD(fr, 2);
	pb[6] = PACK_WORD(fr, 4);
	pb[7] = PACK_WORD(fr, 6);
	pb[8] = PACK_WORD(fr, 8);
	pb[9] = PACK_WORD(fr, 10);
	pb[10] = PACK_WORD(fr, 12);
	pb[11] = PACK_WORD(fr, 14);
	pb[12] = xya;
	pb[13] = v1;
	pb[14] = v2;
	pb[15] = i;
	pb[16] = fgcolor;

	/* opaque background mask */
	pb[17] = ~pb[4];
	pb[18] = ~pb[5];
	pb[19] = ~pb[6];
	pb[20] = ~pb[7];
	pb[21] = ~pb[8];
	pb[22] = ~pb[9];
	pb[23] = ~pb[10];
	pb[24] = ~pb[11];
	pb[25] = xya;
	pb[26] = v1;
	pb[27] = v2;
	pb[28] = i;
	pb[29] = bgcolor;

	if (font->fontheight > 16) {
		i = ((font->fontheight - 16) << 2) - 1;
		r += 16;
		v1 = (c << 19) | ((r << 3) + i);
		v2 = ((c + font->fontwidth) << 19) | (v1 & 0xffff);

		/* lower part of fg character */
		pb[30] = PACK_WORD(fr, 16);
		pb[31] = PACK_WORD(fr, 18);
		pb[32] = PACK_WORD(fr, 20);
		pb[33] = PACK_WORD(fr, 22);
		pb[34] = PACK_WORD(fr, 24);
		pb[35] = PACK_WORD(fr, 26);
		pb[36] = PACK_WORD(fr, 28);
		pb[37] = PACK_WORD(fr, 30);
		pb[38] = xya;
		pb[39] = v1;
		pb[40] = v2;
		pb[41] = i;
		pb[42] = fgcolor;

		/* opaque background */
		pb[43] = ~pb[30];
		pb[44] = ~pb[31];
		pb[45] = ~pb[32];
		pb[46] = ~pb[33];
		pb[47] = ~pb[34];
		pb[48] = ~pb[35];
		pb[49] = ~pb[36];
		pb[50] = ~pb[37];
		pb[51] = xya;
		pb[52] = v1;
		pb[53] = v2;
		pb[54] = i;
		pb[55] = bgcolor;
	}

	px_send_packet(pxi, pb);
}


/*
 * Map a character.
 */
static int
px_mapchar(cookie, c, cp)
	void *cookie;
	int c;
	u_int *cp;
{
	struct px_info *pxi;

	pxi = (struct px_info *)cookie;

	if (c < pxi->pxi_font->firstchar) {
		*cp = ' ';
		return (0);
	}

	if (c - pxi->pxi_font->firstchar >= pxi->pxi_font->numchars) {
		*cp = ' ';
		return (0);
	}

	*cp = c;
	return (5);
}


/*
 * Position|{enable|disable} the cursor at the specified location.
 */
static void
px_cursor(cookie, on, row, col)
	void *cookie;
	int on, row, col;
{
	struct px_info *pxi;

	pxi = (struct px_info *)cookie;
#if 0
	if (row < 0)
		row = 0;
	else if (row > pxi->pxi_max_row)
		row = pxi->pxi_max_row;

	if (col < 0)
		col = 0;
	else if (col > pxi->pxi_max_col)
		col = pxi->pxi_max_col;
#endif
	/*
	 * These magic offsets (370, 37) were obtained by looking at sfb.c
	 * and then bumping them until they worked right.
	 */
	pxi->pxi_curx = col * pxi->pxi_font->fontwidth + 370;
	pxi->pxi_cury = row * pxi->pxi_font->fontheight + 37;

	if (on)
		pxi->pxi_flg |= PX_CURSOR_ENABLE;
	else
		pxi->pxi_flg &= ~PX_CURSOR_ENABLE;

	pxi->pxi_dirty |= (PX_DIRTY_CURSOR_ENABLE | PX_DIRTY_CURSOR_POS);

	if (pxi->pxi_option)
		px_bt459_flush(pxi);
}


/*
 * Move the cursor for qvss. This is disgusting.
 */
static void
px_cursor_hack(fi, x, y)
	struct fbinfo *fi;
	int x, y;
{
	struct px_info *pxi;

	pxi = (struct px_info *)fi->fi_base;
	pxi->pxi_curx = x + 370;
	pxi->pxi_cury = y + 37;
	pxi->pxi_dirty |= PX_DIRTY_CURSOR_POS;

	if (pxi->pxi_option)
		px_bt459_flush(pxi);
}


int
pxopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	extern struct fbinfo *firstfi;
	struct stic_regs *stic;
	struct px_info *pxi;
	int s;

	if (minor(dev) >= NPX || px_unit[minor(dev)] == NULL)
		return (ENXIO);

	pxi = px_unit[minor(dev)];

	if (pxi->pxi_flg & PX_OPEN)
		return (EBUSY);

	pxi->pxi_flg = (pxi->pxi_flg | PX_OPEN) & ~PX_CURSOR_ENABLE;
	pxi->pxi_flg &= ~PX_ISR_MASK;
	pxi->pxi_dirty |= PX_DIRTY_CURSOR_ENABLE;
	pxi->pxi_fbinfo.fi_open = 1;

	/*
	 * Set up event queue for later
	 */
	firstfi = &pxi->pxi_fbinfo; /* XXX */
	pmEventQueueInit(&pxi->pxi_fbuaccess.scrInfo.qe);
	genConfigMouse();

	/* Turn packet-done interrupts on */
	stic = pxi->pxi_stic;
	s = stic->ipdvint | STIC_INT_P_WE | STIC_INT_P_EN;
	s &= ~(STIC_INT_E_WE | STIC_INT_V_WE | STIC_INT_P);
	stic->ipdvint = s;
	tc_wmb();

	return (0);
}

int
pxclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	extern struct fbinfo *firstfi;
	struct stic_regs *stic;
	struct px_info *pxi;
	int s;

	if (minor(dev) >= NPX || px_unit[minor(dev)] == NULL)
		return (EBADF);

	pxi = px_unit[minor(dev)];

	if ((pxi->pxi_flg & PX_OPEN) == 0)
		return (EBADF);

	pxi->pxi_flg = (pxi->pxi_flg & ~PX_OPEN) | PX_ENABLE;
	pxi->pxi_fbinfo.fi_open = 0;
	genDeconfigMouse();
	firstfi = NULL;

	/* Disable interrupt driven operation */
	s = splhigh();
	pxi->pxi_lpw = 0;
	pxi->pxi_lpr = 0;
	pxi->pxi_flg &= ~PX_ISR_MASK;
	splx(s);

	/* Turn packet-done interrupts off */
	stic = pxi->pxi_stic;
	s = stic->ipdvint | STIC_INT_P_WE;
	s &= ~(STIC_INT_E_WE | STIC_INT_V_WE | STIC_INT_P_EN);
	stic->ipdvint = s;
	tc_wmb();

	px_init_stic(pxi, 0);
	px_bt459_init(pxi);
	px_rect(pxi, 0, 0, 1280, 1024, 0);
	pxi->pxi_dirty |= PX_DIRTY_ENABLE; /* make sure video is enabled */
	if (pxi->pxi_option)
		px_bt459_flush(pxi);
	return (0);
}

int
pxioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct pmax_fbtty *fbtty;
	struct px_info *pxi;
	pmKpCmd *kpCmdPtr;
	u_char *cp;
	u_int *ptr;
	int i;

	if (minor(dev) >= NPX || px_unit[minor(dev)] == NULL)
		return (EBADF);

	pxi = px_unit[minor(dev)];
	fbtty = pxi->pxi_fbinfo.fi_glasstty;

	if ((pxi->pxi_flg & PX_OPEN) == 0)
		return (EBADF);

	switch (cmd) {
	case QIOCGINFO:
		/*
		 * Map card info.
		 */
		return (px_mmap_info(p, dev, (vm_offset_t *)data));

	case QIOCPMSTATE:
		/*
		 * Set mouse state.
		 */
		pxi->pxi_curx = ((pmCursor *)data)->x + 370;
		pxi->pxi_cury = ((pmCursor *)data)->y + 37;
		pxi->pxi_dirty |= PX_DIRTY_CURSOR_POS;
		break;

	case QIOCINIT:
		px_rect(pxi, 0, 0, 1280, 1024, 0);
		break;

	case QIOVIDEOON:
		pxi->pxi_flg |= PX_ENABLE;
		pxi->pxi_dirty |= PX_DIRTY_ENABLE;
		break;

	case QIOVIDEOOFF:
		pxi->pxi_flg &= ~PX_ENABLE;
		pxi->pxi_dirty |= PX_DIRTY_ENABLE;
		break;

	case QIOWCURSOR:
		px_conv_cursor(pxi, (u_short *)data);
		pxi->pxi_flg |= PX_CURSOR_ENABLE;
		pxi->pxi_dirty |= PX_DIRTY_CURSOR | PX_DIRTY_CURSOR_ENABLE;
		break;

	case QIOWCURSORCOLOR:
		ptr = (u_int *)data;

		for (i = 0; i < 6; i++)
			pxi->pxi_curcmap[i] = *ptr++ >> 8;

		pxi->pxi_dirty |= PX_DIRTY_CURSOR_CMAP;
		break;

	case QIOSETCMAP:
		i = ((ColorMap *)data)->index;
		if (i < 0 || i > 255)
			return (-1);

		pxi->pxi_cmap_idx = i;
		pxi->pxi_cmap_cnt = 1;
		i = (i << 1) + i;
		pxi->pxi_cmap[i+0] = ((ColorMap *)data)->Entry.red;
		pxi->pxi_cmap[i+1] = ((ColorMap *)data)->Entry.green;
		pxi->pxi_cmap[i+2] = ((ColorMap *)data)->Entry.blue;
		pxi->pxi_dirty |= PX_DIRTY_CMAP;
		break;

	case QIOCKPCMD:
		/*
		 * Keyboard command.
		 */
		kpCmdPtr = (pmKpCmd *)data;
		if (kpCmdPtr->nbytes == 0)
			kpCmdPtr->cmd |= 0x80;
		(*fbtty->KBDPutc)(fbtty->kbddev, (int)kpCmdPtr->cmd);
		cp = &kpCmdPtr->par[0];
		for (; kpCmdPtr->nbytes > 0; cp++, kpCmdPtr->nbytes--) {
			if (kpCmdPtr->nbytes == 1)
				*cp |= 0x80;
			(*fbtty->KBDPutc)(fbtty->kbddev, (int)*cp);
		}
		break;

	case QIOKERNLOOP:
		genConfigMouse();
		break;

	case QIOKERNUNLOOP:
		genDeconfigMouse();
		break;

	default:
		printf("px%d: unknown ioctl %lx\n", minor(dev), cmd);
		return (EINVAL);
	}

	return (0);
}


int
pxpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct fbinfo *fi = &px_unit[minor(dev)]->pxi_fbinfo;
	int revents = 0;

	if (events & (POLLIN | POLLRDNORM)) {
		if (fi->fi_fbu->scrInfo.qe.eHead !=
		    fi->fi_fbu->scrInfo.qe.eTail)
		 	revents |= (events & (POLLIN | POLLRDNORM));
		else
	  		selrecord(p, &fi->fi_selp);
	}

	return (revents);
}

int
pxmmap(dev, off, prot)
	dev_t dev;
	int off, prot;
{
	struct px_info *pxi;

	if (minor(dev) >= NPX || px_unit[minor(dev)] == NULL)
		return (EBADF);

	pxi = px_unit[minor(dev)];

	if ((pxi->pxi_flg & PX_OPEN) == 0)
		return (EBADF);
	
	/* 
	 * STIC control registers
	 */	
	if (off < NBPG)
		return mips_btop(MIPS_KSEG1_TO_PHYS(pxi->pxi_stic) + off);
	off -= NBPG;
	
	/*
	 * STIC poll registers
	 */
	if (off < sizeof(int32_t) * 4096)
		return mips_btop(MIPS_KSEG1_TO_PHYS(pxi->pxi_poll) + off);
	off -= sizeof(int32_t) * 4096;

	/*
	 * 'struct px_info' and ringbuffer
	 */ 
	if (off < PXMAP_INFO_SIZE + PXMAP_RBUF_SIZE)
		return mips_btop(MIPS_KSEG1_TO_PHYS(pxi) + off);
	off -= (PXMAP_INFO_SIZE + PXMAP_RBUF_SIZE);

	return (-1);
}


/*
 * mmap info struct for this card into userspace.
 */
static int
px_mmap_info (p, dev, va)
	struct proc *p;
	dev_t dev;
	vm_offset_t *va;
{
	struct specinfo si;
	struct vnode vn;
	vm_prot_t prot;
	vm_size_t size;
	int flags;

	vn.v_type = VCHR;			/* XXX */
	vn.v_specinfo = &si;			/* XXX */
	vn.v_rdev = dev;			/* XXX */

	size = sizeof(struct px_map);
	prot = VM_PROT_READ | VM_PROT_WRITE;
	flags = MAP_SHARED | MAP_FILE;
	*va = round_page(p->p_vmspace->vm_taddr + MAXTSIZ + MAXDSIZ);
	return uvm_mmap(&p->p_vmspace->vm_map, va, size, prot,
	    VM_PROT_ALL, flags, (caddr_t)&vn, 0,
	    p->p_rlimit[RLIMIT_MEMLOCK].rlim_cur);
}
