/*	$NetBSD: uha.c,v 1.8 1997/06/06 23:31:05 thorpej Exp $	*/

#undef UHADEBUG
#ifdef DDB
#define	integrate
#else
#define	integrate	static inline
#endif

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright (c) 1994, 1996, 1997 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
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

/*
 * Ported for use with the UltraStor 14f by Gary Close (gclose@wvnvms.wvnet.edu)
 * Slight fixes to timeouts to run with the 34F
 * Thanks to Julian Elischer for advice and help with this port.
 *
 * Originally written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * commenced: Sun Sep 27 18:14:01 PDT 1992
 * slight mod to make work with 34F as well: Wed Jun  2 18:05:48 WST 1993
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#include <dev/ic/uhareg.h>
#include <dev/ic/uhavar.h>

#ifndef	DDB
#define Debugger() panic("should call debugger here (uha.c)")
#endif /* ! DDB */

#define	UHA_MAXXFER	((UHA_NSEG - 1) << PGSHIFT)

integrate void uha_reset_mscp __P((struct uha_softc *, struct uha_mscp *));
void uha_free_mscp __P((struct uha_softc *, struct uha_mscp *));
integrate void uha_init_mscp __P((struct uha_softc *, struct uha_mscp *));
struct uha_mscp *uha_get_mscp __P((struct uha_softc *, int));
void uhaminphys __P((struct buf *));
int uha_scsi_cmd __P((struct scsi_xfer *));
int uha_create_mscps __P((struct uha_softc *, void *, size_t));

struct scsi_adapter uha_switch = {
	uha_scsi_cmd,
	uhaminphys,
	0,
	0,
};

/* the below structure is so we have a default dev struct for out link struct */
struct scsi_device uha_dev = {
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
};

struct cfdriver uha_cd = {
	NULL, "uha", DV_DULL
};

#define	UHA_ABORT_TIMEOUT	2000	/* time to wait for abort (mSec) */

/* XXX Should put this in a better place. */
#define	offsetof(type, member)	((size_t)(&((type *)0)->member))

/*
 * Attach all the sub-devices we can find
 */
void
uha_attach(sc, upd)
	struct uha_softc *sc;
	struct uha_probe_data *upd;
{

	TAILQ_INIT(&sc->sc_free_mscp);

	(sc->init)(sc);

	/*
	 * fill in the prototype scsi_link.
	 */
	sc->sc_link.channel = SCSI_CHANNEL_ONLY_ONE;
	sc->sc_link.adapter_softc = sc;
	sc->sc_link.adapter_target = upd->sc_scsi_dev;
	sc->sc_link.adapter = &uha_switch;
	sc->sc_link.device = &uha_dev;
	sc->sc_link.openings = 2;
	sc->sc_link.max_target = 7;

	/*
	 * ask the adapter what subunits are present
	 */
	config_found(&sc->sc_dev, &sc->sc_link, scsiprint);
}

integrate void
uha_reset_mscp(sc, mscp)
	struct uha_softc *sc;
	struct uha_mscp *mscp;
{

	mscp->flags = 0;
}

/*
 * A mscp (and hence a mbx-out) is put onto the free list.
 */
void
uha_free_mscp(sc, mscp)
	struct uha_softc *sc;
	struct uha_mscp *mscp;
{
	int s;

	s = splbio();

	uha_reset_mscp(sc, mscp);
	TAILQ_INSERT_HEAD(&sc->sc_free_mscp, mscp, chain);

	/*
	 * If there were none, wake anybody waiting for one to come free,
	 * starting with queued entries.
	 */
	if (mscp->chain.tqe_next == 0)
		wakeup(&sc->sc_free_mscp);

	splx(s);
}

integrate void
uha_init_mscp(sc, mscp)
	struct uha_softc *sc;
	struct uha_mscp *mscp;
{
	bus_dma_tag_t dmat = sc->sc_dmat;
	int hashnum;

	/*
	 * XXX Should we put a DIAGNOSTIC check for multiple
	 * XXX MSCP inits here?
	 */

	bzero(mscp, sizeof(struct uha_mscp));

