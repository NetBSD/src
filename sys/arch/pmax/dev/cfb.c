/*	$NetBSD: cfb.c,v 1.9 1995/08/29 09:56:23 jonathan Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)cfb.c	8.1 (Berkeley) 6/10/93
 */

/*
 *  devGraphics.c --
 *
 *     	This file contains machine-dependent routines for the graphics device.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/dev/ds3100.md/RCS/devGraphics.c,
 *	v 9.2 90/02/13 22:16:24 shirriff Exp  SPRITE (DECWRL)";
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <cfb.h>
#if NCFB > 0
#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/mman.h>

#include <vm/vm.h>

#include <machine/machConst.h>
#include <machine/pmioctl.h>

#include <pmax/pmax/maxine.h>
#include <pmax/pmax/cons.h>
#include <pmax/pmax/pmaxtype.h>

#include <pmax/dev/device.h>
#include <pmax/dev/cfbreg.h>
#include <pmax/dev/fbreg.h>

#include <dc.h>
#include <dtop.h>
#include <scc.h>

#include <sys/device.h>
#include <machine/autoconf.h>

#define PMAX	/* enable /dev/pm compatibility */

/*
 * These need to be mapped into user space.
 */
struct fbuaccess cfbu;
struct pmax_fb cfbfb;

/*
 * Forward references.
 */
extern struct cfdriver cfb;

static void cfbScreenInit __P(());
static void cfbLoadCursor __P((u_short *ptr));
static void cfbRestoreCursorColor __P(());
static void cfbCursorColor __P((u_int *color));
void cfbPosCursor __P((int x, int y));
static void cfbInitColorMap __P(());
static void cfbLoadColorMap __P ((ColorMap *ptr));
static void bt459_set_cursor_ram(), bt459_video_on(), bt459_video_off();
static void bt459_select_reg(), bt459_write_reg();
static u_char bt459_read_reg();
static void cfbConfigMouse(), cfbDeconfigMouse();


void cfbKbdEvent(), cfbMouseEvent(), cfbMouseButtons();
#if NDC > 0
extern void (*dcDivertXInput)();
extern void (*dcMouseEvent)();
extern void (*dcMouseButtons)();
#endif
#if NSCC > 0
extern void (*sccDivertXInput)();
extern void (*sccMouseEvent)();
extern void (*sccMouseButtons)();
#endif
#if NDTOP > 0
extern void (*dtopDivertXInput)();
extern void (*dtopMouseEvent)();
extern void (*dtopMouseButtons)();
#endif
extern int pmax_boardtype;
extern u_short defCursor[32];
extern struct consdev cn_tab;
extern void framebuffer_in_slot();


#define	CFB_OFFSET_VRAM		0x0		/* from module's base */
#define CFB_OFFSET_BT459	0x200000	/* Bt459 registers */
#define CFB_OFFSET_IREQ		0x300000	/* Interrupt req. control */
#define CFB_OFFSET_ROM		0x380000	/* Diagnostic ROM */
#define CFB_OFFSET_RESET	0x3c0000	/* Bt459 resets on writes */
#define CFB_FB_SIZE		0x100000	/* frame buffer size */

/*
 * Autoconfiguration data for config.new.
 * Use static-sized softc until config.old and old autoconfig
 * code is completely gone.
 */

int cfbmatch __P((struct device *, void *, void *));
void cfbattach __P((struct device *, struct device *, void *));

struct cfdriver cfbcd = {
	NULL, "cfb", cfbmatch, cfbattach, DV_DULL, sizeof(struct device), 0
};

int
cfbmatch(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	struct cfdata *cf = match;
	struct confargs *ca = aux;
	static int ncfbs = 1;

	/* make sure that we're looking for this type of device. */
	if (!BUS_MATCHNAME(ca, "PMAG-BA "))
		return (0);

#ifdef notyet
	/* if it can't have the one mentioned, reject it */
	if (cf->cf_unit >= ncfbs)
		return (0);
#endif
	return (1);
}

