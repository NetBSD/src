/* $NetBSD: wsdisplay_compat_usl.c,v 1.34 2006/10/10 10:23:58 elad Exp $ */

/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wsdisplay_compat_usl.c,v 1.34 2006/10/10 10:23:58 elad Exp $");

#include "opt_compat_freebsd.h"
#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/kauth.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wscons_callbacks.h>
#include <dev/wscons/wsdisplay_usl_io.h>

#include "opt_wsdisplay_compat.h"

struct usl_syncdata {
	struct wsscreen *s_scr;
	struct proc *s_proc;
	pid_t s_pid;
	int s_flags;
#define SF_DETACHPENDING 1
#define SF_ATTACHPENDING 2
	int s_acqsig, s_relsig;
	int s_frsig; /* unused */
	void (*s_callback)(void *, int, int);
	void *s_cbarg;
	struct callout s_attach_ch;
	struct callout s_detach_ch;
};

static int usl_sync_init(struct wsscreen *, struct usl_syncdata **,
			      struct proc *, int, int, int);
static void usl_sync_done(struct usl_syncdata *);
static int usl_sync_check(struct usl_syncdata *);
static struct usl_syncdata *usl_sync_get(struct wsscreen *);

static int usl_detachproc(void *, int, void (*)(void *, int, int), void *);
static int usl_detachack(struct usl_syncdata *, int);
static void usl_detachtimeout(void *);
static int usl_attachproc(void *, int, void (*)(void *, int, int), void *);
static int usl_attachack(struct usl_syncdata *, int);
static void usl_attachtimeout(void *);

static const struct wscons_syncops usl_syncops = {
	usl_detachproc,
	usl_attachproc,
#define _usl_sync_check ((int (*)(void *))usl_sync_check)
	_usl_sync_check,
#define _usl_sync_destroy ((void (*)(void *))usl_sync_done)
	_usl_sync_destroy
};

#ifndef WSCOMPAT_USL_SYNCTIMEOUT
#define WSCOMPAT_USL_SYNCTIMEOUT 5 /* seconds */
#endif
static int wscompat_usl_synctimeout = WSCOMPAT_USL_SYNCTIMEOUT;

static int
usl_sync_init(struct wsscreen *scr, struct usl_syncdata **sdp,
	struct proc *p, int acqsig, int relsig, int frsig)
{
	struct usl_syncdata *sd;
	int res;

	sd = malloc(sizeof(struct usl_syncdata), M_DEVBUF, M_WAITOK);
	if (!sd)
		return (ENOMEM);
	sd->s_scr = scr;
	sd->s_proc = p;
	sd->s_pid = p->p_pid;
	sd->s_flags = 0;
	sd->s_acqsig = acqsig;
	sd->s_relsig = relsig;
	sd->s_frsig = frsig;
	callout_init(&sd->s_attach_ch);
	callout_init(&sd->s_detach_ch);
	res = wsscreen_attach_sync(scr, &usl_syncops, sd);
	if (res) {
		free(sd, M_DEVBUF);
		return (res);
	}
	*sdp = sd;
	return (0);
}

static void
usl_sync_done(struct usl_syncdata *sd)
{
	if (sd->s_flags & SF_DETACHPENDING) {
		callout_stop(&sd->s_detach_ch);
		(*sd->s_callback)(sd->s_cbarg, 0, 0);
	}
	if (sd->s_flags & SF_ATTACHPENDING) {
		callout_stop(&sd->s_attach_ch);
		(*sd->s_callback)(sd->s_cbarg, ENXIO, 0);
	}
	wsscreen_detach_sync(sd->s_scr);
	free(sd, M_DEVBUF);
}

static int
usl_sync_check(struct usl_syncdata *sd)
{
	if (sd->s_proc == pfind(sd->s_pid))
		return (1);
	printf("usl_sync_check: process %d died\n", sd->s_pid);
	usl_sync_done(sd);
	return (0);
}

