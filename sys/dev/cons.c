/*	$NetBSD: cons.c,v 1.57.12.1 2006/05/24 15:50:07 tron Exp $	*/

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
 * from: Utah $Hdr: cons.c 1.7 92/01/21$
 *
 *	@(#)cons.c	8.2 (Berkeley) 1/12/94
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
 * from: Utah $Hdr: cons.c 1.7 92/01/21$
 *
 *	@(#)cons.c	8.2 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cons.c,v 1.57.12.1 2006/05/24 15:50:07 tron Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <sys/kauth.h>

#include <dev/cons.h>

dev_type_open(cnopen);
dev_type_close(cnclose);
dev_type_read(cnread);
dev_type_write(cnwrite);
dev_type_ioctl(cnioctl);
dev_type_poll(cnpoll);
dev_type_kqfilter(cnkqfilter);

static const struct cdevsw *cn_redirect(dev_t *, int, int *);

const struct cdevsw cons_cdevsw = {
	cnopen, cnclose, cnread, cnwrite, cnioctl,
	nostop, notty, cnpoll, nommap, cnkqfilter, D_TTY
};

struct	tty *constty = NULL;	/* virtual console output device */
struct	consdev *cn_tab;	/* physical console device info */
struct	vnode *cn_devvp[2];	/* vnode for underlying device. */

int
cnopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	const struct cdevsw *cdev;
	dev_t cndev;
	int unit;

	unit = minor(dev);
	if (unit > 1)
		return ENODEV;

	if (cn_tab == NULL)
		return (0);

	/*
	 * always open the 'real' console device, so we don't get nailed
	 * later.  This follows normal device semantics; they always get
	 * open() calls.
	 */
	cndev = cn_tab->cn_dev;
	if (cndev == NODEV) {
		/*
		 * This is most likely an error in the console attach
		 * code. Panicking looks better than jumping into nowhere
		 * through cdevsw below....
		 */
		panic("cnopen: no console device");
	}
	if (dev == cndev) {
		/*
		 * This causes cnopen() to be called recursively, which
		 * is generally a bad thing.  It is often caused when
		 * dev == 0 and cn_dev has not been set, but was probably
		 * initialised to 0.
		 */
		panic("cnopen: cn_tab->cn_dev == dev");
	}
	cdev = cdevsw_lookup(cndev);
	if (cdev == NULL)
		return (ENXIO);

	if (cn_devvp[unit] == NULLVP) {
		/* try to get a reference on its vnode, but fail silently */
		cdevvp(cndev, &cn_devvp[unit]);
	}
	return ((*cdev->d_open)(cndev, flag, mode, l));
}

int
cnclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	const struct cdevsw *cdev;
	struct vnode *vp;
	int unit;

	unit = minor(dev);

	if (cn_tab == NULL)
		return (0);

	/*
	 * If the real console isn't otherwise open, close it.
	 * If it's otherwise open, don't close it, because that'll
	 * screw up others who have it open.
	 */
	dev = cn_tab->cn_dev;
	cdev = cdevsw_lookup(dev);
	if (cdev == NULL)
		return (ENXIO);
	if (cn_devvp[unit] != NULLVP) {
		/* release our reference to real dev's vnode */
		vrele(cn_devvp[unit]);
		cn_devvp[unit] = NULLVP;
	}
	if (vfinddev(dev, VCHR, &vp) && vcount(vp))
		return (0);
	return ((*cdev->d_close)(dev, flag, mode, l));
}

int
cnread(dev_t dev, struct uio *uio, int flag)
{
	const struct cdevsw *cdev;
	int error;

	/*
	 * If we would redirect input, punt.  This will keep strange
	 * things from happening to people who are using the real
	 * console.  Nothing should be using /dev/console for
	 * input (except a shell in single-user mode, but then,
	 * one wouldn't TIOCCONS then).
	 */
	cdev = cn_redirect(&dev, 1, &error);
	if (cdev == NULL)
		return error;
	return ((*cdev->d_read)(dev, uio, flag));
}

int
cnwrite(dev_t dev, struct uio *uio, int flag)
{
	const struct cdevsw *cdev;
	int error;

	/* Redirect output, if that's appropriate. */
	cdev = cn_redirect(&dev, 0, &error);
	if (cdev == NULL)
		return error;

	return ((*cdev->d_write)(dev, uio, flag));
}

int
cnioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	const struct cdevsw *cdev;
	int error;

	/*
	 * Superuser can always use this to wrest control of console
	 * output from the "virtual" console.
	 */
	if (cmd == TIOCCONS && constty != NULL) {
		error = kauth_authorize_generic(l->l_proc->p_cred, KAUTH_GENERIC_ISSUSER, (u_short *) NULL);
		if (error)
			return (error);
		constty = NULL;
		return (0);
	}

	/*
	 * Redirect the ioctl, if that's appropriate.
	 * Note that strange things can happen, if a program does
	 * ioctls on /dev/console, then the console is redirected
	 * out from under it.
	 */
	cdev = cn_redirect(&dev, 0, &error);
	if (cdev == NULL)
		return error;
	return ((*cdev->d_ioctl)(dev, cmd, data, flag, l));
}

/*ARGSUSED*/
int
cnpoll(dev_t dev, int events, struct lwp *l)
{
	const struct cdevsw *cdev;
	int error;

	/*
	 * Redirect the poll, if that's appropriate.
	 * I don't want to think of the possible side effects
	 * of console redirection here.
	 */
	cdev = cn_redirect(&dev, 0, &error);
	if (cdev == NULL)
		return POLLHUP;
	return ((*cdev->d_poll)(dev, events, l));
}

/*ARGSUSED*/
int
cnkqfilter(dev_t dev, struct knote *kn)
{
	const struct cdevsw *cdev;
	int error;

	/*
	 * Redirect the kqfilter, if that's appropriate.
	 * I don't want to think of the possible side effects
	 * of console redirection here.
	 */
	cdev = cn_redirect(&dev, 0, &error);
	if (cdev == NULL)
		return error;
	return ((*cdev->d_kqfilter)(dev, kn));
}

int
cngetc(void)
{
	if (cn_tab == NULL)
		return (0);
	return ((*cn_tab->cn_getc)(cn_tab->cn_dev));
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

	if (c) {
		(*cn_tab->cn_putc)(cn_tab->cn_dev, c);
		if (c == '\n')
			(*cn_tab->cn_putc)(cn_tab->cn_dev, '\r');
	}
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

static const struct cdevsw *
cn_redirect(dev_t *devp, int is_read, int *error)
{
	dev_t dev = *devp;

	/*
	 * Redirect output, if that's appropriate.
	 * If there's no real console, return ENXIO.
	 */
	*error = ENXIO;
	if (constty != NULL && minor(dev) == 0 &&
	    (cn_tab == NULL || (cn_tab->cn_pri != CN_REMOTE))) {
		if (is_read) {
			*error = 0;
			return NULL;
		}
		dev = constty->t_dev;
	} else if (cn_tab == NULL)
		return NULL;
	else
		dev = cn_tab->cn_dev;
	*devp = dev;
	return cdevsw_lookup(dev);
}
