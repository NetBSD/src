/*	$NetBSD: twe.c,v 1.22.2.1 2002/05/26 16:05:36 perry Exp $	*/

/*-
 * Copyright (c) 2000, 2001, 2002 The NetBSD Foundation, Inc.
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

/*-
 * Copyright (c) 2000 Michael Smith
 * Copyright (c) 2000 BSDi
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
 * from FreeBSD: twe.c,v 1.1 2000/05/24 23:35:23 msmith Exp
 */

/*
 * Driver for the 3ware Escalade family of RAID controllers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: twe.c,v 1.22.2.1 2002/05/26 16:05:36 perry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/endian.h>
#include <sys/malloc.h>
#include <sys/disk.h>

#include <uvm/uvm_extern.h>

#include <machine/bswap.h>
#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/twereg.h>
#include <dev/pci/twevar.h>

#define	PCI_CBIO	0x10

static void	twe_aen_handler(struct twe_ccb *, int);
static void	twe_attach(struct device *, struct device *, void *);
static int	twe_init_connection(struct twe_softc *);
static int	twe_intr(void *);
static int	twe_match(struct device *, struct cfdata *, void *);
static int	twe_param_get(struct twe_softc *, int, int, size_t,
			      void (*)(struct twe_ccb *, int), void **);
static void	twe_poll(struct twe_softc *);
static int	twe_print(void *, const char *);
static int	twe_reset(struct twe_softc *);
static int	twe_submatch(struct device *, struct cfdata *, void *);
static int	twe_status_check(struct twe_softc *, u_int);
static int	twe_status_wait(struct twe_softc *, u_int, int);

static inline u_int32_t	twe_inl(struct twe_softc *, int);
static inline void	twe_outl(struct twe_softc *, int, u_int32_t);

struct cfattach twe_ca = {
	sizeof(struct twe_softc), twe_match, twe_attach
};

struct {
	const u_int	aen;		/* High byte non-zero if w/unit */
	const char	*desc;
} static const twe_aen_names[] = {
	{ 0x0000, "queue empty" },
	{ 0x0001, "soft reset" },
	{ 0x0102, "degraded mirror" },
	{ 0x0003, "controller error" },
	{ 0x0104, "rebuild fail" },
	{ 0x0105, "rebuild done" },
	{ 0x0106, "incompatible unit" },
	{ 0x0107, "init done" },
	{ 0x0108, "unclean shutdown" },
	{ 0x0109, "aport timeout" },
	{ 0x010a, "drive error" },
	{ 0x010b, "rebuild started" },
	{ 0x010c, "init started" },
	{ 0x0015, "table undefined" },
	{ 0x00ff, "aen queue full" },
};

static inline u_int32_t
twe_inl(struct twe_softc *sc, int off)
{

	bus_space_barrier(sc->sc_iot, sc->sc_ioh, off, 4,
	    BUS_SPACE_BARRIER_WRITE | BUS_SPACE_BARRIER_READ);
	return (bus_space_read_4(sc->sc_iot, sc->sc_ioh, off));
}

static inline void
twe_outl(struct twe_softc *sc, int off, u_int32_t val)
{

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, off, val);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, off, 4,
	    BUS_SPACE_BARRIER_WRITE);
}

/*
 * Match a supported board.
 */
static int
twe_match(struct device *parent, struct cfdata *cfdata, void *aux)
{
	struct pci_attach_args *pa;

	pa = aux;

	return (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_3WARE &&	 
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_3WARE_ESCALADE ||
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_3WARE_ESCALADE_ASIC));
}

/*
 * Attach a supported board.
 *
 * XXX This doesn't fail gracefully.
 */