static struct usl_syncdata *
usl_sync_get(struct wsscreen *scr)
{
	void *sd;

	if (wsscreen_lookup_sync(scr, &usl_syncops, &sd))
		return (0);
	return (struct usl_syncdata *)sd;
}

static int
usl_detachproc(void *cookie, int waitok, void (*callback)(void *, int, int),
	       void *cbarg)
{
	struct usl_syncdata *sd = cookie;

	if (!usl_sync_check(sd))
		return (0);

	/* we really need a callback */
	if (!callback)
		return (EINVAL);

	/*
	 * Normally, this is called from the controlling process.
	 * Is is supposed to reply with a VT_RELDISP ioctl(), so
	 * it is not useful to tsleep() here.
	 */
	sd->s_callback = callback;
	sd->s_cbarg = cbarg;
	sd->s_flags |= SF_DETACHPENDING;
	psignal(sd->s_proc, sd->s_relsig);
	callout_reset(&sd->s_detach_ch, wscompat_usl_synctimeout * hz,
	    usl_detachtimeout, sd);

	return (EAGAIN);
}

static int
usl_detachack(struct usl_syncdata *sd, int ack)
{
	if (!(sd->s_flags & SF_DETACHPENDING)) {
		printf("usl_detachack: not detaching\n");
		return (EINVAL);
	}

	callout_stop(&sd->s_detach_ch);
	sd->s_flags &= ~SF_DETACHPENDING;

	if (sd->s_callback)
		(*sd->s_callback)(sd->s_cbarg, (ack ? 0 : EIO), 1);

	return (0);
}

static void
usl_detachtimeout(void *arg)
{
	struct usl_syncdata *sd = arg;

	printf("usl_detachtimeout\n");

	if (!(sd->s_flags & SF_DETACHPENDING)) {
		printf("usl_detachtimeout: not detaching\n");
		return;
	}

	sd->s_flags &= ~SF_DETACHPENDING;

	if (sd->s_callback)
		(*sd->s_callback)(sd->s_cbarg, EIO, 0);

	(void) usl_sync_check(sd);
}

static int
usl_attachproc(void *cookie, int waitok, void (*callback)(void *, int, int),
	       void *cbarg)
{
	struct usl_syncdata *sd = cookie;

	if (!usl_sync_check(sd))
		return (0);

	/* we really need a callback */
	if (!callback)
		return (EINVAL);

	sd->s_callback = callback;
	sd->s_cbarg = cbarg;
	sd->s_flags |= SF_ATTACHPENDING;
	psignal(sd->s_proc, sd->s_acqsig);
	callout_reset(&sd->s_attach_ch, wscompat_usl_synctimeout * hz,
	    usl_attachtimeout, sd);

	return (EAGAIN);
}

static int
usl_attachack(struct usl_syncdata *sd, int ack)
{
	if (!(sd->s_flags & SF_ATTACHPENDING)) {
		printf("usl_attachack: not attaching\n");
		return (EINVAL);
	}

	callout_stop(&sd->s_attach_ch);
	sd->s_flags &= ~SF_ATTACHPENDING;

	if (sd->s_callback)
		(*sd->s_callback)(sd->s_cbarg, (ack ? 0 : EIO), 1);

	return (0);
}

static void
usl_attachtimeout(void *arg)
{
	struct usl_syncdata *sd = arg;

	printf("usl_attachtimeout\n");

	if (!(sd->s_flags & SF_ATTACHPENDING)) {
		printf("usl_attachtimeout: not attaching\n");
		return;
	}

	sd->s_flags &= ~SF_ATTACHPENDING;

	if (sd->s_callback)
		(*sd->s_callback)(sd->s_cbarg, EIO, 0);

	(void) usl_sync_check(sd);
}