	/*
	 * Create the DMA maps for this MSCP.
	 */
	if (bus_dmamap_create(dmat, sizeof(struct uha_mscp), 1,
	    sizeof(struct uha_mscp), 0, BUS_DMA_NOWAIT | sc->sc_dmaflags,
	    &mscp->dmamap_self) ||

					/* XXX What's a good value for this? */
	    bus_dmamap_create(dmat, UHA_MAXXFER, UHA_NSEG, UHA_MAXXFER,
	    0, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW | sc->sc_dmaflags,
	    &mscp->dmamap_xfer))
		panic("uha_init_mscp: can't create DMA maps");

	/*
	 * Load the permanent DMA maps.
	 */
	if (bus_dmamap_load(dmat, mscp->dmamap_self, mscp,
	    sizeof(struct uha_mscp), NULL, BUS_DMA_NOWAIT))
		panic("uha_init_mscp: can't load permanent maps");

	/*
	 * put in the phystokv hash table
	 * Never gets taken out.
	 */
	mscp->hashkey = mscp->dmamap_self->dm_segs[0].ds_addr;
	hashnum = MSCP_HASH(mscp->hashkey);
	mscp->nexthash = sc->sc_mscphash[hashnum];
	sc->sc_mscphash[hashnum] = mscp;
	uha_reset_mscp(sc, mscp);
}

/*
 * Create a set of MSCPs and add them to the free list.
 */
int
uha_create_mscps(sc, mem, size)
	struct uha_softc *sc;
	void *mem;
	size_t size;
{
	bus_dma_segment_t seg;
	struct uha_mscp *mscp;
	int rseg, error;

	if (sc->sc_nummscps >= UHA_MSCP_MAX)
		return (0);

	if ((mscp = mem) != NULL)
		goto have_mem;

	size = NBPG;
	error = bus_dmamem_alloc(sc->sc_dmat, size, NBPG, 0, &seg, 1, &rseg,
	    BUS_DMA_NOWAIT);
	if (error)
		return (error);

	error = bus_dmamem_map(sc->sc_dmat, &seg, rseg, size,
	    (caddr_t *)&mscp, BUS_DMA_NOWAIT|BUS_DMAMEM_NOSYNC);
	if (error) {
		bus_dmamem_free(sc->sc_dmat, &seg, rseg);
		return (error);
	}

 have_mem:
	bzero(mscp, size);
	while (size > sizeof(struct uha_mscp)) {
		uha_init_mscp(sc, mscp);
		sc->sc_nummscps++;
		TAILQ_INSERT_TAIL(&sc->sc_free_mscp, mscp, chain);
		(caddr_t)mscp += ALIGN(sizeof(struct uha_mscp));
		size -= ALIGN(sizeof(struct uha_mscp));
		if (sc->sc_nummscps >= UHA_MSCP_MAX)
			break;
	}

	return (0);
}

/*
 * Get a free mscp
 *
 * If there are none, see if we can allocate a new one.  If so, put it in the
 * hash table too otherwise either return an error or sleep.
 */
struct uha_mscp *
uha_get_mscp(sc, flags)
	struct uha_softc *sc;
	int flags;
{
	struct uha_mscp *mscp;
	int s;

	s = splbio();

	/*
	 * If we can and have to, sleep waiting for one to come free
	 * but only if we can't allocate a new one
	 */
	for (;;) {
		mscp = sc->sc_free_mscp.tqh_first;
		if (mscp) {
			TAILQ_REMOVE(&sc->sc_free_mscp, mscp, chain);
			break;
		}
		if (sc->sc_nummscps < UHA_MSCP_MAX) {
			if (uha_create_mscps(sc, NULL, 0)) {
				printf("%s: can't allocate mscps\n",
				    sc->sc_dev.dv_xname);
				goto out;
			}
			continue;
		}
		if ((flags & SCSI_NOSLEEP) != 0)
			goto out;
		tsleep(&sc->sc_free_mscp, PRIBIO, "uhamsc", 0);
	}

	mscp->flags |= MSCP_ALLOC;

out:
	splx(s);
	return (mscp);
}

/*
 * given a physical address, find the mscp that it corresponds to.
 */
struct uha_mscp *
uha_mscp_phys_kv(sc, mscp_phys)
	struct uha_softc *sc;
	u_long mscp_phys;
{
	int hashnum = MSCP_HASH(mscp_phys);
	struct uha_mscp *mscp = sc->sc_mscphash[hashnum];

	while (mscp) {
		if (mscp->hashkey == mscp_phys)
			break;
		mscp = mscp->nexthash;
	}
	return (mscp);
}

