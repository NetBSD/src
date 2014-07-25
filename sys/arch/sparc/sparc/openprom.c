/*	$NetBSD: openprom.c,v 1.28 2014/07/25 08:10:35 dholland Exp $ */

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
 *	@(#)openprom.c	8.1 (Berkeley) 6/11/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: openprom.c,v 1.28 2014/07/25 08:10:35 dholland Exp $");

#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/event.h>

#include <machine/bsd_openprom.h>
#include <machine/promlib.h>
#include <machine/openpromio.h>

dev_type_open(openpromopen);
dev_type_ioctl(openpromioctl);

const struct cdevsw openprom_cdevsw = {
	.d_open = openpromopen,
	.d_close = nullclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = openpromioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static	int lastnode;			/* speed hack */

static int openpromcheckid(int, int);
static int openpromgetstr(int, char *, char **);

int
openpromopen(dev_t dev, int flags, int mode, struct lwp *l)
{

#if defined(SUN4)
	if (cputyp==CPU_SUN4)
		return (ENODEV);
#endif

	return (0);
}

/*
 * Verify target ID is valid (exists in the OPENPROM tree), as
 * listed from node ID sid forward.
 */
static int
openpromcheckid(int sid, int tid)
{

	for (; sid != 0; sid = nextsibling(sid))
		if (sid == tid || openpromcheckid(firstchild(sid), tid))
			return (1);

	return (0);
}

static int
openpromgetstr(int len, char *user, char **cpp)
{
	int error;
	char *cp;

	/* Reject obvious bogus requests */
	if ((u_int)len > (8 * 1024) - 1)
		return (ENAMETOOLONG);

	*cpp = cp = malloc(len + 1, M_TEMP, M_WAITOK);
	error = copyin(user, cp, len);
	cp[len] = '\0';
	return (error);
}

int
openpromioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct opiocdesc *op;
	int node, optionsnode, len, ok, error, s;
	char *name, *value, *nextprop;

	optionsnode = prom_getoptionsnode();

	/* All too easy... */
	if (cmd == OPIOCGETOPTNODE) {
		*(int *)data = optionsnode;
		return (0);
	}

	/* Verify node id */
	op = (struct opiocdesc *)data;
	node = op->op_nodeid;
	if (node != 0 && node != lastnode && node != optionsnode) {
		/* Not an easy one, must search for it */
		s = splhigh();
		ok = openpromcheckid(findroot(), node);
		splx(s);
		if (!ok)
			return (EINVAL);
		lastnode = node;
	}

	name = value = NULL;
	error = 0;
	switch (cmd) {

	case OPIOCGET:
		if ((flags & FREAD) == 0)
			return (EBADF);
		if (node == 0)
			return (EINVAL);
		error = openpromgetstr(op->op_namelen, op->op_name, &name);
		if (error)
			break;
		s = splhigh();
		len = prom_proplen(node, name);
		splx(s);
		if (len > op->op_buflen) {
			error = ENOMEM;
			break;
		}
		op->op_buflen = len;
		/* -1 means no entry; 0 means no value */
		if (len <= 0)
			break;
		value = malloc(len, M_TEMP, M_WAITOK);
		s = splhigh();
		error = prom_getprop(node, name, 1, &len, &value);
		splx(s);
		if (error != 0)
			break;
		error = copyout(value, op->op_buf, len);
		break;

	case OPIOCSET:
		if ((flags & FWRITE) == 0)
			return (EBADF);
		if (node == 0)
			return (EINVAL);
		error = openpromgetstr(op->op_namelen, op->op_name, &name);
		if (error)
			break;
		error = openpromgetstr(op->op_buflen, op->op_buf, &value);
		if (error)
			break;
		s = splhigh();
		len = prom_setprop(node, name, value, op->op_buflen + 1);
		splx(s);
		if (len != op->op_buflen)
			error = EINVAL;
		break;

	case OPIOCNEXTPROP:
		if ((flags & FREAD) == 0)
			return (EBADF);
		if (node == 0)
			return (EINVAL);
		error = openpromgetstr(op->op_namelen, op->op_name, &name);
		if (error)
			break;
		s = splhigh();
		nextprop = prom_nextprop(node, name);
		splx(s);
		len = strlen(nextprop);
		if (len > op->op_buflen)
			len = op->op_buflen;
		else
			op->op_buflen = len;
		error = copyout(nextprop, op->op_buf, len);
		break;

	case OPIOCGETNEXT:
		if ((flags & FREAD) == 0)
			return (EBADF);
		s = splhigh();
		node = nextsibling(node);
		splx(s);
		*(int *)data = lastnode = node;
		break;

	case OPIOCGETCHILD:
		if ((flags & FREAD) == 0)
			return (EBADF);
		if (node == 0)
			return (EINVAL);
		s = splhigh();
		node = firstchild(node);
		splx(s);
		*(int *)data = lastnode = node;
		break;

	case OPIOCFINDDEVICE:
		if ((flags & FREAD) == 0)
			return (EBADF);
		error = openpromgetstr(op->op_namelen, op->op_name, &name);
		if (error)
			break;
		node = prom_finddevice(name);
		if (node == 0 || node == -1) {
			error = ENOENT;
			break;
		}
		op->op_nodeid = lastnode = node;
		break;

	default:
		return (ENOTTY);
	}

	if (name)
		free(name, M_TEMP);
	if (value)
		free(value, M_TEMP);

	return (error);
}
