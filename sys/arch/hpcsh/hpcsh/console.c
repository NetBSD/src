/*	$NetBSD: console.c,v 1.2.2.2 2001/03/27 15:30:59 bouyer Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include "biconsdev.h"
#include "hpcfb.h"
#include "pfckbd.h"
#include "sci.h"
#include "scif.h"
#include "com.h"
#include "hd64461if.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h> /* cdev_decl */
#include <dev/cons.h> /* consdev */
#include <dev/cons.h>

#include <machine/bootinfo.h>

#if NBICONSDEV > 0
#include <dev/hpc/biconsvar.h>
#include <dev/hpc/bicons.h>
#define DPRINTF(arg) printf arg
#else
#define DPRINTF(arg)
#endif

#if NHPCFB > 0
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/hpc/hpcfbvar.h>
#endif
#if NPFCKBD > 0
#include <hpcsh/dev/pfckbdvar.h>
#endif

/* serial console */
#define scicnpollc	nullcnpollc
cdev_decl(sci);
cons_decl(sci);
#define scifcnpollc	nullcnpollc
cdev_decl(scif);
cons_decl(scif);
cdev_decl(com);
cons_decl(com);

/* builtin video console */
#define biconscnpollc	nullcnpollc
cdev_decl(biconsdev);
cons_decl(bicons);

struct consdev constab[] = {
#if NBICONSDEV > 0
	cons_init(bicons),
#endif
#if NSCI > 0
	cons_init(sci),
#endif
#if NSCIF > 0
	cons_init(scif),
#endif
#if NHD64461IF > 0 && NCOM > 0
	cons_init(com),
#endif
	{ 0 } /* terminator */
};
#define CN_SELECTED	(CN_REMOTE + 1)		/* highest priority */
#define CN_ENABLE(x)	set_console(x##cnputc, x##cnprobe)

static int initialized;
static void set_console(void (*)(dev_t, int), void (*)(struct consdev *));
static void disable_console(void);
static void cn_nonprobe(struct consdev *);

void
consinit()
{
	if (initialized)
		return;

	/* select console */
	disable_console();

	switch (bootinfo->bi_cnuse) {
	case BI_CNUSE_BUILTIN:
#if NBICONSDEV > 0
		CN_ENABLE(bicons);
#endif
		break;
	case BI_CNUSE_SCI:
#if NSCI > 0
		CN_ENABLE(sci);
#endif
		break;
	case BI_CNUSE_SCIF:
#if NSCIF > 0
		CN_ENABLE(scif);
#endif
		break;
	case BI_CNUSE_HD64461COM:
#if NHD64461IF > 0 && NCOM > 0
		CN_ENABLE(com);
#endif
		break;
	}

#if NBICONSDEV > 0
	if (!initialized) { /* use builtin console instead */
		bootinfo->bi_cnuse = BI_CNUSE_BUILTIN;
		CN_ENABLE(bicons);
	}
#endif

	if (initialized)
		cninit();

	if (bootinfo->bi_cnuse == BI_CNUSE_BUILTIN) {
#if NPFCKBD > 0
		pfckbd_cnattach();
#endif
#if NHPCFB > 0
		hpcfb_cnattach(0);
#endif
	}
}

static void
set_console(void (*putc_func)(dev_t, int),
	    void (*probe_func)(struct consdev *))
{
	struct consdev *cp;

	for (cp = constab; cp->cn_probe; cp++) {
		if (cp->cn_putc == putc_func) {
			cp->cn_probe = probe_func;
			initialized = 1;
			break;
		}
	}
}

static void
disable_console()
{
	struct consdev *cp;	

	for (cp = constab; cp->cn_probe; cp++)
		cp->cn_probe = cn_nonprobe;
}

static void
cn_nonprobe(struct consdev *cp)
{
	cp->cn_pri = CN_DEAD;
}
