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
 *	from: @(#)sfb.c	8.1 (Berkeley) 6/10/93
 *      $Id: sfb.c,v 1.1 1995/04/11 10:21:51 mellon Exp $
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
 * $Id: sfb.c,v 1.1 1995/04/11 10:21:51 mellon Exp $
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

#include <sfb.h>
#if NSFB > 0
#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/mman.h>
#include <sys/tty.h>

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

#define PMAX	/* enable /dev/pm compatibility */

/*
 * These need to be mapped into user space.
 */
struct fbuaccess sfbu [NSFB];
struct pmax_fb sfbfb [NSFB];

/*
 * Forward references.
 */

int sfbprobe (struct pmax_ctlr *);
int sfbopen (dev_t, int);
int sfbclose (dev_t, int);
int sfbioctl (dev_t, int, caddr_t, int, struct proc *);
int sfbselect (dev_t, int, struct proc *);
int sfbmap (dev_t, int, int);
static void sfbLoadCursor (u_short *, struct pmax_fb *);
static void bt459_set_cursor_ram (int, u_char, struct pmax_fb *);
static void bt459_select_reg (bt459_regmap_t *, int);
static void bt459_write_reg (bt459_regmap_t *, int, int);
static u_char bt459_read_reg (bt459_regmap_t *, int);
int sfbinit (char *, int);
static void sfbScreenInit (struct pmax_fb *);
static void sfbRestoreCursorColor (struct pmax_fb *);
static void sfbCursorColor (unsigned int [], struct pmax_fb *);
void sfbPosCursor (int, int, struct pmax_fb *);
static void sfbInitColorMap (struct pmax_fb *);
static void sfbLoadColorMap (ColorMap *, struct pmax_fb *);
static void bt459_video_on (struct pmax_fb *);
static void bt459_video_off (struct pmax_fb *);
void sfbKbdEvent (int);
void sfbMouseEvent (MouseReport *);
void sfbMouseButtons (MouseReport *);
static void sfbConfigMouse (void);
static void sfbDeconfigMouse (void);

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
/* extern u_short defCursor[32]; */

int	sfbprobe();
struct	driver sfbdriver = {
	"sfb", sfbprobe, 0, 0,
};

#define	SFB_OFFSET_VRAM		0x201000	/* from module's base */
#define SFB_OFFSET_BT459	0x1C0000	/* Bt459 registers */
#define SFB_ASIC_OFFSET		0x100000	/* SFB ASIC registers... */
#define  SFB_COPY_REG0		0x100000	/* Copy Buffer Register0 */
#define  SFB_COPY_REG1		0x100004	/* Copy Buffer Register1 */
#define  SFB_COPY_REG2		0x100008	/* Copy Buffer Register2 */
#define  SFB_COPY_REG3		0x10000C	/* Copy Buffer Register3 */
#define  SFB_COPY_REG4		0x100010	/* Copy Buffer Register4 */
#define  SFB_COPY_REG5		0x100014	/* Copy Buffer Register5 */
#define  SFB_COPY_REG6		0x100018	/* Copy Buffer Register6 */
#define  SFB_COPY_REG7		0x10001C	/* Copy Buffer Register7 */
#define  SFB_FOREGROUND		0x100020	/* Foreground */
#define  SFB_BACKGROUND		0x100024	/* Background */
#define  SFB_PLANEMASK		0x100028	/* PlaneMask */
#define  SFB_PIXELMASK		0x10002C	/* PixelMask Register */
#define  SFB_MODE		0x100030	/* Mode Register */
#define  SFB_BOOL_OP		0x100034	/* Boolean Op Register */
#define  SFB_PIXELSHIFT		0x100038	/* PixelShift Register */
#define  SFB_ADDRESS		0x10003C	/* Address Register */
#define  SFB_BRESENHAM1		0x100040	/* Bresenham Register 1 */
#define  SFB_BRESENHAM2		0x100044	/* Bresenham Register 2 */
#define  SFB_BRESENHAM3		0x100048	/* Bresenham Register 3 */
#define  SFB_BCONT		0x10004C	/* BCont */
#define  SFB_DEEP		0x100050	/* Deep Register */
#define  SFB_START		0x100054	/* Start Register */
#define  SFB_CLEAR		0x100058	/* Clear Interrupt */
#define  SFB_VREFRESH_COUNT	0x100060	/* Video Refresh Count */
#define  SFB_VHORIZONTAL	0x100064	/* Video Horizontal Setup */
#define  SFB_VVERTICAL		0x100068	/* Video Vertical Setup */
#define  SFB_VBASE		0x10006C	/* Video Base Address */
#define  SFB_VVALID		0x100070	/* Video Valid */
#define  SFB_INTERRUPT_ENABLE	0x100074	/* Enable/disable interrupts */
#define  SFB_TCCLK		0x100078	/* TCCLK count */
#define  SFB_VIDCLK		0x10007C	/* VIDCLK count */
#define SFB_FB_SIZE		0x1FF000	/* frame buffer size */

/* No analog (from cfb.c) */
/* #define SFB_OFFSET_IREQ	0x300000	/* Interrupt req. control */
/* #define SFB_OFFSET_ROM	0x100000	/* Diagnostic ROM */
/* #define SFB_OFFSET_RESET	0x3c0000	/* Bt459 resets on writes */

/*
 * Test to see if device is present.
 * Return true if found and initialized ok.
 */
/*ARGSUSED*/
sfbprobe(cp)
	register struct pmax_ctlr *cp;
{
	register struct pmax_fb *fp = &sfbfb [cp -> pmax_unit];

	if (cp -> pmax_unit > NSFB)
		return (0);
	if (!fp->initialized && !sfbinit(cp -> pmax_addr, cp -> pmax_unit))
		return (0);
	printf("sfb%d at nexus0 csr 0x%x priority %d\n",
		cp->pmax_unit, cp->pmax_addr, cp->pmax_pri);
	return (1);
}

/*ARGSUSED*/
sfbopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct pmax_fb *fp = &sfbfb [minor(dev)];
	int s;

	if (minor(dev) > NSFB)
		return (ENXIO);
	if (!fp->initialized)
		return (ENXIO);
	if (fp->GraphicsOpen)
		return (EBUSY);

	fp->GraphicsOpen = 1;
	sfbInitColorMap(fp);
	/*
	 * Set up event queue for later
	 */
	fp->fbu->scrInfo.qe.eSize = PM_MAXEVQ;
	fp->fbu->scrInfo.qe.eHead = fp->fbu->scrInfo.qe.eTail = 0;
	fp->fbu->scrInfo.qe.tcSize = MOTION_BUFFER_SIZE;
	fp->fbu->scrInfo.qe.tcNext = 0;
	fp->fbu->scrInfo.qe.timestamp_ms = TO_MS(time);
	sfbConfigMouse();
	return (0);
}

