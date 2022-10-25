/*	$NetBSD: cons.c,v 1.92 2022/10/25 23:21:33 riastradh Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
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
 * from: Utah $Hdr: cons.c 1.7 92/01/21$
 *
 *	@(#)cons.c	8.2 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cons.c,v 1.92 2022/10/25 23:21:33 riastradh Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <sys/kauth.h>
#include <sys/mutex.h>
#include <sys/module.h>
#include <sys/atomic.h>
#include <sys/pserialize.h>

#include <dev/cons.h>

#include "nullcons.h"

dev_type_open(cnopen);
dev_type_close(cnclose);
dev_type_read(cnread);
dev_type_write(cnwrite);
dev_type_ioctl(cnioctl);
dev_type_poll(cnpoll);
dev_type_kqfilter(cnkqfilter);

static bool cn_redirect(dev_t *, int, int *, struct tty **);
static void cn_release(struct tty *);

const struct cdevsw cons_cdevsw = {
	.d_open = cnopen,
	.d_close = cnclose,
	.d_read = cnread,
	.d_write = cnwrite,
	.d_ioctl = cnioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = cnpoll,
	.d_mmap = nommap,
	.d_kqfilter = cnkqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY|D_MPSAFE,
};

static struct kmutex cn_lock;

struct	tty *volatile constty;	/* virtual console output device */
struct	consdev *cn_tab;	/* physical console device info */
struct	vnode *cn_devvp[2];	/* vnode for underlying device. */

void
cn_set_tab(struct consdev *tab)
{

	/*
	 * This is a point that we should have KASSERT(cold) or add
	 * synchronization in case this can happen after cold boot.
	 * However, cn_tab initialization is so critical to any
	 * diagnostics or debugging that we need to tread carefully
	 * about introducing new ways to crash.  So let's put the
	 * assertion in only after we've audited most or all of the
	 * cn_tab updates.
	 */
	cn_tab = tab;
}

int
cnopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	dev_t cndev;
	int unit, error;

	unit = minor(dev);
	if (unit > 1)
		return ENODEV;

	mutex_enter(&cn_lock);

	if (cn_tab == NULL) {
		error = 0;
		goto out;
	}

	/*
	 * always open the 'real' console device, so we don't get nailed
	 * later.  This follows normal device semantics; they always get
	 * open() calls.
	 */
	cndev = cn_tab->cn_dev;
#if NNULLCONS > 0
	if (cndev == NODEV) {
		nullconsattach(0);
	}
#else /* NNULLCONS > 0 */
	if (cndev == NODEV) {
		/*
		 * This is most likely an error in the console attach
		 * code. Panicking looks better than jumping into nowhere
		 * through cdevsw below....
		 */
		panic("cnopen: no console device");
	}
#endif /* NNULLCONS > 0 */
	if (dev == cndev) {
		/*
		 * This causes cnopen() to be called recursively, which
		 * is generally a bad thing.  It is often caused when
		 * dev == 0 and cn_dev has not been set, but was probably
		 * initialised to 0.
		 */
		panic("cnopen: cn_tab->cn_dev == dev");
	}
	if (cn_devvp[unit] != NULLVP) {
		error = 0;
		goto out;
	}
	if ((error = cdevvp(cndev, &cn_devvp[unit])) != 0) {
		printf("cnopen: unable to get vnode reference\n");
		goto out;
	}
	vn_lock(cn_devvp[unit], LK_EXCLUSIVE | LK_RETRY);
	error = VOP_OPEN(cn_devvp[unit], flag, kauth_cred_get());
	VOP_UNLOCK(cn_devvp[unit]);

out:	mutex_exit(&cn_lock);
	return error;
}

int
cnclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct vnode *vp;
	int unit, error;

	unit = minor(dev);
	if (unit > 1)
		return ENODEV;

	mutex_enter(&cn_lock);

	if (cn_tab == NULL) {
		error = 0;
		goto out;
	}

	vp = cn_devvp[unit];
	cn_devvp[unit] = NULL;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_CLOSE(vp, flag, kauth_cred_get());
	VOP_UNLOCK(vp);
	vrele(vp);

out:	mutex_exit(&cn_lock);
	return error;
}

int
cnread(dev_t dev, struct uio *uio, int flag)
{
	struct tty *ctp = NULL;
	int error;

	/*
	 * If we would redirect input, punt.  This will keep strange
	 * things from happening to people who are using the real
	 * console.  Nothing should be using /dev/console for
	 * input (except a shell in single-user mode, but then,
	 * one wouldn't TIOCCONS then).
	 */
	if (!cn_redirect(&dev, 1, &error, &ctp))
		return error;
	error = cdev_read(dev, uio, flag);
	cn_release(ctp);
	return error;
}

int
cnwrite(dev_t dev, struct uio *uio, int flag)
{
	struct tty *ctp = NULL;
	int error;

	/* Redirect output, if that's appropriate. */
	if (!cn_redirect(&dev, 0, &error, &ctp))
		return error;
	error = cdev_write(dev, uio, flag);
	cn_release(ctp);
	return error;
}

int
cnioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct tty *ctp = NULL;
	int error;

	error = 0;

	/*
	 * Superuser can always use this to wrest control of console
	 * output from the "virtual" console.
	 */
	if (cmd == TIOCCONS) {
		struct tty *tp;

		mutex_enter(&constty_lock);
		tp = atomic_load_relaxed(&constty);
		if (tp == NULL) {
			mutex_exit(&constty_lock);
			goto passthrough; /* XXX ??? */
		}
		error = kauth_authorize_device_tty(l->l_cred,
		    KAUTH_DEVICE_TTY_VIRTUAL, tp);
		if (!error)
			atomic_store_relaxed(&constty, NULL);
		mutex_exit(&constty_lock);
		return error;
	}