int
wsdisplay_usl_ioctl1(struct wsdisplay_softc *sc, u_long cmd, caddr_t data,
		     int flag, struct lwp *l)
{
	int idx, maxidx;

	switch (cmd) {
	    case VT_OPENQRY:
		maxidx = wsdisplay_maxscreenidx(sc);
		for (idx = 0; idx <= maxidx; idx++) {
			if (wsdisplay_screenstate(sc, idx) == 0) {
				*(int *)data = idx + 1;
				return (0);
			}
		}
		return (ENXIO);
	    case VT_GETACTIVE:
		idx = wsdisplay_getactivescreen(sc);
		*(int *)data = idx + 1;
		return (0);
	    case VT_ACTIVATE:
	    	/*
	    	 * a gross and disgusting hack to make this abused up ioctl, 
		 * which is a gross and disgusting hack on its own, work on
		 * LP64/BE - we want the lower 32bit so we simply dereference
		 * the argument pointer as long. May cause problems with 32bit
		 * kernels on sparc64?
		 */

		idx = *(long *)data - 1;
		if (idx < 0)
			return (EINVAL);
		return (wsdisplay_switch((struct device *)sc, idx, 1));
	    case VT_WAITACTIVE:
		idx = *(long *)data - 1;
		if (idx < 0)
			return (EINVAL);
		return (wsscreen_switchwait(sc, idx));
	    case VT_GETSTATE:
#define ss ((struct vt_stat *)data)
		idx = wsdisplay_getactivescreen(sc);
		ss->v_active = idx + 1;
		ss->v_state = 0;
		maxidx = wsdisplay_maxscreenidx(sc);
		for (idx = 0; idx <= maxidx; idx++)
			if (wsdisplay_screenstate(sc, idx) == EBUSY)
				ss->v_state |= (1 << (idx + 1));
#undef ss
		return (0);

#ifdef WSDISPLAY_COMPAT_PCVT
	    case VGAPCVTID:
#define id ((struct pcvtid *)data)
		strlcpy(id->name, "pcvt", sizeof(id->name));
		id->rmajor = 3;
		id->rminor = 32;
#undef id
		return (0);
#endif
#ifdef WSDISPLAY_COMPAT_SYSCONS
	    case CONS_GETVERS:
		*(int *)data = 0x200;    /* version 2.0 */
		return (0);
#endif

	    default:
		return (EPASSTHROUGH);
	}
}