static void
twe_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa;
	struct twe_softc *sc;
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
	pcireg_t csr;
	const char *intrstr;
	int size, i, rv, rseg;
	size_t max_segs, max_xfer;
	struct twe_param *dtp, *ctp;
	bus_dma_segment_t seg;
	struct twe_cmd *tc;
	struct twe_attach_args twea;
	struct twe_ccb *ccb;

	sc = (struct twe_softc *)self;
	pa = aux;
	pc = pa->pa_pc;
	sc->sc_dmat = pa->pa_dmat;
	SIMPLEQ_INIT(&sc->sc_ccb_queue);
	SLIST_INIT(&sc->sc_ccb_freelist);

	printf(": 3ware Escalade\n");

	if (pci_mapreg_map(pa, PCI_CBIO, PCI_MAPREG_TYPE_IO, 0,
	    &sc->sc_iot, &sc->sc_ioh, NULL, NULL)) {
		printf("%s: can't map i/o space\n", sc->sc_dv.dv_xname);
		return;
	}

	/* Enable the device. */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    csr | PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: can't map interrupt\n", sc->sc_dv.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_BIO, twe_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: can't establish interrupt", sc->sc_dv.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	if (intrstr != NULL)
		printf("%s: interrupting at %s\n", sc->sc_dv.dv_xname, intrstr);

	/*
	 * Allocate and initialise the command blocks and CCBs.
	 */
        size = sizeof(struct twe_cmd) * TWE_MAX_QUEUECNT;

	if ((rv = bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, &seg, 1, 
	    &rseg, BUS_DMA_NOWAIT)) != 0) {
		printf("%s: unable to allocate commands, rv = %d\n",
		    sc->sc_dv.dv_xname, rv);
		return;
	}

	if ((rv = bus_dmamem_map(sc->sc_dmat, &seg, rseg, size, 
	    (caddr_t *)&sc->sc_cmds,
	    BUS_DMA_NOWAIT | BUS_DMA_COHERENT)) != 0) {
		printf("%s: unable to map commands, rv = %d\n",
		    sc->sc_dv.dv_xname, rv);
		return;
	}

	if ((rv = bus_dmamap_create(sc->sc_dmat, size, size, 1, 0, 
	    BUS_DMA_NOWAIT, &sc->sc_dmamap)) != 0) {
		printf("%s: unable to create command DMA map, rv = %d\n",
		    sc->sc_dv.dv_xname, rv);
		return;
	}

	if ((rv = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap, sc->sc_cmds, 
	    size, NULL, BUS_DMA_NOWAIT)) != 0) {
		printf("%s: unable to load command DMA map, rv = %d\n",
		    sc->sc_dv.dv_xname, rv);
		return;
	}

	sc->sc_cmds_paddr = sc->sc_dmamap->dm_segs[0].ds_addr;
	memset(sc->sc_cmds, 0, size);

	ccb = malloc(sizeof(*ccb) * TWE_MAX_QUEUECNT, M_DEVBUF, M_NOWAIT);
	sc->sc_ccbs = ccb;
	tc = (struct twe_cmd *)sc->sc_cmds;
	max_segs = twe_get_maxsegs();
	max_xfer = twe_get_maxxfer(max_segs);

	for (i = 0; i < TWE_MAX_QUEUECNT; i++, tc++, ccb++) {
		ccb->ccb_cmd = tc;
		ccb->ccb_cmdid = i;
		ccb->ccb_flags = 0;
		rv = bus_dmamap_create(sc->sc_dmat, max_xfer,
		    max_segs, PAGE_SIZE, 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
		    &ccb->ccb_dmamap_xfer);
		if (rv != 0) {
			printf("%s: can't create dmamap, rv = %d\n",
			    sc->sc_dv.dv_xname, rv);
			return;
		}
		/* Save one CCB for parameter retrieval. */
		if (i != 0)
			SLIST_INSERT_HEAD(&sc->sc_ccb_freelist, ccb,
			    ccb_chain.slist);
	}

	/* Wait for the controller to become ready. */
	if (twe_status_wait(sc, TWE_STS_MICROCONTROLLER_READY, 6)) {
		printf("%s: microcontroller not ready\n", sc->sc_dv.dv_xname);
		return;
	}

	twe_outl(sc, TWE_REG_CTL, TWE_CTL_DISABLE_INTRS);

	/* Reset the controller. */
	if (twe_reset(sc)) {
		printf("%s: reset failed\n", sc->sc_dv.dv_xname);
		return;
	}

	/* Find attached units. */ 
	rv = twe_param_get(sc, TWE_PARAM_UNITSUMMARY,
	    TWE_PARAM_UNITSUMMARY_Status, TWE_MAX_UNITS, NULL, (void **)&dtp);
	if (rv != 0) {
		printf("%s: can't detect attached units (%d)\n",
		    sc->sc_dv.dv_xname, rv);
		return;
	}

	/* For each detected unit, collect size and store in an array. */
	for (i = 0, sc->sc_nunits = 0; i < TWE_MAX_UNITS; i++) {
		/* Unit present? */
		if ((dtp->tp_data[i] & TWE_PARAM_UNITSTATUS_Online) == 0) {
			sc->sc_dsize[i] = 0;
	   		continue;
	   	}

		rv = twe_param_get(sc, TWE_PARAM_UNITINFO + i,
		    TWE_PARAM_UNITINFO_Capacity, 4, NULL, (void **)&ctp);
		if (rv != 0) {
			printf("%s: error %d fetching capacity for unit %d\n",
			    sc->sc_dv.dv_xname, rv, i);
			continue;
		}

		sc->sc_dsize[i] = le32toh(*(u_int32_t *)ctp->tp_data);
		free(ctp, M_DEVBUF);
		sc->sc_nunits++;
	}
	free(dtp, M_DEVBUF);

	/* Initialise connection with controller and enable interrupts. */
	twe_init_connection(sc);
	twe_outl(sc, TWE_REG_CTL, TWE_CTL_CLEAR_ATTN_INTR |
	    TWE_CTL_UNMASK_RESP_INTR |
	    TWE_CTL_ENABLE_INTRS);

	/* Attach sub-devices. */
	for (i = 0; i < TWE_MAX_UNITS; i++) {
		if (sc->sc_dsize[i] == 0)
			continue;
		twea.twea_unit = i;
		config_found_sm(&sc->sc_dv, &twea, twe_print, twe_submatch);
	}
}