/*
 * The original TURBOChannel cfb interrupts on every vertical
 * retrace, and we can't disable the board from requesting those
 * interrupts.  The 4.4BSD kernel never enabled those interrupts;
 * but there's a kernel design bug on the 3MIN, where the cfb
 * interrupts at spl0, spl1, or spl2.
 */
void
cfb_intr(int unit)
{
	register struct pmax_fb *fp=  &cfbfb;
	char *slot_addr = (((char *)fp->fr_addr) - CFB_OFFSET_VRAM);
	
	/* reset vertical-retrace interrupt by writing a dont-care */
	*(int*) (slot_addr+CFB_OFFSET_IREQ) = 0;
}

void
cfbattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct confargs *ca = aux;

	if (!cfbinit(BUS_CVTADDR(ca)))
		return;

	framebuffer_in_slot(ca->ca_slotpri);

	/* no interrupts for CFB */
	/*BUS_INTR_ESTABLISH(ca, sccintr, self->dv_unit);*/
	/* Vertical-retrace interrupts aren't honoured, except on 3MIN XXX */
	if (pmax_boardtype == DS_3MIN) {
		BUS_INTR_ESTABLISH(ca, cfb_intr, self->dv_unit);
	}
	printf("\n");
}

/*ARGSUSED*/
cfbopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct pmax_fb *fp = &cfbfb;
	int s;

	if (!fp->initialized)
		return (ENXIO);
	if (fp->GraphicsOpen)
		return (EBUSY);

	fp->GraphicsOpen = 1;
	cfbInitColorMap();
	/*
	 * Set up event queue for later
	 */
	fp->fbu->scrInfo.qe.eSize = PM_MAXEVQ;
	fp->fbu->scrInfo.qe.eHead = fp->fbu->scrInfo.qe.eTail = 0;
	fp->fbu->scrInfo.qe.tcSize = MOTION_BUFFER_SIZE;
	fp->fbu->scrInfo.qe.tcNext = 0;
	fp->fbu->scrInfo.qe.timestamp_ms = TO_MS(time);
	cfbConfigMouse();
	return (0);
}

/*ARGSUSED*/
cfbclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct pmax_fb *fp = &cfbfb;
	int s;

	if (!fp->GraphicsOpen)
		return (EBADF);

	fp->GraphicsOpen = 0;
	cfbInitColorMap();
	cfbDeconfigMouse();
	cfbScreenInit();
	bzero((caddr_t)fp->fr_addr, 1024 * 864);
	cfbPosCursor(fp->col * 8, fp->row * 15);
	return (0);
}

/*ARGSUSED*/
cfbioctl(dev, cmd, data, flag, p)
	dev_t dev;
	caddr_t data;
	struct proc *p;
{
	register struct pmax_fb *fp = &cfbfb;
	int s;

	switch (cmd) {
	case QIOCGINFO:
		return (fbmmap(fp, dev, data, p));

	case QIOCPMSTATE:
		/*
		 * Set mouse state.
		 */
		fp->fbu->scrInfo.mouse = *(pmCursor *)data;
		cfbPosCursor(fp->fbu->scrInfo.mouse.x, fp->fbu->scrInfo.mouse.y);
		break;

	case QIOCINIT:
		/*
		 * Initialize the screen.
		 */
		cfbScreenInit();
		break;

	case QIOCKPCMD:
	    {
		pmKpCmd *kpCmdPtr;
		unsigned char *cp;

		kpCmdPtr = (pmKpCmd *)data;
		if (kpCmdPtr->nbytes == 0)
			kpCmdPtr->cmd |= 0x80;
		if (!fp->GraphicsOpen)
			kpCmdPtr->cmd |= 1;
		(*fp->KBDPutc)(fp->kbddev, (int)kpCmdPtr->cmd);
		cp = &kpCmdPtr->par[0];
		for (; kpCmdPtr->nbytes > 0; cp++, kpCmdPtr->nbytes--) {
			if (kpCmdPtr->nbytes == 1)
				*cp |= 0x80;
			(*fp->KBDPutc)(fp->kbddev, (int)*cp);
		}
		break;
	    }

	case QIOCADDR:
		*(PM_Info **)data = &fp->fbu->scrInfo;
		break;

	case QIOWCURSOR:
		cfbLoadCursor((unsigned short *)data);
		break;

	case QIOWCURSORCOLOR:
		cfbCursorColor((unsigned int *)data);
		break;

	case QIOSETCMAP:
		cfbLoadColorMap((ColorMap *)data);
		break;

	case QIOKERNLOOP:
		cfbConfigMouse();
		break;

	case QIOKERNUNLOOP:
		cfbDeconfigMouse();
		break;

	case QIOVIDEOON:
		cfbRestoreCursorColor();
		bt459_video_on();
		break;

	case QIOVIDEOOFF:
		bt459_video_off();
		break;

	default:
		printf("cfb0: Unknown ioctl command %x\n", cmd);
		return (EINVAL);
	}
	return (0);
}

