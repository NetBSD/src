/*	$NetBSD: ahb.c,v 1.8.2.3 1997/05/18 17:38:50 thorpej Exp $	*/

#undef	AHBDEBUG
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

#include <dev/eisa/eisareg.h>
#include <dev/eisa/eisavar.h>
#include <dev/eisa/eisadevs.h>
#include <dev/eisa/ahbreg.h>

#ifndef DDB
#define Debugger() panic("should call debugger here (aha1742.c)")
#endif /* ! DDB */

#define AHB_ECB_MAX	32	/* store up to 32 ECBs at one time */
#define	ECB_HASH_SIZE	32	/* hash table size for phystokv */
#define	ECB_HASH_SHIFT	9
#define ECB_HASH(x)	((((long)(x))>>ECB_HASH_SHIFT) & (ECB_HASH_SIZE - 1))

#define AHB_MAXXFER	((AHB_NSEG - 1) << PGSHIFT)

struct ahb_softc {
	struct device sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_dma_tag_t sc_dmat;
	void *sc_ih;

	struct ahb_ecb *sc_ecbhash[ECB_HASH_SIZE];
	TAILQ_HEAD(, ahb_ecb) sc_free_ecb;
	struct ahb_ecb *sc_immed_ecb;	/* an outstanding immediete command */
	int sc_numecbs;
	struct scsi_link sc_link;
};

struct ahb_probe_data {
	int sc_irq;
	int sc_scsi_dev;
};

void	ahb_send_mbox __P((struct ahb_softc *, int, struct ahb_ecb *));
void	ahb_send_immed __P((struct ahb_softc *, u_long, struct ahb_ecb *));
int	ahbintr __P((void *));
void	ahb_free_ecb __P((struct ahb_softc *, struct ahb_ecb *));
struct	ahb_ecb *ahb_get_ecb __P((struct ahb_softc *, int));
struct	ahb_ecb *ahb_ecb_phys_kv __P((struct ahb_softc *, physaddr));
void	ahb_done __P((struct ahb_softc *, struct ahb_ecb *));
int	ahb_find __P((bus_space_tag_t, bus_space_handle_t, struct ahb_probe_data *));
void	ahb_init __P((struct ahb_softc *));
void	ahbminphys __P((struct buf *));
int	ahb_scsi_cmd __P((struct scsi_xfer *));
int	ahb_poll __P((struct ahb_softc *, struct scsi_xfer *, int));
void	ahb_timeout __P((void *));
int	ahb_create_ecbs __P((struct ahb_softc *));

integrate void ahb_reset_ecb __P((struct ahb_softc *, struct ahb_ecb *));
integrate void ahb_init_ecb __P((struct ahb_softc *, struct ahb_ecb *));

struct scsi_adapter ahb_switch = {
	ahb_scsi_cmd,
	ahbminphys,
	0,
	0,
};

/* the below structure is so we have a default dev struct for our link struct */
struct scsi_device ahb_dev = {
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
};

#ifdef __BROKEN_INDIRECT_CONFIG
int	ahbmatch __P((struct device *, void *, void *));
#else
int	ahbmatch __P((struct device *, struct cfdata *, void *));
#endif
void	ahbattach __P((struct device *, struct device *, void *));

struct cfattach ahb_ca = {
	sizeof(struct ahb_softc), ahbmatch, ahbattach
};

struct cfdriver ahb_cd = {
	NULL, "ahb", DV_DULL
};

#define	AHB_ABORT_TIMEOUT	2000	/* time to wait for abort (mSec) */

/* XXX Should put this in a better place. */
#define	offsetof(type, member)	((size_t)(&((type *)0)->member))

/*
 * Check the slots looking for a board we recognise
 * If we find one, note it's address (slot) and call
 * the actual probe routine to check it out.
 */
int
ahbmatch(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct eisa_attach_args *ea = aux;
	bus_space_tag_t iot = ea->ea_iot;
	bus_space_handle_t ioh;
	int rv;

	/* must match one of our known ID strings */
	if (strcmp(ea->ea_idstring, "ADP0000") &&
	    strcmp(ea->ea_idstring, "ADP0001") &&
	    strcmp(ea->ea_idstring, "ADP0002") &&
	    strcmp(ea->ea_idstring, "ADP0400"))
		return (0);

	if (bus_space_map(iot, EISA_SLOT_ADDR(ea->ea_slot),
	    EISA_SLOT_SIZE, 0, &ioh))
		return (0);

	rv = !ahb_find(iot, ioh, NULL);

	bus_space_unmap(iot, ioh, EISA_SLOT_SIZE);

	return (rv);
}