passthrough:
	/*
	 * Redirect the ioctl, if that's appropriate.
	 * Note that strange things can happen, if a program does
	 * ioctls on /dev/console, then the console is redirected
	 * out from under it.
	 */
	if (!cn_redirect(&dev, 0, &error, &ctp))
		return error;
	error = cdev_ioctl(dev, cmd, data, flag, l);
	cn_release(ctp);
	return error;
}

/*ARGSUSED*/
int
cnpoll(dev_t dev, int events, struct lwp *l)
{
	struct tty *ctp = NULL;
	int error;

	/*
	 * Redirect the poll, if that's appropriate.
	 * I don't want to think of the possible side effects
	 * of console redirection here.
	 */
	if (!cn_redirect(&dev, 0, &error, &ctp))
		return POLLHUP;
	error = cdev_poll(dev, events, l);
	cn_release(ctp);
	return error;
}

/*ARGSUSED*/
int
cnkqfilter(dev_t dev, struct knote *kn)
{
	struct tty *ctp = NULL;
	int error;

	/*
	 * Redirect the kqfilter, if that's appropriate.
	 * I don't want to think of the possible side effects
	 * of console redirection here.
	 */
	if (!cn_redirect(&dev, 0, &error, &ctp))
		return error;
	error = cdev_kqfilter(dev, kn);
	cn_release(ctp);
	return error;
}

int
cngetc(void)
{
	if (cn_tab == NULL)
		return (0);
	int s = splhigh();
	for (;;) {
		const int rv = (*cn_tab->cn_getc)(cn_tab->cn_dev);
		if (rv >= 0) {
			splx(s);
			return rv;
		}
		docritpollhooks();
	}
}

int
cngetsn(char *cp, int size)
{
	char *lp;
	int c, len;

	cnpollc(1);

	lp = cp;
	len = 0;
	for (;;) {
		c = cngetc();
		switch (c) {
		case '\n':
		case '\r':
			printf("\n");
			*lp++ = '\0';
			cnpollc(0);
			return (len);
		case '\b':
		case '\177':
		case '#':
			if (len) {
				--len;
				--lp;
				printf("\b \b");
			}
			continue;
		case '@':
		case 'u'&037:	/* CTRL-u */
			len = 0;
			lp = cp;
			printf("\n");
			continue;
		default:
			if (len + 1 >= size || c < ' ') {
				printf("\007");
				continue;
			}
			printf("%c", c);
			++len;
			*lp++ = c;
		}
	}
}

void
cnputc(int c)
{

	if (cn_tab == NULL)
		return;

/*
 * XXX
 * for some reason this causes ARCS firmware to output an endless stream of
 * whitespaces with n32 kernels, so use the pre-1.74 code for now until I can
 * figure out why this happens
 */
#ifndef sgimips
	if (c) {
		if (c == '\n') {
			(*cn_tab->cn_putc)(cn_tab->cn_dev, '\r');
			docritpollhooks();
		}
		(*cn_tab->cn_putc)(cn_tab->cn_dev, c);
	}
#else
	if (c) {
		(*cn_tab->cn_putc)(cn_tab->cn_dev, c);
		if (c == '\n') {
			docritpollhooks();
			(*cn_tab->cn_putc)(cn_tab->cn_dev, '\r');
		}
	}
#endif
}

void
cnpollc(int on)
{
	static int refcount = 0;

	if (cn_tab == NULL)
		return;
	if (!on)
		--refcount;
	if (refcount == 0)
		(*cn_tab->cn_pollc)(cn_tab->cn_dev, on);
	if (on)
		++refcount;
}

void
nullcnpollc(dev_t dev, int on)
{

}

void
cnbell(u_int pitch, u_int period, u_int volume)
{

	if (cn_tab == NULL || cn_tab->cn_bell == NULL)
		return;
	(*cn_tab->cn_bell)(cn_tab->cn_dev, pitch, period, volume);
}

void
cnflush(void)
{
	if (cn_tab == NULL || cn_tab->cn_flush == NULL)
		return;
	(*cn_tab->cn_flush)(cn_tab->cn_dev);
}

void
cnhalt(void)
{
	if (cn_tab == NULL || cn_tab->cn_halt == NULL)
		return;
	(*cn_tab->cn_halt)(cn_tab->cn_dev);
}

/*
 * Redirect output, if that's appropriate.  If there's no real console,
 * return ENXIO.
 */
static bool
cn_redirect(dev_t *devp, int is_read, int *error, struct tty **ctpp)
{
	dev_t dev = *devp;
	struct tty *ctp;
	int s;
	bool ok = false;

	*error = ENXIO;
	*ctpp = NULL;
	s = pserialize_read_enter();
	if ((ctp = atomic_load_consume(&constty)) != NULL && minor(dev) == 0 &&
	    (cn_tab == NULL || (cn_tab->cn_pri != CN_REMOTE))) {
		if (is_read) {
			*error = 0;
			goto out;
		}
		tty_acquire(ctp);
		*ctpp = ctp;
		dev = ctp->t_dev;
	} else if (cn_tab == NULL)
		goto out;
	else
		dev = cn_tab->cn_dev;
	ok = true;
	*devp = dev;
out:	pserialize_read_exit(s);
	return ok;
}

static void
cn_release(struct tty *ctp)
{

	if (ctp == NULL)
		return;
	tty_release(ctp);
}

MODULE(MODULE_CLASS_DRIVER, cons, NULL);

static int
cons_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		mutex_init(&cn_lock, MUTEX_DEFAULT, IPL_NONE);
		return 0;
	case MODULE_CMD_FINI:
		mutex_destroy(&cn_lock);
		return 0;
	default:
		return ENOTTY;
	}
}
