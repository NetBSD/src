/*	$NetBSD: cpu_cons.c,v 1.1 1995/04/11 10:08:42 mellon Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * from: Utah Hdr: cons.c 1.1 90/07/09
 *
 *	@(#)cons.c	8.2 (Berkeley) 1/11/94
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>

#include <pmax/stand/dec_prom.h>

#include <pmax/dev/device.h>
#include <pmax/dev/sccreg.h>
#include <pmax/pmax/kn01.h>
#include <pmax/pmax/kn02.h>
#include <pmax/pmax/kmin.h>
#include <pmax/pmax/maxine.h>
#include <pmax/pmax/kn03.h>
#include <pmax/pmax/asic.h>
#include <pmax/pmax/turbochannel.h>
#include <pmax/pmax/pmaxtype.h>
/* #include <pmax/pmax/cons.h> */
#include <dev/cons.h>

#include <machine/machConst.h>
#include <machine/pmioctl.h>
#include <pmax/dev/fbreg.h>

#include <sys/device.h>
#include <machine/fbio.h>
#include <sparc/rcons/raster.h>
#include <machine/fbvar.h>

#include <pm.h>
#include <cfb.h>
#include <mfb.h>
#include <xcfb.h>
#include <sfb.h>
#include <dc.h>
#include <dtop.h>
#include <scc.h>
#include <le.h>
#include <asc.h>

#if NDC > 0
extern int dcGetc(), dcparam();
extern void dcPutc();
#endif
#if NDTOP > 0
extern int dtopKBDGetc();
#endif
#if NSCC > 0
extern int sccGetc(), sccparam();
extern void sccPutc();
#endif
static int romgetc __P ((dev_t));
static void romputc __P ((dev_t, int));
static void rompollc __P((dev_t, int));

int	pmax_boardtype;		/* Mother board type */

/*
 * Major device numbers for possible console devices. XXX
 */
#define	DTOPDEV		15
#define	DCDEV		16
#define	SCCDEV		17

/*
 * Console I/O is redirected to the appropriate device, either a screen and
 * keyboard, a serial port, or the "virtual" console.
 */

extern struct consdev *cn_tab;	/* Console I/O table... */