cfbselect(dev, flag, p)
	dev_t dev;
	int flag;
	struct proc *p;
{
	struct pmax_fb *fp = &cfbfb;

	switch (flag) {
	case FREAD:
		if (fp->fbu->scrInfo.qe.eHead != fp->fbu->scrInfo.qe.eTail)
			return (1);
		selrecord(p, &fp->selp);
		break;
	}

	return (0);
}

/*
 * Return the physical page number that corresponds to byte offset 'off'.
 */
/*ARGSUSED*/
cfbmmap(dev, off, prot)
	dev_t dev;
{
	int len;

	len = pmax_round_page(((vm_offset_t)&cfbu & PGOFSET) + sizeof(cfbu));
	if (off < len)
		return pmax_btop(MACH_CACHED_TO_PHYS(&cfbu) + off);
	off -= len;
	if (off >= cfbfb.fr_size)
		return (-1);
	return pmax_btop(MACH_UNCACHED_TO_PHYS(cfbfb.fr_addr) + off);
}

static u_char	cursor_RGB[6];	/* cursor color 2 & 3 */

/*
 * XXX This assumes 2bits/cursor pixel so that the 1Kbyte cursor RAM
 * defines a 64x64 cursor. If the bt459 does not map the cursor RAM
 * this way, this code is Screwed!
 */
static void
cfbLoadCursor(cursor)
	u_short *cursor;
{
#ifdef PMAX
	register int i, j, k, pos;
	register u_short ap, bp, out;

	/*
	 * Fill in the cursor sprite using the A and B planes, as provided
	 * for the pmax.
	 * XXX This will have to change when the X server knows that this
	 * is not a pmax display.
	 */
	pos = 0;
	for (k = 0; k < 16; k++) {
		ap = *cursor;
		bp = *(cursor + 16);
		j = 0;
		while (j < 4) {
			out = 0;
			for (i = 0; i < 4; i++) {
#ifndef CURSOR_EB
				out = (out << 2) | ((ap & 0x1) << 1) |
					(bp & 0x1);
#else
				out = ((out >> 2) & 0x3f) |
					((ap & 0x1) << 7) |
					((bp & 0x1) << 6);
#endif
				ap >>= 1;
				bp >>= 1;
			}
			bt459_set_cursor_ram(pos, out);
			pos++;
			j++;
		}
		while (j < 16) {
			bt459_set_cursor_ram(pos, 0);
			pos++;
			j++;
		}
		cursor++;
	}
	while (pos < 1024) {
		bt459_set_cursor_ram(pos, 0);
		pos++;
	}
#endif /* PMAX */
}

/*
 * Set a cursor ram value.
 */
static void
bt459_set_cursor_ram(pos, val)
	int pos;
	register u_char val;
{
	register bt459_regmap_t *regs = (bt459_regmap_t *)
		(cfbfb.fr_addr + CFB_OFFSET_BT459);
	register int cnt;
	u_char nval;

	cnt = 0;
	do {
		bt459_write_reg(regs, BT459_REG_CRAM_BASE + pos, val);
		nval = bt459_read_reg(regs, BT459_REG_CRAM_BASE + pos);
	} while (val != nval && ++cnt < 10);
}

