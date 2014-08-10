/*	$NetBSD: altq_conf.c,v 1.20.2.1 2014/08/10 06:53:38 tls Exp $	*/
/*	$KAME: altq_conf.c,v 1.24 2005/04/13 03:44:24 suz Exp $	*/

/*
 * Copyright (C) 1997-2003
 *	Sony Computer Science Laboratories Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY SONY CSL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL SONY CSL OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: altq_conf.c,v 1.20.2.1 2014/08/10 06:53:38 tls Exp $");

#ifdef _KERNEL_OPT
#include "opt_altq.h"
#include "opt_inet.h"
#endif

/*
 * altq device interface.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/kauth.h>

#include <net/if.h>

#include <altq/altq.h>
#include <altq/altqconf.h>
#include <altq/altq_conf.h>

#ifdef ALTQ3_COMPAT

#ifdef ALTQ_CBQ
altqdev_decl(cbq);
#endif
#ifdef ALTQ_WFQ
altqdev_decl(wfq);
#endif
#ifdef ALTQ_AFMAP
altqdev_decl(afm);
#endif
#ifdef ALTQ_FIFOQ
altqdev_decl(fifoq);
#endif
#ifdef ALTQ_RED
altqdev_decl(red);
#endif
#ifdef ALTQ_RIO
altqdev_decl(rio);
#endif
#ifdef ALTQ_LOCALQ
altqdev_decl(localq);
#endif
#ifdef ALTQ_HFSC
altqdev_decl(hfsc);
#endif
#ifdef ALTQ_CDNR
altqdev_decl(cdnr);
#endif
#ifdef ALTQ_BLUE
altqdev_decl(blue);
#endif
#ifdef ALTQ_PRIQ
altqdev_decl(priq);
#endif
#ifdef ALTQ_JOBS
altqdev_decl(jobs);
#endif

/*
 * altq minor device (discipline) table
 */
static struct altqsw altqsw[] = {				/* minor */
	{"altq", noopen,	noclose,	noioctl},  /* 0 (reserved) */
#ifdef ALTQ_CBQ
	{"cbq",	cbqopen,	cbqclose,	cbqioctl},	/* 1 */
#else
	{"noq",	noopen,		noclose,	noioctl},	/* 1 */
#endif
#ifdef ALTQ_WFQ
	{"wfq",	wfqopen,	wfqclose,	wfqioctl},	/* 2 */
#else
	{"noq",	noopen,		noclose,	noioctl},	/* 2 */
#endif
#ifdef ALTQ_AFMAP
	{"afm",	afmopen,	afmclose,	afmioctl},	/* 3 */
#else
	{"noq",	noopen,		noclose,	noioctl},	/* 3 */
#endif
#ifdef ALTQ_FIFOQ
	{"fifoq", fifoqopen,	fifoqclose,	fifoqioctl},	/* 4 */
#else
	{"noq",	noopen,		noclose,	noioctl},	/* 4 */
#endif
#ifdef ALTQ_RED
	{"red", redopen,	redclose,	redioctl},	/* 5 */
#else
	{"noq",	noopen,		noclose,	noioctl},	/* 5 */
#endif
#ifdef ALTQ_RIO
	{"rio", rioopen,	rioclose,	rioioctl},	/* 6 */
#else
	{"noq",	noopen,		noclose,	noioctl},	/* 6 */
#endif
#ifdef ALTQ_LOCALQ
	{"localq",localqopen,	localqclose,	localqioctl}, /* 7 (local use) */
#else
	{"noq",	noopen,		noclose,	noioctl},  /* 7 (local use) */
#endif
#ifdef ALTQ_HFSC
	{"hfsc",hfscopen,	hfscclose,	hfscioctl},	/* 8 */
#else
	{"noq",	noopen,		noclose,	noioctl},	/* 8 */
#endif
#ifdef ALTQ_CDNR
	{"cdnr",cdnropen,	cdnrclose,	cdnrioctl},	/* 9 */
#else
	{"noq",	noopen,		noclose,	noioctl},	/* 9 */
#endif
#ifdef ALTQ_BLUE
	{"blue",blueopen,	blueclose,	blueioctl},	/* 10 */
#else
	{"noq",	noopen,		noclose,	noioctl},	/* 10 */
#endif
#ifdef ALTQ_PRIQ
	{"priq",priqopen,	priqclose,	priqioctl},	/* 11 */
#else
	{"noq",	noopen,		noclose,	noioctl},	/* 11 */
#endif
#ifdef ALTQ_JOBS
	{"jobs",jobsopen,	jobsclose,	jobsioctl},	/* 12 */
#else
	{"noq", noopen,		noclose,	noioctl},	/* 12 */
#endif
};

/*
 * altq major device support
 */
int	naltqsw = sizeof (altqsw) / sizeof (altqsw[0]);

dev_type_open(altqopen);
dev_type_close(altqclose);
dev_type_ioctl(altqioctl);

const struct cdevsw altq_cdevsw = {
	.d_open = altqopen,
	.d_close = altqclose,
	.d_read = noread,
	.d_write = nowrite, 
	.d_ioctl = altqioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER,
};

int
altqopen(dev_t dev, int flag, int fmt, struct lwp *l)
{
	int unit = minor(dev);

	if (unit == 0)
		return (0);
	if (unit < naltqsw)
		return (*altqsw[unit].d_open)(dev, flag, fmt, l);

	return ENXIO;
}

int
altqclose(dev_t dev, int flag, int fmt, struct lwp *l)
{
	int unit = minor(dev);

	if (unit == 0)
		return (0);
	if (unit < naltqsw)
		return (*altqsw[unit].d_close)(dev, flag, fmt, l);

	return ENXIO;
}

