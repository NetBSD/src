/*	$NetBSD: iop.c,v 1.4 2000/11/14 18:48:14 thorpej Exp $	*/

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

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <dev/i2o/i2o.h>
#include <dev/i2o/iopreg.h>
#include <dev/i2o/iopvar.h>

#define IOP_INL(x, o)		\
    bus_space_read_4((x)->sc_iot, (x)->sc_ioh, (o))
#define IOP_OUTL(x, o, d)	\
    bus_space_write_4((x)->sc_iot, (x)->sc_ioh, (o), (d))

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
#define	IFVERBOSE(x)	x
#else
#define	IFVERBOSE(x)
#endif

#define IOP_MSGHASH_NBUCKETS	64
#define	IOP_MSGHASH(tctx)	(&iop_msghashtbl[(tctx) & iop_msghash])

static TAILQ_HEAD(iop_msghashhead, iop_msg) *iop_msghashtbl;
static u_long	iop_msghash;
static void	*iop_sdh;
static struct	pool *iop_msgpool;

extern struct cfdriver iop_cd;

#define	IC_CONFIGURE	0x01	/* Try to configure devices of this class */
#define	IC_HIGHLEVEL	0x02	/* This is a `high level' device class */

struct iop_class {
	int	ic_class;
	int	ic_flags;
#ifdef I2OVERBOSE
	const char	*ic_caption;
#endif
} static const iop_class[] = {
	{	
		I2O_CLASS_EXECUTIVE,
		0,
		IFVERBOSE("executive")
	},
	{
		I2O_CLASS_DDM,
		0,
		IFVERBOSE("device driver module")
	},
	{
		I2O_CLASS_RANDOM_BLOCK_STORAGE,
		IC_CONFIGURE | IC_HIGHLEVEL,
		IFVERBOSE("random block storage")
	},
	{
		I2O_CLASS_SEQUENTIAL_STORAGE,
		IC_CONFIGURE | IC_HIGHLEVEL,
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
		IFVERBOSE("fibrechannel peripheral")
	},
 	{
 		I2O_CLASS_SCSI_PERIPHERAL,
 		0,
 		IFVERBOSE("SCSI peripheral")
 	},
	{
		I2O_CLASS_ATE_PORT,
		IC_CONFIGURE,
		IFVERBOSE("ATE port")
	},
	{	
		I2O_CLASS_ATE_PERIPHERAL,
		0,
		IFVERBOSE("ATE peripheral")
	},
	{	
		I2O_CLASS_FLOPPY_CONTROLLER,
		IC_CONFIGURE,
		IFVERBOSE("floppy controller")
	},
	{
		I2O_CLASS_FLOPPY_DEVICE,
		0,
		IFVERBOSE("floppy device")
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

static void	iop_config_interrupts(struct device *);
static void	iop_config_interrupts0(struct iop_softc *, int, int);
static void	iop_devinfo(int, char *);
static int	iop_print(void *, const char *);
static void	iop_shutdown(void *);
static int	iop_submatch(struct device *, struct cfdata *, void *);
static int	iop_vendor_print(void *, const char *);

static int	iop_alloc_dmamem(struct iop_softc *, int, bus_dmamap_t *, 
				 caddr_t *, bus_addr_t *);
static int	iop_hrt_get(struct iop_softc *);
static int	iop_hrt_get0(struct iop_softc *, struct i2o_hrt *, int);
static int	iop_lct_get0(struct iop_softc *, struct i2o_lct *, int);
static int	iop_ofifo_init(struct iop_softc *);
static int	iop_poll(struct iop_softc *);
static void	iop_release_mfa(struct iop_softc *, u_int32_t);
static int	iop_reset(struct iop_softc *);
static int	iop_status_get(struct iop_softc *);
static int	iop_systab_set(struct iop_softc *);

#ifdef I2ODEBUG
static void	iop_reply_print(struct iop_softc *, struct iop_msg *,
				struct i2o_reply *);
#endif

/*
 * Initialise the adapter.
 */
int
iop_init(struct iop_softc *sc, const char *intrstr)
{
	int rv;
	u_int32_t mask;
	static int again;
	char ident[64];

	if (again == 0) {
		/* Create the shared message wrapper pool and hash. */
		iop_msgpool = pool_create(sizeof(struct iop_msg), 0, 0, 0,
		    "ioppl", 0, NULL, NULL, M_DEVBUF);
		iop_msghashtbl = hashinit(IOP_MSGHASH_NBUCKETS, HASH_TAILQ,
		    M_DEVBUF, M_NOWAIT, &iop_msghash);
		again = 1;
	}

	/*
	 * Reset the IOP and request status.
	 */
	printf("I2O adapter");
	if ((rv = iop_reset(sc)) != 0)
		return (rv);
	if ((rv = iop_status_get(sc)) != 0) {
		printf("%s: not responding\n", sc->sc_dv.dv_xname);
		return (rv);
	}

	iop_strvis(sc->sc_status.productid, sizeof(sc->sc_status.productid),
	    ident, sizeof(ident));
	printf(" <%s>", ident);

	/* Allocate reply frames and initialise the IOP's outbound FIFO. */
	sc->sc_maxreplycnt = le32toh(sc->sc_status.maxoutboundmframes);
	if (sc->sc_maxreplycnt > IOP_MAX_HW_REPLYCNT)
		sc->sc_maxreplycnt = IOP_MAX_HW_REPLYCNT;
	if ((rv = iop_ofifo_init(sc)) != 0)
		return (rv);

	/* Bring the IOP online. */
	if ((rv = iop_hrt_get(sc)) != 0)
		return (rv);
	if ((rv = iop_systab_set(sc)) != 0)
		return (rv);
	if ((rv = iop_simple_cmd(sc, I2O_TID_IOP, I2O_EXEC_SYS_ENABLE,
	    IOP_ICTX)) != 0)
		return (rv);

	/* Defer configuration of children until interrupts are working. */
	config_interrupts((struct device *)sc, iop_config_interrupts);

	/* Configure shutdownhook. */
	if (iop_sdh == NULL)
		iop_sdh = shutdownhook_establish(iop_shutdown, NULL);

	/* Ensure interrupts are enabled at the IOP. */
	mask = IOP_INL(sc, IOP_REG_INTR_MASK);
	IOP_OUTL(sc, IOP_REG_INTR_MASK, mask & ~IOP_INTR_OFIFO);

	printf("\n");
	if (intrstr != NULL)
		printf("%s: interrupting at %s\n", sc->sc_dv.dv_xname,
		    intrstr);

	sc->sc_maxqueuecnt = le32toh(sc->sc_status.maxinboundmframes);
	if (sc->sc_maxqueuecnt > IOP_MAX_HW_QUEUECNT)
		sc->sc_maxqueuecnt = IOP_MAX_HW_QUEUECNT;

#ifdef I2ODEBUG
	printf("%s: queue depths: inbound %d/%d, outbound %d/%d\n",
	    sc->sc_dv.dv_xname, 
	    sc->sc_maxqueuecnt, le32toh(sc->sc_status.maxinboundmframes),
	    sc->sc_maxreplycnt, le32toh(sc->sc_status.maxoutboundmframes));
#endif

	SIMPLEQ_INIT(&sc->sc_queue);
	return (0);
}

/*
 * Attempt to match and attach child devices.
 */
static void
iop_config_interrupts(struct device *self)
{
	struct iop_attach_args ia;
	struct iop_softc *sc;
	int rv;

	sc = (struct iop_softc *)self;

	/* Read the LCT. */
	if ((rv = iop_lct_get(sc)) != 0)
		printf("%s: failed to read LCT (%d)\n", sc->sc_dv.dv_xname, rv);

	/* Attempt to match and attach a product-specific extension. */
	ia.ia_class = I2O_CLASS_ANY;
	ia.ia_tid = I2O_TID_IOP;
	config_found_sm(self, &ia, iop_vendor_print, iop_submatch);

	/*
	 * Match and attach child devices.  We do two runs: the first to
	 * match "high level" devices, and the second to match "low level"
	 * devices (low level devices may be parents of high level devices).
	 *
	 * XXX sc_lctmap shouldn't be allocated here.
	 */
	sc->sc_lctmap = malloc(sc->sc_nlctent * sizeof(int8_t), M_DEVBUF,
	    M_NOWAIT);
	memset(sc->sc_lctmap, 0, sc->sc_nlctent * sizeof(int8_t));
	iop_config_interrupts0(sc, IC_CONFIGURE | IC_HIGHLEVEL,
	    IC_CONFIGURE | IC_HIGHLEVEL);
	iop_config_interrupts0(sc, IC_CONFIGURE, IC_CONFIGURE | IC_HIGHLEVEL);
}

/*
 * Attempt to match and attach device classes with the specified flag
 * pattern.
 */
static void
iop_config_interrupts0(struct iop_softc *sc, int pat, int mask)
{
	struct iop_attach_args ia;
	const struct i2o_lct_entry *le;
	int i, j, nent, doit;

	nent = sc->sc_nlctent;
	for (i = 0, le = sc->sc_lct->entry; i < nent; i++, le++) {
		if ((sc->sc_lctmap[i] & IOP_LCTMAP_INUSE) != 0)
			continue;

		ia.ia_class = le16toh(le->classid) & 4095;
		doit = 0;

		for (j = 0; j < sizeof(iop_class) / sizeof(iop_class[0]); j++)
			if (ia.ia_class == iop_class[j].ic_class)
				if ((iop_class[j].ic_flags & mask) == pat) {
					doit = 1;
					break;
				}

		/*
		 * Try to configure the device if the pattern matches.  If
		 * the device is matched, mark it as being in use.
 		 */
		if (doit) {
			ia.ia_tid = le32toh(le->localtid) & 4095;
			if (config_found_sm(&sc->sc_dv, &ia, iop_print,
			    iop_submatch))
				sc->sc_lctmap[i] |= IOP_LCTMAP_INUSE;
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

static int
iop_vendor_print(void *aux, const char *pnp)
{

	if (pnp != NULL)
		printf("vendor specific extension at %s", pnp);
	return (UNCONF);
}

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
		iop_simple_cmd(sc, I2O_TID_IOP, I2O_EXEC_SYS_QUIESCE, IOP_ICTX);
		iop_simple_cmd(sc, I2O_TID_IOP, I2O_EXEC_IOP_CLEAR, IOP_ICTX);
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
 * Allocate DMA safe memory.
 */
static int
iop_alloc_dmamem(struct iop_softc *sc, int size, bus_dmamap_t *dmamap, 
		 caddr_t *kva, bus_addr_t *paddr)
{
	int rseg, rv;
	bus_dma_segment_t seg;
	
	if ((rv = bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, 
	    &seg, 1, &rseg, BUS_DMA_NOWAIT)) != 0) {
		printf("%s: dmamem_alloc = %d\n", sc->sc_dv.dv_xname, rv);
		return (rv);
	}

	if ((rv = bus_dmamem_map(sc->sc_dmat, &seg, rseg, size, kva,
	    BUS_DMA_NOWAIT | BUS_DMA_COHERENT)) != 0) {
		printf("%s: dmamem_map = %d\n", sc->sc_dv.dv_xname, rv);
		return (rv);
	}

	if ((rv = bus_dmamap_create(sc->sc_dmat, size, size, 1, 0, 
	    BUS_DMA_NOWAIT, dmamap)) != 0) {
		printf("%s: dmamap_create = %d\n", sc->sc_dv.dv_xname, rv);
		return (rv);
	}

	if ((rv = bus_dmamap_load(sc->sc_dmat, *dmamap, *kva, size, 
	    NULL, BUS_DMA_NOWAIT)) != 0) {
		printf("%s: dmamap_load = %d\n", sc->sc_dv.dv_xname, rv);
		return (rv);
	}

	*paddr = sc->sc_rep_dmamap->dm_segs[0].ds_addr;
	return (0);
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
	struct i2o_exec_outbound_init *mb;
	int i, rv;

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOWAIT | IM_NOICTX)) != 0)
		return (rv);

	mb = (struct i2o_exec_outbound_init *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_exec_outbound_init);
	mb->msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_EXEC_OUTBOUND_INIT);
	mb->msgictx = IOP_ICTX;
	mb->msgtctx = im->im_tctx;
	mb->pagesize = PAGE_SIZE;
	mb->flags = 0x80 | ((IOP_MAX_MSG_SIZE >> 2) << 16);

	status = 0;
	iop_msg_map(sc, im, (void *)&status, sizeof(status), 0);

	if ((rv = iop_msg_send(sc, im, 0)) != 0) {
		iop_msg_free(sc, NULL, im);
		return (rv);
	}

	DELAY(500000);	/* XXX */

	iop_msg_unmap(sc, im);
	iop_msg_free(sc, NULL, im);

	if (status != I2O_EXEC_OUTBOUND_INIT_IN_PROGRESS &&
	    status != I2O_EXEC_OUTBOUND_INIT_COMPLETE) {
		printf("%s: outbound queue failed to initialize (%08x)\n", 
		    sc->sc_dv.dv_xname, status);
		return (ENXIO);
	}

#ifdef I2ODEBUG
	if (status != I2O_EXEC_OUTBOUND_INIT_COMPLETE)
		printf("%s: outbound FIFO init not complete yet\n", 
		    sc->sc_dv.dv_xname);
#endif

	/* If we need to allocate DMA safe memory, do it now. */
	if (sc->sc_rep_phys == 0) {
		sc->sc_rep_size = sc->sc_maxreplycnt * IOP_MAX_MSG_SIZE;
		iop_alloc_dmamem(sc, sc->sc_rep_size, &sc->sc_rep_dmamap,
		    &sc->sc_rep, &sc->sc_rep_phys);
	}

	/* Populate the outbound FIFO. */
	for (i = sc->sc_maxreplycnt, addr = sc->sc_rep_phys; i; i--) {
		IOP_OUTL(sc, IOP_REG_OFIFO, (u_int32_t)addr);
		DELAY(10);
		addr += IOP_MAX_MSG_SIZE;
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

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOWAIT | IM_NOINTR)) != 0)
		return (rv);

	mb = (struct i2o_exec_hrt_get *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_exec_hrt_get);
	mb->msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_EXEC_HRT_GET);
	mb->msgictx = IOP_ICTX;
	mb->msgtctx = im->im_tctx;

	iop_msg_map(sc, im, hrt, size, 0);
	rv = iop_msg_send(sc, im, 5000);
	iop_msg_unmap(sc, im);
	iop_msg_free(sc, NULL, im);
	return (rv);
}