/*
 * Attach all the sub-devices we can find
 */
void
ahbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct eisa_attach_args *ea = aux;
	struct ahb_softc *sc = (void *)self;
	bus_space_tag_t iot = ea->ea_iot;
	bus_space_handle_t ioh;
	eisa_chipset_tag_t ec = ea->ea_ec;
	eisa_intr_handle_t ih;
	const char *model, *intrstr;
	struct ahb_probe_data apd;

	if (!strcmp(ea->ea_idstring, "ADP0000"))
		model = EISA_PRODUCT_ADP0000;
	else if (!strcmp(ea->ea_idstring, "ADP0001"))
		model = EISA_PRODUCT_ADP0001;
	else if (!strcmp(ea->ea_idstring, "ADP0002"))
		model = EISA_PRODUCT_ADP0002;
	else if (!strcmp(ea->ea_idstring, "ADP0400"))
		model = EISA_PRODUCT_ADP0400;
	else
		model = "unknown model!";
	printf(": %s\n", model);

	if (bus_space_map(iot, EISA_SLOT_ADDR(ea->ea_slot),
	   EISA_SLOT_SIZE, 0, &ioh))
		panic("ahbattach: could not map I/O addresses");

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_dmat = ea->ea_dmat;
	if (ahb_find(iot, ioh, &apd))
		panic("ahbattach: ahb_find failed!");

	ahb_init(sc);
	TAILQ_INIT(&sc->sc_free_ecb);

	/*
	 * fill in the prototype scsi_link.
	 */
	sc->sc_link.channel = SCSI_CHANNEL_ONLY_ONE;
	sc->sc_link.adapter_softc = sc;
	sc->sc_link.adapter_target = apd.sc_scsi_dev;
	sc->sc_link.adapter = &ahb_switch;
	sc->sc_link.device = &ahb_dev;
	sc->sc_link.openings = 4;
	sc->sc_link.max_target = 7;

	if (eisa_intr_map(ec, apd.sc_irq, &ih)) {
		printf("%s: couldn't map interrupt (%d)\n",
		    sc->sc_dev.dv_xname, apd.sc_irq);
		return;
	}
	intrstr = eisa_intr_string(ec, ih);
	sc->sc_ih = eisa_intr_establish(ec, ih, IST_LEVEL, IPL_BIO,
	    ahbintr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	if (intrstr != NULL)
		printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname,
		    intrstr);

	/*
	 * ask the adapter what subunits are present
	 */
	config_found(self, &sc->sc_link, scsiprint);
}

/*
 * Function to send a command out through a mailbox
 */
void
ahb_send_mbox(sc, opcode, ecb)
	struct ahb_softc *sc;
	int opcode;
	struct ahb_ecb *ecb;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int wait = 300;	/* 1ms should be enough */

	while (--wait) {
		if ((bus_space_read_1(iot, ioh, G2STAT) & (G2STAT_BUSY | G2STAT_MBOX_EMPTY))
		    == (G2STAT_MBOX_EMPTY))
			break;
		delay(10);
	}
	if (!wait) {
		printf("%s: board not responding\n", sc->sc_dev.dv_xname);
		Debugger();
	}

	/*
	 * don't know if this will work.
	 * XXX WHAT DOES THIS COMMENT MEAN?!  --thorpej
	 */
	bus_space_write_4(iot, ioh, MBOXOUT0,
	    ecb->dmamap_self->dm_segs[0].ds_addr);
	bus_space_write_1(iot, ioh, ATTN, opcode | ecb->xs->sc_link->target);

	if ((ecb->xs->flags & SCSI_POLL) == 0)
		timeout(ahb_timeout, ecb, (ecb->timeout * hz) / 1000);
}

/*
 * Function to  send an immediate type command to the adapter
 */
