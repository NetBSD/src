/*	$NetBSD: pm.c,v 1.30 2000/01/08 01:02:36 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pm.c,v 1.30 2000/01/08 01:02:36 simonb Exp $");


#include <sys/param.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <vm/vm.h>

#include <machine/autoconf.h>
#include <machine/bus.h>		/* XXX wbflush() */

#include <machine/pmioctl.h>
#include <machine/fbio.h>
#include <machine/fbvar.h>

#include <pmax/dev/fbreg.h>
#include <pmax/dev/pmvar.h>

#include <pmax/dev/pmreg.h>
#include <pmax/dev/bt478var.h>


/*
 * These need to be mapped into user space.
 */
extern struct fbuaccess pmu;
struct fbuaccess pmu;
static u_short curReg;		/* copy of PCCRegs.cmdr since it's read only */

/*
 * rcons methods and globals.
 */
struct pmax_fbtty pmfb;

/*
 * Forward references.
 */
void		pmScreenInit __P((struct fbinfo *fi));
static void	pmLoadCursor __P((struct fbinfo *fi, u_short *ptr));
void		pmPosCursor __P((struct fbinfo *fi, int x, int y));

void		bt478CursorColor __P((struct fbinfo *fi, u_int *color));
void		bt478InitColorMap __P((struct fbinfo *fi));

void		pccCursorOn  __P((struct fbinfo *fi));
void		pccCursorOff __P((struct fbinfo *fi));
void		pmInitColorMap __P((struct fbinfo *fi));

int		pminit __P((struct fbinfo *fi, caddr_t base, int unit,
		    int console));
int		pmattach __P((struct fbinfo *fi, int unit,
		    int cold_console_flag));

static int	pm_video_on __P((struct fbinfo *));
static int	pm_video_off __P((struct fbinfo *));

/* new-style raster-cons "driver" methods */

struct fbdriver pm_driver = {
	pm_video_on,
	pm_video_off,
	pmInitColorMap,		/* pcc cursor wrapper for bt478InitColorMap */
	bt478GetColorMap,
	bt478LoadColorMap,
	pmPosCursor,
	pmLoadCursor,
	bt478CursorColor,
};



/*
 * Machine-independent backend to attach a pm device.
 * assumes the following fields in struct fbinfo *fi have been set
 * by the MD front-end:
 *
 * fi->fi_pixels	framebuffer raster memory
 * fi->fi_vdac		vdac register address
 * fi->fi_base		address of programmable cursor chip registers
 * fi->fi_type.fb_depth	1 (mono) or 8 (colour)
 * fi->fi_fbu		QVSS-compatible user-mapped fbinfo struct
 */
int
pmattach(fi, unit, cold_console_flag)
	struct fbinfo *fi;
	int unit;
	int cold_console_flag;
{
	PCCRegs *pcc = (PCCRegs *)fi->fi_base;

	/* check for no frame buffer */
	if (badaddr((char *)fi->fi_pixels, 4))
		return (0);

	/* Fill in the stuff that differs from monochrome to color. */
	if (fi->fi_type.fb_depth == 1) {
		fi->fi_type.fb_depth = 1;
		fi->fi_type.fb_cmsize = 0;
		fi->fi_type.fb_boardtype = PMAX_FBTYPE_PM_MONO;
		fi->fi_type.fb_size = 0x40000;
		fi->fi_linebytes = 256;
	} else {
		fi->fi_type.fb_depth = 8;
		fi->fi_type.fb_cmsize = 256;
		fi->fi_type.fb_boardtype = PMAX_FBTYPE_PM_COLOR;
		fi->fi_type.fb_size = 0x100000;
		fi->fi_linebytes = 1024;
	}

	/* Fill in main frame buffer info struct. */

	fi->fi_driver = &pm_driver;
	fi->fi_pixelsize =
		((fi->fi_type.fb_depth == 1) ? 1024 / 8 : 1024) * 864;
	fi->fi_blanked = 0;
	fi->fi_type.fb_width = 1024;
	fi->fi_type.fb_height = 864;


	/*
	 * Compatibility glue
	 */
	fi->fi_glasstty = &pmfb;


	/*
	 * Initialize the screen.
	 */
	pcc->cmdr = PCC_FOPB | PCC_VBHI;

	/* Initialize the cursor register on . */
	/* Turn off the hardware cursor sprite for rcons text mode. */
	curReg = 0;	/* XXX */
	pccCursorOff(fi);

	/*
	 * Initialize the color map, the screen, and the mouse.
	 */
	bt478init(fi);

	/*
	 * Initialize old-style pmax screen info.
	 */
	fi->fi_fbu->scrInfo.max_row = 56;
	fi->fi_fbu->scrInfo.max_col = 80;

	init_pmaxfbu(fi);

	/* These are non-zero on the kn01 framebuffer. Why? */
	fi->fi_fbu->scrInfo.min_cur_x = -15;
	fi->fi_fbu->scrInfo.min_cur_y = -15;


#ifdef notanymore
	bt478InitColorMap(fi);	/* done inside bt478init() */
#endif

	/*
	 * Connect to the raster-console pseudo-driver.
	 */
	fi->fi_glasstty = &pmfb; /*XXX*/
	fbconnect((fi->fi_type.fb_depth == 1) ? "KN01 mfb" : "KN01 cfb",
		  fi, cold_console_flag);


#ifdef fpinitialized
	fp->initialized = 1;
#endif
	return (1);
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
void
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


/*
 * Turn hardware cursor off by turning off the enable for A and B
 * overlay planes. video output under the cursor sprite is then
 * determined by the framebuffer contents.
 */
void pccCursorOff(fi)
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
void pmInitColorMap(fi)
	struct fbinfo *fi;
{
	bt478InitColorMap(fi);
	pccCursorOff(fi);
}


/*
 * Enable the video display.
 */
static int
pm_video_on (fi)
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
static int pm_video_off (fi)
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