/*
 * Generic register access
 */
static void
bt459_select_reg(regs, regno)
	bt459_regmap_t *regs;
{
	regs->addr_lo = regno;
	regs->addr_hi = regno >> 8;
	MachEmptyWriteBuffer();
}

static void
bt459_write_reg(regs, regno, val)
	bt459_regmap_t *regs;
{
	regs->addr_lo = regno;
	regs->addr_hi = regno >> 8;
	MachEmptyWriteBuffer();
	regs->addr_reg = val;
	MachEmptyWriteBuffer();
}

static u_char
bt459_read_reg(regs, regno)
	bt459_regmap_t *regs;
{
	regs->addr_lo = regno;
	regs->addr_hi = regno >> 8;
	MachEmptyWriteBuffer();
	return (regs->addr_reg);
}

/*
 * Initialization
 */
int
cfbinit(cp)
	char *cp;
{
	register bt459_regmap_t *regs;
	register struct pmax_fb *fp = &cfbfb;

	/* check for no frame buffer */
	if (badaddr(cp, 4))
		return (0);

	fp->isMono = 0;
	fp->fr_addr = (char *)(cp + CFB_OFFSET_VRAM);
	fp->fr_size = CFB_FB_SIZE;
	/*
	 * Must be in Uncached space since the fbuaccess structure is
	 * mapped into the user's address space uncached.
	 */
	fp->fbu = (struct fbuaccess *)
		MACH_PHYS_TO_UNCACHED(MACH_CACHED_TO_PHYS(&cfbu));
	fp->posCursor = cfbPosCursor;
	if (tb_kbdmouseconfig(fp))
		return (0);

	/*
	 * Initialize the screen.
	 */
	regs = (bt459_regmap_t *)(fp->fr_addr + CFB_OFFSET_BT459);

	if (bt459_read_reg(regs, BT459_REG_ID) != 0x4a)
		return (0);

	/* Reset the chip */
	*(volatile int *)(fp->fr_addr + CFB_OFFSET_RESET) = 0;
	DELAY(2000);	/* ???? check right time on specs! ???? */

	/* use 4:1 input mux */
	bt459_write_reg(regs, BT459_REG_CMD0, 0x40);

	/* no zooming, no panning */
	bt459_write_reg(regs, BT459_REG_CMD1, 0x00);

	/*
	 * signature test, X-windows cursor, no overlays, SYNC* PLL,
	 * normal RAM select, 7.5 IRE pedestal, do sync
	 */
#ifndef PMAX
	bt459_write_reg(regs, BT459_REG_CMD2, 0xc2);
#else /* PMAX */
	bt459_write_reg(regs, BT459_REG_CMD2, 0xc0);
#endif /* PMAX */

	/* get all pixel bits */
	bt459_write_reg(regs, BT459_REG_PRM, 0xff);

	/* no blinking */
	bt459_write_reg(regs, BT459_REG_PBM, 0x00);

	/* no overlay */
	bt459_write_reg(regs, BT459_REG_ORM, 0x00);

	/* no overlay blink */
	bt459_write_reg(regs, BT459_REG_OBM, 0x00);

	/* no interleave, no underlay */
	bt459_write_reg(regs, BT459_REG_ILV, 0x00);

	/* normal operation, no signature analysis */
	bt459_write_reg(regs, BT459_REG_TEST, 0x00);

	/*
	 * no blinking, 1bit cross hair, XOR reg&crosshair,
	 * no crosshair on either plane 0 or 1,
	 * regular cursor on both planes.
	 */
	bt459_write_reg(regs, BT459_REG_CCR, 0xc0);

	/* home cursor */
	bt459_write_reg(regs, BT459_REG_CXLO, 0x00);
	bt459_write_reg(regs, BT459_REG_CXHI, 0x00);
	bt459_write_reg(regs, BT459_REG_CYLO, 0x00);
	bt459_write_reg(regs, BT459_REG_CYHI, 0x00);

	/* no crosshair window */
	bt459_write_reg(regs, BT459_REG_WXLO, 0x00);
	bt459_write_reg(regs, BT459_REG_WXHI, 0x00);
	bt459_write_reg(regs, BT459_REG_WYLO, 0x00);
	bt459_write_reg(regs, BT459_REG_WYHI, 0x00);
	bt459_write_reg(regs, BT459_REG_WWLO, 0x00);
	bt459_write_reg(regs, BT459_REG_WWHI, 0x00);
	bt459_write_reg(regs, BT459_REG_WHLO, 0x00);
	bt459_write_reg(regs, BT459_REG_WHHI, 0x00);

	/*
	 * Initialize screen info.
	 */
	fp->fbu->scrInfo.max_row = 56;
	fp->fbu->scrInfo.max_col = 80;
	fp->fbu->scrInfo.max_x = 1024;
	fp->fbu->scrInfo.max_y = 864;
	fp->fbu->scrInfo.max_cur_x = 1023;
	fp->fbu->scrInfo.max_cur_y = 863;
	fp->fbu->scrInfo.version = 11;
	fp->fbu->scrInfo.mthreshold = 4;
	fp->fbu->scrInfo.mscale = 2;
	fp->fbu->scrInfo.min_cur_x = 0;
	fp->fbu->scrInfo.min_cur_y = 0;
	fp->fbu->scrInfo.qe.timestamp_ms = TO_MS(time);
	fp->fbu->scrInfo.qe.eSize = PM_MAXEVQ;
	fp->fbu->scrInfo.qe.eHead = fp->fbu->scrInfo.qe.eTail = 0;
	fp->fbu->scrInfo.qe.tcSize = MOTION_BUFFER_SIZE;
	fp->fbu->scrInfo.qe.tcNext = 0;

	/*
	 * Initialize the color map, the screen, and the mouse.
	 */
	cfbInitColorMap();
	cfbScreenInit();
	fbScroll(fp);

	fp->initialized = 1;
	if (cn_tab.cn_fb == (struct pmax_fb *)0)
		cn_tab.cn_fb = fp;
	return (1);
}