/*
 * Reset the controller.  Currently only useful at attach time; must be
 * called with interrupts blocked.
 */
static int
twe_reset(struct twe_softc *sc)
{
	struct twe_param *tp;
	u_int aen, status;
	volatile u_int32_t junk;
	int got, rv;

	/* Issue a soft reset. */
	twe_outl(sc, TWE_REG_CTL, TWE_CTL_ISSUE_SOFT_RESET |
	    TWE_CTL_CLEAR_HOST_INTR |
	    TWE_CTL_CLEAR_ATTN_INTR |
	    TWE_CTL_MASK_CMD_INTR |
	    TWE_CTL_MASK_RESP_INTR |
	    TWE_CTL_CLEAR_ERROR_STS |
	    TWE_CTL_DISABLE_INTRS);

	if (twe_status_wait(sc, TWE_STS_ATTN_INTR, 15)) {
		printf("%s: no attention interrupt\n",
		    sc->sc_dv.dv_xname);
		return (-1);
	}

	/* Pull AENs out of the controller; look for a soft reset AEN. */
	for (got = 0;;) {
		rv = twe_param_get(sc, TWE_PARAM_AEN, TWE_PARAM_AEN_UnitCode,
		    2, NULL, (void **)&tp);
		if (rv != 0)
			printf("%s: error %d while draining response queue\n",
			    sc->sc_dv.dv_xname, rv);
		aen = TWE_AEN_CODE(le16toh(*(u_int16_t *)tp->tp_data));
		free(tp, M_DEVBUF);
		if (aen == TWE_AEN_QUEUE_EMPTY)
			break;
		if (aen == TWE_AEN_SOFT_RESET)
			got = 1;
	}
	if (!got) {
		printf("%s: reset not reported\n", sc->sc_dv.dv_xname);
		return (-1);
	}

	/* Check controller status. */
	status = twe_inl(sc, TWE_REG_STS);
	if (twe_status_check(sc, status)) {
		printf("%s: controller errors detected\n",
		    sc->sc_dv.dv_xname);
		return (-1);
	}

	/* Drain the response queue. */
	for (;;) {
		status = twe_inl(sc, TWE_REG_STS);
		if (twe_status_check(sc, status) != 0) {
			printf("%s: can't drain response queue\n",
			    sc->sc_dv.dv_xname);
			return (-1);
		}
		if ((status & TWE_STS_RESP_QUEUE_EMPTY) != 0)
			break;
		junk = twe_inl(sc, TWE_REG_RESP_QUEUE);
	}

	return (0);
}

