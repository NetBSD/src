/*	$NetBSD: fb.c,v 1.35.6.2 2001/10/10 11:56:24 fvdl Exp $	*/

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
 *	@(#)fb.c	8.1 (Berkeley) 6/10/93
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
 * This file has all the routines common to the various frame buffer drivers
 * including a generic ioctl routine. The pmax_fb structure is passed into the
 * routines and has device specifics stored in it.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/poll.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/vnode.h>

#include <machine/conf.h>
#include <dev/sun/fbio.h>
#include <machine/fbvar.h>
#include <machine/pmioctl.h>

#include <pmax/dev/fbreg.h>
#include <pmax/dev/qvssvar.h>
#include <pmax/dev/rconsvar.h>

#include <pmax/pmax/cons.h>
#include <pmax/pmax/pmaxtype.h>

#include "dc.h"
#include "scc.h"
#include "dtop.h"

/*
 * This framebuffer driver is a generic driver for all supported
 * framebuffers on NetBSD/pmax.  The match and attach functions call
 * out to probe/init functions in subdrivers for each specific baseboard or
 * expansion  bus framebuffers. The driver softc are maintained here, as
 * are the handlers for user-level requests (open/ioctl/close).
 *
 * Hardware dependencies are handled by calls through the "fbdriver"
 * method table, which the subdriver
 */

/* qvss/pm compatible and old 4.4bsd/pmax driver functions */


#if NDC > 0
#include <machine/dc7085cons.h>
#include <pmax/dev/dcvar.h>
#endif

#if NDTOP > 0
#include <pmax/dev/dtopvar.h>
#endif

#if NSCC > 0
#include <pmax/tc/sccvar.h>
#endif

/*
 * LK-201 and successor keycode mapping
*/
#include <pmax/dev/lk201var.h>

#include "fb.h"

static int fbndevs;
struct fbinfo fi_console;
struct fbinfo *fbdevs[NFB];
static u_int8_t cmap_bits[768];         /* colormap for console */

/*
 * The "blessed" framebuffer; the fb that gets
 * the qvss-style ring buffer of mouse/kbd events, and is used
 * for glass-tty fb console output.
 */
struct fbinfo *firstfi = NULL;

/*
 * attach routine: required for pseudo-device
 */
void
fbattach(n)
	int n;
{

}

void
fbcnalloc(fip)
	struct fbinfo **fip;
{
	struct fbinfo *fi = &fi_console;

	firstfi = fi; /* XXX */
	fi->fi_cmap_bits = cmap_bits;
	fbdevs[fbndevs++] = fi;
	*fip = fi;
}

int
fballoc(fip)
	struct fbinfo **fip;
{
	struct fbinfo *fi;

	if (fbndevs >= NFB)
	return (-1);
	fi = malloc(sizeof(struct fbinfo), M_DEVBUF, M_NOWAIT);
	if (fi == NULL)
	goto nomemory;
	fi->fi_cmap_bits = malloc(768, M_DEVBUF, M_NOWAIT);
	if (fi->fi_cmap_bits == NULL) {
		free(fi, M_DEVBUF);
		goto nomemory;
	}
	if (fbndevs == 0)               /* XXX */
		firstfi = fi;           /* XXX */
	*fip = fbdevs[fbndevs++] = fi;
	return (0);

	nomemory:
	return (-1);
}

void
fbconnect(fi)
	struct fbinfo *fi;
{
	if (&fi_console == fi)
		rcons_connect(fi);
}

cdev_decl(fb);                /* generic framebuffer pseudo-device */

#include "fb_usrreq.c"	/* old pm-compatible driver that supports X11R5/R6 */


/*
 * Configure the keyboard/mouse based on machine type for turbochannel
 * display boards.
 */
int
tb_kbdmouseconfig(fi)
	struct fbinfo *fi;
{

	if (fi == NULL || fi->fi_glasstty == NULL) {
#if defined(DEBUG) || defined(DIAGNOSTIC)
		printf("tb_kbdmouseconfig: given non-console framebuffer\n");
#endif
		return 1;
	}

	switch (systype) {

#if NDC > 0
	case DS_PMAX:
	case DS_3MAX:
		fi->fi_glasstty->KBDPutc = dcPutc;
		fi->fi_glasstty->kbddev = makedev(DCDEV, DCKBD_PORT);
		break;
#endif /* NDC */

#if NSCC > 0
	case DS_3MIN:
	case DS_3MAXPLUS:
		fi->fi_glasstty->KBDPutc = sccPutc;
		fi->fi_glasstty->kbddev = makedev(SCCDEV, SCCKBD_PORT);
		break;
#endif /* NSCC */

#if NDTOP > 0
	case DS_MAXINE:
		fi->fi_glasstty->KBDPutc = dtopKBDPutc;
		fi->fi_glasstty->kbddev = makedev(DTOPDEV, DTOPKBD_PORT);
		break;
#endif	/* NDTOP */

	default:
		printf("Can't configure keyboard/mouse\n");
		return (1);
	};

	return (0);
}
