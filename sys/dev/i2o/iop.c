/*	$NetBSD: iop.c,v 1.10 2001/01/03 21:17:05 ad Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*
 * Support for I2O IOPs (intelligent I/O processors).
 */

#include "opt_i2o.h"
#include "iop.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/endian.h>
#include <sys/pool.h>
#include <sys/conf.h>
#include <sys/kthread.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <dev/i2o/i2o.h>
#include <dev/i2o/iopreg.h>
#include <dev/i2o/iopvar.h>

#define POLL(ms, cond)				\
do {						\
	int i;					\
	for (i = (ms) * 10; i; i--) {		\
		if (cond)			\
			break;			\
		DELAY(100);			\
	}					\
} while (/* CONSTCOND */0);

#ifdef I2ODEBUG
#define DPRINTF(x)	printf x
#else
#define	DPRINTF(x)
#endif

#ifdef I2OVERBOSE
#define IFVERBOSE(x)	x
#else
#define	IFVERBOSE(x)
#endif

#define	COMMENT(x)	""

#define IOP_ICTXHASH_NBUCKETS	16
#define	IOP_ICTXHASH(ictx)	(&iop_ictxhashtbl[(ictx) & iop_ictxhash])
#define IOP_TCTXHASH_NBUCKETS	64
#define	IOP_TCTXHASH(tctx)	(&iop_tctxhashtbl[(tctx) & iop_tctxhash])

static LIST_HEAD(, iop_initiator) *iop_ictxhashtbl;
static u_long	iop_ictxhash;
static TAILQ_HEAD(, iop_msg) *iop_tctxhashtbl;
static u_long	iop_tctxhash;
static void	*iop_sdh;
static struct	pool *iop_msgpool;
static struct	i2o_systab *iop_systab;
static int	iop_systab_size;

extern struct cfdriver iop_cd;

#define	IC_CONFIGURE	0x01

struct iop_class {
	u_short	ic_class;
	u_short	ic_flags;
	const char	*ic_caption;
} static const iop_class[] = {
	{	
		I2O_CLASS_EXECUTIVE,
		0,
		COMMENT("executive")
	},
	{
		I2O_CLASS_DDM,
		0,
		COMMENT("device driver module")
	},
	{
		I2O_CLASS_RANDOM_BLOCK_STORAGE,
		IC_CONFIGURE,
		IFVERBOSE("random block storage")
	},
	{
		I2O_CLASS_SEQUENTIAL_STORAGE,
		IC_CONFIGURE,
		IFVERBOSE("sequential storage")
	},
	{
		I2O_CLASS_LAN,
		IC_CONFIGURE,
		IFVERBOSE("LAN port")
	},
	{
		I2O_CLASS_WAN,
		IC_CONFIGURE,
		IFVERBOSE("WAN port")
	},
	{
		I2O_CLASS_FIBRE_CHANNEL_PORT,
		IC_CONFIGURE,
		IFVERBOSE("fibrechannel port")
	},
	{
		I2O_CLASS_FIBRE_CHANNEL_PERIPHERAL,
		0,
		COMMENT("fibrechannel peripheral")
	},
 	{
 		I2O_CLASS_SCSI_PERIPHERAL,
 		0,
 		COMMENT("SCSI peripheral")
 	},
	{
		I2O_CLASS_ATE_PORT,
		IC_CONFIGURE,
		IFVERBOSE("ATE port")
	},
	{	
		I2O_CLASS_ATE_PERIPHERAL,
		0,
		COMMENT("ATE peripheral")
	},
	{	
		I2O_CLASS_FLOPPY_CONTROLLER,
		IC_CONFIGURE,
		IFVERBOSE("floppy controller")
	},
	{
		I2O_CLASS_FLOPPY_DEVICE,
		0,
		COMMENT("floppy device")
	},
	{
		I2O_CLASS_BUS_ADAPTER_PORT,
		IC_CONFIGURE,
		IFVERBOSE("bus adapter port" )
	},
};

#if defined(I2ODEBUG) && defined(I2OVERBOSE)
static const char *iop_status[] = {
	"success",
	"abort (dirty)",
	"abort (no data transfer)",
	"abort (partial transfer)",
	"error (dirty)",
	"error (no data transfer)",
	"error (partial transfer)",
	"undefined error code",
	"process abort (dirty)",
	"process abort (no data transfer)",
	"process abort (partial transfer)",
	"transaction error",
};
#endif

static inline u_int32_t	iop_inl(struct iop_softc *, int);
static inline void	iop_outl(struct iop_softc *, int, u_int32_t);

static void	iop_config_interrupts(struct device *);
static void	iop_configure_devices(struct iop_softc *);
static void	iop_devinfo(int, char *);
static int	iop_print(void *, const char *);
static int	iop_reconfigure(struct iop_softc *, u_int32_t, int);
static void	iop_shutdown(void *);
static int	iop_submatch(struct device *, struct cfdata *, void *);
#ifdef notyet
static int	iop_vendor_print(void *, const char *);
#endif

static void	iop_create_reconf_thread(void *);
static void	iop_intr_event(struct device *, struct iop_msg *, void *);
static int	iop_hrt_get(struct iop_softc *);
static int	iop_hrt_get0(struct iop_softc *, struct i2o_hrt *, int);
static int	iop_lct_get0(struct iop_softc *, struct i2o_lct *, int,
			     u_int32_t);
static int	iop_msg_wait(struct iop_softc *, struct iop_msg *, int);
static int	iop_ofifo_init(struct iop_softc *);
static int	iop_handle_reply(struct iop_softc *, u_int32_t);
static void	iop_reconf_thread(void *);
static void	iop_release_mfa(struct iop_softc *, u_int32_t);
static int	iop_reset(struct iop_softc *);
static int	iop_status_get(struct iop_softc *);
static int	iop_systab_set(struct iop_softc *);

#ifdef I2ODEBUG
static void	iop_reply_print(struct iop_softc *, struct iop_msg *,
				struct i2o_reply *);
#endif

cdev_decl(iop);

static inline u_int32_t
iop_inl(struct iop_softc *sc, int off)
{

	bus_space_barrier(sc->sc_iot, sc->sc_ioh, off, 4,
	    BUS_SPACE_BARRIER_WRITE | BUS_SPACE_BARRIER_READ);
	return (bus_space_read_4(sc->sc_iot, sc->sc_ioh, off));
}

static inline void
iop_outl(struct iop_softc *sc, int off, u_int32_t val)
{

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, off, val);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, off, 4,
	    BUS_SPACE_BARRIER_WRITE);
}

/*
 * Initialise the adapter.
 */
void
iop_init(struct iop_softc *sc, const char *intrstr)
{
	int rv;
	u_int32_t mask;
	static int again;
	char ident[64];

	if (again == 0) {
		/* Create the shared message wrapper pool and hashes. */
		iop_msgpool = pool_create(sizeof(struct iop_msg), 0, 0, 0,
		    "ioppl", 0, NULL, NULL, M_DEVBUF);
		iop_ictxhashtbl = hashinit(IOP_ICTXHASH_NBUCKETS, HASH_LIST,
		    M_DEVBUF, M_NOWAIT, &iop_ictxhash);
		iop_tctxhashtbl = hashinit(IOP_TCTXHASH_NBUCKETS, HASH_TAILQ,
		    M_DEVBUF, M_NOWAIT, &iop_tctxhash);
		again = 1;
	}

	/* Reset the IOP and request status. */
	printf("I2O adapter");

	if ((rv = iop_reset(sc)) != 0) {
		printf("%s: not responding (reset)\n", sc->sc_dv.dv_xname);
		return;
	}
	if ((rv = iop_status_get(sc)) != 0) {
		printf("%s: not responding (get status)\n", sc->sc_dv.dv_xname);
		return;
	}
	DPRINTF((" (state=%d)",
	    (le32toh(sc->sc_status.segnumber) >> 16) & 0xff));
	sc->sc_flags |= IOP_HAVESTATUS;

	iop_strvis(sc, sc->sc_status.productid, sizeof(sc->sc_status.productid),
	    ident, sizeof(ident));
	printf(" <%s>\n", ident);

#ifdef I2ODEBUG
	printf("%s: orgid=0x%04x version=%d\n", sc->sc_dv.dv_xname,
	    le16toh(sc->sc_status.orgid),
	    (le32toh(sc->sc_status.segnumber) >> 12) & 15);
	printf("%s: type want have cbase\n", sc->sc_dv.dv_xname);
	printf("%s: mem  %04x %04x %08x\n", sc->sc_dv.dv_xname,
	    le32toh(sc->sc_status.desiredprivmemsize),
	    le32toh(sc->sc_status.currentprivmemsize),
	    le32toh(sc->sc_status.currentprivmembase));
	printf("%s: i/o  %04x %04x %08x\n", sc->sc_dv.dv_xname,
	    le32toh(sc->sc_status.desiredpriviosize),
	    le32toh(sc->sc_status.currentpriviosize),
	    le32toh(sc->sc_status.currentpriviobase));
#endif

	sc->sc_maxreplycnt = le32toh(sc->sc_status.maxoutboundmframes);
	if (sc->sc_maxreplycnt > IOP_MAX_HW_REPLYCNT)
		sc->sc_maxreplycnt = IOP_MAX_HW_REPLYCNT;
	sc->sc_maxqueuecnt = le32toh(sc->sc_status.maxinboundmframes);
	if (sc->sc_maxqueuecnt > IOP_MAX_HW_QUEUECNT)
		sc->sc_maxqueuecnt = IOP_MAX_HW_QUEUECNT;

	if (iop_ofifo_init(sc) != 0) {
		printf("%s: unable to init oubound FIFO\n", sc->sc_dv.dv_xname);
		return;
	}

	/*
 	 * Defer further configuration until (a) interrupts are working and
 	 * (b) we have enough information to build the system table.
 	 */
	config_interrupts((struct device *)sc, iop_config_interrupts);

	/* Configure shutdown hook before we start any device activity. */
	if (iop_sdh == NULL)
		iop_sdh = shutdownhook_establish(iop_shutdown, NULL);

	/* Ensure interrupts are enabled at the IOP. */
	mask = iop_inl(sc, IOP_REG_INTR_MASK);
	iop_outl(sc, IOP_REG_INTR_MASK, mask & ~IOP_INTR_OFIFO);

	if (intrstr != NULL)
		printf("%s: interrupting at %s\n", sc->sc_dv.dv_xname,
		    intrstr);

#ifdef I2ODEBUG
	printf("%s: queue depths: inbound %d/%d, outbound %d/%d\n",
	    sc->sc_dv.dv_xname, 
	    sc->sc_maxqueuecnt, le32toh(sc->sc_status.maxinboundmframes),
	    sc->sc_maxreplycnt, le32toh(sc->sc_status.maxoutboundmframes));
#endif

	lockinit(&sc->sc_conflock, PRIBIO, "iopconf", hz * 30, 0);
	SIMPLEQ_INIT(&sc->sc_queue);
}

