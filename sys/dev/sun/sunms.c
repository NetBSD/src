/*	$NetBSD: sunms.c,v 1.14 2003/05/30 23:34:06 petrov Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunms.c,v 1.14 2003/05/30 23:34:06 petrov Exp $");

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

#include <machine/vuid_event.h>

#include <sys/tty.h>
#include <dev/sun/event_var.h>
#include <dev/sun/msvar.h>
#include <dev/sun/kbd_ms_ttyvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include "ms.h"
#include "wsmouse.h"
#if NMS > 0

#ifdef SUN_MS_BPS
int	sunms_bps = SUN_MS_BPS;
#else
int	sunms_bps = MS_DEFAULT_BPS;
#endif

static int	sunms_match(struct device *, struct cfdata *, void *);
static void	sunms_attach(struct device *, struct device *, void *);
static int	sunmsiopen(struct device *, int mode);
int	sunmsinput(int, struct tty *);

CFATTACH_DECL(ms_tty, sizeof(struct ms_softc),
    sunms_match, sunms_attach, NULL, NULL);

struct  linesw sunms_disc =
	{ "sunms", 8, ttylopen, ttylclose, ttyerrio, ttyerrio, ttynullioctl,
	  sunmsinput, ttstart, nullmodem, ttpoll };	/* 8- SUNMOUSEDISC */

int	sunms_enable(void *);
int	sunms_ioctl(void *, u_long, caddr_t, int, struct proc *);
void	sunms_disable(void *);

const struct wsmouse_accessops	sunms_accessops = {
	sunms_enable,
	sunms_ioctl,
	sunms_disable,
};

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
#if NWSMOUSE > 0
	struct wsmousedev_attach_args a;
#endif

	cf = ms->ms_dev.dv_cfdata;
	ms_unit = ms->ms_dev.dv_unit;
	tp->t_sc  = ms;
	tp->t_dev = args->kmta_dev;
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

#if NWSMOUSE > 0
	/*
	 * attach wsmouse
	 */
	a.accessops = &sunms_accessops;
	a.accesscookie = ms;

	ms->ms_wsmousedev = config_found(self, &a, wsmousedevprint);
#endif
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
	struct proc *p = curproc ? curproc : &proc0;
	struct termios t;
	const struct cdevsw *cdev;
	int error;

	cdev = cdevsw_lookup(tp->t_dev);
	if (cdev == NULL)
		return (ENXIO);

	/* Open the lower device */
	if ((error = (*cdev->d_open)(tp->t_dev, O_NONBLOCK|flags,
				     0/* ignored? */, p)) != 0)
		return (error);

	/* Now configure it for the console. */
	tp->t_ospeed = 0;
	t.c_ispeed = sunms_bps;
	t.c_ospeed = sunms_bps;
	t.c_cflag =  CLOCAL|CS8;
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

int
sunms_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
/*	struct ms_softc *sc = v; */

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_PS2; /* XXX  */
		break;
		
	default:
		return (EPASSTHROUGH);
	}
	return (0);
}

int
sunms_enable(v)
	void *v;
{
	struct ms_softc *ms = v;
	int err;
	int s;

	if (ms->ms_ready)
		return EBUSY;

	err = sunmsiopen(v, 0);
	if (err)
		return err;

	s = spltty();
	ms->ms_ready = 2;
	splx(s);

	return 0;
}

void
sunms_disable(v)
	void *v;
{
	struct ms_softc *ms = v;
	int s;

	s = spltty();
	ms->ms_ready = 0;
	splx(s);
}
#endif