/*ARGSUSED*/
sfbclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct pmax_fb *fp = &sfbfb [minor (dev)];
	int s;

	if (minor (dev) > NSFB)
		return (EBADF);
	if (!fp->GraphicsOpen)
		return (EBADF);

	fp->GraphicsOpen = 0;
	sfbInitColorMap(fp);
	sfbDeconfigMouse();
	sfbScreenInit(fp);
	bzero((caddr_t)fp->fr_addr, 1280 * 1024);
	sfbPosCursor(fp->col * 8, fp->row * 15, fp);
	return (0);
}

/*ARGSUSED*/
sfbioctl(dev, cmd, data, flag, p)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	register struct pmax_fb *fp = &sfbfb [minor (dev)];
	int s;

	if (minor (dev) > NSFB)
		return (EBADF);

	switch (cmd) {
	case QIOCGINFO:
/*		return (fbmmap(fp, dev, data, p)); */
		return -1;

	case QIOCPMSTATE:
		/*
		 * Set mouse state.
		 */
		fp->fbu->scrInfo.mouse = *(pmCursor *)data;
		sfbPosCursor(fp->fbu->scrInfo.mouse.x,
			     fp->fbu->scrInfo.mouse.y, fp);
		break;

	case QIOCINIT:
		/*
		 * Initialize the screen.
		 */
		sfbScreenInit(fp);
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
		sfbLoadCursor((unsigned short *)data, fp);
		break;

	case QIOWCURSORCOLOR:
		sfbCursorColor((unsigned int *)data, fp);
		break;

	case QIOSETCMAP:
		sfbLoadColorMap((ColorMap *)data, fp);
		break;

	case QIOKERNLOOP:
		sfbConfigMouse();
		break;

	case QIOKERNUNLOOP:
		sfbDeconfigMouse();
		break;

	case QIOVIDEOON:
		sfbRestoreCursorColor(fp);
		bt459_video_on(fp);
		break;

	case QIOVIDEOOFF:
		bt459_video_off(fp);
		break;

	default:
		printf("sfb0: Unknown ioctl command %x\n", cmd);
		return (EINVAL);
	}
	return (0);
}

sfbselect(dev, flag, p)
	dev_t dev;
	int flag;
	struct proc *p;
{
	struct pmax_fb *fp = &sfbfb [minor (dev)] ;

