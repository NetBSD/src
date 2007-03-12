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
 *	i4b_i4bdrv.c - i4b userland interface driver
 *	--------------------------------------------
 *
 *	$Id: i4b_i4bdrv.c,v 1.30.4.1 2007/03/12 06:00:12 rmind Exp $
 *
 * $FreeBSD$
 *
 *      last edit-date: [Fri Jan  5 11:33:47 2001]
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i4b_i4bdrv.c,v 1.30.4.1 2007/03/12 06:00:12 rmind Exp $");

#include "isdn.h"

#if NISDN > 0

#include <sys/param.h>

#if defined(__FreeBSD__)
#include <sys/ioccom.h>
#include <sys/malloc.h>
#include <sys/uio.h>
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
#include <sys/select.h>
#include <net/if.h>

#ifdef __FreeBSD__

#if defined(__FreeBSD__) && __FreeBSD__ == 3
#include "opt_devfs.h"
#endif

#ifdef DEVFS
#include <sys/devfsext.h>
#endif

#endif /* __FreeBSD__*/

#ifdef __FreeBSD__
#include <machine/i4b_debug.h>
#include <machine/i4b_ioctl.h>
#include <machine/i4b_cause.h>
#else
#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_ioctl.h>
#include <netisdn/i4b_cause.h>
#endif

#include <netisdn/i4b_l3l4.h>
#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_global.h>

#include <netisdn/i4b_l4.h>

#ifdef OS_USES_POLL
#include <sys/poll.h>
#endif

struct selinfo select_rd_info;

static struct ifqueue i4b_rdqueue;
static int openflag = 0;
static int selflag = 0;
static int readflag = 0;

#if defined(__FreeBSD__) && __FreeBSD__ == 3
#ifdef DEVFS
static void *devfs_token;
#endif
#endif

#ifndef __FreeBSD__

#define	PDEVSTATIC	/* - not static - */
PDEVSTATIC void isdnattach __P((void));
PDEVSTATIC int isdnopen __P((dev_t dev, int flag, int fmt, struct lwp *l));
PDEVSTATIC int isdnclose __P((dev_t dev, int flag, int fmt, struct lwp *l));
PDEVSTATIC int isdnread __P((dev_t dev, struct uio *uio, int ioflag));
PDEVSTATIC int isdnioctl __P((dev_t dev, u_long cmd, void *data, int flag, struct lwp *l));

#ifdef OS_USES_POLL
PDEVSTATIC int isdnpoll __P((dev_t dev, int events, struct lwp *l));
PDEVSTATIC int isdnkqfilter __P((dev_t dev, struct knote *kn));
#else
PDEVSTATIC int isdnselect __P((dev_t dev, int rw, struct lwp *l));
#endif

#endif /* #ifndef __FreeBSD__ */

#if BSD > 199306 && defined(__FreeBSD__)

#define PDEVSTATIC	static

PDEVSTATIC	d_open_t	i4bopen;
PDEVSTATIC	d_close_t	i4bclose;
PDEVSTATIC	d_read_t	i4bread;
PDEVSTATIC	d_ioctl_t	i4bioctl;

#ifdef OS_USES_POLL
PDEVSTATIC	d_poll_t	i4bpoll;
#define POLLFIELD		i4bpoll
#else
PDEVSTATIC	d_select_t	i4bselect;
#define POLLFIELD		i4bselect
#endif

#define CDEV_MAJOR 60

#if defined(__FreeBSD__) && __FreeBSD__ >= 4
static struct cdevsw i4b_cdevsw = {
	/* open */      i4bopen,
	/* close */     i4bclose,
	/* read */      i4bread,
	/* write */     nowrite,
	/* ioctl */     i4bioctl,
	/* poll */      POLLFIELD,
	/* mmap */      nommap,
	/* strategy */  nostrategy,
	/* name */      "i4b",
	/* maj */       CDEV_MAJOR,
	/* dump */      nodump,
	/* psize */     nopsize,
	/* flags */     0,
	/* bmaj */      -1
};
#else
static struct cdevsw i4b_cdevsw = {
	i4bopen,	i4bclose,	i4bread,	nowrite,
	i4bioctl,	nostop,		nullreset,	nodevtotty,
	POLLFIELD,	nommap,		NULL,		"i4b", NULL,	-1
};
#endif

