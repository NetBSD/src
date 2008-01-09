/*	$NetBSD: ata.c,v 1.90.8.2 2008/01/09 01:52:24 matt Exp $	*/

/*
 * Copyright (c) 1998, 2001 Manuel Bouyer.  All rights reserved.
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
 *  This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ata.c,v 1.90.8.2 2008/01/09 01:52:24 matt Exp $");

#include "opt_ata.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/kthread.h>
#include <sys/errno.h>
#include <sys/ataio.h>
#include <sys/kmem.h>
#include <sys/simplelock.h>
#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/ata/ataconf.h>
#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>	/* for PIOBM */

#include "locators.h"

#include "atapibus.h"
#include "ataraid.h"

#if NATARAID > 0
#include <dev/ata/ata_raidvar.h>
#endif

#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10
#define DEBUG_DETACH 0x20
#define	DEBUG_XFERS  0x40
#ifdef ATADEBUG
int atadebug_mask = 0;
#define ATADEBUG_PRINT(args, level) \
	if (atadebug_mask & (level)) \
		printf args
#else
#define ATADEBUG_PRINT(args, level)
#endif

POOL_INIT(ata_xfer_pool, sizeof(struct ata_xfer), 0, 0, 0, "ataspl", NULL,
    IPL_BIO);

/*
 * A queue of atabus instances, used to ensure the same bus probe order
 * for a given hardware configuration at each boot.
 */
struct atabus_initq_head atabus_initq_head =
    TAILQ_HEAD_INITIALIZER(atabus_initq_head);
struct simplelock atabus_interlock = SIMPLELOCK_INITIALIZER;

/*****************************************************************************
 * ATA bus layer.
 *
 * ATA controllers attach an atabus instance, which handles probing the bus
 * for drives, etc.
 *****************************************************************************/

dev_type_open(atabusopen);
dev_type_close(atabusclose);
dev_type_ioctl(atabusioctl);

const struct cdevsw atabus_cdevsw = {
	atabusopen, atabusclose, noread, nowrite, atabusioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_OTHER
};

extern struct cfdriver atabus_cd;

static bool atabus_resume(device_t);
static bool atabus_suspend(device_t);

/*
 * atabusprint:
 *
 *	Autoconfiguration print routine used by ATA controllers when
 *	attaching an atabus instance.
 */
int
atabusprint(void *aux, const char *pnp)
{
	struct ata_channel *chan = aux;

	if (pnp)
		aprint_normal("atabus at %s", pnp);
	aprint_normal(" channel %d", chan->ch_channel);

	return (UNCONF);
}

/*
 * ataprint:
 *
 *	Autoconfiguration print routine.
 */
int
ataprint(void *aux, const char *pnp)
{
	struct ata_device *adev = aux;

	if (pnp)
		aprint_normal("wd at %s", pnp);
	aprint_normal(" drive %d", adev->adev_drv_data->drive);

	return (UNCONF);
}

/*
 * ata_channel_attach:
 *
 *	Common parts of attaching an atabus to an ATA controller channel.
 */
void
ata_channel_attach(struct ata_channel *chp)
{

	if (chp->ch_flags & ATACH_DISABLED)
		return;

	callout_init(&chp->ch_callout, 0);

	TAILQ_INIT(&chp->ch_queue->queue_xfer);
	chp->ch_queue->queue_freeze = 0;
	chp->ch_queue->queue_flags = 0;
	chp->ch_queue->active_xfer = NULL;

	chp->atabus = config_found_ia(&chp->ch_atac->atac_dev, "ata", chp,
		atabusprint);
}

static void
atabusconfig(struct atabus_softc *atabus_sc)
{
	struct ata_channel *chp = atabus_sc->sc_chan;
	struct atac_softc *atac = chp->ch_atac;
	int i, s;
	struct atabus_initq *atabus_initq = NULL;

	/* Probe for the drives. */
	/* XXX for SATA devices we will power up all drives at once */
	(*atac->atac_probe)(chp);

	ATADEBUG_PRINT(("atabusattach: ch_drive_flags 0x%x 0x%x\n",
	    chp->ch_drive[0].drive_flags, chp->ch_drive[1].drive_flags),
	    DEBUG_PROBE);

	/* If no drives, abort here */
	for (i = 0; i < chp->ch_ndrive; i++)
		if ((chp->ch_drive[i].drive_flags & DRIVE) != 0)
			break;
	if (i == chp->ch_ndrive)
		goto out;

	/* Shortcut in case we've been shutdown */
	if (chp->ch_flags & ATACH_SHUTDOWN)
		goto out;

	/* Make sure the devices probe in atabus order to avoid jitter. */
	simple_lock(&atabus_interlock);
	while(1) {
		atabus_initq = TAILQ_FIRST(&atabus_initq_head);
		if (atabus_initq->atabus_sc == atabus_sc)
			break;
		ltsleep(&atabus_initq_head, PRIBIO, "ata_initq", 0,
		    &atabus_interlock);
	}
	simple_unlock(&atabus_interlock);

	/*
	 * Attach an ATAPI bus, if needed.
	 */
	for (i = 0; i < chp->ch_ndrive; i++) {
		if (chp->ch_drive[i].drive_flags & DRIVE_ATAPI) {
#if NATAPIBUS > 0
			(*atac->atac_atapibus_attach)(atabus_sc);
#else
			/*
			 * Fake the autoconfig "not configured" message
			 */
			aprint_normal("atapibus at %s not configured\n",
			    atac->atac_dev.dv_xname);
			chp->atapibus = NULL;
			s = splbio();
			for (i = 0; i < chp->ch_ndrive; i++)
				chp->ch_drive[i].drive_flags &= ~DRIVE_ATAPI;
			splx(s);
#endif
			break;
		}
	}

	for (i = 0; i < chp->ch_ndrive; i++) {
		struct ata_device adev;
		if ((chp->ch_drive[i].drive_flags &
		    (DRIVE_ATA | DRIVE_OLD)) == 0) {
			continue;
		}
		memset(&adev, 0, sizeof(struct ata_device));
		adev.adev_bustype = atac->atac_bustype_ata;
		adev.adev_channel = chp->ch_channel;
		adev.adev_openings = 1;
		adev.adev_drv_data = &chp->ch_drive[i];
		chp->ata_drives[i] = config_found_ia(&atabus_sc->sc_dev,
		    "ata_hl", &adev, ataprint);
		if (chp->ata_drives[i] != NULL)
			ata_probe_caps(&chp->ch_drive[i]);
		else {
			s = splbio();
			chp->ch_drive[i].drive_flags &=
			    ~(DRIVE_ATA | DRIVE_OLD);
			splx(s);
		}
	}

	/* now that we know the drives, the controller can set its modes */
	if (atac->atac_set_modes) {
		(*atac->atac_set_modes)(chp);
		ata_print_modes(chp);
	}
#if NATARAID > 0
	if (atac->atac_cap & ATAC_CAP_RAID)
		for (i = 0; i < chp->ch_ndrive; i++)
			if (chp->ata_drives[i] != NULL)
				ata_raid_check_component(chp->ata_drives[i]);
#endif /* NATARAID > 0 */

	/*
	 * reset drive_flags for unattached devices, reset state for attached
	 * ones
	 */
	s = splbio();
	for (i = 0; i < chp->ch_ndrive; i++) {
		if (chp->ch_drive[i].drv_softc == NULL)
			chp->ch_drive[i].drive_flags = 0;
		else
			chp->ch_drive[i].state = 0;
	}
	splx(s);

 out:
	if (atabus_initq == NULL) {
		simple_lock(&atabus_interlock);
		while(1) {
			atabus_initq = TAILQ_FIRST(&atabus_initq_head);
			if (atabus_initq->atabus_sc == atabus_sc)
				break;
			ltsleep(&atabus_initq_head, PRIBIO, "ata_initq", 0,
			    &atabus_interlock);
		}
		simple_unlock(&atabus_interlock);
	}
	simple_lock(&atabus_interlock);
	TAILQ_REMOVE(&atabus_initq_head, atabus_initq, atabus_initq);
	simple_unlock(&atabus_interlock);

	free(atabus_initq, M_DEVBUF);
	wakeup(&atabus_initq_head);

	ata_delref(chp);

	config_pending_decr();
}