void
ahb_send_immed(sc, cmd, ecb)
	struct ahb_softc *sc;
	u_long cmd;
	struct ahb_ecb *ecb;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int wait = 100;	/* 1 ms enough? */

	while (--wait) {
		if ((bus_space_read_1(iot, ioh, G2STAT) & (G2STAT_BUSY | G2STAT_MBOX_EMPTY))
		    == (G2STAT_MBOX_EMPTY))
			break;
		delay(10);
	}
	if (!wait) {
		printf("%s: board not responding\n", sc->sc_dev.dv_xname);
		Debugger();
	}

	bus_space_write_4(iot, ioh, MBOXOUT0, cmd);	/* don't know this will work */
	bus_space_write_1(iot, ioh, G2CNTRL, G2CNTRL_SET_HOST_READY);
	bus_space_write_1(iot, ioh, ATTN, OP_IMMED | ecb->xs->sc_link->target);

	if ((ecb->xs->flags & SCSI_POLL) == 0)
		timeout(ahb_timeout, ecb, (ecb->timeout * hz) / 1000);
}

/*
 * Catch an interrupt from the adaptor
 */
int
ahbintr(arg)
	void *arg;
{
	struct ahb_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct ahb_ecb *ecb;
	u_char ahbstat;
	u_long mboxval;

#ifdef	AHBDEBUG
	printf("%s: ahbintr ", sc->sc_dev.dv_xname);
#endif /* AHBDEBUG */

	if ((bus_space_read_1(iot, ioh, G2STAT) & G2STAT_INT_PEND) == 0)
		return 0;

	for (;;) {
		/*
		 * First get all the information and then
		 * acknowlege the interrupt
		 */
		ahbstat = bus_space_read_1(iot, ioh, G2INTST);
		mboxval = bus_space_read_4(iot, ioh, MBOXIN0);
		bus_space_write_1(iot, ioh, G2CNTRL, G2CNTRL_CLEAR_EISA_INT);

#ifdef	AHBDEBUG
		printf("status = 0x%x ", ahbstat);
#endif /* AHBDEBUG */

		/*
		 * Process the completed operation
		 */
		switch (ahbstat & G2INTST_INT_STAT) {
		case AHB_ECB_OK:
		case AHB_ECB_RECOVERED:
		case AHB_ECB_ERR:
			ecb = ahb_ecb_phys_kv(sc, mboxval);
			if (!ecb) {
				printf("%s: BAD ECB RETURNED!\n",
				    sc->sc_dev.dv_xname);
				goto next;	/* whatever it was, it'll timeout */
			}
			break;

		case AHB_IMMED_ERR:
			ecb = sc->sc_immed_ecb;
			sc->sc_immed_ecb = 0;
			ecb->flags |= ECB_IMMED_FAIL;
			break;

		case AHB_IMMED_OK:
			ecb = sc->sc_immed_ecb;
			sc->sc_immed_ecb = 0;
			break;

		default:
			printf("%s: unexpected interrupt %x\n",
			    sc->sc_dev.dv_xname, ahbstat);
			goto next;
		}

		untimeout(ahb_timeout, ecb);
		ahb_done(sc, ecb);

	next:
		if ((bus_space_read_1(iot, ioh, G2STAT) & G2STAT_INT_PEND) == 0)
			return 1;
	}
}

integrate void
ahb_reset_ecb(sc, ecb)
	struct ahb_softc *sc;
	struct ahb_ecb *ecb;
{

	ecb->flags = 0;
}

/*
 * A ecb (and hence a mbx-out is put onto the
 * free list.
 */
void
ahb_free_ecb(sc, ecb)
	struct ahb_softc *sc;
	struct ahb_ecb *ecb;
{
	int s;

	s = splbio();

	ahb_reset_ecb(sc, ecb);
	TAILQ_INSERT_HEAD(&sc->sc_free_ecb, ecb, chain);

	/*
	 * If there were none, wake anybody waiting for one to come free,
	 * starting with queued entries.
	 */
	if (ecb->chain.tqe_next == 0)
		wakeup(&sc->sc_free_ecb);

	splx(s);
}

/*
 * Create a set of ecbs and add them to the free list.
 */
integrate void
ahb_init_ecb(sc, ecb)
	struct ahb_softc *sc;
	struct ahb_ecb *ecb;
{
	bus_dma_tag_t dmat = sc->sc_dmat;
	int hashnum;

	/*
	 * XXX Should we put a DIAGNOSTIC check for multiple
	 * XXX ECB inits here?
	 */

	bzero(ecb, sizeof(struct ahb_ecb));