	if (minor (dev) > NSFB)
		return (EBADF);

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
sfbmap(dev, off, prot)
	dev_t dev;
{
	int len;
	struct pmax_fb *fp = &sfbfb [minor (dev)] ;

	if (minor (dev) > NSFB)
		return (EBADF);

	len = pmax_round_page(((vm_offset_t)(fp -> fbu) & PGOFSET)
			      + sizeof (struct fbuaccess));
	if (off < len)
		return pmax_btop(MACH_CACHED_TO_PHYS(fp -> fbu) + off);
	off -= len;
	if (off >= fp -> fr_size)
		return (-1);
	return pmax_btop(MACH_UNCACHED_TO_PHYS(fp -> fr_addr) + off);
}

static u_char	cursor_RGB[6];	/* cursor color 2 & 3 */

/*
 * XXX This assumes 2bits/cursor pixel so that the 1Kbyte cursor RAM
 * defines a 64x64 cursor. If the bt459 does not map the cursor RAM
 * this way, this code is Screwed!
 */
static void
sfbLoadCursor(cursor, fp)
	u_short *cursor;
	struct pmax_fb *fp;
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
			bt459_set_cursor_ram(pos, out, fp);
			pos++;
			j++;
		}
		while (j < 16) {
			bt459_set_cursor_ram(pos, 0, fp);
			pos++;
			j++;
		}
		cursor++;
	}
	while (pos < 1024) {
		bt459_set_cursor_ram(pos, 0, fp);
		pos++;
	}
#endif /* PMAX */
}

/*
 * Set a cursor ram value.
 */
static void
bt459_set_cursor_ram(pos, val, fp)
	int pos;
	register u_char val;
	struct pmax_fb *fp;
{
	register bt459_regmap_t *regs;
	register int cnt;
	u_char nval;
	regs = (bt459_regmap_t *)(fp -> fr_addr
				  - SFB_OFFSET_VRAM + SFB_OFFSET_BT459);

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
sfbinit(base, unit)
	char *base;
	int unit;
{
	register bt459_regmap_t *regs;
	register struct pmax_fb *fp;
	u_char foo;
	extern struct tty *fbconstty;

printf ("sfbinit: %x %d\n", base, unit);
	if (unit > NSFB)
		return (0);
	fp = &sfbfb [unit];

	/* check for no frame buffer */
	if (badaddr(base + SFB_OFFSET_VRAM, 4))
		return (0);

	fp->isMono = 0;
	fp->fr_addr = (char *)(base + SFB_OFFSET_VRAM);
	fp->fr_size = SFB_FB_SIZE;
	/*
	 * Must be in Uncached space since the fbuaccess structure is
	 * mapped into the user's address space uncached.
	 */
	fp->fbu = (struct fbuaccess *)
		MACH_PHYS_TO_UNCACHED(MACH_CACHED_TO_PHYS(&sfbu
							  [unit]));
	fp->posCursor = sfbPosCursor;
	if (tb_kbdmouseconfig(fp))
		return (0);

	/*
	 * Initialize the screen.
	 */
	regs = (bt459_regmap_t *)(fp->fr_addr 
				  - SFB_OFFSET_VRAM + SFB_OFFSET_BT459);

	foo = bt459_read_reg (regs, BT459_REG_ID);
	if (bt459_read_reg(regs, BT459_REG_ID) != 0x4a)
		return (0);

#ifdef FOO /* @@@ DON'T KNOW HOW TO DO THIS @@@ */
	/* Reset the chip */
	*(volatile int *)(fp->fr_addr + SFB_OFFSET_RESET) = 0;
	DELAY(2000);	/* ???? check right time on specs! ???? */
#endif /* FOO */

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
	fp->fbu->scrInfo.max_row = 68;
	fp->fbu->scrInfo.max_col = 80;
	fp->fbu->scrInfo.max_x = 1280;
	fp->fbu->scrInfo.max_y = 1024;
	fp->fbu->scrInfo.max_cur_x = 1279;
	fp->fbu->scrInfo.max_cur_y = 1023;
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
	sfbInitColorMap(fp);
	sfbScreenInit(fp);
/*	fbScroll(fp); */

	/* Initialize the SFB ASIC... */
	*((int *)(base + SFB_MODE)) = 0;
	*((int *)(base + SFB_PLANEMASK)) = 0xFFFFFFFF;

	fp->initialized = 1;
	if (!fbconstty) {
		int i;
		for (i = 0; i < 1024; i++) {
			fp -> fr_addr [i] = i >> 2;
			fp -> fr_addr [i + 1280] = i >> 2;
			fp -> fr_addr [i + 2560] = i >> 2;
			fp -> fr_addr [i + 3840] = i >> 2;
		}
		cons_fb_init (fp, 8, 1280, 256);
	}
	return (1);
}

/*
 * ----------------------------------------------------------------------------
 *
 * sfbScreenInit --
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
sfbScreenInit(fp)
	struct pmax_fb *fp;
{
	/*
	 * Home the cursor.
	 * We want an LSI terminal emulation.  We want the graphics
	 * terminal to scroll from the bottom. So start at the bottom.
	 */
	fp->row = 67;
	fp->col = 0;