/*
 * Read the IOP's hardware resource table.  Once read, not much is done with
 * the HRT; it's stored for later retrieval by a user-space program.  Reading
 * the HRT is a required part of the IOP initalization sequence.
 */
static int
iop_hrt_get(struct iop_softc *sc)
{
	struct i2o_hrt hrthdr, *hrt;
	int size, rv;

	if ((rv = iop_hrt_get0(sc, &hrthdr, sizeof(hrthdr))) != 0)
		return (rv);

	size = (htole32(hrthdr.nentries) - 1) * sizeof(struct i2o_hrt_entry) + 
	    sizeof(struct i2o_hrt);
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
 * configuration table.  Must be called with interrupts enabled.
 */
static int
iop_lct_get0(struct iop_softc *sc, struct i2o_lct *lct, int size)
{
	struct iop_msg *im;
	struct i2o_exec_lct_notify *mb;
	int rv;

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOINTR)) != 0)
		return (rv);
		
	memset(lct, 0, size);

	mb = (struct i2o_exec_lct_notify *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_exec_lct_notify);
	mb->msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_EXEC_LCT_NOTIFY);
	mb->msgictx = IOP_ICTX;
	mb->msgtctx = im->im_tctx;
	mb->classid = I2O_CLASS_ANY;
	mb->changeindicator = 0;

	iop_msg_map(sc, im, lct, size, 0);
	if ((rv = iop_msg_enqueue(sc, im)) == 0)
		rv = iop_msg_wait(sc, im, 1000);
	iop_msg_unmap(sc, im);
	iop_msg_free(sc, NULL, im);
	return (rv);
}