	/*
	 * Create the DMA maps for this ECB.
	 */
	if (bus_dmamap_create(dmat, sizeof(struct ahb_ecb), 1, 
	    sizeof(struct ahb_ecb), 0, BUS_DMA_NOWAIT, &ecb->dmamap_self) ||

					/* XXX What's a good value for this? */
	    bus_dmamap_create(dmat, AHB_MAXXFER, AHB_NSEG, AHB_MAXXFER,
	    0, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW, &ecb->dmamap_xfer))
		panic("ahb_init_ecb: can't create DMA maps");

	/*
	 * Load the permanent DMA maps.
	 */
	if (bus_dmamap_load(dmat, ecb->dmamap_self, ecb,
	    sizeof(struct ahb_ecb), NULL, BUS_DMA_NOWAIT))
		panic("ahb_init_ecb: can't load permanent maps");

	/*
	 * put in the phystokv hash table
	 * Never gets taken out.
	 */
	ecb->hashkey = ecb->dmamap_self->dm_segs[0].ds_addr;
	hashnum = ECB_HASH(ecb->hashkey);
	ecb->nexthash = sc->sc_ecbhash[hashnum];
	sc->sc_ecbhash[hashnum] = ecb;
	ahb_reset_ecb(sc, ecb);
}

int
ahb_create_ecbs(sc)
	struct ahb_softc *sc;
{
	bus_dma_segment_t seg;
	bus_size_t size;
	struct ahb_ecb *ecb;
	int rseg, error;

	size = NBPG;
	error = bus_dmamem_alloc(sc->sc_dmat, size, NBPG, 0, &seg, 1, &rseg,
	    BUS_DMA_NOWAIT);
	if (error)
		return (error);

	error = bus_dmamem_map(sc->sc_dmat, &seg, rseg, size,
	    (caddr_t *)&ecb, BUS_DMA_NOWAIT|BUS_DMAMEM_NOSYNC);
	if (error) {
		bus_dmamem_free(sc->sc_dmat, &seg, rseg);
		return (error);
	}

	bzero(ecb, size);
	while (size > sizeof(struct ahb_ecb)) {
		ahb_init_ecb(sc, ecb);
		sc->sc_numecbs++;
		TAILQ_INSERT_TAIL(&sc->sc_free_ecb, ecb, chain);
		(caddr_t)ecb += ALIGN(sizeof(struct ahb_ecb));
		size -= ALIGN(sizeof(struct ahb_ecb));
	}

	return (0);
}

/*
 * Get a free ecb
 *
 * If there are none, see if we can allocate a new one. If so, put it in the
 * hash table too otherwise either return an error or sleep.
 */
struct ahb_ecb *
ahb_get_ecb(sc, flags)
	struct ahb_softc *sc;
	int flags;
{
	struct ahb_ecb *ecb;
	int s;

	s = splbio();

	/*
	 * If we can and have to, sleep waiting for one to come free
	 * but only if we can't allocate a new one.
	 */
	for (;;) {
		ecb = sc->sc_free_ecb.tqh_first;
		if (ecb) {
			TAILQ_REMOVE(&sc->sc_free_ecb, ecb, chain);
			break;
		}
		if (sc->sc_numecbs < AHB_ECB_MAX) {
			if (ahb_create_ecbs(sc)) {
				printf("%s: can't allocate ecbs\n",
				    sc->sc_dev.dv_xname);
				goto out;
			}
			continue;
		}
		if ((flags & SCSI_NOSLEEP) != 0)
			goto out;
		tsleep(&sc->sc_free_ecb, PRIBIO, "ahbecb", 0);
	}

	ecb->flags |= ECB_ALLOC;

out:
	splx(s);
	return ecb;
}

/*
 * given a physical address, find the ecb that it corresponds to.
 */
struct ahb_ecb *
ahb_ecb_phys_kv(sc, ecb_phys)
	struct ahb_softc *sc;
	physaddr ecb_phys;
{
	int hashnum = ECB_HASH(ecb_phys);
	struct ahb_ecb *ecb = sc->sc_ecbhash[hashnum];

	while (ecb) {
		if (ecb->hashkey == ecb_phys)
			break;
		ecb = ecb->nexthash;
	}
	return ecb;
}

/*
 * We have a ecb which has been processed by the adaptor, now we look to see
 * how the operation went.
 */