struct consdev cd = {
	(void (*)(struct consdev *))0,		/* probe */
	(void (*)(struct consdev *))0,		/* init */
	(int  (*)(dev_t))     romgetc,		/* getc */
	(void (*)(dev_t, int))romputc,		/* putc */
	(void (*)(dev_t, int))rompollc,		/* pollc */
	makedev (0, 0),
	CN_DEAD,
};

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
consinit()
{
	register int kbd, crt;
	register char *oscon;
	int screen = 0;

	cn_tab = &cd;
	(*callv -> _printf)("consinit\n");

	/*
	 * First get the "osconsole" environment variable.
	 */
	oscon = (*callv->_getenv)("osconsole");
	crt = kbd = -1;
	if (oscon && *oscon >= '0' && *oscon <= '9') {
		kbd = *oscon - '0';
		screen = 0;
		while (*++oscon) {
			if (*oscon == ',')
				screen = 1;
			else if (screen &&
			    *oscon >= '0' && *oscon <= '9') {
				crt = kbd;
				kbd = *oscon - '0';
				break;
			}
		}
	}
	if (pmax_boardtype == DS_PMAX && kbd == 1)
		screen = 1;
	/*
	 * The boot program uses PMAX ROM entrypoints so the ROM sets
	 * osconsole to '1' like the PMAX.
	 */
	if (pmax_boardtype == DS_3MAX && crt == -1 && kbd == 1) {
		screen = 1;
		crt = 0;
		kbd = 7;
	}

	/*
	 * First try the keyboard/crt cases then fall through to the
	 * remote serial lines.
	 */
	if (screen) {
	    switch (pmax_boardtype) {
	    case DS_PMAX:
#if NDC > 0 && NPM > 0
		if (pminit()) {
			cd.cn_dev = makedev(DCDEV, DCKBD_PORT);
			cd.cn_getc = dcGetc;
			cd.cn_pri = CN_INTERNAL;
			return;
		}
#endif /* NDC and NPM */
		goto remcons;

	    case DS_MAXINE:
#if NDTOP > 0
		if (kbd == 3) {
			cd.cn_dev = makedev(DTOPDEV, 0);
			cd.cn_getc = dtopKBDGetc;
		} else
#endif /* NDTOP */
			goto remcons;
#if NXCFB > 0
		if (crt == 3 && xcfbinit()) {
			cd.cn_pri = CN_INTERNAL;
			return;
		}
#endif /* XCFB */
		break;

	    case DS_3MAX:
#if NDC > 0
		if (kbd == 7) {
			cd.cn_dev = makedev(DCDEV, DCKBD_PORT);
			cd.cn_getc = dcGetc;
		} else
#endif /* NDC */
			goto remcons;
		break;

	    case DS_3MIN:
	    case DS_3MAXPLUS:
#if NSCC > 0
		if (kbd == 3) {
			cd.cn_dev = makedev(SCCDEV, SCCKBD_PORT);
			cd.cn_getc = sccGetc;
		} else
#endif /* NSCC */
			goto remcons;
		break;

	    default:
		goto remcons;
	    };

	    /*
	     * Check for a suitable turbochannel frame buffer.
	     */
	    if (tc_slot_info[crt].driver_name) {
#if NMFB > 0
		if (strcmp(tc_slot_info[crt].driver_name, "mfb") == 0 &&
		    mfbinit(tc_slot_info[crt].k1seg_address)) {
			cd.cn_pri = CN_NORMAL;
			return;
		}
#endif /* NMFB */
#if NSFB > 0
		if (strcmp(tc_slot_info[crt].driver_name, "sfb") == 0 &&
		    sfbinit(tc_slot_info[crt].k1seg_address, 0)) {
			cd.cn_pri = CN_NORMAL;
			return;
		}
#endif /* NSFB */
#if NCFB > 0
		if (strcmp(tc_slot_info[crt].driver_name, "cfb") == 0 &&
		    cfbinit(tc_slot_info[crt].k1seg_address)) {
			cd.cn_pri = CN_NORMAL;
			return;
		}
#endif /* NCFB */
		printf("crt: %s not supported as console device\n",
			tc_slot_info[crt].driver_name);
	    } else
		printf("No crt console device in slot %d\n", crt);
	}
remcons:
	/*
	 * Configure a serial port as a remote console.
	 */
	switch (pmax_boardtype) {
	case DS_PMAX:
#if NDC > 0
		if (kbd == 4)
			cd.cn_dev = makedev(DCDEV, DCCOMM_PORT);
		else
			cd.cn_dev = makedev(DCDEV, DCPRINTER_PORT);
		cd.cn_getc = dcGetc;
		cd.cn_putc = dcPutc;
		cd.cn_pri = CN_REMOTE;
#endif /* NDC */
		break;

	case DS_3MAX:
#if NDC > 0
		cd.cn_dev = makedev(DCDEV, DCPRINTER_PORT);
		cd.cn_getc = dcGetc;
		cd.cn_putc = dcPutc;
		cd.cn_pri = CN_REMOTE;
#endif /* NDC */
		break;

	case DS_3MIN:
	case DS_3MAXPLUS:
#if NSCC > 0
		cd.cn_dev = makedev(SCCDEV, SCCCOMM3_PORT);
		cd.cn_getc = sccGetc;
		cd.cn_putc = sccPutc;
		cd.cn_pri = CN_REMOTE;
#endif /* NSCC */
		break;

	case DS_MAXINE:
#if NSCC > 0
		cd.cn_dev = makedev(SCCDEV, SCCCOMM2_PORT);
		cd.cn_getc = sccGetc;
		cd.cn_putc = sccPutc;
		cd.cn_pri = CN_REMOTE;
#endif /* NSCC */
		break;
	};
	if (cd.cn_dev == NODEV)
		printf("Can't configure console!\n");
}

/*
 * Get character from ROM console.
 */
static int romgetc(dev)
	dev_t dev;
{
	int s = splhigh ();
	int chr;
	chr = (*callv->_getchar)();
	splx (s);
	return chr;
}

/*
 * Print a character on ROM console.
 */
static void romputc (dev, c)
	dev_t dev;
	register int c;
{
	int s;
	s = splhigh();
	(*callv->_printf)("%c", c);
	splx(s);
}

static void rompollc (dev, c)
	dev_t dev;
	register int c;
{
	return;
}