int
wsdisplay_usl_ioctl2(struct wsdisplay_softc *sc, struct wsscreen *scr,
		     u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct proc *p = l->l_proc;
	int intarg = 0, res;
	u_long req;
	void *arg;
	struct usl_syncdata *sd;
	struct wskbd_bell_data bd;

	switch (cmd) {
	    case VT_SETMODE:
#define newmode ((struct vt_mode *)data)
		if (newmode->mode == VT_PROCESS) {
			res = usl_sync_init(scr, &sd, p, newmode->acqsig,
					    newmode->relsig, newmode->frsig);
			if (res)
				return (res);
		} else {
			sd = usl_sync_get(scr);
			if (sd)
				usl_sync_done(sd);
		}
#undef newmode
		return (0);
	    case VT_GETMODE:
#define cmode ((struct vt_mode *)data)
		sd = usl_sync_get(scr);
		if (sd) {
			cmode->mode = VT_PROCESS;
			cmode->relsig = sd->s_relsig;
			cmode->acqsig = sd->s_acqsig;
			cmode->frsig = sd->s_frsig;
		} else
			cmode->mode = VT_AUTO;
#undef cmode
		return (0);
	    case VT_RELDISP:
#define d (*(long *)data)
		sd = usl_sync_get(scr);
		if (!sd)
			return (EINVAL);
		switch (d) {
		    case VT_FALSE:
		    case VT_TRUE:
			return (usl_detachack(sd, (d == VT_TRUE)));
		    case VT_ACKACQ:
			return (usl_attachack(sd, 1));
		    default:
			return (EINVAL);
		}
#undef d

	    case KDENABIO:
#if defined(__i386__)
#if defined(COMPAT_10) || defined(COMPAT_11) || defined(COMPAT_FREEBSD)
		if (kauth_authorize_machdep(l->l_cred, KAUTH_MACHDEP_X86,
		    KAUTH_REQ_MACHDEP_X86_IOPL, NULL, NULL, NULL) != 0)
			return (EPERM);
#endif
#endif
		/* FALLTHRU */
	    case KDDISABIO:
#if defined(__i386__)
#if defined(COMPAT_10) || defined(COMPAT_11) || defined(COMPAT_FREEBSD)
		{
			/* XXX NJWLWP */
		struct trapframe *fp = (struct trapframe *)curlwp->l_md.md_regs;
		if (cmd == KDENABIO)
			fp->tf_eflags |= PSL_IOPL;
		else
			fp->tf_eflags &= ~PSL_IOPL;
		}
#endif
#endif
		return (0);
	    case KDSETRAD:
		/* XXX ignore for now */
		return (0);

	    default:
		return (EPASSTHROUGH);

	    /*
	     * the following are converted to wsdisplay ioctls
	     */
	    case KDSETMODE:
		req = WSDISPLAYIO_SMODE;
#define d (*(int *)data)
		switch (d) {
		    case KD_GRAPHICS:
			intarg = WSDISPLAYIO_MODE_MAPPED;
			break;
		    case KD_TEXT:
			intarg = WSDISPLAYIO_MODE_EMUL;
			break;
		    default:
			return (EINVAL);
		}
#undef d
		arg = &intarg;
		break;
	    case KDMKTONE:
		req = WSKBDIO_COMPLEXBELL;
#define d (*(int *)data)
		if (d) {
#define PCVT_SYSBEEPF	1193182
			if (d >> 16) {
				bd.which = WSKBD_BELL_DOPERIOD;
				bd.period = d >> 16; /* ms */
			}
			else
				bd.which = 0;
			if (d & 0xffff) {
				bd.which |= WSKBD_BELL_DOPITCH;
				bd.pitch = PCVT_SYSBEEPF/(d & 0xffff); /* Hz */
			}
		} else
			bd.which = 0; /* default */
#undef d
		arg = &bd;
		break;
	    case KDSETLED:
		req = WSKBDIO_SETLEDS;
		intarg = 0;
#define d (*(int *)data)
		if (d & LED_CAP)
			intarg |= WSKBD_LED_CAPS;
		if (d & LED_NUM)
			intarg |= WSKBD_LED_NUM;
		if (d & LED_SCR)
			intarg |= WSKBD_LED_SCROLL;
#undef d
		arg = &intarg;
		break;
	    case KDGETLED:
		req = WSKBDIO_GETLEDS;
		arg = &intarg;
		break;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	    case KDSKBMODE:
		req = WSKBDIO_SETMODE;
		switch (*(int *)data) {
		    case K_RAW:
			intarg = WSKBD_RAW;
			break;
		    case K_XLATE:
			intarg = WSKBD_TRANSLATED;
			break;
		    default:
			return (EINVAL);
		}
		arg = &intarg;
		break;
	    case KDGKBMODE:
		req = WSKBDIO_GETMODE;
		arg = &intarg;
		break;
#endif
	}

	res = wsdisplay_internal_ioctl(sc, scr, req, arg, flag, l);
	if (res != EPASSTHROUGH)
		return (res);

	switch (cmd) {
	    case KDGETLED:
#define d (*(int *)data)
		d = 0;
		if (intarg & WSKBD_LED_CAPS)
			d |= LED_CAP;
		if (intarg & WSKBD_LED_NUM)
			d |= LED_NUM;
		if (intarg & WSKBD_LED_SCROLL)
			d |= LED_SCR;
#undef d
		break;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	    case KDGKBMODE:
		*(int *)data = (intarg == WSKBD_RAW ? K_RAW : K_XLATE);
		break;
#endif
	}

	return (0);
}