/*
 * Read the IOP's logical configuration table.  Must be called with
 * interrupts enabled.
 */
int
iop_lct_get(struct iop_softc *sc)
{
	int size, rv;
	struct i2o_lct lcthdr, *lct;

	/* Determine LCT size. */
	if ((rv = iop_lct_get0(sc, &lcthdr, sizeof(lcthdr))) != 0)
		return (rv);

	size = le16toh(lcthdr.tablesize) << 2;
	if ((lct = (struct i2o_lct *)malloc(size, M_DEVBUF, M_WAITOK)) == NULL)
		return (ENOMEM);

	/* Request the entire LCT. */
	if ((rv = iop_lct_get0(sc, lct, size)) != 0) {
		free(lct, M_DEVBUF);
		return (rv);
	}

	/* Swap in the new LCT. */
	if ((rv = iop_lct_lock(sc)) != 0) {
		free(lct, M_DEVBUF);
		return (rv);
	}
	if (sc->sc_lct != NULL)
		free(sc->sc_lct, M_DEVBUF);
	sc->sc_lct = lct;
	sc->sc_nlctent = ((le16toh(sc->sc_lct->tablesize) << 2) -
	    sizeof(struct i2o_lct) + sizeof(struct i2o_lct_entry)) /
	    sizeof(struct i2o_lct_entry);
	iop_lct_unlock(sc);
	return (0);
}

