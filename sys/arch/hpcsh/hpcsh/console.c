/*	$NetBSD: console.c,v 1.14.14.1 2009/05/13 17:17:47 jym Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: console.c,v 1.14.14.1 2009/05/13 17:17:47 jym Exp $");

#include "opt_kgdb.h"
#include "biconsdev.h"
#include "hpcfb.h"
#include "scif.h"
#include "hd64461uart.h"
#include "hd64465uart.h"
#include "hd64461video.h"
#include "wskbd.h"
#include "pfckbd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <dev/cons.h> /* consdev */

#include <machine/bootinfo.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/hpc/bicons.h>
#include <dev/hpc/biconsvar.h>
#include <dev/hpc/hpcfbvar.h>
#include <sh3/dev/scifvar.h>
#include <hpcsh/dev/pfckbdvar.h>
#include <hpcsh/dev/hd64461/hd64461uartvar.h>
#include <hpcsh/dev/hd64465/hd64465uartvar.h>

/* Serial console */
#define scifcnpollc	nullcnpollc
cons_decl(scif);
#define	hd64461uartcnputc	comcnputc
#define	hd64461uartcngetc	comcngetc
#define	hd64461uartcnpollc	comcnpollc
cons_decl(hd64461uart);
#define	hd64465uartcnputc	comcnputc
#define	hd64465uartcngetc	comcngetc
#define	hd64465uartcnpollc	comcnpollc
cons_decl(hd64465uart);

/* Builtin video console */
#if NBICONSDEV > 0
#define biconscnpollc	nullcnpollc
cons_decl(bicons);
#endif

/* HD64461 video module */
#if NHD64461VIDEO > 0
cons_decl(hd64461video_);
#if NWSKBD > 0
#include <dev/wscons/wskbdvar.h>
#define hd64461video_cngetc	wskbd_cngetc
#else  /* NWSKBD == 0 */
int
hd64461video_cngetc(dev_t dev)
{
	printf("no input method. reboot me.\n");
	for (;;)
		continue;
	/* NOTREACHED */
}
#endif /* NWSKBD == 0 */
#define hd64461video_cnputc	wsdisplay_cnputc
#define	hd64461video_cnpollc	nullcnpollc
#endif /* NHD64461VIDEO > 0 */

struct consdev constab[] = {
#if NBICONSDEV > 0
	cons_init(bicons),
#endif
#if NHD64461VIDEO > 0
	cons_init(hd64461video_),
#endif
#if NSCIF > 0
	cons_init(scif),
#endif
#if NHD64461UART > 0
	cons_init(hd64461uart),
#endif
#if NHD64465UART > 0
	cons_init(hd64465uart),
#endif
	{ NULL }		/* terminator */
};

#define CN_ENABLE(x)	set_console(x ## cninit, x ## cnprobe)

static int initialized;
static int attach_kbd  __attribute__((__unused__)) = 0;
static void set_console(void (*)(struct consdev *), void (*)(struct consdev *));
static void disable_console(void);
static void cn_nonprobe(struct consdev *);
#if NBICONSDEV > 0
static void enable_bicons(void);
#endif

void
consinit(void)
{
	if (initialized)
		return;

	/* select console */
	disable_console();

	switch (bootinfo->bi_cnuse) {
	case BI_CNUSE_BUILTIN:
#if NBICONSDEV > 0
		enable_bicons();
#endif
		break;
	case BI_CNUSE_HD64461VIDEO:
#if NHD64461VIDEO > 0
		CN_ENABLE(hd64461video_);
		attach_kbd = 1;
#endif
		break;
	case BI_CNUSE_SCIF:
#if NSCIF > 0
		CN_ENABLE(scif);
#endif
		break;
	case BI_CNUSE_HD64461COM:
#if NHD64461UART > 0
		CN_ENABLE(hd64461uart);
#endif
		break;
	case BI_CNUSE_HD64465COM:
#if NHD64465UART > 0
		CN_ENABLE(hd64465uart);
#endif
		break;
	}

#if NBICONSDEV > 0
	if (!initialized) { /* use builtin console instead */
		enable_bicons();
	}
#endif

	if (initialized) {
		cninit();
	}

#if NPFCKBD > 0
	if (attach_kbd)
		pfckbd_cnattach();
#endif

#if NHPCFB > 0 && NBICONSDEV > 0
	if (cn_tab->cn_putc == biconscnputc)
		hpcfb_cnattach(0);
#endif

#ifdef KGDB
#if NSCIF > 0
	scif_kgdb_init();
#endif
#if NHD64461UART > 0
	hd64461uart_kgdb_init();
#endif
#if NHD64465UART > 0
	hd64465uart_kgdb_init();
#endif
#endif /* KGDB */
}

static void
set_console(void (*init_func)(struct consdev *),
    void (*probe_func)(struct consdev *))
{
	struct consdev *cp;

	for (cp = constab; cp->cn_probe; cp++) {
		if (cp->cn_init == init_func) {
			cp->cn_probe = probe_func;
			initialized = 1;
			break;
		}
	}
}

static void
disable_console(void)
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

#if NBICONSDEV > 0
static void
enable_bicons(void)
{

	bootinfo->bi_cnuse = BI_CNUSE_BUILTIN;
	bicons_set_priority(CN_INTERNAL);
	CN_ENABLE(bicons);
	attach_kbd = 1;
}
#endif /* NBICONSDEV > 0 */