/*
 * Print autoconfiguration message for a sub-device.
 */
static int
twe_print(void *aux, const char *pnp)
{
	struct twe_attach_args *twea;

	twea = aux;

	if (pnp != NULL)
		printf("block device at %s", pnp);
	printf(" unit %d", twea->twea_unit);
	return (UNCONF);
}

/*
 * Match a sub-device.
 */
static int
twe_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct twe_attach_args *twea;

	twea = aux;

	if (cf->tweacf_unit != TWECF_UNIT_DEFAULT &&
	    cf->tweacf_unit != twea->twea_unit)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

/*
 * Interrupt service routine.
 */
static int
twe_intr(void *arg)
{
	struct twe_softc *sc;
	u_int status;
	int caught, rv;

	sc = arg;
	caught = 0;
	status = twe_inl(sc, TWE_REG_STS);
	twe_status_check(sc, status);

	/* Host interrupts - purpose unknown. */
	if ((status & TWE_STS_HOST_INTR) != 0) {
#ifdef DIAGNOSTIC
		printf("%s: host interrupt\n", sc->sc_dv.dv_xname);
#endif
		twe_outl(sc, TWE_REG_CTL, TWE_CTL_CLEAR_HOST_INTR);
		caught = 1;
	}

	/*
	 * Attention interrupts, signalled when a controller or child device
	 * state change has occurred.
	 */
	if ((status & TWE_STS_ATTN_INTR) != 0) {
		if ((sc->sc_flags & TWEF_AEN) == 0) {
			rv = twe_param_get(sc, TWE_PARAM_AEN,
			    TWE_PARAM_AEN_UnitCode, 2, twe_aen_handler,
			    NULL);
			if (rv != 0) {
				printf("%s: unable to retrieve AEN (%d)\n",
				    sc->sc_dv.dv_xname, rv);
				twe_outl(sc, TWE_REG_CTL,
				    TWE_CTL_CLEAR_ATTN_INTR);
			} else
				sc->sc_flags |= TWEF_AEN;
		}
		caught = 1;
	}

	/*
	 * Command interrupts, signalled when the controller can accept more
	 * commands.  We don't use this; instead, we try to submit commands
	 * when we receive them, and when other commands have completed. 
	 * Mask it so we don't get another one.
	 */
	if ((status & TWE_STS_CMD_INTR) != 0) {
#ifdef DIAGNOSTIC
		printf("%s: command interrupt\n", sc->sc_dv.dv_xname);
#endif
		twe_outl(sc, TWE_REG_CTL, TWE_CTL_MASK_CMD_INTR);
		caught = 1;
	}

	if ((status & TWE_STS_RESP_INTR) != 0) {
		twe_poll(sc);
		caught = 1;
	}

	return (caught);
}

/*
 * Handle an AEN returned by the controller.
 */
static void
twe_aen_handler(struct twe_ccb *ccb, int error)
{
	struct twe_softc *sc;
	struct twe_param *tp;
	const char *str;
	u_int aen;
	int i, hu, rv;

	sc = (struct twe_softc *)ccb->ccb_tx.tx_dv;
	tp = ccb->ccb_tx.tx_context;
	twe_ccb_unmap(sc, ccb);

	if (error) {
		printf("%s: error retrieving AEN\n", sc->sc_dv.dv_xname);
		aen = TWE_AEN_QUEUE_EMPTY;
	} else
		aen = le16toh(*(u_int16_t *)tp->tp_data);
	free(tp, M_DEVBUF);
	twe_ccb_free(sc, ccb);

	if (TWE_AEN_CODE(aen) == TWE_AEN_QUEUE_EMPTY) {
		twe_outl(sc, TWE_REG_CTL, TWE_CTL_CLEAR_ATTN_INTR);
		sc->sc_flags &= ~TWEF_AEN;
		return;
	}

	str = "<unknown>";
	i = 0;
	hu = 0;
		
	while (i < sizeof(twe_aen_names) / sizeof(twe_aen_names[0])) {
		if (TWE_AEN_CODE(twe_aen_names[i].aen) == TWE_AEN_CODE(aen)) {
			str = twe_aen_names[i].desc;
			hu = (TWE_AEN_UNIT(twe_aen_names[i].aen) != 0);
			break;
		}
		i++;
	}
	printf("%s: AEN 0x%04x (%s) received", sc->sc_dv.dv_xname,
	    TWE_AEN_CODE(aen), str);
	if (hu != 0)
		printf(" for unit %d", TWE_AEN_UNIT(aen));
	printf("\n");

	/*
	 * Chain another retrieval in case interrupts have been
	 * coalesced.
	 */
	rv = twe_param_get(sc, TWE_PARAM_AEN, TWE_PARAM_AEN_UnitCode, 2,
	    twe_aen_handler, NULL);
	if (rv != 0)
		printf("%s: unable to retrieve AEN (%d)\n",
		    sc->sc_dv.dv_xname, rv);
}