/*
 * Perform autoconfiguration tasks.
 */
static void
iop_config_interrupts(struct device *self)
{
	struct iop_softc *sc, *iop;
	struct i2o_systab_entry *ste;
	int rv, i, niop;

	sc = (struct iop_softc *)self;
	LIST_INIT(&sc->sc_iilist);

	printf("%s: configuring...\n", sc->sc_dv.dv_xname);

	if (iop_hrt_get(sc) != 0) {
		printf("%s: unable to retrieve HRT\n", sc->sc_dv.dv_xname);
		return;
	}

	/*
 	 * Build the system table.
 	 */
	if (iop_systab == NULL) {
		for (i = 0, niop = 0; i < iop_cd.cd_ndevs; i++) {
			if ((iop = device_lookup(&iop_cd, i)) == NULL)
				continue;
			if ((iop->sc_flags & IOP_HAVESTATUS) == 0)
				continue;
			if (iop_status_get(iop) != 0) {
				printf("%s: unable to retrieve status\n",
				    sc->sc_dv.dv_xname);
				iop->sc_flags &= ~IOP_HAVESTATUS;
				continue;
			}
			niop++;
		}
		if (niop == 0)
			return;

		i = sizeof(struct i2o_systab_entry) * (niop - 1) +
		    sizeof(struct i2o_systab);
		iop_systab_size = i;
		iop_systab = malloc(i, M_DEVBUF, M_NOWAIT);

		memset(iop_systab, 0, i);
		iop_systab->numentries = niop;
		iop_systab->version = I2O_VERSION_11;

		for (i = 0, ste = iop_systab->entry; i < iop_cd.cd_ndevs; i++) {
			if ((iop = device_lookup(&iop_cd, i)) == NULL)
				continue;
			if ((iop->sc_flags & IOP_HAVESTATUS) == 0)
				continue;

			ste->orgid = iop->sc_status.orgid;
			ste->iopid = iop->sc_dv.dv_unit + 2;
			ste->segnumber =
			    htole32(le32toh(iop->sc_status.segnumber) & ~4095);
			ste->iopcaps = iop->sc_status.iopcaps;
			ste->inboundmsgframesize =
			    iop->sc_status.inboundmframesize;
			ste->inboundmsgportaddresslow =
			    htole32(iop->sc_memaddr + IOP_REG_IFIFO);
			ste++;
		}
	}

	if (iop_systab_set(sc) != 0) {
		printf("%s: unable to set system table\n", sc->sc_dv.dv_xname);
		return;
	}
	if (iop_simple_cmd(sc, I2O_TID_IOP, I2O_EXEC_SYS_ENABLE, IOP_ICTX, 1,
	    5000) != 0) {
		printf("%s: unable to enable system\n", sc->sc_dv.dv_xname);
		return;
	}

	/*
	 * Set up an event handler for this IOP.
	 */
	sc->sc_eventii.ii_dv = self;
	sc->sc_eventii.ii_intr = iop_intr_event;
	sc->sc_eventii.ii_flags = II_DISCARD | II_UTILITY;
	sc->sc_eventii.ii_tid = I2O_TID_IOP;
	if (iop_initiator_register(sc, &sc->sc_eventii) != 0) {
		printf("%s: unable to register initiator", sc->sc_dv.dv_xname);
		return;
	}
	if (iop_util_eventreg(sc, &sc->sc_eventii, 0xffffffff)) {
		printf("%s: unable to register for events", sc->sc_dv.dv_xname);
		return;
	}

#ifdef notyet
	/* Attempt to match and attach a product-specific extension. */
	ia.ia_class = I2O_CLASS_ANY;
	ia.ia_tid = I2O_TID_IOP;
	config_found_sm(self, &ia, iop_vendor_print, iop_submatch);
#endif

	if ((rv = iop_reconfigure(sc, 0, 0)) != 0) {
		printf("%s: configure failed (%d)\n", sc->sc_dv.dv_xname, rv);
		return;
	}

	kthread_create(iop_create_reconf_thread, sc);
}

/*
 * Create the reconfiguration thread.  Called after the standard kernel
 * threads have been created.
 */
static void
iop_create_reconf_thread(void *cookie)
{
	struct iop_softc *sc;
	int rv;

	sc = cookie;

	sc->sc_flags |= IOP_ONLINE;
	rv = kthread_create1(iop_reconf_thread, sc, &sc->sc_reconf_proc,
	    "%s", sc->sc_dv.dv_xname);
	if (rv != 0) {
		printf("%s: unable to create reconfiguration thread (%d)",
		    sc->sc_dv.dv_xname, rv);
		return;
	}
}

/*
 * Reconfiguration thread; listens for LCT change notification, and
 * initiates re-configuration if recieved.
 */
static void
iop_reconf_thread(void *cookie)
{
	struct iop_softc *sc;
	struct i2o_lct lct;
	u_int32_t chgind;

	sc = cookie;

	for (;;) {
		chgind = le32toh(sc->sc_chgindicator) + 1;

		if (iop_lct_get0(sc, &lct, sizeof(lct), chgind) == 0) {
			DPRINTF(("%s: async reconfiguration (0x%08x)\n",
			    sc->sc_dv.dv_xname, le32toh(lct.changeindicator)));
			iop_reconfigure(sc, lct.changeindicator, LK_NOWAIT);
		}

		tsleep(iop_reconf_thread, PWAIT, "iopzzz", hz * 5);
	}
}

/*
 * Reconfigure: find new and removed devices.
 */
