/*	$NetBSD: cfb.c,v 1.36 2000/01/09 03:55:29 simonb Exp $	*/

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
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
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

#include "fb.h"
#include "cfb.h"

#if NCFB > 0
#include <sys/param.h>
#include <sys/systm.h>					/* printf() */
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/device.h>
#include <dev/tc/tcvar.h>

#include <mips/cpuregs.h>		/* mips cached->uncached */
#include <machine/pmioctl.h>
#include <machine/fbio.h>
#include <machine/fbvar.h>
#include <pmax/dev/cfbvar.h>		/* XXX dev/tc ? */

#include <pmax/dev/bt459.h>
#include <pmax/dev/fbreg.h>

#include <machine/autoconf.h>


#define PMAX	/* enable /dev/pm compatibility */

/*
 * These need to be mapped into user space.
 */
static struct fbuaccess		cfbu;
static struct pmax_fbtty	cfbfb;

/*
 * Method table for standard framebuffer operations on a CFB.
 * The  CFB uses a Brooktree bt479 ramdac.
 */
struct fbdriver cfb_driver = {
	bt459_video_on,
	bt459_video_off,
	bt459InitColorMap,
	bt459GetColorMap,
	bt459LoadColorMap,
	bt459PosCursor,
	bt459LoadCursor,
	bt459CursorColor
};

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

static int	cfbmatch __P((struct device *, struct cfdata *, void *));
static void	cfbattach __P((struct device *, struct device *, void *));
static int	cfb_intr __P((void *sc));

struct cfattach cfb_ca = {
	sizeof(struct fbsoftc), cfbmatch, cfbattach
};

static int
cfbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct tc_attach_args *ta = aux;

#ifdef FBDRIVER_DOES_ATTACH
	/* leave configuration  to the fb driver */
	return 0;
#endif

	/* make sure that we're looking for this type of device. */
	if (!TC_BUS_MATCHNAME(ta, "PMAG-BA "))
		return (0);

	return (1);
}

/*
 * Attach a device.  Hand off all the work to cfbinit(),
 * so console-config code can attach cfb devices very early in boot,
 * to use as system console.
 */
static void
cfbattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct tc_attach_args *ta = aux;
	caddr_t base = 	(caddr_t)(ta->ta_addr);
	int unit = self->dv_unit;
	struct fbinfo *fi;
	
	/* Allocate a struct fbinfo and point the softc at it */
	if (fballoc(base, &fi) == 0 && !cfbinit(fi, base, unit, 0))
			return;

	if ((((struct fbsoftc *)self)->sc_fi = fi) == NULL)
		return;

	/*
	 * 3MIN does not mask un-established TC option interrupts,
	 * so establish a handler.
	 * XXX Should store cmap updates in softc and apply in the
	 * interrupt handler, which interrupts during vertical-retrace.
	 */
	tc_intr_establish(parent, ta->ta_cookie, TC_IPL_NONE, cfb_intr, fi);
	fbconnect("PMAG-BA", fi, 0);
	printf("\n");
}



/*
 * CFB initialization.  This is divorced from cfbattch() so that
 * a console framebuffer can be initialized early during boot.
 */
int
cfbinit(fi, cfbaddr, unit, silent)
	struct fbinfo *fi;
	caddr_t cfbaddr;
	int unit;
	int silent;
{

	/* check for no frame buffer */
	if (badaddr(cfbaddr, 4)) {
		printf("cfb: bad address 0x%p\n", cfbaddr);
		return (0);
	}

	/* Fill in main frame buffer info struct. */
	fi->fi_unit = unit;
	fi->fi_pixels = (caddr_t)(cfbaddr + CFB_OFFSET_VRAM);
	fi->fi_pixelsize = 1024 * 864;
	fi->fi_base = (caddr_t)(cfbaddr + CFB_OFFSET_VRAM);
	fi->fi_vdac = (caddr_t)(cfbaddr + CFB_OFFSET_BT459);
	fi->fi_size = (fi->fi_pixels + CFB_FB_SIZE) - fi->fi_base;
	fi->fi_linebytes = 1024;
	fi->fi_driver = &cfb_driver;
	fi->fi_blanked = 0;

	/* Fill in Frame Buffer Type struct. */
	fi->fi_type.fb_boardtype = PMAX_FBTYPE_CFB;
	fi->fi_type.fb_width = 1024;
	fi->fi_type.fb_height = 864;
	fi->fi_type.fb_depth = 8;
	fi->fi_type.fb_cmsize = 256;
	fi->fi_type.fb_size = CFB_FB_SIZE;

	/* Reset the chip */
	if (!bt459init(fi)) {
		printf("cfb%d: vdac init failed.\n", unit);
		return (0);
	}

	/*
	 * qvss/pm-style mmap()ed event queue compatibility glue
	 */

	/*
	 * Must be in Uncached space since the fbuaccess structure is
	 * mapped into the user's address space uncached.
	 */
	fi->fi_fbu = (struct fbuaccess *)
		MIPS_PHYS_TO_KSEG1(MIPS_KSEG0_TO_PHYS(&cfbu));

	/* This is glass-tty state but it's in the shared structure. Ick. */
	fi->fi_fbu->scrInfo.max_row = 56;
	fi->fi_fbu->scrInfo.max_col = 80;
	init_pmaxfbu(fi);

	/* Initialize old-style pmax glass-tty screen info. */
	fi->fi_glasstty = &cfbfb;

	/* Initialize the color map, the screen, and the mouse. */
	if (tb_kbdmouseconfig(fi)) {
		printf(" (mouse/keyboard config failed)");
		return (0);
	}

	/* Connect to the raster-console pseudo-driver */
	fbconnect("PMAG-BA", fi, silent);
	return (1);
}


/*
 * The original TURBOChannel cfb interrupts on every vertical
 * retrace, and we can't disable the board from requesting those
 * interrupts.  The 4.4BSD kernel never enabled those interrupts;
 * but there's a kernel design bug on the 3MIN, where disabling
 * (or enabling) TC option interrupts has no effect the interrupts
 * are mapped to R3000 interrupts and always seem to be taken.
 * This function simply dismisses CFB interrupts, or the interrupt
 * request from the card will still be active.
 */
static int
cfb_intr(sc)
	void *sc;
{
	struct fbinfo *fi;
	caddr_t slot_addr;

	fi = (struct fbinfo *)sc;
	slot_addr = (((caddr_t)fi->fi_base) - CFB_OFFSET_VRAM);
	
	/* reset vertical-retrace interrupt by writing a dont-care */
	*(int*) (slot_addr + CFB_OFFSET_IREQ) = 0;
	return (0);
}

#endif /* NCFB */
