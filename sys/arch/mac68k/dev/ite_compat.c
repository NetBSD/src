/*	$NetBSD: ite_compat.c,v 1.4 2001/05/14 09:27:06 scw Exp $	*/

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

/*
 * The main thing to realize about this emulator is that the old console
 * emulator was largely compatible with the DEC VT-220.  Since the
 * wsdiplay driver has a more complete emulation of that terminal, it's
 * reasonable to pass virtually everything up to that driver without
 * modification.
 */

#include "ite.h"
#include "wsdisplay.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/ttycom.h>

#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/iteioctl.h>

cdev_decl(ite);
cdev_decl(wsdisplay);
void		iteattach __P((int));

static int	ite_initted = 0;
static int	ite_bell_freq = 1880;
static int	ite_bell_length = 10;
static int	ite_bell_volume = 100;


/*ARGSUSED*/
void
iteattach(n)
	int n;
{
#if NWSDISPLAY > 0
	int maj;

	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == wsdisplayopen)
			break;
	KASSERT(maj < nchrdev);

	if (maj != major(cn_tab->cn_dev))
		return;

	ite_initted = 1;
#endif
}

/*
 * Tty handling functions
 */

/*ARGSUSED*/
int
iteopen(dev, mode, devtype, p)
	dev_t dev;
	int mode;
	int devtype;
	struct proc *p;
{
	return ite_initted ? (0) : (ENXIO);
}

/*ARGSUSED*/
int
iteclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	return ite_initted ? (0) : (ENXIO);
}

/*ARGSUSED*/
int
iteread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	return ite_initted ?
	    wsdisplayread(cn_tab->cn_dev, uio, flag) : (ENXIO);
}

/*ARGSUSED*/
int
itewrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	return ite_initted ?
	    wsdisplaywrite(cn_tab->cn_dev, uio, flag) : (ENXIO);
}

/*ARGSUSED*/
struct tty *
itetty(dev)
	dev_t dev;
{
	return ite_initted ? wsdisplaytty(cn_tab->cn_dev) : (NULL);
}

/*ARGSUSED*/
void 
itestop(struct tty *tp, int flag)
{
}

/*ARGSUSED*/
int
iteioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	if (!ite_initted)
		return (ENXIO);

	switch (cmd) {
	case ITEIOC_RINGBELL:
		return mac68k_ring_bell(ite_bell_freq,
		    ite_bell_length, ite_bell_volume);
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

			ite_bell_freq = bp->freq;
			ite_bell_length = bp->len;
			ite_bell_volume = bp->vol;
			return (0);
		}
	case ITEIOC_GETBELL:
		{
			struct bellparams *bp = (void *)addr;

			ite_bell_freq = bp->freq;
			ite_bell_length = bp->len;
			ite_bell_volume = bp->vol;
			return (0);
		}
	default:
		return wsdisplayioctl(cn_tab->cn_dev, cmd, addr, flag, p);
	}

	return (ENOTTY);
}

/*ARGSUSED*/
int
itepoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	return ite_initted ?
	    wsdisplaypoll(cn_tab->cn_dev, events, p) : (ENXIO);
}