/*
 * Execute a TWE_OP_GET_PARAM command.  If a callback function is provided,
 * it will be called with generated context when the command has completed. 
 * If no callback is provided, the command will be executed synchronously
 * and a pointer to a buffer containing the data returned.
 *
 * The caller or callback is responsible for freeing the buffer.
 */
static int
twe_param_get(struct twe_softc *sc, int table_id, int param_id, size_t size,
	      void (*func)(struct twe_ccb *, int), void **pbuf)
{
	struct twe_ccb *ccb;
	struct twe_cmd *tc;
	struct twe_param *tp;
	int rv, s;

	rv = twe_ccb_alloc(sc, &ccb,
	    TWE_CCB_PARAM | TWE_CCB_DATA_IN | TWE_CCB_DATA_OUT);
	if (rv != 0)
		return (rv);

	tp = malloc(TWE_SECTOR_SIZE, M_DEVBUF, M_NOWAIT);
	if (pbuf != NULL)
		*pbuf = tp;

	ccb->ccb_data = tp;
	ccb->ccb_datasize = TWE_SECTOR_SIZE;
	ccb->ccb_tx.tx_handler = func;
	ccb->ccb_tx.tx_context = tp;
	ccb->ccb_tx.tx_dv = &sc->sc_dv;

	tc = ccb->ccb_cmd;
	tc->tc_size = 2;
	tc->tc_opcode = TWE_OP_GET_PARAM | (tc->tc_size << 5);
	tc->tc_unit = 0;
	tc->tc_count = htole16(1);

	/* Fill in the outbound parameter data. */
	tp->tp_table_id = htole16(table_id);
	tp->tp_param_id = param_id;
	tp->tp_param_size = size;

	/* Map the transfer. */
	if ((rv = twe_ccb_map(sc, ccb)) != 0) {
		twe_ccb_free(sc, ccb);
		free(tp, M_DEVBUF);
		return (rv);
	}

	/* Submit the command and either wait or let the callback handle it. */
	if (func == NULL) {
		s = splbio();
		rv = twe_ccb_poll(sc, ccb, 5);
		twe_ccb_unmap(sc, ccb);
		twe_ccb_free(sc, ccb);
		splx(s);
		if (rv != 0)
			free(tp, M_DEVBUF);
	} else {
		twe_ccb_enqueue(sc, ccb);
		rv = 0;
	}

	return (rv);
}

/*
 * Execute a TWE_OP_INIT_CONNECTION command.  Return non-zero on error. 
 * Must be called with interrupts blocked.
 */
static int
twe_init_connection(struct twe_softc *sc)
{
	struct twe_ccb *ccb;
	struct twe_cmd *tc;
	int rv;

	if ((rv = twe_ccb_alloc(sc, &ccb, 0)) != 0)
		return (rv);

	/* Build the command. */
	tc = ccb->ccb_cmd;
	tc->tc_size = 3;
	tc->tc_opcode = TWE_OP_INIT_CONNECTION;
	tc->tc_unit = 0;
	tc->tc_count = htole16(TWE_MAX_CMDS);
	tc->tc_args.init_connection.response_queue_pointer = 0;

	/* Submit the command for immediate execution. */
	rv = twe_ccb_poll(sc, ccb, 5);
	twe_ccb_free(sc, ccb);
	return (rv);
}