/*
 * atabus_thread:
 *
 *	Worker thread for the ATA bus.
 */
static void
atabus_thread(void *arg)
{
	struct atabus_softc *sc = arg;
	struct ata_channel *chp = sc->sc_chan;
	struct ata_xfer *xfer;
	int i, s;

	s = splbio();
	chp->ch_flags |= ATACH_TH_RUN;

	/*
	 * Probe the drives.  Reset all flags to 0 to indicate to controllers
	 * that can re-probe that all drives must be probed..
	 *
	 * Note: ch_ndrive may be changed during the probe.
	 */
	for (i = 0; i < ATA_MAXDRIVES; i++)
		chp->ch_drive[i].drive_flags = 0;
	splx(s);

	/* Configure the devices on the bus. */
	atabusconfig(sc);

	s = splbio();
	for (;;) {
		if ((chp->ch_flags & (ATACH_TH_RESET | ATACH_SHUTDOWN)) == 0 &&
		    (chp->ch_queue->active_xfer == NULL ||
		     chp->ch_queue->queue_freeze == 0)) {
			chp->ch_flags &= ~ATACH_TH_RUN;
			(void) tsleep(&chp->ch_thread, PRIBIO, "atath", 0);
			chp->ch_flags |= ATACH_TH_RUN;
		}
		if (chp->ch_flags & ATACH_SHUTDOWN) {
			break;
		}
		if (chp->ch_flags & ATACH_TH_RESET) {
			/*
			 * ata_reset_channel() will freeze 2 times, so
			 * unfreeze one time. Not a problem as we're at splbio
			 */
			chp->ch_queue->queue_freeze--;
			ata_reset_channel(chp, AT_WAIT | chp->ch_reset_flags);
		} else if (chp->ch_queue->active_xfer != NULL &&
			   chp->ch_queue->queue_freeze == 1) {
			/*
			 * Caller has bumped queue_freeze, decrease it.
			 */
			chp->ch_queue->queue_freeze--;
			xfer = chp->ch_queue->active_xfer;
			KASSERT(xfer != NULL);
			(*xfer->c_start)(xfer->c_chp, xfer);
		} else if (chp->ch_queue->queue_freeze > 1)
			panic("ata_thread: queue_freeze");
	}
	splx(s);
	chp->ch_thread = NULL;
	wakeup(&chp->ch_flags);
	kthread_exit(0);
}

/*
 * atabus_match:
 *
 *	Autoconfiguration match routine.
 */
static int
atabus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ata_channel *chp = aux;

	if (chp == NULL)
		return (0);

	if (cf->cf_loc[ATACF_CHANNEL] != chp->ch_channel &&
	    cf->cf_loc[ATACF_CHANNEL] != ATACF_CHANNEL_DEFAULT)
		return (0);

	return (1);
}

/*
 * atabus_attach:
 *
 *	Autoconfiguration attach routine.
 */