/*
 * Request the specified parameter group from the target.  Must be called
 * with interrupts enabled.
 */
int
iop_params_get(struct iop_softc *sc, int tid, int group, void *buf, int size)
{
	struct iop_msg *im;
	struct i2o_util_params_get *mb;
	int rv;
	struct {
		struct	i2o_param_op_list_header olh;
		struct	i2o_param_op_all_template oat;
	} req;

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOINTR)) != 0)
		return (rv);

	mb = (struct i2o_util_params_get *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_util_params_get);
	mb->msgfunc = I2O_MSGFUNC(tid, I2O_UTIL_PARAMS_GET);
	mb->msgictx = IOP_ICTX;
	mb->msgtctx = im->im_tctx;
	mb->flags = 0;

	req.olh.count = htole16(1);
	req.olh.reserved = htole16(0);
	req.oat.operation = htole16(I2O_PARAMS_OP_FIELD_GET);
	req.oat.fieldcount = htole16(0xffff);
	req.oat.group = htole16(group);

	iop_msg_map(sc, im, &req, sizeof(req), 1);
	iop_msg_map(sc, im, buf, size, 0);

	if ((rv = iop_msg_enqueue(sc, im)) == 0)
		rv = iop_msg_wait(sc, im, 1000);
	iop_msg_unmap(sc, im);
	iop_msg_free(sc, NULL, im);
	return (rv);
}	

/*
 * Execute a simple command (no parameters) and poll on completion.
 */
int
iop_simple_cmd(struct iop_softc *sc, int tid, int function, int ictx)
{
	struct iop_msg *im;
	struct i2o_msg *mb;
	int rv;

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOWAIT | IM_NOINTR)) != 0)
		return (rv);

	mb = (struct i2o_msg *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_msg);
	mb->msgfunc = I2O_MSGFUNC(tid, function);
	mb->msgictx = ictx;
	mb->msgtctx = im->im_tctx;

	rv = iop_msg_send(sc, im, 5000);
	iop_msg_free(sc, NULL, im);
	return (rv);
}