/*
 * ----------------------------------------------------------------------------
 *
 * cfbScreenInit --
 *
 *	Initialize the screen.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The screen is initialized.
 *
 * ----------------------------------------------------------------------------
 */
static void
cfbScreenInit()
{
	register struct pmax_fb *fp = &cfbfb;

	/*
	 * Home the cursor.
	 * We want an LSI terminal emulation.  We want the graphics
	 * terminal to scroll from the bottom. So start at the bottom.
	 */
	fp->row = 55;
	fp->col = 0;

	/*
	 * Load the cursor with the default values
	 *
	 */
	cfbLoadCursor(defCursor);
}

/*
 * ----------------------------------------------------------------------------
 *
 * RestoreCursorColor --
 *
 *	Routine to restore the color of the cursor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
static void
cfbRestoreCursorColor()
{
	bt459_regmap_t *regs = (bt459_regmap_t *)(cfbfb.fr_addr + CFB_OFFSET_BT459);
	register int i;

#ifndef PMAX
	bt459_select_reg(regs, BT459_REG_CCOLOR_2);
	for (i = 0; i < 6; i++) {
		regs->addr_reg = cursor_RGB[i];
		MachEmptyWriteBuffer();
	}
#else /* PMAX */
	bt459_select_reg(regs, BT459_REG_CCOLOR_1);
	for (i = 0; i < 3; i++) {
		regs->addr_reg = cursor_RGB[i];
		MachEmptyWriteBuffer();
	}
	bt459_select_reg(regs, BT459_REG_CCOLOR_3);
	for (i = 3; i < 6; i++) {
		regs->addr_reg = cursor_RGB[i];
		MachEmptyWriteBuffer();
	}
#endif /* PMAX */
}

