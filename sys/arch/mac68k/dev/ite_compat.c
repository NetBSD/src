/*	$NetBSD: ite_compat.c,v 1.1.2.1 2000/02/07 07:54:13 scottr Exp $	*/

/*
 * Copyright
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