/*
 * Post the system table to the IOP.  We don't maintain a full system table
 * as we should - it describes only "this" IOP and is built on the stack
 * here for the one time that we will use it: posting to the IOP.
 */
static int
iop_systab_set(struct iop_softc *sc)
{
	struct i2o_iop_entry systab;
	struct iop_msg *im;
	u_int32_t mema[2], ioa[2];
	struct i2o_exec_sys_tab_set *mb;
	int rv;

	memset(&systab, 0, sizeof(systab));
	systab.orgid = sc->sc_status.orgid;
	systab.iopid = htole32(le32toh(sc->sc_status.iopid) & 4095);
	systab.segnumber = sc->sc_status.segnumber;
	systab.iopcaps = sc->sc_status.iopcaps;
	systab.inboundmsgframesize = sc->sc_status.inboundmframesize;
	systab.inboundmsgportaddresslow =
	    htole32(sc->sc_memaddr + IOP_REG_IFIFO);

	/* Record private memory and I/O spaces. */
	mema[0] = htole32(sc->sc_memaddr);
	mema[1] = htole32(sc->sc_memsize);
	ioa[0] = htole32(sc->sc_ioaddr);
	ioa[1] = htole32(sc->sc_iosize);

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOWAIT | IM_NOINTR)) != 0)
		return (rv);

	mb = (struct i2o_exec_sys_tab_set *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_exec_sys_tab_set);
	mb->msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_EXEC_SYS_TAB_SET);
	mb->msgictx = IOP_ICTX;
	mb->msgtctx = im->im_tctx;
	mb->iopid = (2 + sc->sc_dv.dv_unit) & 4095;
	mb->segnumber = le32toh(sc->sc_status.segnumber) & 4095;

	iop_msg_map(sc, im, &systab, sizeof(systab), 1);
	iop_msg_map(sc, im, mema, sizeof(mema), 1);
	iop_msg_map(sc, im, ioa, sizeof(ioa), 1);

	rv = iop_msg_send(sc, im, 5000);
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
	 * IOP is now in the INIT state.  Wait no more than 5 seconds for
	 * the inbound queue to become responsive.
	 */
	DELAY(1000);
	POLL(5000, (mfa = IOP_INL(sc, IOP_REG_IFIFO)) != IOP_MFA_EMPTY);
	if (mfa == IOP_MFA_EMPTY) {
		printf("%s: reset failed\n", sc->sc_dv.dv_xname);
		return (EIO);
	}

	iop_release_mfa(sc, mfa);
	return (0);
}

/*
 * Register a new initiator.
 */
int
iop_initiator_register(struct iop_softc *sc, struct iop_initiator *ii)
{
	int i;

	/* Find a free slot.  If no slots are free, puke. */
	for (i = 0; i < IOP_MAX_INITIATORS; i++)
		if (sc->sc_itab[i] == NULL)
			break;
	if (i == IOP_MAX_INITIATORS)
		return (ENOMEM);

#ifdef notyet
	ii->ii_maxqueuecnt = IOP_MAX_PI_QUEUECNT;
	ii->ii_queuecnt = 0;
#endif
	ii->ii_ictx = i + 1;
	sc->sc_itab[i] = ii;
	return (0);
}

/*
 * Unregister an initiator.
 */
void
iop_initiator_unregister(struct iop_softc *sc, struct iop_initiator *ii)
{

#ifdef notyet
#ifdef I2ODEBUG
	if (ii->ii_queuecnt != 0)
		panic("iop_inititator_unregister: busy");
#endif
#endif
	sc->sc_itab[ii->ii_ictx - 1] = NULL;
}

/*
 * Attempt to read a reply frame from the adapter.  If we get one, deal with
 * it.
 */
