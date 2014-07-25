/*	$NetBSD: wdog.c,v 1.18 2014/07/25 08:10:34 dholland Exp $ */

/*-
 * Copyright (C) 2000 SAITOH Masanobu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdog.c,v 1.18 2014/07/25 08:10:34 dholland Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/conf.h>

#include <machine/cpu.h>
#include <machine/intr.h>

#include <sh3/frame.h>
#include <sh3/wdtreg.h>
#include <sh3/wdogvar.h>
#include <sh3/exception.h>

struct wdog_softc {
	device_t sc_dev;
	int flags;
};

static int wdogmatch(device_t, cfdata_t, void *);
static void wdogattach(device_t, device_t, void *);
static int wdogintr(void *);

CFATTACH_DECL_NEW(wdog, sizeof(struct wdog_softc),
    wdogmatch, wdogattach, NULL, NULL);

extern struct cfdriver wdog_cd;

dev_type_open(wdogopen);
dev_type_close(wdogclose);
dev_type_ioctl(wdogioctl);

const struct cdevsw wdog_cdevsw = {
	.d_open = wdogopen,
	.d_close = wdogclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = wdogioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

void
wdog_wr_cnt(unsigned char x)
{

	SHREG_WTCNT_W = WTCNT_W_M | (unsigned short) x;
}

void
wdog_wr_csr(unsigned char x)
{

	SHREG_WTCSR_W = WTCSR_W_M | (unsigned short) x;
}

static int
wdogmatch(device_t parent, cfdata_t cfp, void *aux)
{

	if (strcmp(cfp->cf_name, "wdog"))
		return (0);

	return (1);
}

/*
 *  functions for probeing.
 */
/* ARGSUSED */
static void
wdogattach(device_t parent, device_t self, void *aux)
{
	struct wdog_softc *sc;

	sc = device_private(self);
	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal(": internal watchdog timer\n");

	wdog_wr_csr(WTCSR_WT | WTCSR_CKS_4096);	/* default to wt mode */

	intc_intr_establish(SH_INTEVT_WDT_ITI, IST_LEVEL, IPL_SOFTCLOCK,
	    wdogintr, 0);
}

/*ARGSUSED*/
int
wdogopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct wdog_softc *sc;

	sc = device_lookup_private(&wdog_cd, minor(dev));
	if (sc == NULL)
		return (ENXIO);

	if (sc->flags & WDOGF_OPEN)
		return (EBUSY);
	sc->flags |= WDOGF_OPEN;
	return (0);
}

/*ARGSUSED*/
int
wdogclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct wdog_softc *sc;

	sc = device_lookup_private(&wdog_cd, minor(dev));

	if (sc->flags & WDOGF_OPEN)
		sc->flags = 0;

	return (0);
}

extern unsigned int maxwdog;

/*ARGSUSED*/
int
wdogioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int error = 0;
	int request;

	switch (cmd) {
	case SIOWDOGSETMODE:
		request = *(int *)data;

		switch (request) {
		case WDOGM_RESET:
			wdog_wr_csr(SHREG_WTCSR_R | WTCSR_WT);
			break;
		case WDOGM_INTR:
			wdog_wr_csr(SHREG_WTCSR_R & ~WTCSR_WT);
			break;
		default:
			error = EINVAL;
			break;
		}
		break;
	case SIORESETWDOG:
		wdog_wr_cnt(0);		/* reset to zero */
		break;
	case SIOSTARTWDOG:
		wdog_wr_csr(WTCSR_WT | WTCSR_CKS_4096);
		wdog_wr_cnt(0);		/* reset to zero */
		wdog_wr_csr(SHREG_WTCSR_R | WTCSR_TME);	/* start!!! */
		break;
	case SIOSTOPWDOG:
		wdog_wr_csr(SHREG_WTCSR_R & ~WTCSR_TME); /* stop */
		break;
	case SIOSETWDOG:
		request = *(int *)data;

		if (request > 2) {
			error = EINVAL;
			break;
		}
		break;
	default:
		error = EINVAL;
		break;
	}

	return (error);
}

int
wdogintr(void *arg)
{
	struct trapframe *frame = arg;

	wdog_wr_csr(SHREG_WTCSR_R & ~WTCSR_IOVF); /* clear overflow bit */
	wdog_wr_cnt(0);			/* reset to zero */
	printf("wdog trapped: spc = %x\n", frame->tf_spc);

	return (0);
}