static int
iop_reconfigure(struct iop_softc *sc, u_int32_t chgind, int lkflags)
{
	struct iop_msg *im;
	struct i2o_hba_bus_scan *mb;
	struct i2o_lct_entry *le;
	struct iop_initiator *ii, *nextii;
	int rv, tid, i;

	lkflags |= LK_EXCLUSIVE | LK_RECURSEFAIL;
	if ((rv = lockmgr(&sc->sc_conflock, lkflags, NULL)) != 0) {
		DPRINTF(("iop_reconfigure: unable to acquire lock\n"));
		return (rv);
	}

	/*
	 * If the reconfiguration request isn't the result of LCT change
	 * notification, then be more thorough: ask all bus ports to scan
	 * their busses.  Wait up to 5 minutes for each bus port to complete
	 * the request.
	 */
	if (chgind == 0) {
		if ((rv = iop_lct_get(sc)) != 0) {
			DPRINTF(("iop_reconfigure: unable to read LCT\n"));
			goto done;
		}

		le = sc->sc_lct->entry;
		for (i = 0; i < sc->sc_nlctent; i++, le++) {
			if ((le16toh(le->classid) & 4095) !=
			    I2O_CLASS_BUS_ADAPTER_PORT)
				continue;
			tid = le32toh(le->localtid) & 4095;

			rv = iop_msg_alloc(sc, NULL, &im, IM_NOINTR);
			if (rv != 0) {
				DPRINTF(("iop_reconfigure: alloc msg\n"));
				goto done;
			}

			mb = (struct i2o_hba_bus_scan *)im->im_msg;
			mb->msgflags = I2O_MSGFLAGS(i2o_hba_bus_scan);
			mb->msgfunc = I2O_MSGFUNC(tid, I2O_HBA_BUS_SCAN);
			mb->msgictx = IOP_ICTX;
			mb->msgtctx = im->im_tctx;

			DPRINTF(("%s: scanning bus %d\n", sc->sc_dv.dv_xname,
			    tid));

			rv = iop_msg_enqueue(sc, im, 5*60*1000);
			iop_msg_free(sc, NULL, im);
			if (rv != 0) {
				DPRINTF(("iop_reconfigure: scan failed\n"));
				goto done;
			}
		}
	} else if (chgind == sc->sc_chgindicator) {
		DPRINTF(("%s: LCT unchanged (async)\n", sc->sc_dv.dv_xname));
		goto done;
	}

	/* Re-read the LCT and determine if it has changed. */
	if ((rv = iop_lct_get(sc)) != 0) {
		DPRINTF(("iop_reconfigure: unable to re-read LCT\n"));
		goto done;
	}
	DPRINTF(("%s: %d LCT entries\n", sc->sc_dv.dv_xname, sc->sc_nlctent));

	if (sc->sc_lct->changeindicator == sc->sc_chgindicator) {
		DPRINTF(("%s: LCT unchanged\n", sc->sc_dv.dv_xname));
		/* Nothing to do. */
		rv = 0;
		goto done;
	}
	DPRINTF(("%s: LCT changed\n", sc->sc_dv.dv_xname));
	sc->sc_chgindicator = sc->sc_lct->changeindicator;

	if (sc->sc_tidmap != NULL)
		free(sc->sc_tidmap, M_DEVBUF);
	sc->sc_tidmap = malloc(sc->sc_nlctent * sizeof(struct iop_tidmap),
	    M_DEVBUF, M_NOWAIT);

	/* Match and attach child devices. */
	iop_configure_devices(sc);

	for (ii = LIST_FIRST(&sc->sc_iilist); ii != NULL; ii = nextii) {
		nextii = ii;
		do {
			if ((nextii = LIST_NEXT(nextii, ii_list)) == NULL)
				break;
		} while ((nextii->ii_flags & II_UTILITY) != 0);
		if ((ii->ii_flags & II_UTILITY) != 0)
			continue;

		/* Detach devices that were configured, but are now gone. */
		for (i = 0; i < sc->sc_nlctent; i++)
			if (ii->ii_tid == sc->sc_tidmap[i].it_tid)
				break;
		if (i == sc->sc_nlctent ||
		    (sc->sc_tidmap[i].it_flags & IT_CONFIGURED) == 0)
			config_detach(ii->ii_dv, DETACH_FORCE);

		/*
		 * Tell initiators that existed before the re-configuration
		 * to re-configure.
		 */
		if (ii->ii_reconfig == NULL)
			continue;
		if ((rv = (*ii->ii_reconfig)(ii->ii_dv)) != 0)
			printf("%s: %s failed reconfigure (%d)\n",
			    sc->sc_dv.dv_xname, ii->ii_dv->dv_xname, rv);
	}
	rv = 0;

done:
	lockmgr(&sc->sc_conflock, LK_RELEASE, NULL);
	return (rv);
}

/*
 * Configure I2O devices into the system.
 */
static void
iop_configure_devices(struct iop_softc *sc)
{
	struct iop_attach_args ia;
	struct iop_initiator *ii;
	const struct i2o_lct_entry *le;
	struct device *dv;
	int i, j, nent;

	nent = sc->sc_nlctent;
	for (i = 0, le = sc->sc_lct->entry; i < nent; i++, le++) {
		sc->sc_tidmap[i].it_tid = le32toh(le->localtid) & 4095;
		sc->sc_tidmap[i].it_flags = 0;
		sc->sc_tidmap[i].it_dvname[0] = '\0';

		/*
		 * Ignore the device if it's in use.
		 */
		if ((le32toh(le->usertid) & 4095) != 4095)
			continue;

		ia.ia_class = le16toh(le->classid) & 4095;
		ia.ia_tid = sc->sc_tidmap[i].it_tid;

		/* Ignore uninteresting devices. */
		for (j = 0; j < sizeof(iop_class) / sizeof(iop_class[0]); j++)
			if (iop_class[j].ic_class == ia.ia_class)
				break;
		if (j < sizeof(iop_class) / sizeof(iop_class[0]) &&
		    (iop_class[j].ic_flags & IC_CONFIGURE) == 0)
			continue;

		/*
		 * Try to configure the device only if it's not already
		 * configured.
 		 */
 		LIST_FOREACH(ii, &sc->sc_iilist, ii_list) {
 			if ((ii->ii_flags & II_UTILITY) != 0)
 				continue;
 			if (ia.ia_tid == ii->ii_tid) {
				sc->sc_tidmap[i].it_flags |= IT_CONFIGURED;
				strcpy(sc->sc_tidmap[i].it_dvname,
				    ii->ii_dv->dv_xname);
				break;
			}
		}
		if (ii != NULL)
			continue;

		dv = config_found_sm(&sc->sc_dv, &ia, iop_print, iop_submatch);
		if (dv != NULL) {
			sc->sc_tidmap[i].it_flags |= IT_CONFIGURED;
			strcpy(sc->sc_tidmap[i].it_dvname, dv->dv_xname);
		}
	}
}

static void
iop_devinfo(int class, char *devinfo)
{
#ifdef I2OVERBOSE
	int i;

	for (i = 0; i < sizeof(iop_class) / sizeof(iop_class[0]); i++)
		if (class == iop_class[i].ic_class)
			break;
	
	if (i == sizeof(iop_class) / sizeof(iop_class[0]))
		sprintf(devinfo, "device (class 0x%x)", class);
	else
		strcpy(devinfo, iop_class[i].ic_caption);
#else

	sprintf(devinfo, "device (class 0x%x)", class);
#endif
}

static int
iop_print(void *aux, const char *pnp)
{
	struct iop_attach_args *ia;
	char devinfo[256];

	ia = aux;

	if (pnp != NULL) {
		iop_devinfo(ia->ia_class, devinfo);
		printf("%s at %s", devinfo, pnp);
	}
	printf(" tid %d", ia->ia_tid);
	return (UNCONF);
}

#ifdef notyet
static int
iop_vendor_print(void *aux, const char *pnp)
{

	if (pnp != NULL)
		printf("vendor specific extension at %s", pnp);
	return (UNCONF);
}
#endif

static int
iop_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct iop_attach_args *ia;
	
	ia = aux;

	if (cf->iopcf_tid != IOPCF_TID_DEFAULT && cf->iopcf_tid != ia->ia_tid)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

/*
 * Shut down all configured IOPs.
 */ 
static void
iop_shutdown(void *junk)
{
	struct iop_softc *sc;
	int i;

	printf("shutting down iop devices... ");

	for (i = 0; i < iop_cd.cd_ndevs; i++) {
		if ((sc = device_lookup(&iop_cd, i)) == NULL)
			continue;
		if ((sc->sc_flags & IOP_ONLINE) == 0)
			continue;
		iop_simple_cmd(sc, I2O_TID_IOP, I2O_EXEC_SYS_QUIESCE, IOP_ICTX,
		    0, 5000);
		iop_simple_cmd(sc, I2O_TID_IOP, I2O_EXEC_IOP_CLEAR, IOP_ICTX,
		    0, 5000);
	}

	/* Wait.  Some boards could still be flushing, stupidly enough. */
	delay(5000*1000);
	printf(" done\n");
}

/*
 * Retrieve adapter status.
 */
static int
iop_status_get(struct iop_softc *sc)
{
	struct iop_msg *im;
	struct i2o_exec_status_get *mb;
	int rv, s;

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOWAIT | IM_NOICTX)) != 0)
		return (rv);

	mb = (struct i2o_exec_status_get *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_exec_status_get);
	mb->msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_EXEC_STATUS_GET);
	mb->reserved[0] = 0;
	mb->reserved[1] = 0;
	mb->reserved[2] = 0;
	mb->reserved[3] = 0;
	mb->addrlow = kvtop((caddr_t)&sc->sc_status);	/* XXX */
	mb->addrhigh = 0;
	mb->length = sizeof(sc->sc_status);

	s = splbio();
	memset(&sc->sc_status, 0, sizeof(sc->sc_status));

	if ((rv = iop_msg_send(sc, im, 0)) != 0) {
		splx(s);
		iop_msg_free(sc, NULL, im);
		return (rv);
	}

	/* XXX */
	POLL(2500, *((volatile u_char *)&sc->sc_status.syncbyte) == 0xff);

	splx(s);
	iop_msg_free(sc, NULL, im);
	return (*((volatile u_char *)&sc->sc_status.syncbyte) != 0xff);
}

/*
 * Initalize and populate the adapter's outbound FIFO.
 */
