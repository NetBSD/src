/*	$NetBSD: tty_bsdpty.c,v 1.1 2004/11/10 17:29:54 christos Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: tty_bsdpty.c,v 1.1 2004/11/10 17:29:54 christos Exp $");

#include "opt_ptm.h"

#ifdef COMPAT_BSDPTY
/* bsd tty implementation for pty multiplexor driver /dev/ptm{,x} */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/signalvar.h>
#include <sys/uio.h>
#include <sys/filedesc.h>
#include <sys/conf.h>
#include <sys/poll.h>
#include <sys/malloc.h>
#include <sys/pty.h>

/*
 * pts == /dev/tty[pqrs]?
 * ptc == /dev/pty[pqrs]?
 */

#define TTY_TEMPLATE	"/dev/XtyXX"
#define TTY_NAMESIZE	sizeof(TTY_TEMPLATE)
/* XXX this needs to come from somewhere sane, and work with MAKEDEV */
#define TTY_LETTERS	"pqrstuvwxyzPQRST"
#define TTY_OLD_SUFFIX  "0123456789abcdef"
#define TTY_NEW_SUFFIX  "ghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

static int pty_makename(struct ptm_pty *, char *, size_t, dev_t, char);
static int pty_allocvp(struct ptm_pty *, struct proc *, struct vnode **,
    dev_t, char);

struct ptm_pty ptm_bsdpty = {
	pty_allocvp,
	pty_makename,
	NULL
};

static int
/*ARGSUSED*/
pty_makename(struct ptm_pty *ptm, char *buf, size_t bufsiz, dev_t dev, char c)
{
	size_t nt;
	dev_t minor = minor(dev);
	if (bufsiz < TTY_NAMESIZE)
		return EINVAL;
	(void)memcpy(buf, TTY_TEMPLATE, TTY_NAMESIZE);

	buf[5] = c;

	if (minor < 256) {
		nt = sizeof(TTY_OLD_SUFFIX) - 1;
		buf[8] = TTY_LETTERS[minor / nt];
		buf[9] = TTY_OLD_SUFFIX[minor % nt];
	} else {
		minor -= 256;
		nt = sizeof(TTY_NEW_SUFFIX) - sizeof(TTY_OLD_SUFFIX);
		buf[8] = TTY_LETTERS[minor / nt];
		buf[9] = TTY_NEW_SUFFIX[minor % nt];
	}
	return 0;
}


static int
/*ARGSUSED*/
pty_allocvp(struct ptm_pty *ptm, struct proc *p, struct vnode **vp, dev_t dev,
    char ms)
{
	int error;
	struct nameidata nd;
	char name[TTY_NAMESIZE];

	error = (*ptm->makename)(ptm, name, sizeof(name), dev, ms);
	if (error)
		return error;

	NDINIT(&nd, LOOKUP, NOFOLLOW|LOCKLEAF, UIO_SYSSPACE, name, p);
	if ((error = namei(&nd)) != 0)
		return error;
	*vp = nd.ni_vp;
	return 0;
}
#endif