static void
atabus_attach(struct device *parent, struct device *self, void *aux)
{
	struct atabus_softc *sc = (void *) self;
	struct ata_channel *chp = aux;
	struct atabus_initq *initq;
	int error;

	sc->sc_chan = chp;

	aprint_normal("\n");
	aprint_naive("\n");

	if (ata_addref(chp))
		return;

	initq = malloc(sizeof(*initq), M_DEVBUF, M_WAITOK);
	initq->atabus_sc = sc;
	TAILQ_INSERT_TAIL(&atabus_initq_head, initq, atabus_initq);
	config_pending_incr();

	if ((error = kthread_create(PRI_NONE, 0, NULL, atabus_thread, sc,
	    &chp->ch_thread, "%s", sc->sc_dev.dv_xname)) != 0)
		aprint_error("%s: unable to create kernel thread: error %d\n",
		    sc->sc_dev.dv_xname, error);

	if (!pmf_device_register(self, atabus_suspend, atabus_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

/*
 * atabus_activate:
 *
 *	Autoconfiguration activation routine.
 */
static int
atabus_activate(struct device *self, enum devact act)
{
	struct atabus_softc *sc = (void *) self;
	struct ata_channel *chp = sc->sc_chan;
	struct device *dev = NULL;
	int s, i, error = 0;

	s = splbio();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		/*
		 * We might deactivate the children of atapibus twice
		 * (once bia atapibus, once directly), but since the
		 * generic autoconfiguration code maintains the DVF_ACTIVE
		 * flag, it's safe.
		 */
		if ((dev = chp->atapibus) != NULL) {
			error = config_deactivate(dev);
			if (error)
				goto out;
		}

		for (i = 0; i < chp->ch_ndrive; i++) {
			if ((dev = chp->ch_drive[i].drv_softc) != NULL) {
				ATADEBUG_PRINT(("atabus_activate: %s: "
				    "deactivating %s\n", sc->sc_dev.dv_xname,
				    dev->dv_xname),
				    DEBUG_DETACH);
				error = config_deactivate(dev);
				if (error)
					goto out;
			}
		}
		break;
	}
 out:
	splx(s);

#ifdef ATADEBUG
	if (dev != NULL && error != 0)
		ATADEBUG_PRINT(("atabus_activate: %s: "
		    "error %d deactivating %s\n", sc->sc_dev.dv_xname,
		    error, dev->dv_xname), DEBUG_DETACH);
#endif /* ATADEBUG */

	return (error);
}

/*
 * atabus_detach:
 *
 *	Autoconfiguration detach routine.
 */
static int
atabus_detach(struct device *self, int flags)
{
	struct atabus_softc *sc = (void *) self;
	struct ata_channel *chp = sc->sc_chan;
	struct device *dev = NULL;
	int s, i, error = 0;

	/* Shutdown the channel. */
	s = splbio();		/* XXX ALSO NEED AN INTERLOCK HERE. */
	chp->ch_flags |= ATACH_SHUTDOWN;
	splx(s);
	wakeup(&chp->ch_thread);
	while (chp->ch_thread != NULL)
		(void) tsleep(&chp->ch_flags, PRIBIO, "atadown", 0);

	/*
	 * Detach atapibus and its children.
	 */
	if ((dev = chp->atapibus) != NULL) {
		ATADEBUG_PRINT(("atabus_detach: %s: detaching %s\n",
		    sc->sc_dev.dv_xname, dev->dv_xname), DEBUG_DETACH);
		error = config_detach(dev, flags);
		if (error)
			goto out;
	}

	/*
	 * Detach our other children.
	 */
	for (i = 0; i < chp->ch_ndrive; i++) {
		if (chp->ch_drive[i].drive_flags & DRIVE_ATAPI)
			continue;
		if ((dev = chp->ch_drive[i].drv_softc) != NULL) {
			ATADEBUG_PRINT(("atabus_detach: %s: detaching %s\n",
			    sc->sc_dev.dv_xname, dev->dv_xname),
			    DEBUG_DETACH);
			error = config_detach(dev, flags);
			if (error)
				goto out;
		}
	}

 out:
#ifdef ATADEBUG
	if (dev != NULL && error != 0)
		ATADEBUG_PRINT(("atabus_detach: %s: error %d detaching %s\n",
		    sc->sc_dev.dv_xname, error, dev->dv_xname),
		    DEBUG_DETACH);
#endif /* ATADEBUG */

	return (error);
}

CFATTACH_DECL(atabus, sizeof(struct atabus_softc),
    atabus_match, atabus_attach, atabus_detach, atabus_activate);

/*****************************************************************************
 * Common ATA bus operations.
 *****************************************************************************/

/* Get the disk's parameters */
int
ata_get_params(struct ata_drive_datas *drvp, u_int8_t flags,
    struct ataparams *prms)
{
	struct ata_command ata_c;
	struct ata_channel *chp = drvp->chnl_softc;
	struct atac_softc *atac = chp->ch_atac;
	char *tb;
	int i, rv;
	u_int16_t *p;

	ATADEBUG_PRINT(("%s\n", __func__), DEBUG_FUNCS);

	tb = kmem_zalloc(DEV_BSIZE, KM_SLEEP);
	memset(prms, 0, sizeof(struct ataparams));
	memset(&ata_c, 0, sizeof(struct ata_command));

	if (drvp->drive_flags & DRIVE_ATA) {
		ata_c.r_command = WDCC_IDENTIFY;
		ata_c.r_st_bmask = WDCS_DRDY;
		ata_c.r_st_pmask = WDCS_DRQ;
		ata_c.timeout = 3000; /* 3s */
	} else if (drvp->drive_flags & DRIVE_ATAPI) {
		ata_c.r_command = ATAPI_IDENTIFY_DEVICE;
		ata_c.r_st_bmask = 0;
		ata_c.r_st_pmask = WDCS_DRQ;
		ata_c.timeout = 10000; /* 10s */
	} else {
		ATADEBUG_PRINT(("ata_get_parms: no disks\n"),
		    DEBUG_FUNCS|DEBUG_PROBE);
		rv = CMD_ERR;
		goto out;
	}
	ata_c.flags = AT_READ | flags;
	ata_c.data = tb;
	ata_c.bcount = DEV_BSIZE;
	if ((*atac->atac_bustype_ata->ata_exec_command)(drvp,
						&ata_c) != ATACMD_COMPLETE) {
		ATADEBUG_PRINT(("ata_get_parms: wdc_exec_command failed\n"),
		    DEBUG_FUNCS|DEBUG_PROBE);
		rv = CMD_AGAIN;
		goto out;
	}
	if (ata_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
		ATADEBUG_PRINT(("ata_get_parms: ata_c.flags=0x%x\n",
		    ata_c.flags), DEBUG_FUNCS|DEBUG_PROBE);
		rv = CMD_ERR;
		goto out;
	}
	/* if we didn't read any data something is wrong */
	if ((ata_c.flags & AT_XFDONE) == 0) {
		rv = CMD_ERR;
		goto out;
	}

	/* Read in parameter block. */
	memcpy(prms, tb, sizeof(struct ataparams));

	/*
	 * Shuffle string byte order.
	 * ATAPI NEC, Mitsumi and Pioneer drives and
	 * old ATA TDK CompactFlash cards
	 * have different byte order.
	 */
#if BYTE_ORDER == BIG_ENDIAN
# define M(n)	prms->atap_model[(n) ^ 1]
#else
# define M(n)	prms->atap_model[n]
#endif
	if (
#if BYTE_ORDER == BIG_ENDIAN
	    !
#endif
	    ((drvp->drive_flags & DRIVE_ATAPI) ?
	     ((M(0) == 'N' && M(1) == 'E') ||
	      (M(0) == 'F' && M(1) == 'X') ||
	      (M(0) == 'P' && M(1) == 'i')) :
	     ((M(0) == 'T' && M(1) == 'D' && M(2) == 'K')))) {
		rv = CMD_OK;
		goto out;
	     }
#undef M
	for (i = 0; i < sizeof(prms->atap_model); i += 2) {
		p = (u_int16_t *)(prms->atap_model + i);
		*p = bswap16(*p);
	}
	for (i = 0; i < sizeof(prms->atap_serial); i += 2) {
		p = (u_int16_t *)(prms->atap_serial + i);
		*p = bswap16(*p);
	}
	for (i = 0; i < sizeof(prms->atap_revision); i += 2) {
		p = (u_int16_t *)(prms->atap_revision + i);
		*p = bswap16(*p);
	}

	rv = CMD_OK;
 out:
	kmem_free(tb, DEV_BSIZE);
	return rv;
}

int
ata_set_mode(struct ata_drive_datas *drvp, u_int8_t mode, u_int8_t flags)
{
	struct ata_command ata_c;
	struct ata_channel *chp = drvp->chnl_softc;
	struct atac_softc *atac = chp->ch_atac;

	ATADEBUG_PRINT(("ata_set_mode=0x%x\n", mode), DEBUG_FUNCS);
	memset(&ata_c, 0, sizeof(struct ata_command));

	ata_c.r_command = SET_FEATURES;
	ata_c.r_st_bmask = 0;
	ata_c.r_st_pmask = 0;
	ata_c.r_features = WDSF_SET_MODE;
	ata_c.r_count = mode;
	ata_c.flags = flags;
	ata_c.timeout = 1000; /* 1s */
	if ((*atac->atac_bustype_ata->ata_exec_command)(drvp,
						&ata_c) != ATACMD_COMPLETE)
		return CMD_AGAIN;
	if (ata_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
		return CMD_ERR;
	}
	return CMD_OK;
}

#if NATA_DMA
void
ata_dmaerr(struct ata_drive_datas *drvp, int flags)
{
	/*
	 * Downgrade decision: if we get NERRS_MAX in NXFER.
	 * We start with n_dmaerrs set to NERRS_MAX-1 so that the
	 * first error within the first NXFER ops will immediatly trigger
	 * a downgrade.
	 * If we got an error and n_xfers is bigger than NXFER reset counters.
	 */
	drvp->n_dmaerrs++;
	if (drvp->n_dmaerrs >= NERRS_MAX && drvp->n_xfers <= NXFER) {
		ata_downgrade_mode(drvp, flags);
		drvp->n_dmaerrs = NERRS_MAX-1;
		drvp->n_xfers = 0;
		return;
	}
	if (drvp->n_xfers > NXFER) {
		drvp->n_dmaerrs = 1; /* just got an error */
		drvp->n_xfers = 1; /* restart counting from this error */
	}
}
#endif	/* NATA_DMA */

/*
 * freeze the queue and wait for the controller to be idle. Caller has to
 * unfreeze/restart the queue
 */
void
ata_queue_idle(struct ata_queue *queue)
{
	int s = splbio();
	queue->queue_freeze++;
	while (queue->active_xfer != NULL) {
		queue->queue_flags |= QF_IDLE_WAIT;
		tsleep(&queue->queue_flags, PRIBIO, "qidl", 0);
	}
	splx(s);
}

/*
 * Add a command to the queue and start controller.
 *
 * MUST BE CALLED AT splbio()!
 */
void
ata_exec_xfer(struct ata_channel *chp, struct ata_xfer *xfer)
{

	ATADEBUG_PRINT(("ata_exec_xfer %p channel %d drive %d\n", xfer,
	    chp->ch_channel, xfer->c_drive), DEBUG_XFERS);

	/* complete xfer setup */
	xfer->c_chp = chp;

	/* insert at the end of command list */
	TAILQ_INSERT_TAIL(&chp->ch_queue->queue_xfer, xfer, c_xferchain);
	ATADEBUG_PRINT(("atastart from ata_exec_xfer, flags 0x%x\n",
	    chp->ch_flags), DEBUG_XFERS);
	/*
	 * if polling and can sleep, wait for the xfer to be at head of queue
	 */
	if ((xfer->c_flags & (C_POLL | C_WAIT)) ==  (C_POLL | C_WAIT)) {
		while (chp->ch_queue->active_xfer != NULL ||
		    TAILQ_FIRST(&chp->ch_queue->queue_xfer) != xfer) {
			xfer->c_flags |= C_WAITACT;
			tsleep(xfer, PRIBIO, "ataact", 0);
			xfer->c_flags &= ~C_WAITACT;
			if (xfer->c_flags & C_FREE) {
				ata_free_xfer(chp, xfer);
				return;
			}
		}
	}
	atastart(chp);
}

/*
 * Start I/O on a controller, for the given channel.
 * The first xfer may be not for our channel if the channel queues
 * are shared.
 *
 * MUST BE CALLED AT splbio()!
 */
void
atastart(struct ata_channel *chp)
{
	struct atac_softc *atac = chp->ch_atac;
	struct ata_xfer *xfer;

#ifdef ATA_DEBUG
	int spl1, spl2;

	spl1 = splbio();
	spl2 = splbio();
	if (spl2 != spl1) {
		printf("atastart: not at splbio()\n");
		panic("atastart");
	}
	splx(spl2);
	splx(spl1);
#endif /* ATA_DEBUG */

	/* is there a xfer ? */
	if ((xfer = TAILQ_FIRST(&chp->ch_queue->queue_xfer)) == NULL)
		return;

	/* adjust chp, in case we have a shared queue */
	chp = xfer->c_chp;

	if (chp->ch_queue->active_xfer != NULL) {
		return; /* channel aleady active */
	}
	if (__predict_false(chp->ch_queue->queue_freeze > 0)) {
		if (chp->ch_queue->queue_flags & QF_IDLE_WAIT) {
			chp->ch_queue->queue_flags &= ~QF_IDLE_WAIT;
			wakeup(&chp->ch_queue->queue_flags);
		}
		return; /* queue frozen */
	}
	/*
	 * if someone is waiting for the command to be active, wake it up
	 * and let it process the command
	 */
	if (xfer->c_flags & C_WAITACT) {
		ATADEBUG_PRINT(("atastart: xfer %p channel %d drive %d "
		    "wait active\n", xfer, chp->ch_channel, xfer->c_drive),
		    DEBUG_XFERS);
		wakeup(xfer);
		return;
	}
#ifdef DIAGNOSTIC
	if ((chp->ch_flags & ATACH_IRQ_WAIT) != 0)
		panic("atastart: channel waiting for irq");
#endif
	if (atac->atac_claim_hw)
		if (!(*atac->atac_claim_hw)(chp, 0))
			return;

	ATADEBUG_PRINT(("atastart: xfer %p channel %d drive %d\n", xfer,
	    chp->ch_channel, xfer->c_drive), DEBUG_XFERS);
	if (chp->ch_drive[xfer->c_drive].drive_flags & DRIVE_RESET) {
		chp->ch_drive[xfer->c_drive].drive_flags &= ~DRIVE_RESET;
		chp->ch_drive[xfer->c_drive].state = 0;
	}
	chp->ch_queue->active_xfer = xfer;
	TAILQ_REMOVE(&chp->ch_queue->queue_xfer, xfer, c_xferchain);

	if (atac->atac_cap & ATAC_CAP_NOIRQ)
		KASSERT(xfer->c_flags & C_POLL);

	xfer->c_start(chp, xfer);
}

struct ata_xfer *
ata_get_xfer(int flags)
{
	struct ata_xfer *xfer;
	int s;

	s = splbio();
	xfer = pool_get(&ata_xfer_pool,
	    ((flags & ATAXF_NOSLEEP) != 0 ? PR_NOWAIT : PR_WAITOK));
	splx(s);
	if (xfer != NULL) {
		memset(xfer, 0, sizeof(struct ata_xfer));
	}
	return xfer;
}

void
ata_free_xfer(struct ata_channel *chp, struct ata_xfer *xfer)
{
	struct atac_softc *atac = chp->ch_atac;
	int s;

	if (xfer->c_flags & C_WAITACT) {
		/* Someone is waiting for this xfer, so we can't free now */
		xfer->c_flags |= C_FREE;
		wakeup(xfer);
		return;
	}

#if NATA_PIOBM		/* XXX wdc dependent code */
	if (xfer->c_flags & C_PIOBM) {
		struct wdc_softc *wdc = CHAN_TO_WDC(chp);

		/* finish the busmastering PIO */
		(*wdc->piobm_done)(wdc->dma_arg,
		    chp->ch_channel, xfer->c_drive);
		chp->ch_flags &= ~(ATACH_DMA_WAIT | ATACH_PIOBM_WAIT | ATACH_IRQ_WAIT);
	}
#endif

	if (atac->atac_free_hw)
		(*atac->atac_free_hw)(chp);
	s = splbio();
	pool_put(&ata_xfer_pool, xfer);
	splx(s);
}

/*
 * Kill off all pending xfers for a ata_channel.
 *
 * Must be called at splbio().
 */
void
ata_kill_pending(struct ata_drive_datas *drvp)
{
	struct ata_channel *chp = drvp->chnl_softc;
	struct ata_xfer *xfer, *next_xfer;
	int s = splbio();

	for (xfer = TAILQ_FIRST(&chp->ch_queue->queue_xfer);
	    xfer != NULL; xfer = next_xfer) {
		next_xfer = TAILQ_NEXT(xfer, c_xferchain);
		if (xfer->c_chp != chp || xfer->c_drive != drvp->drive)
			continue;
		TAILQ_REMOVE(&chp->ch_queue->queue_xfer, xfer, c_xferchain);
		(*xfer->c_kill_xfer)(chp, xfer, KILL_GONE);
	}

	while ((xfer = chp->ch_queue->active_xfer) != NULL) {
		if (xfer->c_chp == chp && xfer->c_drive == drvp->drive) {
			drvp->drive_flags |= DRIVE_WAITDRAIN;
			(void) tsleep(&chp->ch_queue->active_xfer,
			    PRIBIO, "atdrn", 0);
		} else {
			/* no more xfer for us */
			break;
		}
	}
	splx(s);
}

/*
 * ata_reset_channel:
 *
 *	Reset and ATA channel.
 *
 *	MUST BE CALLED AT splbio()!
 */
void
ata_reset_channel(struct ata_channel *chp, int flags)
{
	struct atac_softc *atac = chp->ch_atac;
	int drive;

#ifdef ATA_DEBUG
	int spl1, spl2;

	spl1 = splbio();
	spl2 = splbio();
	if (spl2 != spl1) {
		printf("ata_reset_channel: not at splbio()\n");
		panic("ata_reset_channel");
	}
	splx(spl2);
	splx(spl1);
#endif /* ATA_DEBUG */

	chp->ch_queue->queue_freeze++;

	/*
	 * If we can poll or wait it's OK, otherwise wake up the
	 * kernel thread to do it for us.
	 */
	if ((flags & (AT_POLL | AT_WAIT)) == 0) {
		if (chp->ch_flags & ATACH_TH_RESET) {
			/* No need to schedule a reset more than one time. */
			chp->ch_queue->queue_freeze--;
			return;
		}
		chp->ch_flags |= ATACH_TH_RESET;
		chp->ch_reset_flags = flags & (AT_RST_EMERG | AT_RST_NOCMD);
		wakeup(&chp->ch_thread);
		return;
	}

	(*atac->atac_bustype_ata->ata_reset_channel)(chp, flags);

	for (drive = 0; drive < chp->ch_ndrive; drive++)
		chp->ch_drive[drive].state = 0;

	chp->ch_flags &= ~ATACH_TH_RESET;
	if ((flags & AT_RST_EMERG) == 0)  {
		chp->ch_queue->queue_freeze--;
		atastart(chp);
	} else {
		/* make sure that we can use polled commands */
		TAILQ_INIT(&chp->ch_queue->queue_xfer);
		chp->ch_queue->queue_freeze = 0;
		chp->ch_queue->active_xfer = NULL;
	}
}

int
ata_addref(struct ata_channel *chp)
{
	struct atac_softc *atac = chp->ch_atac;
	struct scsipi_adapter *adapt = &atac->atac_atapi_adapter._generic;
	int s, error = 0;

	s = splbio();
	if (adapt->adapt_refcnt++ == 0 &&
	    adapt->adapt_enable != NULL) {
		error = (*adapt->adapt_enable)(&atac->atac_dev, 1);
		if (error)
			adapt->adapt_refcnt--;
	}
	splx(s);
	return (error);
}

void
ata_delref(struct ata_channel *chp)
{
	struct atac_softc *atac = chp->ch_atac;
	struct scsipi_adapter *adapt = &atac->atac_atapi_adapter._generic;
	int s;

	s = splbio();
	if (adapt->adapt_refcnt-- == 1 &&
	    adapt->adapt_enable != NULL)
		(void) (*adapt->adapt_enable)(&atac->atac_dev, 0);
	splx(s);
}

void
ata_print_modes(struct ata_channel *chp)
{
	struct atac_softc *atac = chp->ch_atac;
	int drive;
	struct ata_drive_datas *drvp;

	for (drive = 0; drive < chp->ch_ndrive; drive++) {
		drvp = &chp->ch_drive[drive];
		if ((drvp->drive_flags & DRIVE) == 0 || drvp->drv_softc == NULL)
			continue;
		aprint_verbose("%s(%s:%d:%d): using PIO mode %d",
			drvp->drv_softc->dv_xname,
			atac->atac_dev.dv_xname,
			chp->ch_channel, drvp->drive, drvp->PIO_mode);
#if NATA_DMA
		if (drvp->drive_flags & DRIVE_DMA)
			aprint_verbose(", DMA mode %d", drvp->DMA_mode);
#if NATA_UDMA
		if (drvp->drive_flags & DRIVE_UDMA) {
			aprint_verbose(", Ultra-DMA mode %d", drvp->UDMA_mode);
			if (drvp->UDMA_mode == 2)
				aprint_verbose(" (Ultra/33)");
			else if (drvp->UDMA_mode == 4)
				aprint_verbose(" (Ultra/66)");
			else if (drvp->UDMA_mode == 5)
				aprint_verbose(" (Ultra/100)");
			else if (drvp->UDMA_mode == 6)
				aprint_verbose(" (Ultra/133)");
		}
#endif	/* NATA_UDMA */
#endif	/* NATA_DMA */
#if NATA_DMA || NATA_PIOBM
		if (0
#if NATA_DMA
		    || (drvp->drive_flags & (DRIVE_DMA | DRIVE_UDMA))
#endif
#if NATA_PIOBM
		    /* PIOBM capable controllers use DMA for PIO commands */
		    || (atac->atac_cap & ATAC_CAP_PIOBM)
#endif
		    )
			aprint_verbose(" (using DMA)");
#endif	/* NATA_DMA || NATA_PIOBM */
		aprint_verbose("\n");
	}
}

#if NATA_DMA
/*
 * downgrade the transfer mode of a drive after an error. return 1 if
 * downgrade was possible, 0 otherwise.
 *
 * MUST BE CALLED AT splbio()!
 */
int
ata_downgrade_mode(struct ata_drive_datas *drvp, int flags)
{
	struct ata_channel *chp = drvp->chnl_softc;
	struct atac_softc *atac = chp->ch_atac;
	struct device *drv_dev = drvp->drv_softc;
	int cf_flags = device_cfdata(drv_dev)->cf_flags;

	/* if drive or controller don't know its mode, we can't do much */
	if ((drvp->drive_flags & DRIVE_MODE) == 0 ||
	    (atac->atac_set_modes == NULL))
		return 0;
	/* current drive mode was set by a config flag, let it this way */
	if ((cf_flags & ATA_CONFIG_PIO_SET) ||
	    (cf_flags & ATA_CONFIG_DMA_SET) ||
	    (cf_flags & ATA_CONFIG_UDMA_SET))
		return 0;

#if NATA_UDMA
	/*
	 * If we were using Ultra-DMA mode, downgrade to the next lower mode.
	 */
	if ((drvp->drive_flags & DRIVE_UDMA) && drvp->UDMA_mode >= 2) {
		drvp->UDMA_mode--;
		printf("%s: transfer error, downgrading to Ultra-DMA mode %d\n",
		    drv_dev->dv_xname, drvp->UDMA_mode);
	}
#endif

	/*
	 * If we were using ultra-DMA, don't downgrade to multiword DMA.
	 */
	else if (drvp->drive_flags & (DRIVE_DMA | DRIVE_UDMA)) {
		drvp->drive_flags &= ~(DRIVE_DMA | DRIVE_UDMA);
		drvp->PIO_mode = drvp->PIO_cap;
		printf("%s: transfer error, downgrading to PIO mode %d\n",
		    drv_dev->dv_xname, drvp->PIO_mode);
	} else /* already using PIO, can't downgrade */
		return 0;

	(*atac->atac_set_modes)(chp);
	ata_print_modes(chp);
	/* reset the channel, which will schedule all drives for setup */
	ata_reset_channel(chp, flags | AT_RST_NOCMD);
	return 1;
}
#endif	/* NATA_DMA */

/*
 * Probe drive's capabilities, for use by the controller later
 * Assumes drvp points to an existing drive.
 */
void
ata_probe_caps(struct ata_drive_datas *drvp)
{
	struct ataparams params, params2;
	struct ata_channel *chp = drvp->chnl_softc;
	struct atac_softc *atac = chp->ch_atac;
	struct device *drv_dev = drvp->drv_softc;
	int i, printed, s;
	const char *sep = "";
	int cf_flags;

	if (ata_get_params(drvp, AT_WAIT, &params) != CMD_OK) {
		/* IDENTIFY failed. Can't tell more about the device */
		return;
	}
	if ((atac->atac_cap & (ATAC_CAP_DATA16 | ATAC_CAP_DATA32)) ==
	    (ATAC_CAP_DATA16 | ATAC_CAP_DATA32)) {
		/*
		 * Controller claims 16 and 32 bit transfers.
		 * Re-do an IDENTIFY with 32-bit transfers,
		 * and compare results.
		 */
		s = splbio();
		drvp->drive_flags |= DRIVE_CAP32;
		splx(s);
		ata_get_params(drvp, AT_WAIT, &params2);
		if (memcmp(&params, &params2, sizeof(struct ataparams)) != 0) {
			/* Not good. fall back to 16bits */
			s = splbio();
			drvp->drive_flags &= ~DRIVE_CAP32;
			splx(s);
		} else {
			aprint_verbose("%s: 32-bit data port\n",
			    drv_dev->dv_xname);
		}
	}
#if 0 /* Some ultra-DMA drives claims to only support ATA-3. sigh */
	if (params.atap_ata_major > 0x01 &&
	    params.atap_ata_major != 0xffff) {
		for (i = 14; i > 0; i--) {
			if (params.atap_ata_major & (1 << i)) {
				aprint_verbose("%s: ATA version %d\n",
				    drv_dev->dv_xname, i);
				drvp->ata_vers = i;
				break;
			}
		}
	}
#endif

	/* An ATAPI device is at last PIO mode 3 */
	if (drvp->drive_flags & DRIVE_ATAPI)
		drvp->PIO_mode = 3;

	/*
	 * It's not in the specs, but it seems that some drive
	 * returns 0xffff in atap_extensions when this field is invalid
	 */
	if (params.atap_extensions != 0xffff &&
	    (params.atap_extensions & WDC_EXT_MODES)) {
		printed = 0;
		/*
		 * XXX some drives report something wrong here (they claim to
		 * support PIO mode 8 !). As mode is coded on 3 bits in
		 * SET FEATURE, limit it to 7 (so limit i to 4).
		 * If higher mode than 7 is found, abort.
		 */
		for (i = 7; i >= 0; i--) {
			if ((params.atap_piomode_supp & (1 << i)) == 0)
				continue;
			if (i > 4)
				return;
			/*
			 * See if mode is accepted.
			 * If the controller can't set its PIO mode,
			 * assume the defaults are good, so don't try
			 * to set it
			 */
			if (atac->atac_set_modes)
				/*
				 * It's OK to pool here, it's fast enouth
				 * to not bother waiting for interrupt
				 */
				if (ata_set_mode(drvp, 0x08 | (i + 3),
				   AT_WAIT) != CMD_OK)
					continue;
			if (!printed) {
				aprint_verbose("%s: drive supports PIO mode %d",
				    drv_dev->dv_xname, i + 3);
				sep = ",";
				printed = 1;
			}
			/*
			 * If controller's driver can't set its PIO mode,
			 * get the highter one for the drive.
			 */
			if (atac->atac_set_modes == NULL ||
			    atac->atac_pio_cap >= i + 3) {
				drvp->PIO_mode = i + 3;
				drvp->PIO_cap = i + 3;
				break;
			}
		}
		if (!printed) {
			/*
			 * We didn't find a valid PIO mode.
			 * Assume the values returned for DMA are buggy too
			 */
			return;
		}
		s = splbio();
		drvp->drive_flags |= DRIVE_MODE;
		splx(s);
		printed = 0;
		for (i = 7; i >= 0; i--) {
			if ((params.atap_dmamode_supp & (1 << i)) == 0)
				continue;
#if NATA_DMA
			if ((atac->atac_cap & ATAC_CAP_DMA) &&
			    atac->atac_set_modes != NULL)
				if (ata_set_mode(drvp, 0x20 | i, AT_WAIT)
				    != CMD_OK)
					continue;
#endif
			if (!printed) {
				aprint_verbose("%s DMA mode %d", sep, i);
				sep = ",";
				printed = 1;
			}
#if NATA_DMA
			if (atac->atac_cap & ATAC_CAP_DMA) {
				if (atac->atac_set_modes != NULL &&
				    atac->atac_dma_cap < i)
					continue;
				drvp->DMA_mode = i;
				drvp->DMA_cap = i;
				s = splbio();
				drvp->drive_flags |= DRIVE_DMA;
				splx(s);
			}
#endif
			break;
		}
		if (params.atap_extensions & WDC_EXT_UDMA_MODES) {
			printed = 0;
			for (i = 7; i >= 0; i--) {
				if ((params.atap_udmamode_supp & (1 << i))
				    == 0)
					continue;
#if NATA_UDMA
				if (atac->atac_set_modes != NULL &&
				    (atac->atac_cap & ATAC_CAP_UDMA))
					if (ata_set_mode(drvp, 0x40 | i,
					    AT_WAIT) != CMD_OK)
						continue;
#endif
				if (!printed) {
					aprint_verbose("%s Ultra-DMA mode %d",
					    sep, i);
					if (i == 2)
						aprint_verbose(" (Ultra/33)");
					else if (i == 4)
						aprint_verbose(" (Ultra/66)");
					else if (i == 5)
						aprint_verbose(" (Ultra/100)");
					else if (i == 6)
						aprint_verbose(" (Ultra/133)");
					sep = ",";
					printed = 1;
				}
#if NATA_UDMA
				if (atac->atac_cap & ATAC_CAP_UDMA) {
					if (atac->atac_set_modes != NULL &&
					    atac->atac_udma_cap < i)
						continue;
					drvp->UDMA_mode = i;
					drvp->UDMA_cap = i;
					s = splbio();
					drvp->drive_flags |= DRIVE_UDMA;
					splx(s);
				}
#endif
				break;
			}
		}
		aprint_verbose("\n");
	}

	s = splbio();
	drvp->drive_flags &= ~DRIVE_NOSTREAM;
	if (drvp->drive_flags & DRIVE_ATAPI) {
		if (atac->atac_cap & ATAC_CAP_ATAPI_NOSTREAM)
			drvp->drive_flags |= DRIVE_NOSTREAM;
	} else {
		if (atac->atac_cap & ATAC_CAP_ATA_NOSTREAM)
			drvp->drive_flags |= DRIVE_NOSTREAM;
	}
	splx(s);

	/* Try to guess ATA version here, if it didn't get reported */
	if (drvp->ata_vers == 0) {
#if NATA_UDMA
		if (drvp->drive_flags & DRIVE_UDMA)
			drvp->ata_vers = 4; /* should be at last ATA-4 */
		else
#endif
		if (drvp->PIO_cap > 2)
			drvp->ata_vers = 2; /* should be at last ATA-2 */
	}
	cf_flags = device_cfdata(drv_dev)->cf_flags;
	if (cf_flags & ATA_CONFIG_PIO_SET) {
		s = splbio();
		drvp->PIO_mode =
		    (cf_flags & ATA_CONFIG_PIO_MODES) >> ATA_CONFIG_PIO_OFF;
		drvp->drive_flags |= DRIVE_MODE;
		splx(s);
	}
#if NATA_DMA
	if ((atac->atac_cap & ATAC_CAP_DMA) == 0) {
		/* don't care about DMA modes */
		return;
	}
	if (cf_flags & ATA_CONFIG_DMA_SET) {
		s = splbio();
		if ((cf_flags & ATA_CONFIG_DMA_MODES) ==
		    ATA_CONFIG_DMA_DISABLE) {
			drvp->drive_flags &= ~DRIVE_DMA;
		} else {
			drvp->DMA_mode = (cf_flags & ATA_CONFIG_DMA_MODES) >>
			    ATA_CONFIG_DMA_OFF;
			drvp->drive_flags |= DRIVE_DMA | DRIVE_MODE;
		}
		splx(s);
	}
#if NATA_UDMA
	if ((atac->atac_cap & ATAC_CAP_UDMA) == 0) {
		/* don't care about UDMA modes */
		return;
	}
	if (cf_flags & ATA_CONFIG_UDMA_SET) {
		s = splbio();
		if ((cf_flags & ATA_CONFIG_UDMA_MODES) ==
		    ATA_CONFIG_UDMA_DISABLE) {
			drvp->drive_flags &= ~DRIVE_UDMA;
		} else {
			drvp->UDMA_mode = (cf_flags & ATA_CONFIG_UDMA_MODES) >>
			    ATA_CONFIG_UDMA_OFF;
			drvp->drive_flags |= DRIVE_UDMA | DRIVE_MODE;
		}
		splx(s);
	}
#endif	/* NATA_UDMA */
#endif	/* NATA_DMA */
}

/* management of the /dev/atabus* devices */
int
atabusopen(dev_t dev, int flag, int fmt,
    struct lwp *l)
{
	struct atabus_softc *sc;
	int error, unit = minor(dev);

	if (unit >= atabus_cd.cd_ndevs ||
	    (sc = atabus_cd.cd_devs[unit]) == NULL)
		return (ENXIO);

	if (sc->sc_flags & ATABUSCF_OPEN)
		return (EBUSY);

	if ((error = ata_addref(sc->sc_chan)) != 0)
		return (error);

	sc->sc_flags |= ATABUSCF_OPEN;

	return (0);
}


int
atabusclose(dev_t dev, int flag, int fmt,
    struct lwp *l)
{
	struct atabus_softc *sc = atabus_cd.cd_devs[minor(dev)];

	ata_delref(sc->sc_chan);

	sc->sc_flags &= ~ATABUSCF_OPEN;

	return (0);
}

int
atabusioctl(dev_t dev, u_long cmd, void *addr, int flag,
    struct lwp *l)
{
	struct atabus_softc *sc = atabus_cd.cd_devs[minor(dev)];
	struct ata_channel *chp = sc->sc_chan;
	int min_drive, max_drive, drive;
	int error;
	int s;

	/*
	 * Enforce write permission for ioctls that change the
	 * state of the bus.  Host adapter specific ioctls must
	 * be checked by the adapter driver.
	 */
	switch (cmd) {
	case ATABUSIOSCAN:
	case ATABUSIODETACH:
	case ATABUSIORESET:
		if ((flag & FWRITE) == 0)
			return (EBADF);
	}

	switch (cmd) {
	case ATABUSIORESET:
		s = splbio();
		ata_reset_channel(sc->sc_chan, AT_WAIT | AT_POLL);
		splx(s);
		error = 0;
		break;
	case ATABUSIOSCAN:
	{
#if 0
		struct atabusioscan_args *a=
		    (struct atabusioscan_args *)addr;
#endif
		if ((chp->ch_drive[0].drive_flags & DRIVE_OLD) ||
		    (chp->ch_drive[1].drive_flags & DRIVE_OLD))
			return (EOPNOTSUPP);
		return (EOPNOTSUPP);
	}
	case ATABUSIODETACH:
	{
		struct atabusioscan_args *a=
		    (struct atabusioscan_args *)addr;
		if ((chp->ch_drive[0].drive_flags & DRIVE_OLD) ||
		    (chp->ch_drive[1].drive_flags & DRIVE_OLD))
			return (EOPNOTSUPP);
		switch (a->at_dev) {
		case -1:
			min_drive = 0;
			max_drive = 1;
			break;
		case 0:
		case 1:
			min_drive = max_drive = a->at_dev;
			break;
		default:
			return (EINVAL);
		}
		for (drive = min_drive; drive <= max_drive; drive++) {
			if (chp->ch_drive[drive].drv_softc != NULL) {
				error = config_detach(
				    chp->ch_drive[drive].drv_softc, 0);
				if (error)
					return (error);
				chp->ch_drive[drive].drv_softc = NULL;
			}
		}
		error = 0;
		break;
	}
	default:
		error = ENOTTY;
	}
	return (error);
};

static bool
atabus_suspend(device_t dv)
{
	struct atabus_softc *sc = device_private(dv);
	struct ata_channel *chp = sc->sc_chan;

	ata_queue_idle(chp->ch_queue);

	return true;
}

static bool
atabus_resume(device_t dv)
{
	struct atabus_softc *sc = device_private(dv);
	struct ata_channel *chp = sc->sc_chan;
	int s;

	/*
	 * XXX joerg: with wdc, the first channel unfreezes the controler.
	 * Move this the reset and queue idling into wdc.
	 */
	s = splbio();
	if (chp->ch_queue->queue_freeze == 0) {
		splx(s);
		return true;
	}
	KASSERT(chp->ch_queue->queue_freeze > 0);
	/* unfreeze the queue and reset drives */
	chp->ch_queue->queue_freeze--;
	ata_reset_channel(chp, AT_WAIT);
	splx(s);

	return true;
}