void
ahb_done(sc, ecb)
	struct ahb_softc *sc;
	struct ahb_ecb *ecb;
{
	bus_dma_tag_t dmat = sc->sc_dmat;
	struct scsi_sense_data *s1, *s2;
	struct scsi_xfer *xs = ecb->xs;

	SC_DEBUG(xs->sc_link, SDEV_DB2, ("ahb_done\n"));

	/*
	 * If we were a data transfer, unload the map that described
	 * the data buffer.
	 */
	if (xs->datalen) {
		bus_dmamap_sync(dmat, ecb->dmamap_xfer,
		    (xs->flags & SCSI_DATA_IN) ? BUS_DMASYNC_POSTREAD :
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(dmat, ecb->dmamap_xfer);
	}

	/*
	 * Otherwise, put the results of the operation
	 * into the xfer and call whoever started it
	 */
	if ((ecb->flags & ECB_ALLOC) == 0) {
		printf("%s: exiting ecb not allocated!\n", sc->sc_dev.dv_xname);
		Debugger();
	}
	if (ecb->flags & ECB_IMMED) {
		if (ecb->flags & ECB_IMMED_FAIL)
			xs->error = XS_DRIVER_STUFFUP;
		goto done;
	}
	if (xs->error == XS_NOERROR) {
		if (ecb->ecb_status.host_stat != HS_OK) {
			switch (ecb->ecb_status.host_stat) {
			case HS_TIMED_OUT:	/* No response */
				xs->error = XS_SELTIMEOUT;
				break;
			default:	/* Other scsi protocol messes */
				printf("%s: host_stat %x\n",
				    sc->sc_dev.dv_xname, ecb->ecb_status.host_stat);
				xs->error = XS_DRIVER_STUFFUP;
			}
		} else if (ecb->ecb_status.target_stat != SCSI_OK) {
			switch (ecb->ecb_status.target_stat) {
			case SCSI_CHECK:
				s1 = &ecb->ecb_sense;
				s2 = &xs->sense;
				*s2 = *s1;
				xs->error = XS_SENSE;
				break;
			case SCSI_BUSY:
				xs->error = XS_BUSY;
				break;
			default:
				printf("%s: target_stat %x\n",
				    sc->sc_dev.dv_xname, ecb->ecb_status.target_stat);
				xs->error = XS_DRIVER_STUFFUP;
			}
		} else
			xs->resid = 0;
	}
done:
	ahb_free_ecb(sc, ecb);
	xs->flags |= ITSDONE;
	scsi_done(xs);
}

/*
 * Start the board, ready for normal operation
 */
int
ahb_find(iot, ioh, sc)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct ahb_probe_data *sc;
{
	u_char intdef;
	int i, irq, busid;
	int wait = 1000;	/* 1 sec enough? */

	bus_space_write_1(iot, ioh, PORTADDR, PORTADDR_ENHANCED);

#define	NO_NO 1
#ifdef NO_NO
	/*
	 * reset board, If it doesn't respond, assume
	 * that it's not there.. good for the probe
	 */
	bus_space_write_1(iot, ioh, G2CNTRL, G2CNTRL_HARD_RESET);
	delay(1000);
	bus_space_write_1(iot, ioh, G2CNTRL, 0);
	delay(10000);
	while (--wait) {
		if ((bus_space_read_1(iot, ioh, G2STAT) & G2STAT_BUSY) == 0)
			break;
		delay(1000);
	}
	if (!wait) {
#ifdef	AHBDEBUG
		printf("ahb_find: No answer from aha1742 board\n");
#endif /* AHBDEBUG */
		return ENXIO;
	}
	i = bus_space_read_1(iot, ioh, MBOXIN0);
	if (i) {
		printf("self test failed, val = 0x%x\n", i);
		return EIO;
	}

	/* Set it again, just to be sure. */
	bus_space_write_1(iot, ioh, PORTADDR, PORTADDR_ENHANCED);
#endif

	while (bus_space_read_1(iot, ioh, G2STAT) & G2STAT_INT_PEND) {
		printf(".");
		bus_space_write_1(iot, ioh, G2CNTRL, G2CNTRL_CLEAR_EISA_INT);
		delay(10000);
	}

	intdef = bus_space_read_1(iot, ioh, INTDEF);
	switch (intdef & 0x07) {
	case INT9:
		irq = 9;
		break;
	case INT10:
		irq = 10;
		break;
	case INT11:
		irq = 11;
		break;
	case INT12:
		irq = 12;
		break;
	case INT14:
		irq = 14;
		break;
	case INT15:
		irq = 15;
		break;
	default:
		printf("illegal int setting %x\n", intdef);
		return EIO;
	}

	bus_space_write_1(iot, ioh, INTDEF, (intdef | INTEN));	/* make sure we can interrupt */

	/* who are we on the scsi bus? */
	busid = (bus_space_read_1(iot, ioh, SCSIDEF) & HSCSIID);

	/* if we want to return data, do so now */
	if (sc) {
		sc->sc_irq = irq;
		sc->sc_scsi_dev = busid;
	}

	/*
	 * Note that we are going and return (to probe)
	 */
	return 0;
}

void
ahb_init(sc)
	struct ahb_softc *sc;
{

}

void
ahbminphys(bp)
	struct buf *bp;
{