/*
 * We have a mscp which has been processed by the adaptor, now we look to see
 * how the operation went.
 */
void
uha_done(sc, mscp)
	struct uha_softc *sc;
	struct uha_mscp *mscp;
{
	bus_dma_tag_t dmat = sc->sc_dmat;
	struct scsi_sense_data *s1, *s2;
	struct scsi_xfer *xs = mscp->xs;

	SC_DEBUG(xs->sc_link, SDEV_DB2, ("uha_done\n"));

	/*
	 * If we were a data transfer, unload the map that described
	 * the data buffer.
	 */
	if (xs->datalen) {
		bus_dmamap_sync(dmat, mscp->dmamap_xfer,
		    (xs->flags & SCSI_DATA_IN) ? BUS_DMASYNC_POSTREAD :
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(dmat, mscp->dmamap_xfer);
	}

	/*
	 * Otherwise, put the results of the operation
	 * into the xfer and call whoever started it
	 */
	if ((mscp->flags & MSCP_ALLOC) == 0) {
		printf("%s: exiting ccb not allocated!\n", sc->sc_dev.dv_xname);
		Debugger();
		return;
	}
	if (xs->error == XS_NOERROR) {
		if (mscp->host_stat != UHA_NO_ERR) {
			switch (mscp->host_stat) {
			case UHA_SBUS_TIMEOUT:		/* No response */
				xs->error = XS_SELTIMEOUT;
				break;
			default:	/* Other scsi protocol messes */
				printf("%s: host_stat %x\n",
				    sc->sc_dev.dv_xname, mscp->host_stat);
				xs->error = XS_DRIVER_STUFFUP;
			}
		} else if (mscp->target_stat != SCSI_OK) {
			switch (mscp->target_stat) {
			case SCSI_CHECK:
				s1 = &mscp->mscp_sense;
				s2 = &xs->sense;
				*s2 = *s1;
				xs->error = XS_SENSE;
				break;
			case SCSI_BUSY:
				xs->error = XS_BUSY;
				break;
			default:
				printf("%s: target_stat %x\n",
				    sc->sc_dev.dv_xname, mscp->target_stat);
				xs->error = XS_DRIVER_STUFFUP;
			}
		} else
			xs->resid = 0;
	}
	uha_free_mscp(sc, mscp);
	xs->flags |= ITSDONE;
	scsi_done(xs);
}

void
uhaminphys(bp)
	struct buf *bp;
{

	if (bp->b_bcount > UHA_MAXXFER)
		bp->b_bcount = UHA_MAXXFER;
	minphys(bp);
}

/*
 * start a scsi operation given the command and the data address.  Also
 * needs the unit, target and lu.
 */
int
uha_scsi_cmd(xs)
	struct scsi_xfer *xs;
{
	struct scsi_link *sc_link = xs->sc_link;
	struct uha_softc *sc = sc_link->adapter_softc;
	bus_dma_tag_t dmat = sc->sc_dmat;
	struct uha_mscp *mscp;
	struct uha_dma_seg *sg;
	int error, seg, flags, s;

	SC_DEBUG(sc_link, SDEV_DB2, ("uha_scsi_cmd\n"));
	/*
	 * get a mscp (mbox-out) to use. If the transfer
	 * is from a buf (possibly from interrupt time)
	 * then we can't allow it to sleep
	 */
	flags = xs->flags;
	if ((mscp = uha_get_mscp(sc, flags)) == NULL) {
		xs->error = XS_DRIVER_STUFFUP;
		return (TRY_AGAIN_LATER);
	}
	mscp->xs = xs;
	mscp->timeout = xs->timeout;

	/*
	 * Put all the arguments for the xfer in the mscp
	 */
	if (flags & SCSI_RESET) {
		mscp->opcode = UHA_SDR;
		mscp->ca = 0x01;
	} else {
		mscp->opcode = UHA_TSP;
		/* XXX Not for tapes. */
		mscp->ca = 0x01;
		bcopy(xs->cmd, &mscp->scsi_cmd, mscp->scsi_cmd_length);
	}
	mscp->xdir = UHA_SDET;
	mscp->dcn = 0x00;
	mscp->chan = 0x00;
	mscp->target = sc_link->target;
	mscp->lun = sc_link->lun;
	mscp->scsi_cmd_length = xs->cmdlen;
	mscp->sense_ptr = mscp->dmamap_self->dm_segs[0].ds_addr +
	    offsetof(struct uha_mscp, mscp_sense);
	mscp->req_sense_length = sizeof(mscp->mscp_sense);
	mscp->host_stat = 0x00;
	mscp->target_stat = 0x00;

