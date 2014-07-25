/*
 * Copyright (c) 1997, 2000 Hellmuth Michaelis. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
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
 *
 *---------------------------------------------------------------------------
 *
 *	i4b_ctl.c - i4b system control port driver
 *	------------------------------------------
 *
 *	$Id: i4b_ctl.c,v 1.24 2014/07/25 08:10:40 dholland Exp $
 *
 * $FreeBSD$
 *
 *	last edit-date: [Fri Jan  5 11:33:46 2001]
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i4b_ctl.c,v 1.24 2014/07/25 08:10:40 dholland Exp $");

#include "isdnctl.h"

#if NISDNCTL > 0

#include <sys/param.h>

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <sys/ioccom.h>
#include <i386/isa/isa_device.h>
#else
#include <sys/ioctl.h>
#endif

#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <net/if.h>

#ifdef __FreeBSD__

#if defined(__FreeBSD__) && __FreeBSD__ == 3
#include "opt_devfs.h"
#endif

#ifdef DEVFS
#include <sys/devfsext.h>
#endif

#endif /* __FreeBSD__ */

#ifdef __FreeBSD__
#include <machine/i4b_debug.h>
#include <machine/i4b_ioctl.h>
#elif defined(__bsdi__)
#include <i4b/i4b_debug.h>
#include <i4b/i4b_ioctl.h>
#else
#include <sys/bus.h>
#include <sys/device.h>
#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_ioctl.h>
#endif

#include <netisdn/i4b_global.h>
#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_l3l4.h>

#include <netisdn/i4b_l2.h>
#include <netisdn/i4b_l1l2.h>

static int openflag = 0;

#if BSD > 199306 && defined(__FreeBSD__)
static	d_open_t	i4bctlopen;
static	d_close_t	i4bctlclose;
static	d_ioctl_t	i4bctlioctl;

#ifdef OS_USES_POLL
static d_poll_t		i4bctlpoll;
#define POLLFIELD	i4bctlpoll
#else
#define POLLFIELD	noselect
#endif

#define CDEV_MAJOR 55

#if defined(__FreeBSD__) && __FreeBSD__ >= 4
static struct cdevsw i4bctl_cdevsw = {
	/* open */      i4bctlopen,
	/* close */     i4bctlclose,
	/* read */      noread,
	/* write */     nowrite,
	/* ioctl */     i4bctlioctl,
	/* poll */      POLLFIELD,
	/* mmap */      nommap,
	/* strategy */  nostrategy,
	/* name */      "i4bctl",
	/* maj */       CDEV_MAJOR,
	/* dump */      nodump,
	/* psize */     nopsize,
	/* flags */     0,
	/* bmaj */      -1
};
#else
static struct cdevsw i4bctl_cdevsw =
	{ i4bctlopen,	i4bctlclose,	noread,		nowrite,
	  i4bctlioctl,	nostop,		nullreset,	nodevtotty,
	  POLLFIELD,	nommap,		NULL,	"i4bctl", NULL,	-1 };
#endif

static void i4bctlattach(void *);
PSEUDO_SET(i4bctlattach, i4b_i4bctldrv);

#define PDEVSTATIC	static
#endif /* __FreeBSD__ */

#if defined(__FreeBSD__) && __FreeBSD__ == 3
#ifdef DEVFS
static void *devfs_token;
#endif
#endif

#ifndef __FreeBSD__
#define PDEVSTATIC	/* */
void isdnctlattach(void);
int isdnctlopen(dev_t dev, int flag, int fmt, struct lwp *l);
int isdnctlclose(dev_t dev, int flag, int fmt, struct lwp *l);
int isdnctlioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l);
#endif	/* !FreeBSD */

#ifdef __NetBSD__
const struct cdevsw isdnctl_cdevsw = {
	.d_open = isdnctlopen,
	.d_close = isdnctlclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = isdnctlioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};
#endif /* __NetBSD__ */

#if BSD > 199306 && defined(__FreeBSD__)
/*---------------------------------------------------------------------------*
 *	initialization at kernel load time
 *---------------------------------------------------------------------------*/
static void
i4bctlinit(void *unused)
{
#if defined(__FreeBSD__) && __FreeBSD__ >= 4
	cdevsw_add(&i4bctl_cdevsw);
#else
	dev_t dev = makedev(CDEV_MAJOR, 0);
	cdevsw_add(&dev, &i4bctl_cdevsw, NULL);
#endif
}

SYSINIT(i4bctldev, SI_SUB_DRIVERS,SI_ORDER_MIDDLE+CDEV_MAJOR, &i4bctlinit, NULL);

#endif /* BSD > 199306 && defined(__FreeBSD__) */

#ifdef __bsdi__
int i4bctlmatch(device_t parent, cfdata_t cf, void *aux);
void dummy_i4bctlattach(struct device*, struct device *, void *);

#define CDEV_MAJOR 64

static struct cfdriver i4bctlcd =
	{ NULL, "i4bctl", i4bctlmatch, dummy_i4bctlattach, DV_DULL,
	  sizeof(struct cfdriver) };