	if (bp->b_bcount > AHB_MAXXFER)
		bp->b_bcount = AHB_MAXXFER;
	minphys(bp);
}

/*
 * start a scsi operation given the command and the data address.  Also needs
 * the unit, target and lu.
 */
int
ahb_scsi_cmd(xs)
	struct scsi_xfer *xs;
{
	struct scsi_link *sc_link = xs->sc_link;
	struct ahb_softc *sc = sc_link->adapter_softc;
	bus_dma_tag_t dmat = sc->sc_dmat;
	struct ahb_ecb *ecb;
	int error, seg, flags, s;

	SC_DEBUG(sc_link, SDEV_DB2, ("ahb_scsi_cmd\n"));

	/*
	 * get a ecb (mbox-out) to use. If the transfer
	 * is from a buf (possibly from interrupt time)
	 * then we can't allow it to sleep
	 */
	flags = xs->flags;
	if ((ecb = ahb_get_ecb(sc, flags)) == NULL) {
		xs->error = XS_DRIVER_STUFFUP;
		return TRY_AGAIN_LATER;
	}
	ecb->xs = xs;
	ecb->timeout = xs->timeout;

	/*
	 * If it's a reset, we need to do an 'immediate'
	 * command, and store its ecb for later
	 * if there is already an immediate waiting,
	 * then WE must wait
	 */
	if (flags & SCSI_RESET) {
		ecb->flags |= ECB_IMMED;
		if (sc->sc_immed_ecb)
			return TRY_AGAIN_LATER;
		sc->sc_immed_ecb = ecb;

		s = splbio();
		ahb_send_immed(sc, AHB_TARG_RESET, ecb);
		splx(s);

		if ((flags & SCSI_POLL) == 0)
			return SUCCESSFULLY_QUEUED;

		/*
		 * If we can't use interrupts, poll on completion
		 */
		if (ahb_poll(sc, xs, ecb->timeout))
			ahb_timeout(ecb);
		return COMPLETE;
	}

	/*
	 * Put all the arguments for the xfer in the ecb
	 */
	ecb->opcode = ECB_SCSI_OP;
	ecb->opt1 = ECB_SES /*| ECB_DSB*/ | ECB_ARS;
	ecb->opt2 = sc_link->lun | ECB_NRB;
	bcopy(xs->cmd, &ecb->scsi_cmd, ecb->scsi_cmd_length = xs->cmdlen);
	ecb->sense_ptr = ecb->dmamap_self->dm_segs[0].ds_addr +
	    offsetof(struct ahb_ecb, ecb_sense);
	ecb->req_sense_length = sizeof(ecb->ecb_sense);
	ecb->status = ecb->dmamap_self->dm_segs[0].ds_addr +
	    offsetof(struct ahb_ecb, ecb_status);
	ecb->ecb_status.host_stat = 0x00;
	ecb->ecb_status.target_stat = 0x00;