static int
iop_ofifo_init(struct iop_softc *sc)
{
	struct iop_msg *im;
	volatile u_int32_t status;
	bus_addr_t addr;
	bus_dma_segment_t seg;
	struct i2o_exec_outbound_init *mb;
	int i, rseg, rv;

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOWAIT | IM_NOICTX)) != 0)
		return (rv);

	mb = (struct i2o_exec_outbound_init *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_exec_outbound_init);
	mb->msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_EXEC_OUTBOUND_INIT);
	mb->msgictx = IOP_ICTX;
	mb->msgtctx = im->im_tctx;
	mb->pagesize = PAGE_SIZE;
	mb->flags = 0x80 | ((IOP_MAX_REPLY_SIZE >> 2) << 16);	/* XXX */

	status = 0;

	/*
	 * The I2O spec says that there are two SGLs: one for the status
	 * word, and one for a list of discarded MFAs.  It continues to say
	 * that if you don't want to get the list of MFAs, an IGNORE SGL is
	 * necessary; this isn't the case (and in fact appears to be a bad
	 * thing).
	 */
	iop_msg_map(sc, im, (void *)&status, sizeof(status), 0);
	if ((rv = iop_msg_send(sc, im, 0)) != 0) {
		iop_msg_free(sc, NULL, im);
		return (rv);
	}
	iop_msg_unmap(sc, im);
	iop_msg_free(sc, NULL, im);

	/* XXX */
	POLL(5000, status == I2O_EXEC_OUTBOUND_INIT_COMPLETE);
	if (status != I2O_EXEC_OUTBOUND_INIT_COMPLETE) {
		printf("%s: outbound FIFO init failed\n", sc->sc_dv.dv_xname);
		return (EIO);
	}

	/* If we need to allocate DMA safe memory, do it now. */
	if (sc->sc_rep_phys == 0) {
		sc->sc_rep_size = sc->sc_maxreplycnt * IOP_MAX_REPLY_SIZE;

		rv = bus_dmamem_alloc(sc->sc_dmat, sc->sc_rep_size, PAGE_SIZE,
		    0, &seg, 1, &rseg, BUS_DMA_NOWAIT);
		if (rv != 0) {
			printf("%s: dma alloc = %d\n", sc->sc_dv.dv_xname,
			   rv);
			return (rv);
		}

		rv = bus_dmamem_map(sc->sc_dmat, &seg, rseg, sc->sc_rep_size,
		    &sc->sc_rep, BUS_DMA_NOWAIT | BUS_DMA_COHERENT);
		if (rv != 0) {
			printf("%s: dma map = %d\n", sc->sc_dv.dv_xname, rv);
			return (rv);
		}

		rv = bus_dmamap_create(sc->sc_dmat, sc->sc_rep_size, 1,
		    sc->sc_rep_size, 0, BUS_DMA_NOWAIT, &sc->sc_rep_dmamap);
		if (rv != 0) {
			printf("%s: dma create = %d\n", sc->sc_dv.dv_xname, rv);
			return (rv);
		}

		rv = bus_dmamap_load(sc->sc_dmat, sc->sc_rep_dmamap, sc->sc_rep,
		    sc->sc_rep_size, NULL, BUS_DMA_NOWAIT);
		if (rv != 0) {
			printf("%s: dma load = %d\n", sc->sc_dv.dv_xname, rv);
			return (rv);
		}

		sc->sc_rep_phys = sc->sc_rep_dmamap->dm_segs[0].ds_addr;
	}

	/* Populate the outbound FIFO. */
	for (i = sc->sc_maxreplycnt, addr = sc->sc_rep_phys; i != 0; i--) {
		iop_outl(sc, IOP_REG_OFIFO, (u_int32_t)addr);
		addr += IOP_MAX_REPLY_SIZE;
	}

	return (0);
}

/*
 * Read the specified number of bytes from the IOP's hardware resource table.
 */
static int
iop_hrt_get0(struct iop_softc *sc, struct i2o_hrt *hrt, int size)
{
	struct iop_msg *im;
	int rv;
	struct i2o_exec_hrt_get *mb;

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOINTR)) != 0)
		return (rv);

	mb = (struct i2o_exec_hrt_get *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_exec_hrt_get);
	mb->msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_EXEC_HRT_GET);
	mb->msgictx = IOP_ICTX;
	mb->msgtctx = im->im_tctx;

	iop_msg_map(sc, im, hrt, size, 0);
	rv = iop_msg_enqueue(sc, im, 5000);
	iop_msg_unmap(sc, im);
	iop_msg_free(sc, NULL, im);
	return (rv);
}

/*
 * Read the IOP's hardware resource table.
 */
static int
iop_hrt_get(struct iop_softc *sc)
{
	struct i2o_hrt hrthdr, *hrt;
	int size, rv;

	if ((rv = iop_hrt_get0(sc, &hrthdr, sizeof(hrthdr))) != 0)
		return (rv);

	DPRINTF(("%s: %d hrt entries\n", sc->sc_dv.dv_xname,
	    le16toh(hrthdr.numentries)));

	size = sizeof(struct i2o_hrt) + 
	    (htole32(hrthdr.numentries) - 1) * sizeof(struct i2o_hrt_entry);
	hrt = (struct i2o_hrt *)malloc(size, M_DEVBUF, M_NOWAIT);

	if ((rv = iop_hrt_get0(sc, hrt, size)) != 0) {
		free(hrt, M_DEVBUF);
		return (rv);
	}

	if (sc->sc_hrt != NULL)
		free(sc->sc_hrt, M_DEVBUF);
	sc->sc_hrt = hrt;
	return (0);
}

/*
 * Request the specified number of bytes from the IOP's logical
 * configuration table.  If a change indicator is specified, this
 * is an verbatim notification request, so the caller is prepared
 * to wait indefinitely.
 */
static int
iop_lct_get0(struct iop_softc *sc, struct i2o_lct *lct, int size,
	     u_int32_t chgind)
{
	struct iop_msg *im;
	struct i2o_exec_lct_notify *mb;
	int rv;

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOINTR)) != 0)
		return (rv);

	memset(lct, 0, size);
	memset(im->im_msg, 0, sizeof(im->im_msg));

	mb = (struct i2o_exec_lct_notify *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_exec_lct_notify);
	mb->msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_EXEC_LCT_NOTIFY);
	mb->msgictx = IOP_ICTX;
	mb->msgtctx = im->im_tctx;
	mb->classid = I2O_CLASS_ANY;
	mb->changeindicator = chgind;

#ifdef I2ODEBUG
	printf("iop_lct_get0: reading LCT");
	if (chgind != 0)
		printf(" (async)");
	printf("\n");
#endif

	iop_msg_map(sc, im, lct, size, 0);
	rv = iop_msg_enqueue(sc, im, (chgind == 0 ? 120*1000 : 0));
	iop_msg_unmap(sc, im);
	iop_msg_free(sc, NULL, im);
	return (rv);
}

/*
 * Read the IOP's logical configuration table.
 */
int
iop_lct_get(struct iop_softc *sc)
{
	int esize, size, rv;
	struct i2o_lct *lct;

	esize = le32toh(sc->sc_status.expectedlctsize);
	lct = (struct i2o_lct *)malloc(esize, M_DEVBUF, M_WAITOK);
	if (lct == NULL)
		return (ENOMEM);

	if ((rv = iop_lct_get0(sc, lct, esize, 0)) != 0) {
		free(lct, M_DEVBUF);
		return (rv);
	}

	size = le16toh(lct->tablesize) << 2;
	if (esize != size) {
		free(lct, M_DEVBUF);
		lct = (struct i2o_lct *)malloc(size, M_DEVBUF, M_WAITOK);
		if (lct == NULL)
			return (ENOMEM);

		if ((rv = iop_lct_get0(sc, lct, size, 0)) != 0) {
			free(lct, M_DEVBUF);
			return (rv);
		}
	}

	/* Swap in the new LCT. */
	if (sc->sc_lct != NULL)
		free(sc->sc_lct, M_DEVBUF);
	sc->sc_lct = lct;
	sc->sc_nlctent = ((le16toh(sc->sc_lct->tablesize) << 2) -
	    sizeof(struct i2o_lct) + sizeof(struct i2o_lct_entry)) /
	    sizeof(struct i2o_lct_entry);
	return (0);
}

/*
 * Request the specified parameter group from the target.
 */
int
iop_param_op(struct iop_softc *sc, int tid, int write, int group, void *buf,
	     int size)
{
	struct iop_msg *im;
	struct i2o_util_params_op *mb;
	int rv, func, op;
	struct {
		struct	i2o_param_op_list_header olh;
		struct	i2o_param_op_all_template oat;
	} req;

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOINTR)) != 0)
		return (rv);

	if (write) {
		func = I2O_UTIL_PARAMS_SET;
		op = I2O_PARAMS_OP_FIELD_SET;
	} else {
		func = I2O_UTIL_PARAMS_GET;
		op = I2O_PARAMS_OP_FIELD_GET;
	}

	mb = (struct i2o_util_params_op *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_util_params_op);
	mb->msgfunc = I2O_MSGFUNC(tid, func);
	mb->msgictx = IOP_ICTX;
	mb->msgtctx = im->im_tctx;
	mb->flags = 0;

	req.olh.count = htole16(1);
	req.olh.reserved = htole16(0);
	req.oat.operation = htole16(op);
	req.oat.fieldcount = htole16(0xffff);
	req.oat.group = htole16(group);

	memset(buf, 0, size);
	iop_msg_map(sc, im, &req, sizeof(req), 1);
	iop_msg_map(sc, im, buf, size, write);

	rv = iop_msg_enqueue(sc, im, 5000);
	iop_msg_unmap(sc, im);
	iop_msg_free(sc, NULL, im);
	return (rv);
}	

/*
 * Execute a simple command (no parameters).
 */
int
iop_simple_cmd(struct iop_softc *sc, int tid, int function, int ictx,
	       int async, int timo)
{
	struct iop_msg *im;
	struct i2o_msg *mb;
	int rv, fl;

	fl = (async != 0 ? IM_NOWAIT : 0);
	if ((rv = iop_msg_alloc(sc, NULL, &im, fl | IM_NOINTR)) != 0)
		return (rv);

	mb = (struct i2o_msg *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_msg);
	mb->msgfunc = I2O_MSGFUNC(tid, function);
	mb->msgictx = ictx;
	mb->msgtctx = im->im_tctx;

	if (async)
		rv = iop_msg_enqueue(sc, im, timo);
	else
		rv = iop_msg_send(sc, im, timo);
	iop_msg_free(sc, NULL, im);
	return (rv);
}

