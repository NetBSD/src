/*	$NetBSD: pm.c,v 1.40.6.1 2006/04/22 11:37:52 simonb Exp $	*/

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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)pm.c	8.1 (Berkeley) 6/10/93
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: pm.c,v 1.40.6.1 2006/04/22 11:37:52 simonb Exp $");


#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include <pmax/pmax/kn01.h>
#include <pmax/ibus/ibusvar.h>
#include <machine/autoconf.h>
#include <dev/sun/fbio.h>
#include <machine/fbvar.h>
#include <machine/locore.h>		/* wbflush() */
#include <machine/pmioctl.h>
#include <pmax/dev/bt478var.h>
#include <pmax/dev/fbreg.h>
#include <pmax/dev/pmvar.h>
#include <pmax/dev/pmreg.h>



/*
 * These need to be mapped into user space.
 */
static u_short	curReg;		/* copy of PCCRegs.cmdr since it's read only */

static struct fbuaccess pmu;
static struct pmax_fbtty pmfb;	
static struct fbinfo *pm_fi;

static int	pm_video_on __P ((struct fbinfo *));
static int	pm_video_off __P ((struct fbinfo *));
static void	pmInitColorMap __P((struct fbinfo *fi));
static void	pmPosCursor __P((struct fbinfo *fi, int x, int y));
static void	pmLoadCursor __P((struct fbinfo *fi, u_short *ptr));
static void	pccCursorOff __P((struct fbinfo *fi));

static struct fbdriver pm_driver = {
	pm_video_on,
	pm_video_off,
	pmInitColorMap,		/* pcc cursor wrapper for bt478InitColorMap */
	bt478GetColorMap,
	bt478LoadColorMap,
	pmPosCursor,
	pmLoadCursor,
	bt478CursorColor,
};

static int  pmmatch __P((struct device *, struct cfdata *, void *));
static void pmattach __P((struct device *, struct device *, void *));
static int  pminit __P((struct fbinfo *, caddr_t, int));

CFATTACH_DECL(pm_ds, sizeof(struct device),
    pmmatch, pmattach, NULL, NULL);

int
pm_cnattach()
{
	struct fbinfo *fi;
	caddr_t base;

#if 0
	ULTRIX does in this way;
	check the presence of monochrome bit in CSR.
	if set, there is a monochrome framebuffer
	if not set, try two write and read cycles of framebuffer to make
	sure the presence of video memory.
#else
	base = (caddr_t)MIPS_PHYS_TO_KSEG1(KN01_SYS_PCC);
	if (badaddr(base, 4))
		return (0);
#endif

	fbcnalloc(&fi);
	if (pminit(fi, base, 0) < 0)
		return (0);
	pm_fi = fi;
	return (1);
}

static int
pmmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct ibus_attach_args *ia = aux;
	caddr_t pmaddr = (caddr_t)ia->ia_addr;

	/* make sure that we're looking for this type of device. */
	if (strcmp(ia->ia_name, "pm") != 0)
		return (0);

	if (badaddr(pmaddr, 4))
		return (0);

	return (1);
}

static void
pmattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ibus_attach_args *ia = aux;
	caddr_t base = (caddr_t)ia->ia_addr;
	struct fbinfo *fi;

	if (pm_fi)
		fi = pm_fi;
	else {
		if (fballoc(&fi) < 0 || pminit(fi, base, device_unit(self)) < 0)
			return; /* failed */
	}
	((struct fbsoftc *)self)->sc_fi = fi;

	printf(": %dx%dx%d%s",
		fi->fi_type.fb_width,
		fi->fi_type.fb_height,
		fi->fi_type.fb_depth, 
		(pm_fi) ? " console" : "");

	printf("\n");
}

int
pminit(fi, base, unit)
	struct fbinfo *fi;
	caddr_t base;
	int unit;
{
	u_int16_t kn01csr;
	PCCRegs *pcc;

	kn01csr = *(volatile u_int16_t *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CSR);

	fi->fi_unit = unit;
	fi->fi_pixels = (caddr_t)MIPS_PHYS_TO_KSEG1(KN01_PHYS_FBUF_START);
	fi->fi_base = base; /* PCC address */
	fi->fi_vdac = (caddr_t)MIPS_PHYS_TO_KSEG1(KN01_SYS_VDAC);

	if (kn01csr & KN01_CSR_MONO) {
		fi->fi_type.fb_type = PMAX_FBTYPE_PM_MONO;
		fi->fi_type.fb_depth = 1;
		fi->fi_type.fb_cmsize = 0;
		fi->fi_type.fb_size = 0x40000;
		fi->fi_pixelsize = (1024 / 8) * 864;
		fi->fi_linebytes = 2048 / 8;
	}
	else {
		fi->fi_type.fb_type = PMAX_FBTYPE_PM_COLOR;
		fi->fi_type.fb_depth = 8;
		fi->fi_type.fb_cmsize = 256;
		fi->fi_type.fb_size = 0x100000;
		fi->fi_pixelsize = 1024 * 864;
		fi->fi_linebytes = 1024;
	}
	fi->fi_type.fb_width = 1024;
	fi->fi_type.fb_height = 864;
	fi->fi_size = fi->fi_type.fb_size;
	fi->fi_driver = &pm_driver;
	fi->fi_blanked = 0;

	/*
	 * Set mmap'able address of qvss-compatible user info structure.
	 *
	 * Must be in Uncached space since the fbuaccess structure is
	 * mapped into the user's address space uncached.
	 *
	 * XXX can go away when MI support for d_mmap entrypoints added.
	 */
	fi->fi_fbu = (void *)MIPS_PHYS_TO_KSEG1(MIPS_KSEG0_TO_PHYS(&pmu));

	/* This is glass-tty state but it's in the shared structure. Ick. */
	fi->fi_fbu->scrInfo.max_row = 56;
	fi->fi_fbu->scrInfo.max_col = 80;

	/* These are non-zero on the kn01 framebuffer. Why? */
	fi->fi_fbu->scrInfo.min_cur_x = -15;
	fi->fi_fbu->scrInfo.min_cur_y = -15;

	init_pmaxfbu(fi);

	/*
	 * Initialize old-style glass-tty screen info.
	 */
	fi->fi_glasstty = &pmfb;
	/* it's safe to assume serial driver existence in this case */
	tb_kbdmouseconfig(fi);

	/* Initialize the cursor register on . */
	/* Turn off the hardware cursor sprite for rcons text mode. */
	pcc = (PCCRegs *)fi->fi_base;
	pcc->cmdr = PCC_FOPB | PCC_VBHI;
	curReg = 0;	/* XXX */
	pccCursorOff(fi);

	/*
	 * Initialize the color map, the screen, and the mouse.
	 */
	bt478init(fi);

	fbconnect(fi);

	return (0);
}

