/*	$NetBSD: grf_tv.c,v 1.12.32.1 2008/01/02 21:51:14 bouyer Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: grf_tc.c 1.20 93/08/13$
 *
 *	@(#)grf_tc.c	8.4 (Berkeley) 1/12/94
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: grf_tc.c 1.20 93/08/13$
 *
 *	@(#)grf_tc.c	8.4 (Berkeley) 1/12/94
 */

/*
 * Graphics routines for the X68K native custom chip set.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grf_tv.c,v 1.12.32.1 2008/01/02 21:51:14 bouyer Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/grfioctl.h>

#include <x68k/dev/grfvar.h>
#include <x68k/x68k/iodevice.h>

#include <machine/cpu.h>

int cc_init(struct grf_softc *, void *);
int cc_mode(struct grf_softc *, u_long, void *);

/* Initialize hardware.
 * Must fill in the grfinfo structure in g_softc.
 * Returns 0 if hardware not present, non-zero ow.
 */
int
cc_init(struct grf_softc *gp, void *addr)
{
	struct grfinfo *gi = &gp->g_display;

	gi->gd_fbwidth  = 1024;	/* XXX */
	gi->gd_fbheight = 1024;	/* XXX */
	gi->gd_planes  = 4;	/* XXX */
	gi->gd_regaddr = (void *)0x00e80000;	/* XXX */
	gi->gd_regsize = 0x00004000;		/* XXX */
	gi->gd_fbaddr  = (void *)0x00e00000;	/* XXX */
	gi->gd_fbsize  = gi->gd_fbwidth / 8 * gi->gd_fbheight * gi->gd_planes;
	gp->g_regkva   = addr;
	gp->g_fbkva    = addr;
	switch(IODEVbase->io_sram[0x1d]) {
	case 18:
		/*
		 * mode 18, 24kHz
		 */
		gi->gd_dwidth  = 1024;
		gi->gd_dheight =  848;
		break;
	case 19:
		/*
		 * mode 19, 31kHz VGA mode
		 */
		gi->gd_dwidth  = 640;
		gi->gd_dheight = 480;
		break;
	default:
		/*
		 * mode 16, 31kHz (default)
		 */
		gi->gd_dwidth  = 768;
		gi->gd_dheight = 512;
		break;
	}
	gi->gd_colors  = 1 << gi->gd_planes;

	return 1;
}

/*
 * Change the mode of the display.
 * Right now all we can do is grfon/grfoff.
 * Return a UNIX error number or 0 for success.
 */
/*ARGSUSED*/
int
cc_mode(struct grf_softc *gp, u_long cmd, void *data)
{
	int error = 0;

	switch (cmd) {
	case GM_GRFON:
	case GM_GRFOFF:
		break;

	/*
	 * Remember UVA of mapping for GCDESCRIBE.
	 * XXX this should be per-process.
	 */
	case GM_MAP:
		gp->g_data = data;
		break;

	case GM_UNMAP:
		gp->g_data = 0;
		break;

	case GM_GRFSETVMODE:
		if (*(int *)data == 1) {
			struct grfinfo *gi = &gp->g_display;
			volatile struct crtc *crtc = &IODEVbase->io_crtc;
			/* Reset CRTC, set dwidth and dheight accordingly */
			crtc->r20 = (crtc->r20 & 0xFF00) | 0x1a;
			crtc->r08 = 0x1b;
			crtc->r07 = 0x19c;
			crtc->r06 = 0x1c;
			crtc->r05 = 0x02;
			crtc->r04 = 0x019f;
			crtc->r03 = 0x9a;
			crtc->r02 = 0x1a;
			crtc->r01 = 0x09;
			crtc->r00 = 0xa4;
			gi->gd_dwidth = 1024;
			gi->gd_dheight = 768;
		}
		break;

	default:
		error = EINVAL;
		break;
	}
	return error;
}