/*
 * ----------------------------------------------------------------------------
 *
 * CursorColor --
 *
 *	Set the color of the cursor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
static void
cfbCursorColor(color)
	unsigned int color[];
{
	register int i, j;

	for (i = 0; i < 6; i++)
		cursor_RGB[i] = (u_char)(color[i] >> 8);

	cfbRestoreCursorColor();
}

/*
 *----------------------------------------------------------------------
 *
 * PosCursor --
 *
 *	Postion the cursor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
cfbPosCursor(x, y)
	register int x, y;
{
	bt459_regmap_t *regs = (bt459_regmap_t *)(cfbfb.fr_addr + CFB_OFFSET_BT459);
	register struct pmax_fb *fp = &cfbfb;

	if (y < fp->fbu->scrInfo.min_cur_y || y > fp->fbu->scrInfo.max_cur_y)
		y = fp->fbu->scrInfo.max_cur_y;
	if (x < fp->fbu->scrInfo.min_cur_x || x > fp->fbu->scrInfo.max_cur_x)
		x = fp->fbu->scrInfo.max_cur_x;
	fp->fbu->scrInfo.cursor.x = x;		/* keep track of real cursor */
	fp->fbu->scrInfo.cursor.y = y;		/* position, indep. of mouse */

	x += 219;
	y += 34;

	bt459_select_reg(regs, BT459_REG_CXLO);
	regs->addr_reg = x;
	MachEmptyWriteBuffer();
	regs->addr_reg = x >> 8;
	MachEmptyWriteBuffer();
	regs->addr_reg = y;
	MachEmptyWriteBuffer();
	regs->addr_reg = y >> 8;
	MachEmptyWriteBuffer();
}

/*
 * ----------------------------------------------------------------------------
 *
 * InitColorMap --
 *
 *	Initialize the color map.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The colormap is initialized appropriately.
 *
 * ----------------------------------------------------------------------------
 */
static void
cfbInitColorMap()
{
	bt459_regmap_t *regs = (bt459_regmap_t *)(cfbfb.fr_addr + CFB_OFFSET_BT459);
	register int i;

	bt459_select_reg(regs, 0);
	regs->addr_cmap = 0; MachEmptyWriteBuffer();
	regs->addr_cmap = 0; MachEmptyWriteBuffer();
	regs->addr_cmap = 0; MachEmptyWriteBuffer();

	for (i = 1; i < 256; i++) {
		regs->addr_cmap = 0xff; MachEmptyWriteBuffer();
		regs->addr_cmap = 0xff; MachEmptyWriteBuffer();
		regs->addr_cmap = 0xff; MachEmptyWriteBuffer();
	}

	for (i = 0; i < 3; i++) {
		cursor_RGB[i] = 0x00;
		cursor_RGB[i + 3] = 0xff;
	}
	cfbRestoreCursorColor();
}

/*
 * ----------------------------------------------------------------------------
 *
 * LoadColorMap --
 *
 *	Load the color map.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The color map is loaded.
 *
 * ----------------------------------------------------------------------------
 */
static void
cfbLoadColorMap(ptr)
	ColorMap *ptr;
{
	bt459_regmap_t *regs = (bt459_regmap_t *)(cfbfb.fr_addr + CFB_OFFSET_BT459);

	if (ptr->index > 256)
		return;

	bt459_select_reg(regs, ptr->index);

	regs->addr_cmap = ptr->Entry.red; MachEmptyWriteBuffer();
	regs->addr_cmap = ptr->Entry.green; MachEmptyWriteBuffer();
	regs->addr_cmap = ptr->Entry.blue; MachEmptyWriteBuffer();
}

/*
 * Video on/off state.
 */
static struct vstate {
	u_char	color0[3];	/* saved color map entry zero */
	u_char	off;		/* TRUE if display is off */
} vstate;

/*
 * ----------------------------------------------------------------------------
 *
 * bt459_video_on
 *
 *	Enable the video display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The display is enabled.
 *
 * ----------------------------------------------------------------------------
 */