int
altqioctl(dev_t dev, ioctlcmd_t cmd, void *addr, int flag, struct lwp *l)
{
	int unit = minor(dev);

	if (unit == 0) {
		struct ifnet *ifp;
		struct altqreq *typereq;
		struct tbrreq *tbrreq;
		int error;

		switch (cmd) {
		case ALTQGTYPE:
		case ALTQTBRGET:
			break;
		default:
#if (__FreeBSD_version > 400000)
			if ((error = suser(p)) != 0)
				return (error);
#else
			if ((error = kauth_authorize_network(l->l_cred,
			    KAUTH_NETWORK_ALTQ, KAUTH_REQ_NETWORK_ALTQ_CONF,
			    NULL, NULL, NULL)) != 0)
				return (error);
#endif
			break;
		}

		switch (cmd) {
		case ALTQGTYPE:
			typereq = (struct altqreq *)addr;
			if ((ifp = ifunit(typereq->ifname)) == NULL)
				return (EINVAL);
			typereq->arg = (u_long)ifp->if_snd.altq_type;
			return (0);
		case ALTQTBRSET:
			tbrreq = (struct tbrreq *)addr;
			if ((ifp = ifunit(tbrreq->ifname)) == NULL)
				return (EINVAL);
			return tbr_set(&ifp->if_snd, &tbrreq->tb_prof);
		case ALTQTBRGET:
			tbrreq = (struct tbrreq *)addr;
			if ((ifp = ifunit(tbrreq->ifname)) == NULL)
				return (EINVAL);
			return tbr_get(&ifp->if_snd, &tbrreq->tb_prof);
		default:
			return (EINVAL);
		}
	}
	if (unit < naltqsw)
		return (*altqsw[unit].d_ioctl)(dev, cmd, addr, flag, l);

	return ENXIO;
}

#ifdef __FreeBSD__
static int altq_devsw_installed = 0;
#endif

#ifdef __FreeBSD__
static void
altq_drvinit(void *unused)
{
	int unit;

#if 0
	mtx_init(&altq_mtx, "altq global lock", MTX_DEF);
#endif
	altq_devsw_installed = 1;
	printf("altq: attached. Major number assigned automatically.\n");

	/* create minor devices */
	for (unit = 0; unit < naltqsw; unit++) {
		if (unit == 0 || altqsw[unit].d_open != NULL)
			altqsw[unit].dev = make_dev(&altq_cdevsw, unit,
			    UID_ROOT, GID_WHEEL, 0644, "altq/%s",
			    altqsw[unit].d_name);
	}
}

SYSINIT(altqdev,SI_SUB_DRIVERS,SI_ORDER_MIDDLE+CDEV_MAJOR,altq_drvinit,NULL)

#endif /* FreeBSD */

#ifdef ALTQ_KLD
/*
 * KLD support
 */
static int altq_module_register(struct altq_module_data *);
static int altq_module_deregister(struct altq_module_data *);

static struct altq_module_data *altq_modules[ALTQT_MAX];
#if __FreeBSD_version < 502103
static struct altqsw noqdisc = {"noq", noopen, noclose, noioctl};
#else
static struct altqsw noqdisc = {"noq"};
#endif

void altq_module_incref(int type)
{
	if (type < 0 || type >= ALTQT_MAX || altq_modules[type] == NULL)
		return;

	altq_modules[type]->ref++;
}

void altq_module_declref(int type)
{
	if (type < 0 || type >= ALTQT_MAX || altq_modules[type] == NULL)
		return;

	altq_modules[type]->ref--;
}

static int
altq_module_register(struct altq_module_data *mdata)
{
	int type = mdata->type;

	if (type < 0 || type >= ALTQT_MAX)
		return (EINVAL);
#if (__FreeBSD_version < 502103)
	if (altqsw[type].d_open != noopen)
#else
	if (altqsw[type].d_open != NULL)
#endif
		return (EBUSY);
	altqsw[type] = *mdata->altqsw;	/* set discipline functions */
	altq_modules[type] = mdata;	/* save module data pointer */
#if (__FreeBSD_version < 502103)
	make_dev(&altq_cdevsw, type, UID_ROOT, GID_WHEEL, 0644,
		 "altq/%s", altqsw[type].d_name);
#else
	altqsw[type].dev = make_dev(&altq_cdevsw, type, UID_ROOT, GID_WHEEL,
	    0644, "altq/%s", altqsw[type].d_name);
#endif
	return (0);
}

static int
altq_module_deregister(struct altq_module_data *mdata)
{
	int type = mdata->type;

	if (type < 0 || type >= ALTQT_MAX)
		return (EINVAL);
	if (mdata != altq_modules[type])
		return (EINVAL);
	if (altq_modules[type]->ref > 0)
		return (EBUSY);
#if (__FreeBSD_version < 502103)
	destroy_dev(makedev(CDEV_MAJOR, type));
#else
	destroy_dev(altqsw[type].dev);
#endif
	altqsw[type] = noqdisc;
	altq_modules[type] = NULL;
	return (0);
}

int
altq_module_handler(module_t mod, int cmd, void *arg)
{
	struct altq_module_data *data = (struct altq_module_data *)arg;
	int	error = 0;

	switch (cmd) {
	case MOD_LOAD:
		error = altq_module_register(data);
		break;

	case MOD_UNLOAD:
		error = altq_module_deregister(data);
		break;

	default:
		error = EINVAL;
		break;
	}

	return (error);
}

#endif  /* ALTQ_KLD */
#endif /* ALTQ3_COMPAT */