	if (xs->datalen) {
		/*
		 * Map the DMA transfer.
		 */
#ifdef TFS
		if (flags & SCSI_DATA_UIO) {
			error = bus_dmamap_load_uio(sc->sc_dmat,
			    ecb->dmamap_xfer, (struct uio *)xs->data,
			    (flags & SCSI_NOSLEEP) ? BUS_DMA_NOWAIT :
			    BUS_DMA_WAITOK);
		} else
#endif /* TFS */
		{
			error = bus_dmamap_load(sc->sc_dmat,
			    ecb->dmamap_xfer, xs->data, xs->datalen, NULL,
			    (flags & SCSI_NOSLEEP) ? BUS_DMA_NOWAIT :
			    BUS_DMA_WAITOK);
		}

		if (error) {
			if (error == EFBIG) {
				printf("%s: ahb_scsi_cmd, more than %d"
				    " dma segments\n",
				    sc->sc_dev.dv_xname, AHB_NSEG);
			} else {
				printf("%s: ahb_scsi_cmd, error %d loading"
				    " dma map\n",
				    sc->sc_dev.dv_xname, error);
			}
			goto bad;
		}

		bus_dmamap_sync(dmat, ecb->dmamap_xfer,
		    (flags & SCSI_DATA_IN) ? BUS_DMASYNC_PREREAD :
		    BUS_DMASYNC_PREWRITE);

		/*
		 * Load the hardware scatter/gather map with the
		 * contents of the DMA map.
		 */
		for (seg = 0; seg < ecb->dmamap_xfer->dm_nsegs; seg++) {
			ecb->ahb_dma[seg].seg_addr =
			    ecb->dmamap_xfer->dm_segs[seg].ds_addr;
			ecb->ahb_dma[seg].seg_len =
			    ecb->dmamap_xfer->dm_segs[seg].ds_len;
		}

		ecb->data_addr = ecb->dmamap_self->dm_segs[0].ds_addr +
		    offsetof(struct ahb_ecb, ahb_dma);
		ecb->data_length = ecb->dmamap_xfer->dm_nsegs *
		    sizeof(struct ahb_dma_seg);
		ecb->opt1 |= ECB_S_G;
	} else {	/* No data xfer, use non S/G values */
		ecb->data_addr = (physaddr)0;
		ecb->data_length = 0;
	}
	ecb->link_addr = (physaddr)0;

	s = splbio();
	ahb_send_mbox(sc, OP_START_ECB, ecb);
	splx(s);

	/*
	 * Usually return SUCCESSFULLY QUEUED
	 */
	if ((flags & SCSI_POLL) == 0)
		return SUCCESSFULLY_QUEUED;

	/*
	 * If we can't use interrupts, poll on completion
	 */
	if (ahb_poll(sc, xs, ecb->timeout)) {
		ahb_timeout(ecb);
		if (ahb_poll(sc, xs, ecb->timeout))
			ahb_timeout(ecb);
	}
	return COMPLETE;

bad:
	xs->error = XS_DRIVER_STUFFUP;
	ahb_free_ecb(sc, ecb);
	return COMPLETE;
}

/*
 * Function to poll for command completion when in poll mode
 */
int
ahb_poll(sc, xs, count)
	struct ahb_softc *sc;
	struct scsi_xfer *xs;
	int count;
{				/* in msec  */
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	while (count) {
		/*
		 * If we had interrupts enabled, would we
		 * have got an interrupt?
		 */
		if (bus_space_read_1(iot, ioh, G2STAT) & G2STAT_INT_PEND)
			ahbintr(sc);
		if (xs->flags & ITSDONE)
			return 0;
		delay(1000);
		count--;
	}
	return 1;
}

void
ahb_timeout(arg)
	void *arg;
{
	struct ahb_ecb *ecb = arg;
	struct scsi_xfer *xs = ecb->xs;
	struct scsi_link *sc_link = xs->sc_link;
	struct ahb_softc *sc = sc_link->adapter_softc;
	int s;

	sc_print_addr(sc_link);
	printf("timed out");

	s = splbio();

	if (ecb->flags & ECB_IMMED) {
		printf("\n");
		ecb->flags |= ECB_IMMED_FAIL;
		/* XXX Must reset! */
	} else

	/*
	 * If it has been through before, then
	 * a previous abort has failed, don't
	 * try abort again
	 */
	if (ecb->flags & ECB_ABORT) {
		/* abort timed out */
		printf(" AGAIN\n");
		/* XXX Must reset! */
	} else {
		/* abort the operation that has timed out */
		printf("\n");
		ecb->xs->error = XS_TIMEOUT;
		ecb->timeout = AHB_ABORT_TIMEOUT;
		ecb->flags |= ECB_ABORT;
		ahb_send_mbox(sc, OP_ABORT_ECB, ecb);
	}

	splx(s);
}