PDEVSTATIC void i4battach(void *);
PSEUDO_SET(i4battach, i4b_i4bdrv);

static void
i4b_drvinit(void *unused)
{
#if defined(__FreeBSD__) && __FreeBSD__ >= 4
	cdevsw_add(&i4b_cdevsw);
#else
	static int i4b_devsw_installed = 0;
	dev_t dev;

	if( ! i4b_devsw_installed )
	{
		dev = makedev(CDEV_MAJOR,0);
		cdevsw_add(&dev,&i4b_cdevsw,NULL);
		i4b_devsw_installed = 1;
	}
#endif
}

SYSINIT(i4bdev,SI_SUB_DRIVERS,SI_ORDER_MIDDLE+CDEV_MAJOR,i4b_drvinit,NULL)

#endif /* BSD > 199306 && defined(__FreeBSD__) */

#ifdef __NetBSD__
const struct cdevsw isdn_cdevsw = {
	isdnopen, isdnclose, isdnread, nowrite, isdnioctl,
	nostop, notty, isdnpoll, nommap, isdnkqfilter, D_OTHER
};
#endif /* __NetBSD__ */

#ifdef __bsdi__
#include <sys/device.h>
int i4bmatch(struct device *parent, struct cfdata *cf, void *aux);
void dummy_i4battach(struct device*, struct device *, void *);

#define CDEV_MAJOR 65

static struct cfdriver i4bcd =
	{ NULL, "i4b", i4bmatch, dummy_i4battach, DV_DULL,
	  sizeof(struct cfdriver) };
struct devsw i4bsw =
	{ &i4bcd,
	  i4bopen,	i4bclose,	i4bread,	nowrite,
	  i4bioctl,	i4bselect,	nommap,		nostrat,
	  nodump,	nopsize,	0,		nostop
};

int
i4bmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	printf("i4bmatch: aux=0x%x\n", aux);
	return 1;
}
void
dummy_i4battach(struct device *parent, struct device *self, void *aux)
{
	printf("dummy_i4battach: aux=0x%x\n", aux);
}
#endif /* __bsdi__ */