static int
iop_poll(struct iop_softc *sc)
{
	struct iop_msg *im;
	struct i2o_reply *rb;
	struct iop_initiator *ii;
	u_int32_t rmfa;
	u_int off, ictx, tctx, status;

	/* Double read to account for IOP bug. */
	if ((rmfa = IOP_INL(sc, IOP_REG_OFIFO)) == IOP_MFA_EMPTY &&
	    (rmfa = IOP_INL(sc, IOP_REG_OFIFO)) == IOP_MFA_EMPTY)
		return (-1);

	off = (int)(rmfa - sc->sc_rep_phys);
	rb = (struct i2o_reply *)(sc->sc_rep + off);

	/*
	 * Perform reply queue DMA synchronisation.
	 */
	bus_dmamap_sync(sc->sc_dmat, sc->sc_rep_dmamap, off, IOP_MAX_MSG_SIZE,
	    BUS_DMASYNC_POSTREAD);
	if (--sc->sc_stat.is_cur_hwqueue != 0)
		bus_dmamap_sync(sc->sc_dmat, sc->sc_rep_dmamap,
		    0, sc->sc_rep_size, BUS_DMASYNC_PREREAD);

#ifdef I2ODEBUG
	if ((le32toh(rb->msgflags) & I2O_MSGFLAGS_64BIT) != 0)
		panic("iop_poll: 64-bit reply");
#endif
	/* 
	 * Find the initiator.
	 */
	ictx = le32toh(rb->msgictx);
	if (ictx > IOP_MAX_INITIATORS)
		panic("%s: bad ictx returned", sc->sc_dv.dv_xname);
	if (ictx == IOP_ICTX)
		ii = NULL;
	else {
#ifdef I2ODEBUG
		if (sc->sc_itab == NULL)
			panic("iop_poll: itab == NULL; ictx %d", ictx);
#endif
		if ((ii = sc->sc_itab[ictx - 1]) == NULL)
			panic("%s: bad ictx returned", sc->sc_dv.dv_xname);
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
		im = TAILQ_FIRST(IOP_MSGHASH(tctx));
		for (; im != NULL; im = TAILQ_NEXT(im, im_hash))
			if (im->im_tctx == tctx)
				break;
		if (im == NULL || (im->im_flags & IM_ALLOCED) == 0)
			panic("%s: bad tctx returned (%x, %p)",
			    sc->sc_dv.dv_xname, tctx, im);
#ifdef I2ODEBUG
		if ((im->im_flags & IM_REPLIED) != 0)
			panic("%s: duplicate reply", sc->sc_dv.dv_xname);
#endif

		im->im_flags |= IM_REPLIED;

#ifdef I2ODEBUG
		if (rb->reqstatus != 0)
			iop_reply_print(sc, im, rb);
#endif
		/* Notify the initiator. */
		if ((im->im_flags & IM_WAITING) != 0)
			wakeup(im);
		if ((im->im_flags & IM_NOINTR) == 0)
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
	IOP_OUTL(sc, IOP_REG_OFIFO, rmfa);

	/* Run the queue. */
	if ((im = SIMPLEQ_FIRST(&sc->sc_queue)) != NULL)
		iop_msg_enqueue(sc, im);

	return (status);
}

/*
 * Handle an interrupt from the adapter.
 */
int
iop_intr(void *arg)
{
	struct iop_softc *sc;
	int forus;

	sc = arg;
	forus = 0;

	/* Handle replies and dispatch enqueued messages. */
	while ((IOP_INL(sc, IOP_REG_INTR_STATUS) & IOP_INTR_OFIFO) != 0) {
		iop_poll(sc);
		forus = 1;
	}

#ifdef I2ODEBUG
	if (!forus)
		printf("%s: spurious intr\n", sc->sc_dv.dv_xname);
#endif
	return (forus);
}

/* 
 * Allocate a message wrapper.
 */
int
iop_msg_alloc(struct iop_softc *sc, struct iop_initiator *ii,
	      struct iop_msg **imp, int flags)
{
	struct iop_msg *im;
	static int tctx = 666;
	int s, rv, i;

#ifdef I2ODEBUG
	if ((flags & IM_SYSMASK) != 0)
		panic("iop_msg_alloc: system flags specified");
#endif

	s = splbio();	/* XXX */

	if (ii != NULL) {
#ifdef notyet
		/*
		 * If this initiator has exceeded it's maximum allowed queue
		 * depth, sleep until one of its currently queued commands
		 * has completed.
		 */
		if (ii->ii_queuecnt >= ii->ii_maxqueuecnt) {
			if ((flags & IM_NOWAIT) != 0) {
				splx(s);
				return (EAGAIN);
			}
			ii->ii_waitcnt++;
			tsleep(ii, PRIBIO, "iopmsg", 0);
		}
		ii->ii_queuecnt++;
#endif
		if ((ii->ii_flags & II_DISCARD) != 0)
			flags |= IM_DISCARD;
	}

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
		TAILQ_INSERT_TAIL(IOP_MSGHASH(tctx), im, im_hash);

	splx(s);

	im->im_tctx = tctx++;
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
		TAILQ_REMOVE(IOP_MSGHASH(im->im_tctx), im, im_hash);

	im->im_flags = 0;
	pool_put(iop_msgpool, im);

#ifdef notyet
	if (ii != NULL) {
		ii->ii_queuecnt--;
		if (ii->ii_waitcnt != 0) {
			wakeup_one(ii);
			ii->ii_waitcnt--;
		}
	}
#endif

	splx(s);
}

/*
 * Map a data transfer.  Write a scatter gather list into the message frame. 
 */
int
iop_msg_map(struct iop_softc *sc, struct iop_msg *im, void *xferaddr,
	    int xfersize, int out)
{
	struct iop_xfer *ix;
	u_int32_t *mb;
	int rv, seg, flg, i;
	
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

	flg = (out ? IX_OUT : IX_IN);
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
	u_int32_t mfa;
	int rv, status, i, s;

#ifdef I2ODEBUG
	if ((im->im_flags & IM_NOICTX) == 0)
		if (im->im_msg[3] == IOP_ICTX &&
		    (im->im_flags & IM_NOINTR) == 0)
			panic("iop_msg_send: IOP_ICTX and !IM_NOINTR");
	if ((im->im_flags & IM_DISCARD) != 0)
		panic("iop_msg_send: IM_DISCARD");
#endif

	im->im_tid = im->im_msg[1] & 4095;	/* XXX */

	s = splbio();	/* XXX */

	/* Wait up to 250ms for an MFA. */
	POLL(250, (mfa = IOP_INL(sc, IOP_REG_IFIFO)) != IOP_MFA_EMPTY);
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

	/* Post the MFA back to the IOP, thus starting the command. */
	IOP_OUTL(sc, IOP_REG_IFIFO, mfa);

	if (timo == 0) {
		splx(s);
		return (0);
	}

	/* Wait for completion. */
	for (timo *= 10; timo != 0; timo--) {
		if ((IOP_INL(sc, IOP_REG_INTR_STATUS) & IOP_INTR_OFIFO) != 0)
			status = iop_poll(sc);
		if ((im->im_flags & IM_REPLIED) != 0)
			break;
		DELAY(100);
	}

	splx(s);

	if (timo == 0) {
		DPRINTF(("%s: poll - no reply\n", sc->sc_dv.dv_xname));
		rv = EBUSY;
	} else if ((im->im_flags & IM_NOINTR) != 0)
		rv = (status != I2O_STATUS_SUCCESS ? EIO : 0);

	return (rv);
}

/*
 * Try to post a message to the adapter; if that's not possible, enqueue it
 * with us.
 */
int
iop_msg_enqueue(struct iop_softc *sc, struct iop_msg *im)
{
	u_int mfa;
	int s, fromqueue, i;

#ifdef I2ODEBUG
	if (im == NULL)
		panic("iop_msg_enqueue: im == NULL");
	if (sc == NULL)
		panic("iop_msg_enqueue: sc == NULL");
	if ((im->im_flags & IM_NOICTX) != 0)
		panic("iop_msg_enqueue: IM_NOICTX");
	if (im->im_msg[3] == IOP_ICTX && (im->im_flags & IM_NOINTR) == 0)
		panic("iop_msg_send: IOP_ICTX and no IM_NOINTR");
#endif

	im->im_tid = im->im_msg[1] & 4095;	/* XXX */

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
	} else {
		/* Double read to account for IOP bug. */
		if ((mfa = IOP_INL(sc, IOP_REG_IFIFO)) == IOP_MFA_EMPTY)
			mfa = IOP_INL(sc, IOP_REG_IFIFO);
	}

	if (mfa == IOP_MFA_EMPTY) {
		/* Can't transfer to h/w queue - queue with us. */
		if (!fromqueue) {
			SIMPLEQ_INSERT_TAIL(&sc->sc_queue, im, im_queue);
			if (++sc->sc_stat.is_cur_swqueue >
			    sc->sc_stat.is_peak_swqueue)
				sc->sc_stat.is_peak_swqueue =
				    sc->sc_stat.is_cur_swqueue;
		}
		splx(s);
		return (0);
	} else if (fromqueue) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_queue, im, im_queue);
		sc->sc_stat.is_cur_swqueue--;
	}

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

	/* Post the MFA back to the IOP, thus starting the command. */
	IOP_OUTL(sc, IOP_REG_IFIFO, mfa);

	/* If this is a discardable message wrapper, free it. */
	if ((im->im_flags & IM_DISCARD) != 0)
		iop_msg_free(sc, NULL, im);
	splx(s);
	return (0);
}

