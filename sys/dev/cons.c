/*	$NetBSD: cons.c,v 1.40.4.4 2001/09/26 15:28:09 fvdl Exp $	*/

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

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/vnode.h>

#include <miscfs/specfs/specdev.h>

#include <dev/cons.h>

struct	tty *constty = NULL;	/* virtual console output device */
struct	consdev *cn_tab;	/* physical console device info */

int
cnopen(devvp, mode, flag, p)
	struct vnode *devvp;
	int flag, mode;
	struct proc *p;
{
	int error;
	dev_t cndev, rdev;
	struct vnode *vp;

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
		 * code. Panicing looks better than jumping into nowhere
		 * through cdevsw below....
		 */
		panic("cnopen: cn_tab->cn_dev == NODEV\n");
	}
	rdev = vdev_rdev(devvp);
	if (rdev == cndev) {
		/*
		 * This causes cnopen() to be called recursively, which
		 * is generally a bad thing.  It is often caused when
		 * dev == 0 and cn_dev has not been set, but was probably
		 * initialised to 0.
		 */
		panic("cnopen: cn_tab->cn_dev == dev\n");
	}

	if (vfinddev(cndev, VCHR, &vp) == 0) {
		error = cdevvp(cndev, &vp);
		if (error != 0)
			return error;
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	} else
		vget(vp, LK_EXCLUSIVE | LK_RETRY);

	vdev_setprivdata(devvp, vp);

	error = VOP_OPEN(vp, mode, p->p_ucred, p, NULL);
	VOP_UNLOCK(vp, 0);
	if (error != 0)
		vrele(vp);
	return error;
}
 
int
cnclose(devvp, flag, mode, p)
	struct vnode *devvp;
	int flag, mode;
	struct proc *p;
{
	struct vnode *vp;
	int error;

	if (cn_tab == NULL)
		return (0);

	vp = vdev_privdata(devvp);

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_CLOSE(vp, flag, p->p_ucred, p);
	vput(vp);

	return error;
}
 
int
cnread(devvp, uio, flag)
	struct vnode *devvp;
	struct uio *uio;
	int flag;
{
	int error;
	struct vnode *vp;
	struct proc *p = uio->uio_procp;

	/*
	 * If we would redirect input, punt.  This will keep strange
	 * things from happening to people who are using the real
	 * console.  Nothing should be using /dev/console for
	 * input (except a shell in single-user mode, but then,
	 * one wouldn't TIOCCONS then).
	 */
	if (constty != NULL && (cn_tab == NULL || cn_tab->cn_pri != CN_REMOTE))
		return 0;
	else if (cn_tab == NULL)
		return ENXIO;

	vp = vdev_privdata(devvp);

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_READ(vp, uio, flag, p->p_ucred);
	VOP_UNLOCK(vp, 0);

	return error;
}
 
int
cnwrite(devvp, uio, flag)
	struct vnode *devvp;
	struct uio *uio;
	int flag;
{
	int error;
	struct vnode *vp;
	struct proc *p = uio->uio_procp;

	/*
	 * Redirect output, if that's appropriate.
	 * If there's no real console, return ENXIO.
	 */
	if (constty != NULL && (cn_tab == NULL || cn_tab->cn_pri != CN_REMOTE))
		vp = constty->t_devvp;
	else if (cn_tab == NULL)
		return ENXIO;
	else
		vp = vdev_privdata(devvp);

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_WRITE(vp, uio, flag, p->p_ucred);
	VOP_UNLOCK(vp, 0);
	return error;
}

void
cnstop(tp, flag)
	struct tty *tp;
	int flag;
{

}
 
int
cnioctl(devvp, cmd, data, flag, p)
	struct vnode *devvp;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int error;
	struct vnode *vp;

	/*
	 * Superuser can always use this to wrest control of console
	 * output from the "virtual" console.
	 */
	if (cmd == TIOCCONS && constty != NULL) {
		error = suser(p->p_ucred, (u_short *) NULL);
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
	if (constty != NULL && (cn_tab == NULL || cn_tab->cn_pri != CN_REMOTE))
		vp = constty->t_devvp;
	else if (cn_tab == NULL)
		return ENXIO;
	else
		vp = vdev_privdata(devvp);

	return VOP_IOCTL(vp, cmd, data, flag, p->p_ucred, p);
}

/*ARGSUSED*/
int
cnpoll(devvp, events, p)
	struct vnode *devvp;
	int events;
	struct proc *p;
{
	struct vnode *vp;

	/*
	 * Redirect the poll, if that's appropriate.
	 * I don't want to think of the possible side effects
	 * of console redirection here.
	 */
	if (constty != NULL && (cn_tab == NULL || cn_tab->cn_pri != CN_REMOTE))
		vp = constty->t_devvp;
	else if (cn_tab == NULL)
		return ENXIO;
	else
		vp = vdev_privdata(devvp);

	return VOP_POLL(vp, events, p);
}

int
cngetc()
{

	if (cn_tab == NULL)
		return (0);
	return ((*cn_tab->cn_getc)(cn_tab->cn_dev));
}

int
cngetsn(cp, size)
	char *cp;
	int size;
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
cnputc(c)
	int c;
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
cnpollc(on)
	int on;
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
nullcnpollc(dev, on)
	dev_t dev;
	int on;
{

}

void
cnbell(pitch, period, volume)
	u_int pitch, period, volume;
{

	if (cn_tab == NULL || cn_tab->cn_bell == NULL)
		return;
	(*cn_tab->cn_bell)(cn_tab->cn_dev, pitch, period, volume);
}
