/*	$NetBSD: ite_compat.c,v 1.1.2.2 2000/02/07 08:20:48 scottr Exp $	*/

/*
 * Copyright (C) 2000 Scott Reynolds
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

#include "ite.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>

#include <machine/cpu.h>
#include <machine/iteioctl.h>
#include <mac68k/dev/itevar.h>

cdev_decl(ite);
cdev_decl(wsdisplay);
void		iteattach __P((int));

dev_t		wscdev;
static int	bell_freq = 1880;
static int	bell_length = 10;
static int	bell_volume = 100;


/*ARGSUSED*/
void
iteattach(n)
	int n;
{
	/* Do nothing; there's only one console. */
}

/*ARGSUSED*/
void
ite_attach(parent, dev)
	struct device *parent;
	dev_t dev;
{
	wscdev = dev;
}

/*
 * Tty handling functions
 */

int
iteopen(dev, mode, devtype, p)
	dev_t dev;
	int mode;
	int devtype;
	struct proc *p;
{
	return wsdisplayopen(wscdev, mode, devtype, p);
}

int
iteclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	return wsdisplayclose(wscdev, flag, mode, p);
}

int
iteread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	return wsdisplayread(wscdev, uio, flag);
}

int
itewrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	return wsdisplaywrite(wscdev, uio, flag);
}

struct tty *
itetty(dev)
	dev_t dev;
{
	return wsdisplaytty(wscdev);
}

void 
itestop(struct tty *tp, int flag)
{
}

int
iteioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	switch (cmd) {
	case ITEIOC_RINGBELL:
		return mac68k_ring_bell(bell_freq, bell_length, bell_volume);
	case ITEIOC_SETBELL:
		{
			struct bellparams *bp = (void *)addr;

			/* Do some sanity checks. */
			if (bp->freq < 10 || bp->freq > 40000)
				return (EINVAL);
			if (bp->len < 0 || bp->len > 3600)
				return (EINVAL);
			if (bp->vol < 0 || bp->vol > 100)
				return (EINVAL);

			bell_freq = bp->freq;
			bell_length = bp->len;
			bell_volume = bp->vol;
			return (0);
		}
	case ITEIOC_GETBELL:
		{
			struct bellparams *bp = (void *)addr;

			bell_freq = bp->freq;
			bell_length = bp->len;
			bell_volume = bp->vol;
			return (0);
		}
	default:
		return wsdisplayioctl(wscdev, cmd, addr, flag, p);
	}

	return (ENOTTY);
}