/*
 * Wait for the specified message to complete.  Must be called with
 * interrupts enabled.
 */
int
iop_msg_wait(struct iop_softc *sc, struct iop_msg *im, int timo)
{
	int rv;

	im->im_flags |= IM_WAITING;
	if ((im->im_flags & IM_REPLIED) != 0)
		return (0);
	rv = tsleep(im, PRIBIO, "iopmsg", timo);
	if ((im->im_flags & IM_REPLIED) != 0)
		return (0);
	return (rv);
}

/*
 * Release an unused message frame back to the IOP's inbound fifo.
 */
static void
iop_release_mfa(struct iop_softc *sc, u_int32_t mfa)
{

	/* Use the frame to issue a no-op. */
	IOP_OUTL(sc, mfa, I2O_VERSION_11 | (4 << 16));
	IOP_OUTL(sc, mfa + 4, I2O_MSGFUNC(I2O_TID_IOP, I2O_UTIL_NOP));
	IOP_OUTL(sc, mfa + 8, 0);
	IOP_OUTL(sc, mfa + 12, 0);

	IOP_OUTL(sc, IOP_REG_IFIFO, mfa);
}

#ifdef I2ODEBUG
/*
 * Print status information from a failure reply frame.
 */
static void
iop_reply_print(struct iop_softc *sc, struct iop_msg *im,
		struct i2o_reply *rb)
{
	u_int cmd, detail;
#ifdef I2OVERBOSE
	const char *statusstr;
#endif

#ifdef I2ODEBUG
	if ((im->im_flags & IM_REPLIED) == 0)
		panic("iop_msg_print_status: %p not replied to", im);
#endif