struct devsw i4bctlsw =
	{ &i4bctlcd,
	  i4bctlopen,	i4bctlclose,	noread,		nowrite,
	  i4bctlioctl,	seltrue,	nommap,		nostrat,
	  nodump,	nopsize,	0,		nostop
};

int
i4bctlmatch(device_t parent, cfdata_t cf, void *aux)
{
	printf("i4bctlmatch: aux=0x%x\n", aux);
	return 1;
}
void
dummy_i4bctlattach(device_t parent, device_t self, void *aux)
{
	printf("dummy_i4bctlattach: aux=0x%x\n", aux);
}
#endif /* __bsdi__ */
/*---------------------------------------------------------------------------*
 *	interface attach routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC void
#ifdef __FreeBSD__
isdnctlattach(void *dummy)
#else
isdnctlattach(void)
#endif
{

#if defined(__FreeBSD__)
#if __FreeBSD__ == 3

#ifdef DEVFS
	devfs_token = devfs_add_devswf(&i4bctl_cdevsw, 0, DV_CHR,
				       UID_ROOT, GID_WHEEL, 0600,
				       "i4bctl");
#endif

#else
	make_dev(&i4bctl_cdevsw, 0, UID_ROOT, GID_WHEEL, 0600, "i4bctl");
#endif
#endif
}

/*---------------------------------------------------------------------------*
 *	i4bctlopen - device driver open routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnctlopen(dev_t dev, int flag, int fmt,
	struct lwp *l)
{
	if(minor(dev))
		return (ENXIO);

	if(openflag)
		return (EBUSY);

	openflag = 1;

	return (0);
}

/*---------------------------------------------------------------------------*
 *	i4bctlclose - device driver close routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnctlclose(dev_t dev, int flag, int fmt,
	struct lwp *l)
{
	openflag = 0;
	return (0);
}

/*---------------------------------------------------------------------------*
 *	i4bctlioctl - device driver ioctl routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnctlioctl(dev_t dev, u_long cmd, void *data, int flag,
	struct lwp *l)
{
#if DO_I4B_DEBUG
	ctl_debug_t *cdbg;
	int error = 0;
#endif

#if !DO_I4B_DEBUG
	return(ENODEV);
#else
	if(minor(dev))
		return(ENODEV);

	switch(cmd)
	{
		case I4B_CTL_GET_DEBUG:
			cdbg = (ctl_debug_t *)data;
			cdbg->l1 = i4b_l1_debug;
			cdbg->l2 = i4b_l2_debug;
			cdbg->l3 = i4b_l3_debug;
			cdbg->l4 = i4b_l4_debug;
			break;

		case I4B_CTL_SET_DEBUG:
			cdbg = (ctl_debug_t *)data;
			i4b_l1_debug = cdbg->l1;
			i4b_l2_debug = cdbg->l2;
			i4b_l3_debug = cdbg->l3;
			i4b_l4_debug = cdbg->l4;
			break;

                case I4B_CTL_GET_CHIPSTAT:
                {
                        struct chipstat *cst;
			l2_softc_t * scl2;
			cst = (struct chipstat *)data;
			scl2 = (l2_softc_t*)isdn_find_softc_by_isdnif(cst->driver_unit);
			scl2->driver->mph_command_req(scl2->l1_token, CMR_GCST, cst);
                        break;
                }

                case I4B_CTL_CLR_CHIPSTAT:
                {
                        struct chipstat *cst;
			l2_softc_t * scl2;
			cst = (struct chipstat *)data;
			scl2 = (l2_softc_t*)isdn_find_softc_by_isdnif(cst->driver_unit);
			scl2->driver->mph_command_req(scl2->l1_token, CMR_CCST, cst);
                        break;
                }

                case I4B_CTL_GET_LAPDSTAT:
                {
                        l2stat_t *l2s;
                        l2_softc_t *sc;
                        l2s = (l2stat_t *)data;

                        sc = (l2_softc_t*)isdn_find_softc_by_isdnif(l2s->unit);
                        if (sc == NULL) {
                        	error = EINVAL;
				break;
			}

			memcpy(&l2s->lapdstat, &sc->stat, sizeof(lapdstat_t));
                        break;
                }

                case I4B_CTL_CLR_LAPDSTAT:
                {
                        int *up;
                        l2_softc_t *sc;
                        up = (int *)data;

                        sc = (l2_softc_t*)isdn_find_softc_by_isdnif(*up);
                        if (sc == NULL) {
                        	error = EINVAL;
				break;
			}

			memset(&sc->stat, 0, sizeof(lapdstat_t));
                        break;
                }

		default:
			error = ENOTTY;
			break;
	}
	return(error);
#endif /* DO_I4B_DEBUG */
}

#if defined(__FreeBSD__) && defined(OS_USES_POLL)

/*---------------------------------------------------------------------------*
 *	i4bctlpoll - device driver poll routine
 *---------------------------------------------------------------------------*/
static int
isdnctlpoll (dev_t dev, int events, struct lwp *l)
{
	return (ENODEV);
}

#endif

#endif /* NISDNCTL > 0 */