/*---------------------------------------------------------------------------*
 *	interface attach routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC void
#ifdef __FreeBSD__
isdnattach(void *dummy)
#else
isdnattach()
#endif
{
	i4b_rdqueue.ifq_maxlen = IFQ_MAXLEN;

#if defined(__FreeBSD__)
#if __FreeBSD__ == 3

#ifdef DEVFS
	devfs_token = devfs_add_devswf(&i4b_cdevsw, 0, DV_CHR,
				       UID_ROOT, GID_WHEEL, 0600,
				       "i4b");
#endif

#else
	make_dev(&i4b_cdevsw, 0, UID_ROOT, GID_WHEEL, 0600, "i4b");
#endif
#endif
}

/*---------------------------------------------------------------------------*
 *	i4bopen - device driver open routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnopen(dev_t dev, int flag, int fmt, struct lwp *l)
{
	int x;

	if(minor(dev))
		return(ENXIO);

	if(openflag)
		return(EBUSY);

	x = splnet();
	openflag = 1;
	i4b_l4_daemon_attached();
	splx(x);

	return(0);
}

/*---------------------------------------------------------------------------*
 *	i4bclose - device driver close routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnclose(dev_t dev, int flag, int fmt,
	struct lwp *l)
{
	int x = splnet();
	openflag = 0;
	i4b_l4_daemon_detached();
	i4b_Dcleanifq(&i4b_rdqueue);
	splx(x);
	return(0);
}

/*---------------------------------------------------------------------------*
 *	i4bread - device driver read routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnread(dev_t dev, struct uio *uio, int ioflag)
{
	struct mbuf *m;
	int x;
	int error = 0;

	if(minor(dev))
		return(ENODEV);

	x = splnet();
	while(IF_QEMPTY(&i4b_rdqueue))
	{
		readflag = 1;
		error = tsleep((void *) &i4b_rdqueue, (PZERO + 1) | PCATCH, "bird", 0);
		if (error != 0) {
			splx(x);
			return error;
		}
	}

	IF_DEQUEUE(&i4b_rdqueue, m);

	splx(x);

	if(m && m->m_len)
		error = uiomove(m->m_data, m->m_len, uio);
	else
		error = EIO;

	if(m)
		i4b_Dfreembuf(m);

	return(error);
}

/*---------------------------------------------------------------------------*
 *	i4bioctl - device driver ioctl routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnioctl(dev_t dev, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct isdn_l3_driver *d;
	call_desc_t *cd;
	int error = 0;

	if(minor(dev))
		return(ENODEV);

	switch(cmd)
	{
		/* cdid request, reserve cd and return cdid */

		case I4B_CDID_REQ:
		{
			msg_cdid_req_t *mir;
			mir = (msg_cdid_req_t *)data;
			cd = reserve_cd();
			mir->cdid = cd->cdid;
			break;
		}

		/* connect request, dial out to remote */

		case I4B_CONNECT_REQ:
		{
			msg_connect_req_t *mcr;
			mcr = (msg_connect_req_t *)data;	/* setup ptr */

			if((cd = cd_by_cdid(mcr->cdid)) == NULL)/* get cd */
			{
				NDBGL4(L4_ERR, "I4B_CONNECT_REQ ioctl, cdid not found!");
				error = EINVAL;
				break;
			}
			cd->isdnif = -1;
			cd->l3drv = NULL;

			d = isdn_find_l3_by_isdnif(mcr->controller);
			if (d == NULL) {
				error = EINVAL;
				break;
			}

			/* prevent dialling on leased lines */
			if(d->protocol == PROTOCOL_D64S)
			{
				SET_CAUSE_TYPE(cd->cause_in, CAUSET_I4B);
				SET_CAUSE_VAL(cd->cause_in, CAUSE_I4B_LLDIAL);
				i4b_l4_disconnect_ind(cd);
				freecd_by_cd(cd);
				break;
			}

			cd->isdnif = mcr->controller;	/* fill cd */
			cd->l3drv = d;
			cd->bprot = mcr->bprot;
			cd->bchan_driver_index = mcr->driver;
			cd->bchan_driver_unit = mcr->driver_unit;
			cd->cr = get_rand_cr(cd->isdnif);

			cd->shorthold_data.shorthold_algorithm = mcr->shorthold_data.shorthold_algorithm;
			cd->shorthold_data.unitlen_time  = mcr->shorthold_data.unitlen_time;
			cd->shorthold_data.idle_time     = mcr->shorthold_data.idle_time;
			cd->shorthold_data.earlyhup_time = mcr->shorthold_data.earlyhup_time;

			cd->last_aocd_time = 0;
			if(mcr->unitlen_method == ULEN_METHOD_DYNAMIC)
				cd->aocd_flag = 1;
			else
				cd->aocd_flag = 0;

			cd->cunits = 0;

			cd->max_idle_time = 0;	/* this is outgoing */

			cd->dir = DIR_OUTGOING;

			NDBGL4(L4_TIMO, "I4B_CONNECT_REQ times, algorithm=%ld unitlen=%ld idle=%ld earlyhup=%ld",
					(long)cd->shorthold_data.shorthold_algorithm, (long)cd->shorthold_data.unitlen_time,
					(long)cd->shorthold_data.idle_time, (long)cd->shorthold_data.earlyhup_time);

			strlcpy(cd->dst_telno, mcr->dst_telno,
			    sizeof(cd->dst_telno));
			strlcpy(cd->src_telno, mcr->src_telno,
			    sizeof(cd->src_telno));
			cd->display[0] = '\0';

			SET_CAUSE_TYPE(cd->cause_in, CAUSET_I4B);
			SET_CAUSE_VAL(cd->cause_in, CAUSE_I4B_NORMAL);

			/*
			 * If we want a specific channel, check if that
			 * one is available.
			 */
			if ((mcr->channel >= 0) && (mcr->channel < d->nbch)) {
				if(d->bch_state[mcr->channel] != BCH_ST_FREE)
					SET_CAUSE_VAL(cd->cause_in,
					    CAUSE_I4B_NOCHAN);

			/*
			 * If any channel will do, see if any are free.
			 */
			} else if (mcr->channel == CHAN_ANY) {
				int i;

				for (i = 0; i < d->nbch; i++)
					if (d->bch_state[i] == BCH_ST_FREE)
						break;

				if (i == d->nbch)
					SET_CAUSE_VAL(cd->cause_in,
					    CAUSE_I4B_NOCHAN);

			} else {
				SET_CAUSE_VAL(cd->cause_in, CAUSE_I4B_NOCHAN);
			}

			cd->channelid = mcr->channel;

			cd->isdntxdelay = mcr->txdelay;

			if((GET_CAUSE_VAL(cd->cause_in)) != CAUSE_I4B_NORMAL)
			{
				i4b_l4_disconnect_ind(cd);
				freecd_by_cd(cd);
			}
			else
			{
				d->l3driver->N_CONNECT_REQUEST(cd);
			}
			break;
		}

		/* connect response, accept/reject/ignore incoming call */

		case I4B_CONNECT_RESP:
		{
			msg_connect_resp_t *mcrsp;

			mcrsp = (msg_connect_resp_t *)data;

			if((cd = cd_by_cdid(mcrsp->cdid)) == NULL)/* get cd */
			{
				NDBGL4(L4_ERR, "I4B_CONNECT_RESP ioctl, cdid not found!");
				error = EINVAL;
				break;
			}

			T400_stop(cd);

			cd->bchan_driver_index = mcrsp->driver;
			cd->bchan_driver_unit = mcrsp->driver_unit;
			cd->max_idle_time = mcrsp->max_idle_time;

			cd->shorthold_data.shorthold_algorithm = SHA_FIXU;
			cd->shorthold_data.unitlen_time = 0;	/* this is incoming */
			cd->shorthold_data.idle_time = 0;
			cd->shorthold_data.earlyhup_time = 0;

			cd->isdntxdelay = mcrsp->txdelay;

			NDBGL4(L4_TIMO, "I4B_CONNECT_RESP max_idle_time set to %ld seconds", (long)cd->max_idle_time);

			d = isdn_find_l3_by_isdnif(cd->isdnif);
			if (d == NULL) {
				error = EINVAL;
				break;
			}
			d->l3driver->N_CONNECT_RESPONSE(cd, mcrsp->response, mcrsp->cause);
			break;
		}

		/* disconnect request, actively terminate connection */

		case I4B_DISCONNECT_REQ:
		{
			msg_discon_req_t *mdr;

			mdr = (msg_discon_req_t *)data;

			if((cd = cd_by_cdid(mdr->cdid)) == NULL)/* get cd */
			{
				NDBGL4(L4_ERR, "I4B_DISCONNECT_REQ ioctl, cdid %d not found!", mdr->cdid);
				error = EINVAL;
				break;
			}

			/* preset causes with our cause */
			cd->cause_in = cd->cause_out = mdr->cause;

			d = isdn_find_l3_by_isdnif(cd->isdnif);
			if (d == NULL) {
				error = EINVAL;
				break;
			}

			d->l3driver->N_DISCONNECT_REQUEST(cd, mdr->cause);
			break;
		}

		/* controller info request */

		case I4B_CTRL_INFO_REQ:
		{
			msg_ctrl_info_req_t *mcir;
			int isdnif;

			mcir = (msg_ctrl_info_req_t *)data;
			isdnif = mcir->controller;
			memset(mcir, 0, sizeof(msg_ctrl_info_req_t));
			mcir->controller = isdnif;
			mcir->ncontroller
			    = isdn_count_isdnif(&mcir->max_isdnif);
			d = isdn_find_l3_by_isdnif(isdnif);
			if (d != NULL) {
				mcir->tei = d->tei;
				mcir->nbch = d->nbch;
				strncpy(mcir->devname, d->devname, sizeof(mcir->devname)-1);
				strncpy(mcir->cardname, d->card_name, sizeof(mcir->cardname)-1);
			} else {
				error = ENODEV;
			}
			break;
		}

		/* dial response */

		case I4B_DIALOUT_RESP:
		{
			const struct isdn_l4_driver_functions *drv;
			msg_dialout_resp_t *mdrsp;
			void *l4_softc;

			mdrsp = (msg_dialout_resp_t *)data;
			drv = isdn_l4_get_driver(mdrsp->driver, mdrsp->driver_unit);

			if(drv != NULL)	{
				l4_softc = (*drv->get_softc)(mdrsp->driver_unit);
				(*drv->dial_response)(l4_softc, mdrsp->stat, mdrsp->cause);
			}
			break;
		}

		/* update timeout value */

		case I4B_TIMEOUT_UPD:
		{
			msg_timeout_upd_t *mtu;
			int x;

			mtu = (msg_timeout_upd_t *)data;

			NDBGL4(L4_TIMO, "I4B_TIMEOUT_UPD ioctl, alg %d, unit %d, idle %d, early %d!",
					mtu->shorthold_data.shorthold_algorithm, mtu->shorthold_data.unitlen_time,
					mtu->shorthold_data.idle_time, mtu->shorthold_data.earlyhup_time);

			if((cd = cd_by_cdid(mtu->cdid)) == NULL)/* get cd */
			{
				NDBGL4(L4_ERR, "I4B_TIMEOUT_UPD ioctl, cdid not found!");
				error = EINVAL;
				break;
			}

			switch( mtu->shorthold_data.shorthold_algorithm )
			{
				case SHA_FIXU:
					/*
					 * For this algorithm unitlen_time,
					 * idle_time and earlyhup_time are used.
					 */

					if(!(mtu->shorthold_data.unitlen_time >= 0 &&
					     mtu->shorthold_data.idle_time >= 0    &&
					     mtu->shorthold_data.earlyhup_time >= 0))
					{
						NDBGL4(L4_ERR, "I4B_TIMEOUT_UPD ioctl, invalid args for fix unit algorithm!");
						error = EINVAL;
					}
					break;

				case SHA_VARU:
					/*
					 * For this algorithm unitlen_time and
					 * idle_time are used. both must be
					 * positive integers. earlyhup_time is
					 * not used and must be 0.
					 */

					if(!(mtu->shorthold_data.unitlen_time > 0 &&
					     mtu->shorthold_data.idle_time >= 0   &&
					     mtu->shorthold_data.earlyhup_time == 0))
					{
						NDBGL4(L4_ERR, "I4B_TIMEOUT_UPD ioctl, invalid args for var unit algorithm!");
						error = EINVAL;
					}
					break;

				default:
					NDBGL4(L4_ERR, "I4B_TIMEOUT_UPD ioctl, invalid algorithm!");
					error = EINVAL;
					break;
			}

			/*
			 * any error set above requires us to break
			 * out of the outer switch
			 */
			if(error != 0)
				break;

			x = splnet();
			cd->shorthold_data.shorthold_algorithm = mtu->shorthold_data.shorthold_algorithm;
			cd->shorthold_data.unitlen_time = mtu->shorthold_data.unitlen_time;
			cd->shorthold_data.idle_time = mtu->shorthold_data.idle_time;
			cd->shorthold_data.earlyhup_time = mtu->shorthold_data.earlyhup_time;
			splx(x);
			break;
		}

		/* soft enable/disable interface */

		case I4B_UPDOWN_IND:
		{
			msg_updown_ind_t *mui;
			const struct isdn_l4_driver_functions *drv;
			void *l4_softc;

			mui = (msg_updown_ind_t *)data;
			drv = isdn_l4_get_driver(mui->driver, mui->driver_unit);

			if (drv)
			{
				l4_softc = drv->get_softc(mui->driver_unit);
				(*drv->updown_ind)(l4_softc, mui->updown);
			}
			break;
		}

		/* send ALERT request */

		case I4B_ALERT_REQ:
		{
			msg_alert_req_t *mar;

			mar = (msg_alert_req_t *)data;

			if((cd = cd_by_cdid(mar->cdid)) == NULL)
			{
				NDBGL4(L4_ERR, "I4B_ALERT_REQ ioctl, cdid not found!");
				error = EINVAL;
				break;
			}

			T400_stop(cd);

			d = cd->l3drv;
			if (d == NULL) {
				error = EINVAL;
				break;
			}
			d->l3driver->N_ALERT_REQUEST(cd);

			break;
		}

		/* version/release number request */

		case I4B_VR_REQ:
                {
			msg_vr_req_t *mvr;

			mvr = (msg_vr_req_t *)data;

			mvr->version = VERSION;
			mvr->release = REL;
			mvr->step = STEP;
			break;
		}

		/* set D-channel protocol for a controller */

		case I4B_PROT_IND:
		{
			msg_prot_ind_t *mpi;

			mpi = (msg_prot_ind_t *)data;

			d = isdn_find_l3_by_isdnif(mpi->controller);
			if (d == NULL) {
				error = EINVAL;
				break;
			}
			d->protocol = mpi->protocol;

			break;
		}

		case I4B_L4DRIVER_LOOKUP:
		{
			msg_l4driver_lookup_t *lookup = (msg_l4driver_lookup_t*)data;
			lookup->name[L4DRIVER_NAME_SIZ-1] = 0;
			lookup->driver_id = isdn_l4_find_driverid(lookup->name);
			if (lookup->driver_id < 0)
				error = ENXIO;
			break;
		}

		/* Download request */
		case I4B_CTRL_DOWNLOAD:
		{
			struct isdn_dr_prot *prots = NULL, *prots2 = NULL;
			struct isdn_download_request *r =
				(struct isdn_download_request*)data;
			int i;

			d = isdn_find_l3_by_isdnif(r->controller);
			if (d == NULL)
			{
				error = ENODEV;
				goto download_done;
			}

			if(d->l3driver->N_DOWNLOAD == NULL)
			{
				error = ENODEV;
				goto download_done;
			}

			prots = malloc(r->numprotos * sizeof(struct isdn_dr_prot),
					M_DEVBUF, M_WAITOK);

			prots2 = malloc(r->numprotos * sizeof(struct isdn_dr_prot),
					M_DEVBUF, M_WAITOK);

			if(!prots || !prots2)
			{
				error = ENOMEM;
				goto download_done;
			}

			copyin(r->protocols, prots, r->numprotos * sizeof(struct isdn_dr_prot));

			for(i = 0; i < r->numprotos; i++)
			{
				prots2[i].microcode = malloc(prots[i].bytecount, M_DEVBUF, M_WAITOK);
				copyin(prots[i].microcode, prots2[i].microcode, prots[i].bytecount);
				prots2[i].bytecount = prots[i].bytecount;
			}

			error = d->l3driver->N_DOWNLOAD(
						d->l1_token,
						r->numprotos, prots2);

download_done:
			if(prots2)
			{
				for(i = 0; i < r->numprotos; i++)
				{
					if(prots2[i].microcode)
					{
						free(prots2[i].microcode, M_DEVBUF);
					}
				}
				free(prots2, M_DEVBUF);
			}

			if(prots)
			{
				free(prots, M_DEVBUF);
			}
			break;
		}

		/* Diagnostic request */

		case I4B_ACTIVE_DIAGNOSTIC:
		{
			struct isdn_diagnostic_request req, *r =
				(struct isdn_diagnostic_request*)data;

			req.in_param = req.out_param = NULL;
			d = isdn_find_l3_by_isdnif(r->controller);
			if (d == NULL)
			{
				error = ENODEV;
				goto diag_done;
			}

			if (d->l3driver->N_DIAGNOSTICS == NULL)
			{
				error = ENODEV;
				goto diag_done;
			}

			memcpy(&req, r, sizeof(req));

			if(req.in_param_len)
			{
				/* XXX arbitrary limit */
				if (req.in_param_len > I4B_ACTIVE_DIAGNOSTIC_MAXPARAMLEN) {
					error = EINVAL;
					goto diag_done;
				}

				req.in_param = malloc(r->in_param_len, M_DEVBUF, M_WAITOK);

				if(!req.in_param)
				{
					error = ENOMEM;
					goto diag_done;
				}
				error = copyin(r->in_param, req.in_param, req.in_param_len);
				if (error)
					goto diag_done;
			}

			if(req.out_param_len)
			{
				req.out_param = malloc(r->out_param_len, M_DEVBUF, M_WAITOK);

				if(!req.out_param)
				{
					error = ENOMEM;
					goto diag_done;
				}
			}

			error = d->l3driver->N_DIAGNOSTICS(d->l1_token, &req);

			if(!error && req.out_param_len)
				error = copyout(req.out_param, r->out_param, req.out_param_len);

diag_done:
			if(req.in_param)
				free(req.in_param, M_DEVBUF);

			if(req.out_param)
				free(req.out_param, M_DEVBUF);

			break;
		}

		/* default */

		default:
			error = ENOTTY;
			break;
	}

	return(error);
}

