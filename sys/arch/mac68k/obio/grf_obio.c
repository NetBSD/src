/*	$NetBSD: grf_obio.c,v 1.1 1995/04/29 20:23:41 briggs Exp $	*/

/*
 * Copyright (c) 1995 Allen Briggs.  All rights reserved.
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
 *	This product includes software developed by Allen Briggs.
 * 4. The name of the author may not be used to endorse or promote products
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
/*
 * Graphics display driver for the Macintosh internal video for machines
 * that don't map it into a fake nubus card.
 */

#include <sys/param.h>

#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/proc.h>

#include <machine/grfioctl.h>
#include <machine/cpu.h>

#include "nubus.h"
#include "grfvar.h"

extern int	grfiv_probe __P((struct grf_softc *sc, nubus_slot *ignore));
extern int	grfiv_init __P((struct grf_softc *sc));
extern int	grfiv_mode __P((struct grf_softc *sc, int cmd, void *arg));

extern int
grfiv_probe(sc, ignore)
	struct grf_softc *sc;
	nubus_slot *ignore;
{
	extern unsigned long int_video_start;

	if (int_video_start == 0) {
		return 0;
	}
	return 1;
}

extern int
grfiv_init(sc)
	struct	grf_softc *sc;
{
	struct grfmode *gm;
	int i, j;

	sc->card_id = 0;

	strcpy(sc->card_name, "Internal video");

	gm = &(sc->curr_mode);
	gm->mode_id = 0;
	gm->psize = 1;
	gm->ptype = 0;
	gm->width = 640;	/* XXX */
	gm->height = 480;	/* XXX */
	gm->rowbytes = 80;	/* XXX Hack */
	gm->hres = 80;		/* XXX Hack */
	gm->vres = 80;		/* XXX Hack */
	gm->fbsize = gm->width * gm->height;
	gm->fbbase = (caddr_t) 0;

	return 1;
}

extern int
grfiv_mode(sc, cmd, arg)
	struct grf_softc *sc;
	int cmd;
	void *arg;
{
	return 0;
}