/*
 * Poll the controller for completed commands.  Must be called with
 * interrupts blocked.
 */
static void
twe_poll(struct twe_softc *sc)
{
	struct twe_ccb *ccb;
	int found;
	u_int status, cmdid;

	found = 0;

	for (;;) {
		status = twe_inl(sc, TWE_REG_STS);
		twe_status_check(sc, status);

		if ((status & TWE_STS_RESP_QUEUE_EMPTY))
			break;

		found = 1;
		cmdid = twe_inl(sc, TWE_REG_RESP_QUEUE);
		cmdid = (cmdid & TWE_RESP_MASK) >> TWE_RESP_SHIFT;
		if (cmdid >= TWE_MAX_QUEUECNT) {
			printf("%s: bad completion\n", sc->sc_dv.dv_xname);
			continue;
		}

		ccb = sc->sc_ccbs + cmdid;
		if ((ccb->ccb_flags & TWE_CCB_ACTIVE) == 0) {
			printf("%s: bad completion (not active)\n",
			    sc->sc_dv.dv_xname);
			continue;
		}
		ccb->ccb_flags ^= TWE_CCB_COMPLETE | TWE_CCB_ACTIVE;

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap,
		    (caddr_t)ccb->ccb_cmd - sc->sc_cmds,
		    sizeof(struct twe_cmd),
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		/* Pass notification to upper layers. */
		if (ccb->ccb_tx.tx_handler != NULL)
			(*ccb->ccb_tx.tx_handler)(ccb,
			    ccb->ccb_cmd->tc_status != 0 ? EIO : 0);
	}

	/* If any commands have completed, run the software queue. */
	if (found)
		twe_ccb_enqueue(sc, NULL);
}

/*
 * Wait for `status' to be set in the controller status register.  Return
 * zero if found, non-zero if the operation timed out.
 */
static int
twe_status_wait(struct twe_softc *sc, u_int32_t status, int timo)
{

	for (timo *= 10; timo != 0; timo--) {
		if ((twe_inl(sc, TWE_REG_STS) & status) == status)
			break;
		delay(100000);
	}

	return (timo == 0);
}

/*
 * Complain if the status bits aren't what we expect.
 */
static int
twe_status_check(struct twe_softc *sc, u_int status)
{
	int rv;

	rv = 0;

	if ((status & TWE_STS_EXPECTED_BITS) != TWE_STS_EXPECTED_BITS) {
		printf("%s: missing status bits: 0x%08x\n", sc->sc_dv.dv_xname,
		    status & ~TWE_STS_EXPECTED_BITS);
		rv = -1;
	}

	if ((status & TWE_STS_UNEXPECTED_BITS) != 0) {
		printf("%s: unexpected status bits: 0x%08x\n",
		    sc->sc_dv.dv_xname, status & TWE_STS_UNEXPECTED_BITS);
		rv = -1;
	}

	return (rv);
}

/*
 * Allocate and initialise a CCB.
 */
int
twe_ccb_alloc(struct twe_softc *sc, struct twe_ccb **ccbp, int flags)
{
	struct twe_cmd *tc;
	struct twe_ccb *ccb;
	int s;

	s = splbio();	
	if ((flags & TWE_CCB_PARAM) != 0)
		ccb = sc->sc_ccbs;
	else {
		/* Allocate a CCB and command block. */
		if (SLIST_FIRST(&sc->sc_ccb_freelist) == NULL) {
			splx(s);
			return (EAGAIN);
		}
		ccb = SLIST_FIRST(&sc->sc_ccb_freelist);
		SLIST_REMOVE_HEAD(&sc->sc_ccb_freelist, ccb_chain.slist);
	}
#ifdef DIAGNOSTIC
	if ((ccb->ccb_flags & TWE_CCB_ALLOCED) != 0)
		panic("twe_ccb_alloc: CCB already allocated");
	flags |= TWE_CCB_ALLOCED;
#endif
	splx(s);

	/* Initialise some fields and return. */
	ccb->ccb_tx.tx_handler = NULL;
	ccb->ccb_flags = flags;
	tc = ccb->ccb_cmd;
	tc->tc_status = 0;
	tc->tc_flags = 0;
	tc->tc_cmdid = ccb->ccb_cmdid;
	*ccbp = ccb;

	return (0);
}

