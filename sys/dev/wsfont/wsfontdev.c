/* $NetBSD: wsfontdev.c,v 1.20 2022/05/12 23:17:42 uwe Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wsfontdev.c,v 1.20 2022/05/12 23:17:42 uwe Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/event.h>

#include <dev/wsfont/wsfont.h>
#include <dev/wscons/wsconsio.h> /* XXX */

#include "ioconf.h"

#ifdef WSFONT_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif
static int wsfont_isopen;


void
wsfontattach(int n)
{

	wsfont_init();
}

static int
wsfontopen(dev_t dev, int flag, int mode,
    struct lwp *l)
{

	if (wsfont_isopen)
		return (EBUSY);
	wsfont_isopen = 1;
	return (0);
}

static int
wsfontclose(dev_t dev, int flag, int mode,
    struct lwp *l)
{

	wsfont_isopen = 0;
	return (0);
}

static void
fontmatchfunc(struct wsdisplay_font *f, void *cookie, int fontcookie)
{
	struct wsdisplayio_fontinfo *fi = cookie;
	struct wsdisplayio_fontdesc fd;
	int offset;

	DPRINTF("%s %dx%d\n", f->name, f->fontwidth, f->fontheight);
	if (fi->fi_fonts != NULL && fi->fi_buffersize > 0) {
		memset(&fd, 0, sizeof(fd));
		strncpy(fd.fd_name, f->name, sizeof(fd.fd_name) - 1);
		fd.fd_width = f->fontwidth;
		fd.fd_height = f->fontheight;
		offset = sizeof(struct wsdisplayio_fontdesc) * (fi->fi_numentries + 1);
		if (offset > fi->fi_buffersize) {
			fi->fi_fonts = NULL;
		} else
			copyout(&fd, &fi->fi_fonts[fi->fi_numentries],
			    sizeof(struct wsdisplayio_fontdesc));
	}
	fi->fi_numentries++;
}

static int
wsdisplayio_listfonts(struct wsdisplayio_fontinfo *f)
{
	void *addr = f->fi_fonts;
	DPRINTF("%s: %d %d\n", __func__, f->fi_buffersize, f->fi_numentries);
	f->fi_numentries = 0;
	wsfont_walk(fontmatchfunc, f);
	/* check if we ran out of buffer space */
	if (f->fi_fonts == NULL && addr != NULL) return ENOMEM;
	return 0;
}

static int
wsfontioctl(dev_t dev, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	char nbuf[64];
	void *buf;
	int res;

	switch (cmd) {
	case WSDISPLAYIO_LDFONT:
#define d ((struct wsdisplay_font *)data)
		if ((flag & FWRITE) == 0)
			return EPERM;

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
	case WSDISPLAYIO_LISTFONTS:
		return wsdisplayio_listfonts(data);
	default:
		return (EINVAL);
	}
}

const struct cdevsw wsfont_cdevsw = {
	.d_open = wsfontopen,
	.d_close = wsfontclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = wsfontioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};
