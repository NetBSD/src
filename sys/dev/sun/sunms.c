/*	$NetBSD: sunms.c,v 1.4.4.1 2001/10/10 11:57:02 fvdl Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)ms.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Mouse driver (/dev/mouse)
 */

/*
 * Zilog Z8530 Dual UART driver (mouse interface)
 *
 * This is the "slave" driver that will be attached to
 * the "zsc" driver for a Sun mouse.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/syslog.h>
#include <sys/fcntl.h>
#include <sys/tty.h>
#include <sys/vnode.h>

#include <machine/vuid_event.h>

#include <sys/tty.h>
#include <dev/sun/event_var.h>
#include <dev/sun/msvar.h>
#include <dev/sun/kbd_ms_ttyvar.h>

#include "ms.h"
#if NMS > 0

int	sunms_bps = MS_BPS;

static int	sunms_match(struct device *, struct cfdata *, void *);
static void	sunms_attach(struct device *, struct device *, void *);
static int	sunmsiopen(struct device *, int mode);
int	sunmsinput(int, struct tty *);

struct cfattach ms_ca = {
	sizeof(struct ms_softc), sunms_match, sunms_attach
};

struct  linesw sunms_disc =
	{ "sunms", 8, ttylopen, ttylclose, ttyerrio, ttyerrio, nullioctl,
	  sunmsinput, ttstart, nullmodem, ttpoll };	/* 8- SUNMOUSEDISC */

/*
 * ms_match: how is this zs channel configured?
 */
int
sunms_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void   *aux;
{
	struct kbd_ms_tty_attach_args *args = aux;

	if (sunms_bps == 0)
		return 0;

	if (strcmp(args->kmta_name, "mouse") == 0)
		return (1);

	return 0;
}

void
sunms_attach(parent, self, aux)
	struct device *parent, *self;
	void   *aux;

{
	struct ms_softc *ms = (void *) self;
	struct kbd_ms_tty_attach_args *args = aux;
	struct cfdata *cf;
	struct tty *tp = args->kmta_tp;
	int ms_unit;

	cf = ms->ms_dev.dv_cfdata;
	ms_unit = ms->ms_dev.dv_unit;
	tp->t_sc  = ms;
	ms->ms_cs = (struct zs_chanstate *)tp;
        ms->ms_deviopen = sunmsiopen;
        ms->ms_deviclose = NULL;

	printf("\n");

	/* Initialize the speed, etc. */
	if (ttyldisc_add(&sunms_disc, -1) == -1)
		panic("sunms_attach: sunms_disc");
	tp->t_linesw = &sunms_disc;
	tp->t_oflag &= ~OPOST;

	/* Initialize translator. */
	ms->ms_byteno = -1;
}

/*
 * Internal open routine.  This really should be inside com.c
 * But I'm putting it here until we have a generic internal open
 * mechanism.
 */
int
sunmsiopen(dev, flags)
	struct device *dev;
	int flags;
{
	struct ms_softc *ms = (void *) dev;
	struct tty *tp = (struct tty *)ms->ms_cs;
	struct proc *p = curproc;
	struct termios t;
	int maj;
	int error;
	dev_t rdev;

	rdev = vdev_rdev(tp->t_devvp);
	maj = major(rdev);
	if (p == NULL)
		p = &proc0;

	/* Open the lower device */
	if ((error = (*cdevsw[maj].d_open)(tp->t_devvp, O_NONBLOCK|flags,
					   0/* ignored? */, p)) != 0)
		return (error);

	/* Now configure it for the console. */
	tp->t_ospeed = 0;
	t.c_ispeed = sunms_bps;
	t.c_ospeed = sunms_bps;
	t.c_cflag =  CLOCAL;
	(*tp->t_param)(tp, &t);

	return (0);
}

int
sunmsinput(c, tp)
	int c;
	struct tty *tp;
{
	struct ms_softc *ms = (struct ms_softc *)tp->t_sc;

	if (c & TTY_ERRORMASK) c = -1;
	else c &= TTY_CHARMASK;

	/* Pass this up to the "middle" layer. */
	ms_input(ms, c);
	return (0);
}
#endif