/*
 * Post the system table to the IOP.
 */
static int
iop_systab_set(struct iop_softc *sc)
{
	struct i2o_exec_sys_tab_set *mb;
	struct iop_msg *im;
	u_int32_t mema[2], ioa[2];
	int rv;

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOINTR)) != 0)
		return (rv);

	mb = (struct i2o_exec_sys_tab_set *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_exec_sys_tab_set);
	mb->msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_EXEC_SYS_TAB_SET);
	mb->msgictx = IOP_ICTX;
	mb->msgtctx = im->im_tctx;
	mb->iopid = (sc->sc_dv.dv_unit + 2) << 12;
	mb->segnumber = 0;

	/* XXX This is questionable, but better than nothing... */
	mema[0] = le32toh(sc->sc_status.currentprivmembase);
	mema[1] = le32toh(sc->sc_status.currentprivmemsize);
	ioa[0] = le32toh(sc->sc_status.currentpriviobase);
	ioa[1] = le32toh(sc->sc_status.currentpriviosize);

	iop_msg_map(sc, im, iop_systab, iop_systab_size, 1);
	iop_msg_map(sc, im, mema, sizeof(mema), 1);
	iop_msg_map(sc, im, ioa, sizeof(ioa), 1);

	rv = iop_msg_enqueue(sc, im, 5000);
	iop_msg_unmap(sc, im);
	iop_msg_free(sc, NULL, im);
	return (rv);
}

/*
 * Reset the adapter.  Must be called with interrupts disabled.
 */
static int
iop_reset(struct iop_softc *sc)
{
	struct iop_msg *im;
	volatile u_int32_t sw;
	u_int32_t mfa;
	struct i2o_exec_iop_reset *mb;
	int rv;

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOWAIT | IM_NOICTX)) != 0)
		return (rv);

	sw = 0;

	mb = (struct i2o_exec_iop_reset *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_exec_iop_reset);
	mb->msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_EXEC_IOP_RESET);
	mb->reserved[0] = 0;
	mb->reserved[1] = 0;
	mb->reserved[2] = 0;
	mb->reserved[3] = 0;
	mb->statuslow = kvtop((caddr_t)&sw);		/* XXX */
	mb->statushigh = 0;

	if ((rv = iop_msg_send(sc, im, 0)))
		return (rv);
	iop_msg_free(sc, NULL, im);

	POLL(2500, sw != 0);					/* XXX */
	if (sw != I2O_RESET_IN_PROGRESS) {
		printf("%s: reset rejected\n", sc->sc_dv.dv_xname);
		return (EIO);
	}

	/* 
	 * IOP is now in the INIT state.  Wait no more than 10 seconds for
	 * the inbound queue to become responsive.
	 */
	POLL(10000, (mfa = iop_inl(sc, IOP_REG_IFIFO)) != IOP_MFA_EMPTY);
	if (mfa == IOP_MFA_EMPTY) {
		printf("%s: reset failed\n", sc->sc_dv.dv_xname);
		return (EIO);
	}

	if (sw == I2O_RESET_REJECTED)
		printf("%s: reset rejected?\n", sc->sc_dv.dv_xname);

	iop_release_mfa(sc, mfa);
	return (0);
}

/*
 * Register a new initiator.
 */
int
iop_initiator_register(struct iop_softc *sc, struct iop_initiator *ii)
{
	static int ictx;
	static int stctx;

	/* 0 is reserved for system messages. */
	ii->ii_ictx = ++ictx;
	ii->ii_stctx = ++stctx | 0x80000000;

	LIST_INSERT_HEAD(&sc->sc_iilist, ii, ii_list);
	LIST_INSERT_HEAD(IOP_ICTXHASH(ii->ii_ictx), ii, ii_hash);

	return (0);
}

/*
 * Unregister an initiator.
 */
void
iop_initiator_unregister(struct iop_softc *sc, struct iop_initiator *ii)
{

	LIST_REMOVE(ii, ii_list);
	LIST_REMOVE(ii, ii_hash);
}

/*
 * Handle a reply frame from the adapter.
 */
static int
iop_handle_reply(struct iop_softc *sc, u_int32_t rmfa)
{
	struct iop_msg *im;
	struct i2o_reply *rb;
	struct iop_initiator *ii;
	u_int off, ictx, tctx, status, size;

	off = (int)(rmfa - sc->sc_rep_phys);
	rb = (struct i2o_reply *)(sc->sc_rep + off);

	/* Perform reply queue DMA synchronisation... */
	bus_dmamap_sync(sc->sc_dmat, sc->sc_rep_dmamap, off, IOP_MAX_MSG_SIZE,
	    BUS_DMASYNC_POSTREAD);
	if (--sc->sc_stat.is_cur_hwqueue != 0)
		bus_dmamap_sync(sc->sc_dmat, sc->sc_rep_dmamap,
		    0, sc->sc_rep_size, BUS_DMASYNC_PREREAD);

#ifdef I2ODEBUG
	if ((le32toh(rb->msgflags) & I2O_MSGFLAGS_64BIT) != 0)
		panic("iop_handle_reply: 64-bit reply");
#endif
	/* 
	 * Find the initiator.
	 */
	ictx = le32toh(rb->msgictx);
	if (ictx == IOP_ICTX)
		ii = NULL;
	else {
		ii = LIST_FIRST(IOP_ICTXHASH(ictx));
		for (; ii != NULL; ii = LIST_NEXT(ii, ii_hash))
			if (ii->ii_ictx == ictx)
				break;
		if (ii == NULL) {
#ifdef I2ODEBUG
			iop_reply_print(sc, NULL, rb);
#endif
			printf("%s: WARNING: bad ictx returned (%x)",
			    sc->sc_dv.dv_xname, ictx);

			/* Return the reply frame to the IOP's outbound FIFO. */
			iop_outl(sc, IOP_REG_OFIFO, rmfa);
			return (-1);
		}
	}

	status = rb->reqstatus;

	if (ii == NULL || (ii->ii_flags & II_DISCARD) == 0) {
		/*
		 * This initiator tracks state using message wrappers.
		 *
		 * Find the originating message wrapper, and if requested
		 * notify the initiator.
		 */
		tctx = le32toh(rb->msgtctx);
		im = TAILQ_FIRST(IOP_TCTXHASH(tctx));
		for (; im != NULL; im = TAILQ_NEXT(im, im_hash))
			if (im->im_tctx == tctx)
				break;
		if (im == NULL || (im->im_flags & IM_ALLOCED) == 0) {
#ifdef I2ODEBUG
			iop_reply_print(sc, NULL, rb);
#endif
			printf("%s: WARNING: bad tctx returned (%x, %p)",
			    sc->sc_dv.dv_xname, tctx, im);

			/* Return the reply frame to the IOP's outbound FIFO. */
			iop_outl(sc, IOP_REG_OFIFO, rmfa);
			return (-1);
		}
#ifdef I2ODEBUG
		if ((im->im_flags & IM_REPLIED) != 0)
			panic("%s: dup reply", sc->sc_dv.dv_xname);
#endif

		im->im_flags |= IM_REPLIED;

#ifdef I2ODEBUG
		if (rb->reqstatus != 0)
			iop_reply_print(sc, im, rb);
#endif
		/* Notify the initiator. */
		if ((im->im_flags & IM_WAITING) != 0) {
			size = (le32toh(rb->msgflags) >> 14) & ~3;
			if (size > IOP_MAX_REPLY_SIZE)
				size = IOP_MAX_REPLY_SIZE;
			memcpy(im->im_msg, rb, size);
			wakeup(im);
		} else if ((im->im_flags & IM_NOINTR) == 0)
			(*ii->ii_intr)(ii->ii_dv, im, rb);
	} else {
		/*
		 * This initiator discards message wrappers.
		 *
		 * Simply pass the reply frame to the initiator.
		 */
		(*ii->ii_intr)(ii->ii_dv, NULL, rb);
	}

	/* Return the reply frame to the IOP's outbound FIFO. */
	iop_outl(sc, IOP_REG_OFIFO, rmfa);

	/* Run the queue. */
	if ((im = SIMPLEQ_FIRST(&sc->sc_queue)) != NULL)
		iop_msg_enqueue(sc, im, 0);

	return (status);
}

/*
 * Handle an interrupt from the adapter.
 */
int
iop_intr(void *arg)
{
	struct iop_softc *sc;
	u_int32_t rmfa;

	sc = arg;

	if ((iop_inl(sc, IOP_REG_INTR_STATUS) & IOP_INTR_OFIFO) == 0)
		return (0);

	for (;;) {
		/* Double read to account for IOP bug. */
		if ((rmfa = iop_inl(sc, IOP_REG_OFIFO)) == IOP_MFA_EMPTY &&
		    (rmfa = iop_inl(sc, IOP_REG_OFIFO)) == IOP_MFA_EMPTY)
			break;
		iop_handle_reply(sc, rmfa);
	}

	return (1);
}

/*
 * Handle an event signalled by the executive.
 */