static void
bt459_video_on()
{
	bt459_regmap_t *regs = (bt459_regmap_t *)(cfbfb.fr_addr + CFB_OFFSET_BT459);

	if (!vstate.off)
		return;

	/* restore old color map entry zero */
	bt459_select_reg(regs, 0);
	regs->addr_cmap = vstate.color0[0];
	MachEmptyWriteBuffer();
	regs->addr_cmap = vstate.color0[1];
	MachEmptyWriteBuffer();
	regs->addr_cmap = vstate.color0[2];
	MachEmptyWriteBuffer();

	/* enable normal display */
	bt459_write_reg(regs, BT459_REG_PRM, 0xff);
	bt459_write_reg(regs, BT459_REG_CCR, 0xc0);

	vstate.off = 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * bt459_video_off
 *
 *	Disable the video display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The display is disabled.
 *
 * ----------------------------------------------------------------------------
 */
static void
bt459_video_off()
{
	bt459_regmap_t *regs = (bt459_regmap_t *)(cfbfb.fr_addr + CFB_OFFSET_BT459);

	if (vstate.off)
		return;

	/* save old color map entry zero */
	bt459_select_reg(regs, 0);
	vstate.color0[0] = regs->addr_cmap;
	vstate.color0[1] = regs->addr_cmap;
	vstate.color0[2] = regs->addr_cmap;

	/* set color map entry zero to zero */
	bt459_select_reg(regs, 0);
	regs->addr_cmap = 0;
	MachEmptyWriteBuffer();
	regs->addr_cmap = 0;
	MachEmptyWriteBuffer();
	regs->addr_cmap = 0;
	MachEmptyWriteBuffer();

	/* disable display */
	bt459_write_reg(regs, BT459_REG_PRM, 0);
	bt459_write_reg(regs, BT459_REG_CCR, 0);

	vstate.off = 1;
}

/*
 * cfb keyboard and mouse input. Just punt to the generic ones in fb.c
 */
void
cfbKbdEvent(ch)
	int ch;
{
	fbKbdEvent(ch, &cfbfb);
}

void
cfbMouseEvent(newRepPtr)
	MouseReport *newRepPtr;
{
	fbMouseEvent(newRepPtr, &cfbfb);
}

void
cfbMouseButtons(newRepPtr)
	MouseReport *newRepPtr;
{
	fbMouseButtons(newRepPtr, &cfbfb);
}

/*
 * Configure the mouse and keyboard based on machine type
 */
static void
cfbConfigMouse()
{
	int s;

	s = spltty();
	switch (pmax_boardtype) {
#if NDC > 0
	case DS_3MAX:
		dcDivertXInput = cfbKbdEvent;
		dcMouseEvent = cfbMouseEvent;
		dcMouseButtons = cfbMouseButtons;
		break;
#endif
#if NSCC > 1
	case DS_3MIN:
	case DS_3MAXPLUS:
		sccDivertXInput = cfbKbdEvent;
		sccMouseEvent = cfbMouseEvent;
		sccMouseButtons = cfbMouseButtons;
		break;
#endif
#if NDTOP > 0
	case DS_MAXINE:
		dtopDivertXInput = cfbKbdEvent;
		dtopMouseEvent = cfbMouseEvent;
		dtopMouseButtons = cfbMouseButtons;
		break;
#endif
	default:
		printf("Can't configure mouse/keyboard\n");
	};
	splx(s);
}

/*
 * and deconfigure them
 */
static void
cfbDeconfigMouse()
{
	int s;

	s = spltty();
	switch (pmax_boardtype) {
#if NDC > 0
	case DS_3MAX:
		dcDivertXInput = (void (*)())0;
		dcMouseEvent = (void (*)())0;
		dcMouseButtons = (void (*)())0;
		break;
#endif
#if NSCC > 1
	case DS_3MIN:
	case DS_3MAXPLUS:
		sccDivertXInput = (void (*)())0;
		sccMouseEvent = (void (*)())0;
		sccMouseButtons = (void (*)())0;
		break;
#endif
#if NDTOP > 0
	case DS_MAXINE:
		dtopDivertXInput = (void (*)())0;
		dtopMouseEvent = (void (*)())0;
		dtopMouseButtons = (void (*)())0;
		break;
#endif
	default:
		printf("Can't deconfigure mouse/keyboard\n");
	};
}
#endif /* NCFB */