#ifdef OS_USES_SELECT

/*---------------------------------------------------------------------------*
 *	i4bselect - device driver select routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
i4bselect(dev_t dev, int rw, struct lwp *l)
{
	int x;

	if(minor(dev))
		return(ENODEV);

	switch(rw)
	{
		case FREAD:
			if(!IF_QEMPTY(&i4b_rdqueue))
				return(1);
			x = splnet();
			selrecord(l, &select_rd_info);
			selflag = 1;
			splx(x);
			return(0);
			break;

		case FWRITE:
			return(1);
			break;
	}
	return(0);
}

#else /* OS_USES_SELECT */

/*---------------------------------------------------------------------------*
 *	i4bpoll - device driver poll routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnpoll(dev_t dev, int events, struct lwp *l)
{
	int x;

	if(minor(dev))
		return(ENODEV);

	if((events & POLLIN) || (events & POLLRDNORM))
	{
		if(!IF_QEMPTY(&i4b_rdqueue))
			return(1);

		x = splnet();
		selrecord(l, &select_rd_info);
		selflag = 1;
		splx(x);
		return(0);
	}
	else if((events & POLLOUT) || (events & POLLWRNORM))
	{
		return(1);
	}

	return(0);
}

static void
filt_i4brdetach(struct knote *kn)
{
	int s;

	s = splnet();
	SLIST_REMOVE(&select_rd_info.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_i4bread(struct knote *kn, long hint)
{
	struct mbuf *m;

	if (IF_QEMPTY(&i4b_rdqueue))
		return (0);

	IF_POLL(&i4b_rdqueue, m);

	kn->kn_data = m->m_len;
	return (1);
}

static const struct filterops i4bread_filtops =
	{ 1, NULL, filt_i4brdetach, filt_i4bread };

static const struct filterops i4b_seltrue_filtops =
	{ 1, NULL, filt_i4brdetach, filt_seltrue };

int
isdnkqfilter(dev_t dev, struct knote *kn)
{
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &select_rd_info.sel_klist;
		kn->kn_fop = &i4bread_filtops;
		break;

	case EVFILT_WRITE:
		klist = &select_rd_info.sel_klist;
		kn->kn_fop = &i4b_seltrue_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = NULL;

	s = splnet();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}

#endif /* OS_USES_SELECT */