static void
iop_intr_event(struct device *dv, struct iop_msg *im, void *reply)
{
	struct i2o_util_event_register_reply *rb;
	struct iop_softc *sc;
	u_int event;

	sc = (struct iop_softc *)dv;
	rb = reply;
	event = le32toh(rb->event);

#ifndef I2ODEBUG
	if (event == I2O_EVENT_GEN_EVENT_MASK_MODIFIED)
		return;
#endif

	printf("%s: event 0x%08x received\n", dv->dv_xname, event);
}

/* 
 * Allocate a message wrapper.
 */
int
iop_msg_alloc(struct iop_softc *sc, struct iop_initiator *ii,
	      struct iop_msg **imp, int flags)
{
	struct iop_msg *im;
	static int tctxgen = 666;
	int s, rv, i, tctx;

#ifdef I2ODEBUG
	if ((flags & IM_SYSMASK) != 0)
		panic("iop_msg_alloc: system flags specified");
#endif

	s = splbio();	/* XXX */

	if (ii != NULL && (ii->ii_flags & II_DISCARD) != 0) {
		flags |= IM_DISCARD;
		tctx = ii->ii_stctx;
	} else
		tctx = tctxgen++ & 0x7fffffff;

	im = (struct iop_msg *)pool_get(iop_msgpool,
	    (flags & IM_NOWAIT) == 0 ? PR_WAITOK : 0);
	if (im == NULL) {
		splx(s);
		return (ENOMEM);
	}

	/* XXX */
	rv = bus_dmamap_create(sc->sc_dmat, IOP_MAX_XFER, IOP_MAX_SGL_ENTRIES,
	    IOP_MAX_XFER, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
	    &im->im_xfer[0].ix_map);
	if (rv != 0) {
		pool_put(iop_msgpool, im);
		splx(s);
		return (rv);
	}

	if ((flags & (IM_DISCARD | IM_NOICTX)) == 0)
		TAILQ_INSERT_TAIL(IOP_TCTXHASH(tctx), im, im_hash);

	splx(s);

	im->im_tctx = tctx;
	im->im_flags = flags | IM_ALLOCED;
	for (i = 0; i < IOP_MAX_MSG_XFERS; i++)
		im->im_xfer[i].ix_size = 0;
	*imp = im;

	return (0);
}

/* 
 * Free a message wrapper.
 */
void
iop_msg_free(struct iop_softc *sc, struct iop_initiator *ii, struct iop_msg *im)
{
	int s;

#ifdef I2ODEBUG
	if ((im->im_flags & IM_ALLOCED) == 0)
		panic("iop_msg_free: wrapper not allocated");
#endif

	/* XXX */
	bus_dmamap_destroy(sc->sc_dmat, im->im_xfer[0].ix_map);

	s = splbio();	/* XXX */

	if ((im->im_flags & (IM_DISCARD | IM_NOICTX)) == 0)
		TAILQ_REMOVE(IOP_TCTXHASH(im->im_tctx), im, im_hash);

	im->im_flags = 0;
	pool_put(iop_msgpool, im);
	splx(s);
}

/*
 * Map a data transfer.  Write a scatter-gather list into the message frame. 
 */
int
iop_msg_map(struct iop_softc *sc, struct iop_msg *im, void *xferaddr,
	    int xfersize, int out)
{
	struct iop_xfer *ix;
	u_int32_t *mb;
	int rv, seg, i;

	for (i = 0, ix = im->im_xfer; i < IOP_MAX_MSG_XFERS; i++, ix++)
		if (ix->ix_size == 0)
			break;
#ifdef I2ODEBUG
	if (i == IOP_MAX_MSG_XFERS)
		panic("iop_msg_map: too many xfers");
#endif

	/* Only the first DMA map is static. */
	if (i != 0) {
		rv = bus_dmamap_create(sc->sc_dmat, IOP_MAX_XFER,
		    IOP_MAX_SGL_ENTRIES, IOP_MAX_XFER, 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &ix->ix_map);
		if (rv != 0)
			return (rv);
	}

	ix->ix_flags = (out ? IX_OUT : IX_IN);
	ix->ix_size = xfersize;

	rv = bus_dmamap_load(sc->sc_dmat, ix->ix_map, xferaddr, xfersize,
	    NULL, 0);
	if (rv != 0)
		return (rv);
	bus_dmamap_sync(sc->sc_dmat, ix->ix_map, 0, xfersize,
	    out ? BUS_DMASYNC_POSTWRITE : BUS_DMASYNC_POSTREAD);

	mb = im->im_msg + (im->im_msg[0] >> 16);
	if (out)
		out = I2O_SGL_SIMPLE | I2O_SGL_DATA_OUT;
	else
		out = I2O_SGL_SIMPLE;

	for (seg = 0; seg < ix->ix_map->dm_nsegs; seg++) {
#ifdef I2ODEBUG
		if ((seg << 1) + (im->im_msg[0] >> 16) >=
		    (IOP_MAX_MSG_SIZE >> 2))
			panic("iop_map_xfer: message frame too large");
#endif
		if (seg == ix->ix_map->dm_nsegs - 1)
			out |= I2O_SGL_END_BUFFER;
		*mb++ = (u_int32_t)ix->ix_map->dm_segs[seg].ds_len | out;
		*mb++ = (u_int32_t)ix->ix_map->dm_segs[seg].ds_addr;
	}

	/*
	 * If this is the first xfer we've mapped for this message, adjust
	 * the SGL offset field in the message header.
	 */
	if ((im->im_flags & IM_SGLOFFADJ) == 0) {
		im->im_msg[0] += ((im->im_msg[0] >> 16) + seg * 2) << 4;
		im->im_flags |= IM_SGLOFFADJ;
	}
	im->im_msg[0] += (seg << 17);
	return (0);
}

/*
 * Unmap all data transfers associated with a message wrapper.
 */
void
iop_msg_unmap(struct iop_softc *sc, struct iop_msg *im)
{
	struct iop_xfer *ix;
	int i;
	
	for (i = 0, ix = im->im_xfer; i < IOP_MAX_MSG_XFERS; i++, ix++) {
		if (ix->ix_size == 0)
			break;
		bus_dmamap_sync(sc->sc_dmat, ix->ix_map, 0, ix->ix_size,
		    ix->ix_flags & IX_OUT ? BUS_DMASYNC_POSTWRITE :
		    BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, ix->ix_map);

		/* Only the first DMA map is static. */
		if (i != 0)
			bus_dmamap_destroy(sc->sc_dmat, ix->ix_map);

		ix->ix_size = 0;
	}
}

/* 
 * Send a message to the IOP.  Optionally, poll on completion.  Return
 * non-zero if failure status is returned and IM_NOINTR is set.
 */
int
iop_msg_send(struct iop_softc *sc, struct iop_msg *im, int timo)
{
	u_int32_t mfa, rmfa;
	int rv, status, i, s;

#ifdef I2ODEBUG
	if ((im->im_flags & IM_NOICTX) == 0)
		if (im->im_msg[3] == IOP_ICTX &&
		    (im->im_flags & IM_NOINTR) == 0)
			panic("iop_msg_send: IOP_ICTX and !IM_NOINTR");
	if ((im->im_flags & IM_DISCARD) != 0)
		panic("iop_msg_send: IM_DISCARD");
#endif

	s = splbio();	/* XXX */

	/* Wait up to 250ms for an MFA. */
	POLL(250, (mfa = iop_inl(sc, IOP_REG_IFIFO)) != IOP_MFA_EMPTY);
	if (mfa == IOP_MFA_EMPTY) {
		DPRINTF(("%s: mfa not forthcoming\n", sc->sc_dv.dv_xname));
		splx(s);
		return (EBUSY);
	}

	/* Perform reply queue DMA synchronisation and update counters. */
	if ((im->im_flags & IM_NOICTX) == 0) {
		if (sc->sc_stat.is_cur_hwqueue == 0)
			bus_dmamap_sync(sc->sc_dmat, sc->sc_rep_dmamap, 0,
			    sc->sc_rep_size, BUS_DMASYNC_PREREAD);
		for (i = 0; i < IOP_MAX_MSG_XFERS; i++)
			sc->sc_stat.is_bytes += im->im_xfer[i].ix_size;
		sc->sc_stat.is_requests++;
		if (++sc->sc_stat.is_cur_hwqueue > sc->sc_stat.is_peak_hwqueue)
			sc->sc_stat.is_peak_hwqueue =
			    sc->sc_stat.is_cur_hwqueue;
	}

	/* Terminate scatter/gather lists. */
	if ((im->im_flags & IM_SGLOFFADJ) != 0)
		im->im_msg[(im->im_msg[0] >> 16) - 2] |= I2O_SGL_END;

	/* Post the message frame. */
	bus_space_write_region_4(sc->sc_iot, sc->sc_ioh, mfa, 
	    im->im_msg, im->im_msg[0] >> 16);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, mfa,
	    (im->im_msg[0] >> 14) & ~3, BUS_SPACE_BARRIER_WRITE);

	/* Post the MFA back to the IOP, thus starting the command. */
	iop_outl(sc, IOP_REG_IFIFO, mfa);

	if (timo == 0) {
		splx(s);
		return (0);
	}

	/* Wait for completion. */
	for (timo *= 10; timo != 0; timo--) {
		if ((iop_inl(sc, IOP_REG_INTR_STATUS) & IOP_INTR_OFIFO) != 0) {
			/* Double read to account for IOP bug. */
			rmfa = iop_inl(sc, IOP_REG_OFIFO);
			if (rmfa == IOP_MFA_EMPTY)
				rmfa = iop_inl(sc, IOP_REG_OFIFO);
			if (rmfa != IOP_MFA_EMPTY)
				status = iop_handle_reply(sc, rmfa);
		}
		if ((im->im_flags & IM_REPLIED) != 0)
			break;
		DELAY(100);
	}

	splx(s);

	if (timo == 0) {
#ifdef I2ODEBUG
		printf("%s: poll - no reply\n", sc->sc_dv.dv_xname);
		if (iop_status_get(sc) != 0)
			printf("iop_msg_send: unable to retrieve status\n");
		else
			printf("iop_msg_send: IOP state = %d\n",
			    (le32toh(sc->sc_status.segnumber) >> 16) & 0xff); 
#endif
		rv = EBUSY;
	} else if ((im->im_flags & IM_NOINTR) != 0)
		rv = (status != I2O_STATUS_SUCCESS ? EIO : 0);

	return (rv);
}