/*
 * Free a CCB.
 */
void
twe_ccb_free(struct twe_softc *sc, struct twe_ccb *ccb)
{
	int s;

	s = splbio();
	if ((ccb->ccb_flags & TWE_CCB_PARAM) == 0)
		SLIST_INSERT_HEAD(&sc->sc_ccb_freelist, ccb, ccb_chain.slist);
	ccb->ccb_flags = 0;
	splx(s);
}

/*
 * Map the specified CCB's command block and data buffer (if any) into
 * controller visible space.  Perform DMA synchronisation.
 */
int
twe_ccb_map(struct twe_softc *sc, struct twe_ccb *ccb)
{
	struct twe_cmd *tc;
	int flags, nsegs, i, s, rv;
	void *data;

	/*
	 * The data as a whole must be 512-byte aligned.
	 */
	if (((u_long)ccb->ccb_data & (TWE_ALIGNMENT - 1)) != 0) {
		s = splvm();
		/* XXX */
		ccb->ccb_abuf = uvm_km_kmemalloc(kmem_map, NULL,
		    ccb->ccb_datasize, UVM_KMF_NOWAIT);
		splx(s);
		data = (void *)ccb->ccb_abuf;
		if ((ccb->ccb_flags & TWE_CCB_DATA_OUT) != 0)
			memcpy(data, ccb->ccb_data, ccb->ccb_datasize);
	} else {
		ccb->ccb_abuf = (vaddr_t)0;
		data = ccb->ccb_data;
	}

	/*
	 * Map the data buffer into bus space and build the S/G list.
	 */
	rv = bus_dmamap_load(sc->sc_dmat, ccb->ccb_dmamap_xfer, data,
	    ccb->ccb_datasize, NULL, BUS_DMA_NOWAIT | BUS_DMA_STREAMING |
	    ((ccb->ccb_flags & TWE_CCB_DATA_IN) ?
	    BUS_DMA_READ : BUS_DMA_WRITE));
	if (rv != 0) {
		if (ccb->ccb_abuf != (vaddr_t)0) {
			s = splvm();
			/* XXX */
			uvm_km_free(kmem_map, ccb->ccb_abuf,
			    ccb->ccb_datasize);
			splx(s);
		}
		return (rv);
	}

	nsegs = ccb->ccb_dmamap_xfer->dm_nsegs;
	tc = ccb->ccb_cmd;
	tc->tc_size += 2 * nsegs;

	/* The location of the S/G list is dependant upon command type. */
	switch (tc->tc_opcode >> 5) {
	case 2:
		for (i = 0; i < nsegs; i++) {
			tc->tc_args.param.sgl[i].tsg_address =
			    htole32(ccb->ccb_dmamap_xfer->dm_segs[i].ds_addr);
			tc->tc_args.param.sgl[i].tsg_length =
			    htole32(ccb->ccb_dmamap_xfer->dm_segs[i].ds_len);
		}
		/* XXX Needed? */
		for (; i < TWE_SG_SIZE; i++) {
			tc->tc_args.param.sgl[i].tsg_address = 0;
			tc->tc_args.param.sgl[i].tsg_length = 0;
		}
		break;
	case 3:
		for (i = 0; i < nsegs; i++) {
			tc->tc_args.io.sgl[i].tsg_address =
			    htole32(ccb->ccb_dmamap_xfer->dm_segs[i].ds_addr);
			tc->tc_args.io.sgl[i].tsg_length =
			    htole32(ccb->ccb_dmamap_xfer->dm_segs[i].ds_len);
		}
		/* XXX Needed? */
		for (; i < TWE_SG_SIZE; i++) {
			tc->tc_args.io.sgl[i].tsg_address = 0;
			tc->tc_args.io.sgl[i].tsg_length = 0;
		}
		break;
#ifdef DEBUG
	default:
		panic("twe_ccb_map: oops");
#endif
	}

	if ((ccb->ccb_flags & TWE_CCB_DATA_IN) != 0)
		flags = BUS_DMASYNC_PREREAD;
	else
		flags = 0;
	if ((ccb->ccb_flags & TWE_CCB_DATA_OUT) != 0)
		flags |= BUS_DMASYNC_PREWRITE;

	bus_dmamap_sync(sc->sc_dmat, ccb->ccb_dmamap_xfer, 0,
	    ccb->ccb_datasize, flags);
	return (0);
}