	if (xs->datalen) {
		sg = mscp->uha_dma;
		seg = 0;
#ifdef	TFS
		if (flags & SCSI_DATA_UIO) {
			error = bus_dmamap_load_uio(dmat,
			    mscp->dmamap_xfer, (struct uio *)xs->data,
			    (flags & SCSI_NOSLEEP) ? BUS_DMA_NOWAIT :
			    BUS_DMA_WAITOK);
		} else
#endif /*TFS */
		{
			error = bus_dmamap_load(dmat,
			    mscp->dmamap_xfer, xs->data, xs->datalen, NULL,
			    (flags & SCSI_NOSLEEP) ? BUS_DMA_NOWAIT :
			    BUS_DMA_WAITOK);
		}

		if (error) {
			if (error == EFBIG) {
				printf("%s: uha_scsi_cmd, more than %d"
				    " dma segments\n",
				    sc->sc_dev.dv_xname, UHA_NSEG);
			} else {
				printf("%s: uha_scsi_cmd, error %d loading"
				    " dma map\n",
				    sc->sc_dev.dv_xname, error);
			}
			goto bad;
		}

		bus_dmamap_sync(dmat, mscp->dmamap_xfer,
		    (flags & SCSI_DATA_IN) ? BUS_DMASYNC_PREREAD :
		    BUS_DMASYNC_PREWRITE);

		/*
		 * Load the hardware scatter/gather map with the
		 * contents of the DMA map.
		 */
		for (seg = 0; seg < mscp->dmamap_xfer->dm_nsegs; seg++) {
			mscp->uha_dma[seg].seg_addr =
			    mscp->dmamap_xfer->dm_segs[seg].ds_addr;
			mscp->uha_dma[seg].seg_len =
			    mscp->dmamap_xfer->dm_segs[seg].ds_len;
		}

		mscp->data_addr = mscp->dmamap_self->dm_segs[0].ds_addr +
		    offsetof(struct uha_mscp, uha_dma);
		mscp->data_length = xs->datalen;
		mscp->sgth = 0x01;
		mscp->sg_num = seg;
	} else {		/* No data xfer, use non S/G values */
		mscp->data_addr = (physaddr)0;
		mscp->data_length = 0;
		mscp->sgth = 0x00;
		mscp->sg_num = 0;
	}
	mscp->link_id = 0;
	mscp->link_addr = (physaddr)0;

	s = splbio();
	(sc->start_mbox)(sc, mscp);
	splx(s);

	/*
	 * Usually return SUCCESSFULLY QUEUED
	 */
	if ((flags & SCSI_POLL) == 0)
		return (SUCCESSFULLY_QUEUED);

	/*
	 * If we can't use interrupts, poll on completion
	 */
	if ((sc->poll)(sc, xs, mscp->timeout)) {
		uha_timeout(mscp);
		if ((sc->poll)(sc, xs, mscp->timeout))
			uha_timeout(mscp);
	}
	return (COMPLETE);

bad:
	xs->error = XS_DRIVER_STUFFUP;
	uha_free_mscp(sc, mscp);
	return (COMPLETE);
}

void
uha_timeout(arg)
	void *arg;
{
	struct uha_mscp *mscp = arg;
	struct scsi_xfer *xs = mscp->xs;
	struct scsi_link *sc_link = xs->sc_link;
	struct uha_softc *sc = sc_link->adapter_softc;
	int s;

	sc_print_addr(sc_link);
	printf("timed out");

	s = splbio();

	if (mscp->flags & MSCP_ABORT) {
		/* abort timed out */
		printf(" AGAIN\n");
		/* XXX Must reset! */
	} else {
		/* abort the operation that has timed out */
		printf("\n");
		mscp->xs->error = XS_TIMEOUT;
		mscp->timeout = UHA_ABORT_TIMEOUT;
		mscp->flags |= MSCP_ABORT;
		(sc->start_mbox)(sc, mscp);
	}

	splx(s);
}