/*
 * Try to post a message to the adapter; if that's not possible, enqueue it
 * with us.  If a timeout is specified, wait for the message to complete.
 */
int
iop_msg_enqueue(struct iop_softc *sc, struct iop_msg *im, int timo)
{
	u_int mfa;
	int s, fromqueue, i, rv;

#ifdef I2ODEBUG
	if (im == NULL)
		panic("iop_msg_enqueue: im == NULL");
	if (sc == NULL)
		panic("iop_msg_enqueue: sc == NULL");
	if ((im->im_flags & IM_NOICTX) != 0)
		panic("iop_msg_enqueue: IM_NOICTX");
	if (im->im_msg[3] == IOP_ICTX && (im->im_flags & IM_NOINTR) == 0)
		panic("iop_msg_enqueue: IOP_ICTX and no IM_NOINTR");
	if ((im->im_flags & IM_DISCARD) != 0 && timo != 0)
		panic("iop_msg_enqueue: IM_DISCARD && timo != 0");
	if ((im->im_flags & IM_NOINTR) == 0 && timo != 0)
		panic("iop_msg_enqueue: !IM_NOINTR && timo != 0");
#endif

	s = splbio();	/* XXX */
	fromqueue = (im == SIMPLEQ_FIRST(&sc->sc_queue));

	if (sc->sc_stat.is_cur_hwqueue >= sc->sc_maxqueuecnt) {
		/*
		 * While the IOP may be able to accept more inbound message
		 * frames than it advertises, don't push harder than it
		 * wants to go lest we starve it.
		 *
		 * XXX We should be handling IOP resource shortages.
		 */
		mfa = IOP_MFA_EMPTY;
		DPRINTF(("iop_msg_enqueue: exceeded max queue count\n"));
	} else {
		/* Double read to account for IOP bug. */
		if ((mfa = iop_inl(sc, IOP_REG_IFIFO)) == IOP_MFA_EMPTY)
			mfa = iop_inl(sc, IOP_REG_IFIFO);
	}

	if (mfa == IOP_MFA_EMPTY) {
		DPRINTF(("iop_msg_enqueue: no mfa\n"));
		/* Can't transfer to h/w queue - queue with us. */
		if (!fromqueue) {
			SIMPLEQ_INSERT_TAIL(&sc->sc_queue, im, im_queue);
			if (++sc->sc_stat.is_cur_swqueue >
			    sc->sc_stat.is_peak_swqueue)
				sc->sc_stat.is_peak_swqueue =
				    sc->sc_stat.is_cur_swqueue;
		}
		splx(s);
		if ((im->im_flags & IM_NOINTR) != 0)
			rv = iop_msg_wait(sc, im, timo);
		else
			rv = 0;
		return (rv);
	} else if (fromqueue) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_queue, im, im_queue);
		sc->sc_stat.is_cur_swqueue--;
	}

	if ((im->im_flags & IM_NOINTR) != 0)
		im->im_flags |= IM_WAITING;

	/* Perform reply queue DMA synchronisation and update counters. */
	if (sc->sc_stat.is_cur_hwqueue == 0)
		bus_dmamap_sync(sc->sc_dmat, sc->sc_rep_dmamap, 0,
		    sc->sc_rep_size, BUS_DMASYNC_PREREAD);

	for (i = 0; i < IOP_MAX_MSG_XFERS; i++)
		sc->sc_stat.is_bytes += im->im_xfer[i].ix_size;
	sc->sc_stat.is_requests++;
	if (++sc->sc_stat.is_cur_hwqueue > sc->sc_stat.is_peak_hwqueue)
		sc->sc_stat.is_peak_hwqueue = sc->sc_stat.is_cur_hwqueue;

	/* Terminate the scatter/gather list. */
	if ((im->im_flags & IM_SGLOFFADJ) != 0)
		im->im_msg[(im->im_msg[0] >> 16) - 2] |= I2O_SGL_END;

	/* Post the message frame. */
	bus_space_write_region_4(sc->sc_iot, sc->sc_ioh, mfa, 
	    im->im_msg, im->im_msg[0] >> 16);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, mfa,
	    (im->im_msg[0] >> 14) & ~3, BUS_SPACE_BARRIER_WRITE);

	/* Post the MFA back to the IOP, thus starting the command. */
	iop_outl(sc, IOP_REG_IFIFO, mfa);

	/* If this is a discardable message wrapper, free it. */
	if ((im->im_flags & IM_DISCARD) != 0)
		iop_msg_free(sc, NULL, im);
	splx(s);

	if ((im->im_flags & IM_NOINTR) != 0)
		rv = iop_msg_wait(sc, im, timo);
	else
		rv = 0;
	return (rv);
}

/*
 * Wait for the specified message to complete.
 */
static int
iop_msg_wait(struct iop_softc *sc, struct iop_msg *im, int timo)
{
	struct i2o_reply *rb;
	int rv, s;

	s = splbio();
	if ((im->im_flags & IM_REPLIED) != 0) {
		splx(s);
		return (0);
	}
	rv = tsleep(im, PRIBIO, "iopmsg", timo * hz / 1000);
	splx(s);
#ifdef I2ODEBUG
	if (rv != 0) {
		printf("iop_msg_wait: tsleep() == %d\n", rv);
		if (iop_status_get(sc) != 0)
			printf("iop_msg_wait: unable to retrieve status\n");
		else
			printf("iop_msg_wait: IOP state = %d\n",
			    (le32toh(sc->sc_status.segnumber) >> 16) & 0xff); 
	}
#endif

	if ((im->im_flags & (IM_REPLIED | IM_NOSTATUS)) == IM_REPLIED) {
		rb = (struct i2o_reply *)im->im_msg;
		rv = (rb->reqstatus != I2O_STATUS_SUCCESS ? EIO : 0);
	}
	return (rv);
}

/*
 * Release an unused message frame back to the IOP's inbound fifo.
 */
static void
iop_release_mfa(struct iop_softc *sc, u_int32_t mfa)
{

	/* Use the frame to issue a no-op. */
	iop_outl(sc, mfa, I2O_VERSION_11 | (4 << 16));
	iop_outl(sc, mfa + 4, I2O_MSGFUNC(I2O_TID_IOP, I2O_UTIL_NOP));
	iop_outl(sc, mfa + 8, 0);
	iop_outl(sc, mfa + 12, 0);

	iop_outl(sc, IOP_REG_IFIFO, mfa);
}

#ifdef I2ODEBUG
/*
 * Print status information from a failure reply frame.
 */
static void
iop_reply_print(struct iop_softc *sc, struct iop_msg *im,
		struct i2o_reply *rb)
{
	u_int function, detail;
#ifdef I2OVERBOSE
	const char *statusstr;
#endif

	if (im != NULL && (im->im_flags & IM_REPLIED) == 0)
		panic("iop_msg_print_status: %p not replied to", im);

	function = (le32toh(rb->msgfunc) >> 24) & 0xff;
	detail = le16toh(rb->detail);

	printf("%s: reply:\n", sc->sc_dv.dv_xname);

#ifdef I2OVERBOSE
	if (rb->reqstatus < sizeof(iop_status) / sizeof(iop_status[0]))
		statusstr = iop_status[rb->reqstatus];
	else
		statusstr = "undefined error code";

	printf("%s:   function=0x%02x status=0x%02x (%s)\n", 
	    sc->sc_dv.dv_xname, function, rb->reqstatus, statusstr);
#else
	printf("%s:   function=0x%02x status=0x%02x\n", 
	    sc->sc_dv.dv_xname, function, rb->reqstatus);
#endif
	printf("%s:   detail=0x%04x ictx=0x%08x tctx=0x%08x\n",
	    sc->sc_dv.dv_xname, detail, le32toh(rb->msgictx),
	    le32toh(rb->msgtctx));
	printf("%s:   tidi=%d tidt=%d flags=0x%02x\n", sc->sc_dv.dv_xname,
	    (le32toh(rb->msgfunc) >> 12) & 4095, le32toh(rb->msgfunc) & 4095,
	    (le32toh(rb->msgflags) >> 8) & 0xff);
}
#endif

/*
 * Translate an I2O ASCII field into a C string.
 */