/*
 * Unmap the specified CCB's command block and data buffer (if any) and
 * perform DMA synchronisation.
 */
void
twe_ccb_unmap(struct twe_softc *sc, struct twe_ccb *ccb)
{
	int flags, s;

	if ((ccb->ccb_flags & TWE_CCB_DATA_IN) != 0)
		flags = BUS_DMASYNC_POSTREAD;
	else
		flags = 0;
	if ((ccb->ccb_flags & TWE_CCB_DATA_OUT) != 0)
		flags |= BUS_DMASYNC_POSTWRITE;

	bus_dmamap_sync(sc->sc_dmat, ccb->ccb_dmamap_xfer, 0,
	    ccb->ccb_datasize, flags);
	bus_dmamap_unload(sc->sc_dmat, ccb->ccb_dmamap_xfer);

	if (ccb->ccb_abuf != (vaddr_t)0) {
		if ((ccb->ccb_flags & TWE_CCB_DATA_IN) != 0)
			memcpy(ccb->ccb_data, (void *)ccb->ccb_abuf,
			    ccb->ccb_datasize);
		s = splvm();
		/* XXX */
		uvm_km_free(kmem_map, ccb->ccb_abuf, ccb->ccb_datasize);
		splx(s);
	}
}

/*
 * Submit a command to the controller and poll on completion.  Return
 * non-zero on timeout (but don't check status, as some command types don't
 * return status).  Must be called with interrupts blocked.
 */
int
twe_ccb_poll(struct twe_softc *sc, struct twe_ccb *ccb, int timo)
{
	int rv;

	if ((rv = twe_ccb_submit(sc, ccb)) != 0)
		return (rv);

	for (timo *= 1000; timo != 0; timo--) {
		twe_poll(sc);
		if ((ccb->ccb_flags & TWE_CCB_COMPLETE) != 0)
			break;
		DELAY(100);
	}

	return (timo == 0);
}

/*
 * If a CCB is specified, enqueue it.  Pull CCBs off the software queue in
 * the order that they were enqueued and try to submit their command blocks
 * to the controller for execution.
 */
void
twe_ccb_enqueue(struct twe_softc *sc, struct twe_ccb *ccb)
{
	int s;

	s = splbio();

	if (ccb != NULL)
		SIMPLEQ_INSERT_TAIL(&sc->sc_ccb_queue, ccb, ccb_chain.simpleq);

	while ((ccb = SIMPLEQ_FIRST(&sc->sc_ccb_queue)) != NULL) {
		if (twe_ccb_submit(sc, ccb))
			break;
		SIMPLEQ_REMOVE_HEAD(&sc->sc_ccb_queue, ccb, ccb_chain.simpleq);
	}

	splx(s);
}

/*
 * Submit the command block associated with the specified CCB to the
 * controller for execution.  Must be called with interrupts blocked.
 */
int
twe_ccb_submit(struct twe_softc *sc, struct twe_ccb *ccb)
{
	bus_addr_t pa;
	int rv;
	u_int status;

	/* Check to see if we can post a command. */
	status = twe_inl(sc, TWE_REG_STS);
	twe_status_check(sc, status);

	if ((status & TWE_STS_CMD_QUEUE_FULL) == 0) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap,
		    (caddr_t)ccb->ccb_cmd - sc->sc_cmds, sizeof(struct twe_cmd),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		ccb->ccb_flags |= TWE_CCB_ACTIVE;
		pa = sc->sc_cmds_paddr +
		    ccb->ccb_cmdid * sizeof(struct twe_cmd);
		twe_outl(sc, TWE_REG_CMD_QUEUE, (u_int32_t)pa);
		rv = 0;
	} else
		rv = EBUSY;

	return (rv);
}