/*
 * ----------------------------------------------------------------------------
 *
 * pmLoadCursor --
 *
 *	Routine to load the cursor Sprite pattern.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor is loaded into the hardware cursor.
 *	Also, turn on the cursor in case it was disabled before.
 *	If someone sets a cursor pattern, it should probably be displayed.
 *
 * ----------------------------------------------------------------------------
 */
static void
pmLoadCursor(fi, cur)
	struct fbinfo *fi;
	unsigned short *cur;
{
	PCCRegs *pcc = (PCCRegs *)fi->fi_base;
	int i;

	curReg |= PCC_LODSA;
	pcc->cmdr = curReg;
	for (i = 0; i < 32; i++) {
		pcc->memory = cur[i];
		wbflush();
	}
	curReg &= ~PCC_LODSA;

	/* turn on the cursor plane overlays in the PCC chip. */
 	curReg |= (PCC_ENPA | PCC_ENPB);
	pcc->cmdr = curReg;
}



/*
 *----------------------------------------------------------------------
 *
 * pmPosCursor --
 *
 *	Postion the cursor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *	NB:  should not turn on the hardwar cursor, since the
 *	Xserver positions the cursor to upper left corner as the last
 * 	cursor operation before it exits.
 *
 *----------------------------------------------------------------------
 */
static void
pmPosCursor(fi, x, y)
	struct fbinfo *fi;
	int x, y;
{
	PCCRegs *pcc = (PCCRegs *)fi->fi_base;

	if (y < fi->fi_fbu->scrInfo.min_cur_y ||
	    y > fi->fi_fbu->scrInfo.max_cur_y)
		y = fi->fi_fbu->scrInfo.max_cur_y;
	if (x < fi->fi_fbu->scrInfo.min_cur_x ||
	    x > fi->fi_fbu->scrInfo.max_cur_x)
		x = fi->fi_fbu->scrInfo.max_cur_x;
	fi->fi_fbu->scrInfo.cursor.x = x;	/* keep track of real cursor */
	fi->fi_fbu->scrInfo.cursor.y = y;	/* position, indep. of mouse */
	pcc->xpos = PCC_X_OFFSET + x;
	pcc->ypos = PCC_Y_OFFSET + y;
}


#if 0	/* XXX not used */
/*
 * Turn hardware cursor on by turning on the enable for A and B
 * overlay planes. video output under the cursor sprite is then
 * determined by  the A and B plane contents.
 */
void pccCursorOn(fi)
	struct fbinfo *fi;
{
	PCCRegs *pcc = (PCCRegs *)fi -> fi_base;
	pcc -> cmdr = curReg | (PCC_ENPA | PCC_ENPB);
	wbflush();
}
#endif


/*
 * Turn hardware cursor off by turning off the enable for A and B
 * overlay planes. video output under the cursor sprite is then
 * determined by the framebuffer contents.
 */
static void
pccCursorOff(fi)
	struct fbinfo *fi;
{
	PCCRegs *pcc = (PCCRegs *)fi -> fi_base;
	pcc -> cmdr = curReg & ~(PCC_ENPA | PCC_ENPB);
	wbflush();
}


/*
 * Initialize colourmap to default values.
 * The default cursor hardware state is off.
 */
void
pmInitColorMap(fi)
	struct fbinfo *fi;
{
	bt478InitColorMap(fi);
	pccCursorOff(fi);
}


/*
 * Enable the video display.
 */
static int
pm_video_on(fi)
	struct fbinfo *fi;
{
	PCCRegs *pcc = (PCCRegs *)fi -> fi_base;

	if (!fi -> fi_blanked)
		return 0;

	pcc -> cmdr = curReg & ~(PCC_FOPA | PCC_FOPB);
	bt478RestoreCursorColor (fi);
	fi -> fi_blanked = 0;
	return 0;
}

/*
 *  Disable the video display.
 *  Sets the FOPA and FOPB bits in the PCC chip to force the contents
 *  of the A and B overlay planes to 1. Video output is then
 *  determined  by  colourmap entry 12 (0x0c), which we set here to
 *  black.
 */
static int
pm_video_off(fi)
	struct fbinfo *fi;
{
	PCCRegs *pcc = (PCCRegs *)fi -> fi_base;

	if (fi -> fi_blanked)
		return 0;

	bt478BlankCursor (fi);
	pcc -> cmdr = curReg | PCC_FOPA | PCC_FOPB;
	fi -> fi_blanked = 1;
	return 0;
}