void
iop_strvis(struct iop_softc *sc, const char *src, int slen, char *dst, int dlen)
{
	int hc, lc, i, nit;

	dlen--;
	lc = 0;
	hc = 0;
	i = 0;

	/*
	 * DPT use NUL as a space, whereas AMI use it as a terminator.  The
	 * spec has nothing to say about it.  Since AMI fields are usually
	 * filled with junk after the terminator, ...
	 */
	nit = (le16toh(sc->sc_status.orgid) != I2O_ORG_DPT);

	while (slen-- != 0 && dlen-- != 0) {
		if (nit && *src == '\0')
			break;
		else if (*src <= 0x20 || *src >= 0x7f) {
			if (hc)
				dst[i++] = ' ';
		} else {
			hc = 1;
			dst[i++] = *src;
			lc = i;
		}
		src++;
	}
	
	dst[lc] = '\0';
}

/*
 * Claim or unclaim the specified TID.
 */
int
iop_util_claim(struct iop_softc *sc, struct iop_initiator *ii, int release,
	      int flags)
{
	struct iop_msg *im;
	struct i2o_util_claim *mb;
	int rv, func;

	func = release ? I2O_UTIL_CLAIM_RELEASE : I2O_UTIL_CLAIM;

	if ((rv = iop_msg_alloc(sc, ii, &im, IM_NOINTR)) != 0)
		return (rv);

	/* We can use the same structure, as both are identical. */
	mb = (struct i2o_util_claim *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_util_claim);
	mb->msgfunc = I2O_MSGFUNC(ii->ii_tid, func);
	mb->msgictx = ii->ii_ictx;
	mb->msgtctx = im->im_tctx;
	mb->flags = flags;

	rv = iop_msg_enqueue(sc, im, 5000);
	iop_msg_free(sc, ii, im);
	return (rv);
}	

/*
 * Perform an abort.
 */
int iop_util_abort(struct iop_softc *sc, struct iop_initiator *ii, int func,
		  int tctxabort, int flags)
{
	struct iop_msg *im;
	struct i2o_util_abort *mb;
	int rv;

	if ((rv = iop_msg_alloc(sc, ii, &im, IM_NOINTR)) != 0)
		return (rv);

	mb = (struct i2o_util_abort *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_util_abort);
	mb->msgfunc = I2O_MSGFUNC(ii->ii_tid, I2O_UTIL_ABORT);
	mb->msgictx = ii->ii_ictx;
	mb->msgtctx = im->im_tctx;
	mb->flags = (func << 24) | flags;
	mb->tctxabort = tctxabort;

	rv = iop_msg_enqueue(sc, im, 5000);
	iop_msg_free(sc, ii, im);
	return (rv);
}

/*
 * Enable or disable event types for the specified device.
 */
int iop_util_eventreg(struct iop_softc *sc, struct iop_initiator *ii, int mask)
{
	struct iop_msg *im;
	struct i2o_util_event_register *mb;
	int rv;

	if ((rv = iop_msg_alloc(sc, ii, &im, 0)) != 0)
		return (rv);

	mb = (struct i2o_util_event_register *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_util_event_register);
	mb->msgfunc = I2O_MSGFUNC(ii->ii_tid, I2O_UTIL_EVENT_REGISTER);
	mb->msgictx = ii->ii_ictx;
	mb->msgtctx = im->im_tctx;
	mb->eventmask = mask;

	return (iop_msg_enqueue(sc, im, 0));
}

int
iopopen(dev_t dev, int flag, int mode, struct proc *p)
{
	struct iop_softc *sc;
	int unit, error;

	unit = minor(dev);

	sc = device_lookup(&iop_cd, minor(dev));
	if ((sc = iop_cd.cd_devs[unit]) == NULL)
		return (ENXIO);
	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error); 

	if ((sc->sc_flags & IOP_OPEN) != 0)
		return (EBUSY);
	if ((sc->sc_flags & IOP_ONLINE) == 0)
		return (EIO);
	sc->sc_flags |= IOP_OPEN;

	/* XXX */
	sc->sc_ptb = malloc(((MAXPHYS + 3) & ~3) * IOP_MAX_MSG_XFERS, M_DEVBUF,
	    M_WAITOK);
	if (sc->sc_ptb == NULL) {
		sc->sc_flags ^= IOP_OPEN;
		return (ENOMEM);
	}

	return (0);
}

int
iopclose(dev_t dev, int flag, int mode, struct proc *p)
{
	struct iop_softc *sc;

	sc = device_lookup(&iop_cd, minor(dev));
	free(sc->sc_ptb, M_DEVBUF);
	sc->sc_flags ^= IOP_OPEN;
	return (0);
}

int
iopioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct iop_softc *sc;
	struct iovec *iov;
	struct ioppt *pt;
	struct iop_msg *im;
	struct i2o_msg *mb;
	struct i2o_reply *rb;
	int rv, i;
	struct ioppt_buf *ptb;
	void *buf;

	if (securelevel >= 2)
		return (EPERM);

	sc = device_lookup(&iop_cd, minor(dev));

	PHOLD(p);

	switch (cmd) {
	case IOPIOCPT:
		pt = (struct ioppt *)data;

		if (pt->pt_msglen > IOP_MAX_MSG_SIZE ||
		    pt->pt_msglen < sizeof(struct i2o_msg) ||
		    pt->pt_nbufs > IOP_MAX_MSG_XFERS ||
		    pt->pt_nbufs < 0 ||
		    pt->pt_replylen < 0 ||
		    pt->pt_timo < 1000 ||
		    pt->pt_timo > 5*60*1000) {
		    	rv = EINVAL;
		    	break;
		}
		for (i = 0; i < pt->pt_nbufs; i++)
			if (pt->pt_bufs[i].ptb_datalen > ((MAXPHYS + 3) & ~3)) {
				rv = ENOMEM;
				goto bad;
			}

		rv = iop_msg_alloc(sc, NULL, &im, IM_NOINTR | IM_NOSTATUS);
		if (rv != 0)
			break;

		if ((rv = copyin(pt->pt_msg, im->im_msg, pt->pt_msglen)) != 0) {
			iop_msg_free(sc, NULL, im);
			break;
		}

		mb = (struct i2o_msg *)im->im_msg;
		mb->msgictx = IOP_ICTX;
		mb->msgtctx = im->im_tctx;

		for (i = 0; i < pt->pt_nbufs; i++) {
			ptb = &pt->pt_bufs[i];
			buf = sc->sc_ptb + i * ((MAXPHYS + 3) & ~3);

			if (ptb->ptb_out != 0) {
				rv = copyin(ptb->ptb_data, buf,
				    ptb->ptb_datalen);
				if (rv != 0)
					goto bad;
			}

			rv = iop_msg_map(sc, im, buf, ptb->ptb_datalen,
			    ptb->ptb_out != 0);
			if (rv != 0) {
				iop_msg_free(sc, NULL, im);
				goto bad;
			}
		}

		if ((rv = iop_msg_enqueue(sc, im, pt->pt_timo)) == 0) {
			rb = (struct i2o_reply *)im->im_msg;
			i = (le32toh(rb->msgflags) >> 14) & ~3;	/* XXX */
			if (i > IOP_MAX_REPLY_SIZE)
				i = IOP_MAX_REPLY_SIZE;
			if (i > pt->pt_replylen)
				i = pt->pt_replylen;
			rv = copyout(rb, pt->pt_reply, i);
		}

		for (i = 0; i < pt->pt_nbufs; i++) {
			ptb = &pt->pt_bufs[i];
			if (ptb->ptb_out != 0 || rv != 0)
				continue;
			rv = copyout(sc->sc_ptb + i * ((MAXPHYS + 3) & ~3),
			    ptb->ptb_data, ptb->ptb_datalen);
		}

		iop_msg_free(sc, NULL, im);
		break;

	case IOPIOCGLCT:
		iov = (struct iovec *)data;
		rv = lockmgr(&sc->sc_conflock, LK_SHARED, NULL);
		if (rv == 0) {
			i = le16toh(sc->sc_lct->tablesize) << 2;
			if (i > iov->iov_len)
				i = iov->iov_len;
			else
				iov->iov_len = i;
			rv = copyout(sc->sc_lct, iov->iov_base, i);
			lockmgr(&sc->sc_conflock, LK_RELEASE, NULL);
		}
		break;

	case IOPIOCGSTATUS:
		iov = (struct iovec *)data;
		i = sizeof(struct i2o_status);
		if (i > iov->iov_len)
			i = iov->iov_len;
		else
			iov->iov_len = i;
		if ((rv = iop_status_get(sc)) == 0)
			rv = copyout(&sc->sc_status, iov->iov_base, i);
		break;

	case IOPIOCRECONFIG:
		rv = iop_reconfigure(sc, 0, 0);
		break;

	case IOPIOCGTIDMAP:
		iov = (struct iovec *)data;
		rv = lockmgr(&sc->sc_conflock, LK_SHARED, NULL);
		if (rv == 0) {
			i = sizeof(struct iop_tidmap) * sc->sc_nlctent;
			if (i > iov->iov_len)
				i = iov->iov_len;
			else
				iov->iov_len = i;
			rv = copyout(sc->sc_tidmap, iov->iov_base, i);
			lockmgr(&sc->sc_conflock, LK_RELEASE, NULL);
		}
		break;

	default:
#if defined(DIAGNOSTIC) || defined(I2ODEBUG)
		printf("%s: unknown ioctl %lx\n", sc->sc_dv.dv_xname, cmd);
#endif
		rv = ENOTTY;
		break;
	}

bad:
	PRELE(p);
	return (rv);
}
