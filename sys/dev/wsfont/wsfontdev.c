/* $NetBSD: wsfontdev.c,v 1.1 2001/09/03 17:05:20 drochner Exp $ */

/*
 * Copyright (c) 2001
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>

#include <dev/wsfont/wsfont.h>
#include <dev/wscons/wsconsio.h> /* XXX */

void wsfontattach(int);
cdev_decl(wsfont);

static int wsfont_isopen;

void
wsfontattach(n)
	int n;
{

	wsfont_init();
}

int
wsfontopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{

	if (wsfont_isopen)
		return (EBUSY);
	wsfont_isopen = 1;
	return (0);
}

int
wsfontclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{

	wsfont_isopen = 0;
	return (0);
}

int
wsfontioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	char nbuf[16];
	void *buf;
	int res;

	switch (cmd) {
	case WSDISPLAYIO_LDFONT:
#define d ((struct wsdisplay_font *)data)
		if (d->name) {
			res = copyinstr(d->name, nbuf, sizeof(nbuf), 0);
			if (res)
				return (res);
			d->name = nbuf;
		} else
			d->name = "loaded"; /* ??? */
		buf = malloc(d->fontheight * d->stride * d->numchars,
			     M_DEVBUF, M_WAITOK);
		res = copyin(d->data, buf,
			     d->fontheight * d->stride * d->numchars);
		if (res) {
			free(buf, M_DEVBUF);
			return (res);
		}
		d->data = buf;
		res = wsfont_add(d, 1);
		free(buf, M_DEVBUF);
#undef d
		return (res);
	default:
		return (EINVAL);
	}

	return (0);
}