	cmd = le32toh(rb->msgflags) >> 24;
	detail = le16toh(rb->detail);

#ifdef I2OVERBOSE
	if (rb->reqstatus < sizeof(iop_status) / sizeof(iop_status[0]))
		statusstr = iop_status[rb->reqstatus];
	else
		statusstr = "undefined error code";

	printf("%s: tid=%d cmd=0x%02x: status=0x%02x (%s) detail=0x%04x\n", 
	    sc->sc_dv.dv_xname, im->im_tid, cmd, rb->reqstatus, statusstr,
	    detail);
#else
	printf("%s: tid=%d cmd=0x%02x: status=0x%02x detail=0x%04x\n", 
	    sc->sc_dv.dv_xname, im->im_tid, cmd, rb->reqstatus, detail);
#endif
}
#endif

/*
 * Wait for an exclusive lock on the LCT.
 */
int
iop_lct_lock(struct iop_softc *sc)
{
	int rv;

	while ((sc->sc_flags & IOP_LCTLKHELD) != 0)
		if ((rv = tsleep(sc, PRIBIO | PCATCH, "ioplct", 0)) != 0)
			return (rv);
	sc->sc_flags |= IOP_LCTLKHELD;
	return (0);
}

/*
 * Unlock and wake up any waiters.
 */
void
iop_lct_unlock(struct iop_softc *sc)
{

	sc->sc_flags &= ~IOP_LCTLKHELD;
	wakeup_one(sc);
}

/*
 * Translate an I2O ASCII string into a C string.
 *
 * XXX Doesn't belong here.
 */
void
iop_strvis(const char *src, int slen, char *dst, int dlen)
{
	int hc, lc, i;

	dlen--;
	lc = 0;
	hc = 0;
	i = 0;
	
	while (slen-- && dlen--) {
		if (*src <= 0x20 || *src >= 0x7f) {
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
 * Return the index of the LCT entry matching the specified TID.
 */
int
iop_tid_lct_index(struct iop_softc *sc, int tid)
{
	const struct i2o_lct_entry *le;
	int i;

	for (i = 0, le = sc->sc_lct->entry; i < sc->sc_nlctent; i++, le++)
		if ((le32toh(le->localtid) & 4095) == tid)
			return (i);

	return (-1);
}

/*
 * Determine whether the specified target is in use by an OSM (or in turn,
 * by a DDM).  Return a positive non-zero value on error, zero if the TID is
 * in use and a negative non-zero value if the TID is not in use.
 */
int
iop_tid_inuse(struct iop_softc *sc, int tid)
{
	int i;
	
	if ((i = iop_tid_lct_index(sc, tid)) < 0)
		return (ENXIO);
	return (-((sc->sc_lctmap[i] & IOP_LCTMAP_INUSE) == 0));
}

/*
 * Mark all targets used by the specified target as in use.
 */
void
iop_tid_markallused(struct iop_softc *sc, int tid)
{
	const struct i2o_lct_entry *le;
	int i;

	for (i = 0, le = sc->sc_lct->entry; i < sc->sc_nlctent; i++, le++)
		if ((le32toh(le->usertid) & 4095) == tid) {
#ifdef I2ODEBUG
			if ((sc->sc_lctmap[i] & IOP_LCTMAP_INUSE) != 0)
				panic("iop_tid_markallused: multiple use\n");
#endif
			sc->sc_lctmap[i] |= IOP_LCTMAP_INUSE;
		}
}

/*
 * Claim the specified TID.  Must be called with interrupts enabled.
 */
int
iop_tid_claim(struct iop_softc *sc, int tid, int ictx, int flags)
{
	struct iop_msg *im;
	struct i2o_util_claim *mb;
	int rv;

	if ((rv = iop_msg_alloc(sc, NULL, &im, IM_NOINTR)) != 0)
		return (rv);

	mb = (struct i2o_util_claim *)im->im_msg;
	mb->msgflags = I2O_MSGFLAGS(i2o_util_claim);
	mb->msgfunc = I2O_MSGFUNC(tid, I2O_UTIL_CLAIM);
	mb->msgictx = ictx;
	mb->msgtctx = im->im_tctx;
	mb->flags = flags;

	if ((rv = iop_msg_enqueue(sc, im)) == 0)
		rv = iop_msg_wait(sc, im, 1000);
	iop_msg_free(sc, NULL, im);
	return (rv);
}	