/*---------------------------------------------------------------------------*
 *	i4bputqueue - put message into queue to userland
 *---------------------------------------------------------------------------*/
void
i4bputqueue(struct mbuf *m)
{
	int x;

	if(!openflag)
	{
		i4b_Dfreembuf(m);
		return;
	}

	x = splnet();

	if(IF_QFULL(&i4b_rdqueue))
	{
		struct mbuf *m1;
		IF_DEQUEUE(&i4b_rdqueue, m1);
		i4b_Dfreembuf(m1);
		NDBGL4(L4_ERR, "ERROR, queue full, removing entry!");
	}

	IF_ENQUEUE(&i4b_rdqueue, m);

	splx(x);

	if(readflag)
	{
		readflag = 0;
		wakeup((void *) &i4b_rdqueue);
	}

	if(selflag)
	{
		selflag = 0;
		selnotify(&select_rd_info, 0);
	}
}

/*---------------------------------------------------------------------------*
 *	i4bputqueue_hipri - put message into front of queue to userland
 *---------------------------------------------------------------------------*/
void
i4bputqueue_hipri(struct mbuf *m)
{
	int x;

	if(!openflag)
	{
		i4b_Dfreembuf(m);
		return;
	}

	x = splnet();

	if(IF_QFULL(&i4b_rdqueue))
	{
		struct mbuf *m1;
		IF_DEQUEUE(&i4b_rdqueue, m1);
		i4b_Dfreembuf(m1);
		NDBGL4(L4_ERR, "ERROR, queue full, removing entry!");
	}

	IF_PREPEND(&i4b_rdqueue, m);

	splx(x);

	if(readflag)
	{
		readflag = 0;
		wakeup((void *) &i4b_rdqueue);
	}

	if(selflag)
	{
		selflag = 0;
		selnotify(&select_rd_info, 0);
	}
}

void
isdn_isdnif_ready(int isdnif)
{
	struct isdn_l3_driver *d = isdn_find_l3_by_isdnif(isdnif);

	if (d == NULL)
		return;

	printf("ISDN %d at %s, %d B channels\n", isdnif, d->devname, d->nbch);
	if (!openflag) return;

	d->l3driver->N_MGMT_COMMAND(d, CMR_DOPEN, 0);
	i4b_l4_contr_ev_ind(isdnif, 1);
}

#endif /* NISDN > 0 */