	/*
	 * Load the cursor with the default values
	 *
	 */
/*	sfbLoadCursor(defCursor, fp); */
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
sfbRestoreCursorColor(fp)
	struct pmax_fb *fp;
{
	bt459_regmap_t *regs;
	register int i;

	regs = (bt459_regmap_t *)(fp -> fr_addr
				  - SFB_OFFSET_VRAM + SFB_OFFSET_BT459);

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
sfbCursorColor(color, fp)
	unsigned int color[];
	struct pmax_fb *fp;
{
	register int i, j;

	for (i = 0; i < 6; i++)
		cursor_RGB[i] = (u_char)(color[i] >> 8);

	sfbRestoreCursorColor(fp);
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
sfbPosCursor(x, y, fp)
	register int x, y;
	struct pmax_fb *fp;
{
	bt459_regmap_t *regs;
	regs = (bt459_regmap_t *)(fp -> fr_addr
				  - SFB_OFFSET_VRAM + SFB_OFFSET_BT459);

	if (y < fp->fbu->scrInfo.min_cur_y || y > fp->fbu->scrInfo.max_cur_y)
		y = fp->fbu->scrInfo.max_cur_y;
	if (x < fp->fbu->scrInfo.min_cur_x || x > fp->fbu->scrInfo.max_cur_x)
		x = fp->fbu->scrInfo.max_cur_x;
	fp->fbu->scrInfo.cursor.x = x;		/* keep track of real cursor */
	fp->fbu->scrInfo.cursor.y = y;		/* position, indep. of mouse */

	x += 369;
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
sfbInitColorMap(fp)
	struct pmax_fb *fp;
{
	bt459_regmap_t *regs;
	register int i;
	regs = (bt459_regmap_t *)(fp -> fr_addr
				  - SFB_OFFSET_VRAM + SFB_OFFSET_BT459);

	bt459_select_reg(regs, 0);
	regs->addr_cmap = 0; MachEmptyWriteBuffer();
	regs->addr_cmap = 0; MachEmptyWriteBuffer();
	regs->addr_cmap = 0; MachEmptyWriteBuffer();

	for (i = 0; i < 256; i++) {
		regs->addr_cmap = 0xff; MachEmptyWriteBuffer();
		regs->addr_cmap = 0xff; MachEmptyWriteBuffer();
		regs->addr_cmap = 0xff; MachEmptyWriteBuffer();
	}

	for (i = 0; i < 3; i++) {
		cursor_RGB[i] = 0x00;
		cursor_RGB[i + 3] = 0xff;
	}
	sfbRestoreCursorColor(fp);
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
sfbLoadColorMap(ptr, fp)
	ColorMap *ptr;
	struct pmax_fb *fp;
{
	bt459_regmap_t *regs;
	regs = (bt459_regmap_t *)(fp -> fr_addr
				  - SFB_OFFSET_VRAM + SFB_OFFSET_BT459);
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
bt459_video_on(fp)
     struct pmax_fb *fp;
{
	bt459_regmap_t *regs;
	regs = (bt459_regmap_t *)(fp -> fr_addr
				  - SFB_OFFSET_VRAM + SFB_OFFSET_BT459);

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
bt459_video_off(fp)
	struct pmax_fb *fp;
{
	bt459_regmap_t *regs;
	regs = (bt459_regmap_t *)(fp -> fr_addr
				  - SFB_OFFSET_VRAM + SFB_OFFSET_BT459);

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
 * sfb keyboard and mouse input. Just punt to the generic ones in fb.c
 */
void
sfbKbdEvent(ch)
	int ch;
{
	fbKbdEvent(ch, &sfbfb [0]);
}

void
sfbMouseEvent(newRepPtr)
	MouseReport *newRepPtr;
{
	fbMouseEvent(newRepPtr, &sfbfb [0]);
}

void
sfbMouseButtons(newRepPtr)
	MouseReport *newRepPtr;
{
	fbMouseButtons(newRepPtr, &sfbfb [0]);
}

/*
 * Configure the mouse and keyboard based on machine type
 */
static void
sfbConfigMouse()
{
	int s;

	s = spltty();
	switch (pmax_boardtype) {
#if NDC > 0
	case DS_3MAX:
		dcDivertXInput = sfbKbdEvent;
		dcMouseEvent = sfbMouseEvent;
		dcMouseButtons = sfbMouseButtons;
		break;
#endif
#if NSCC > 1
	case DS_3MIN:
		sccDivertXInput = sfbKbdEvent;
		sccMouseEvent = sfbMouseEvent;
		sccMouseButtons = sfbMouseButtons;
		break;
#endif
#if NDTOP > 0
	case DS_MAXINE:
		dtopDivertXInput = sfbKbdEvent;
		dtopMouseEvent = sfbMouseEvent;
		dtopMouseButtons = sfbMouseButtons;
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
sfbDeconfigMouse()
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
#endif /* NSFB */
