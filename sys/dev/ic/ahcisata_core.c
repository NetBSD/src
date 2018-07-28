/*	$NetBSD: ahcisata_core.c,v 1.60.2.1 2018/07/28 04:37:44 pgoyette Exp $	*/

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ahcisata_core.c,v 1.60.2.1 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/disklabel.h>
#include <sys/proc.h>
#include <sys/buf.h>

#include <dev/ata/atareg.h>
#include <dev/ata/satavar.h>
#include <dev/ata/satareg.h>
#include <dev/ata/satafisvar.h>
#include <dev/ata/satafisreg.h>
#include <dev/ata/satapmpreg.h>
#include <dev/ic/ahcisatavar.h>
#include <dev/ic/wdcreg.h>

#include <dev/scsipi/scsi_all.h> /* for SCSI status */

#include "atapibus.h"

#ifdef AHCI_DEBUG
int ahcidebug_mask = 0;
#endif

static void ahci_probe_drive(struct ata_channel *);
static void ahci_setup_channel(struct ata_channel *);

static int  ahci_ata_bio(struct ata_drive_datas *, struct ata_xfer *);
static int  ahci_do_reset_drive(struct ata_channel *, int, int, uint32_t *,
	struct ata_xfer *xfer);
static void ahci_reset_drive(struct ata_drive_datas *, int, uint32_t *);
static void ahci_reset_channel(struct ata_channel *, int);
static int  ahci_exec_command(struct ata_drive_datas *, struct ata_xfer *);
static int  ahci_ata_addref(struct ata_drive_datas *);
static void ahci_ata_delref(struct ata_drive_datas *);
static void ahci_killpending(struct ata_drive_datas *);

static int ahci_cmd_start(struct ata_channel *, struct ata_xfer *);
static int  ahci_cmd_complete(struct ata_channel *, struct ata_xfer *, int);
static void ahci_cmd_poll(struct ata_channel *, struct ata_xfer *);
static void ahci_cmd_abort(struct ata_channel *, struct ata_xfer *);
static void ahci_cmd_done(struct ata_channel *, struct ata_xfer *);
static void ahci_cmd_done_end(struct ata_channel *, struct ata_xfer *);
static void ahci_cmd_kill_xfer(struct ata_channel *, struct ata_xfer *, int);
static int ahci_bio_start(struct ata_channel *, struct ata_xfer *);
static void ahci_bio_poll(struct ata_channel *, struct ata_xfer *);
static void ahci_bio_abort(struct ata_channel *, struct ata_xfer *);
static int  ahci_bio_complete(struct ata_channel *, struct ata_xfer *, int);
static void ahci_bio_kill_xfer(struct ata_channel *, struct ata_xfer *, int) ;
static void ahci_channel_stop(struct ahci_softc *, struct ata_channel *, int);
static void ahci_channel_start(struct ahci_softc *, struct ata_channel *,
				int, int);
void ahci_channel_recover(struct ahci_softc *, struct ata_channel *, int);
static int  ahci_dma_setup(struct ata_channel *, int, void *, size_t, int);

#if NATAPIBUS > 0
static void ahci_atapibus_attach(struct atabus_softc *);
static void ahci_atapi_kill_pending(struct scsipi_periph *);
static void ahci_atapi_minphys(struct buf *);
static void ahci_atapi_scsipi_request(struct scsipi_channel *,
    scsipi_adapter_req_t, void *);
static int ahci_atapi_start(struct ata_channel *, struct ata_xfer *);
static void ahci_atapi_poll(struct ata_channel *, struct ata_xfer *);
static void ahci_atapi_abort(struct ata_channel *, struct ata_xfer *);
static int  ahci_atapi_complete(struct ata_channel *, struct ata_xfer *, int);
static void ahci_atapi_kill_xfer(struct ata_channel *, struct ata_xfer *, int);
static void ahci_atapi_probe_device(struct atapibus_softc *, int);

static const struct scsipi_bustype ahci_atapi_bustype = {
	SCSIPI_BUSTYPE_ATAPI,
	atapi_scsipi_cmd,
	atapi_interpret_sense,
	atapi_print_addr,
	ahci_atapi_kill_pending,
	NULL,
};
#endif /* NATAPIBUS */

#define ATA_DELAY 10000 /* 10s for a drive I/O */
#define ATA_RESET_DELAY 31000 /* 31s for a drive reset */
#define AHCI_RST_WAIT (ATA_RESET_DELAY / 10)

const struct ata_bustype ahci_ata_bustype = {
	SCSIPI_BUSTYPE_ATA,
	ahci_ata_bio,
	ahci_reset_drive,
	ahci_reset_channel,
	ahci_exec_command,
	ata_get_params,
	ahci_ata_addref,
	ahci_ata_delref,
	ahci_killpending
};

static void ahci_intr_port(struct ahci_softc *, struct ahci_channel *);
static void ahci_setup_port(struct ahci_softc *sc, int i);

static void
ahci_enable(struct ahci_softc *sc)
{
	uint32_t ghc;

	ghc = AHCI_READ(sc, AHCI_GHC);
	if (!(ghc & AHCI_GHC_AE)) {
		ghc |= AHCI_GHC_AE;
		AHCI_WRITE(sc, AHCI_GHC, ghc);
	}
}

static int
ahci_reset(struct ahci_softc *sc)
{
	int i;

	/* reset controller */
	AHCI_WRITE(sc, AHCI_GHC, AHCI_GHC_HR);
	/* wait up to 1s for reset to complete */
	for (i = 0; i < 1000; i++) {
		delay(1000);
		if ((AHCI_READ(sc, AHCI_GHC) & AHCI_GHC_HR) == 0)
			break;
	}
	if ((AHCI_READ(sc, AHCI_GHC) & AHCI_GHC_HR)) {
		aprint_error("%s: reset failed\n", AHCINAME(sc));
		return -1;
	}
	/* enable ahci mode */
	ahci_enable(sc);

	if (sc->sc_save_init_data) {
		AHCI_WRITE(sc, AHCI_CAP, sc->sc_init_data.cap);
		if (sc->sc_init_data.cap2)
			AHCI_WRITE(sc, AHCI_CAP2, sc->sc_init_data.cap2);
		AHCI_WRITE(sc, AHCI_PI, sc->sc_init_data.ports);
	}

	return 0;
}

static void
ahci_setup_ports(struct ahci_softc *sc)
{
	int i, port;
	
	for (i = 0, port = 0; i < AHCI_MAX_PORTS; i++) {
		if ((sc->sc_ahci_ports & (1U << i)) == 0)
			continue;
		if (port >= sc->sc_atac.atac_nchannels) {
			aprint_error("%s: more ports than announced\n",
			    AHCINAME(sc));
			break;
		}
		ahci_setup_port(sc, i);
	}
}

static void
ahci_reprobe_drives(struct ahci_softc *sc)
{
	int i, port;
	struct ahci_channel *achp;
	struct ata_channel *chp;

	for (i = 0, port = 0; i < AHCI_MAX_PORTS; i++) {
		if ((sc->sc_ahci_ports & (1U << i)) == 0)
			continue;
		if (port >= sc->sc_atac.atac_nchannels) {
			aprint_error("%s: more ports than announced\n",
			    AHCINAME(sc));
			break;
		}
		achp = &sc->sc_channels[i];
		chp = &achp->ata_channel;

		ahci_probe_drive(chp);
	}
}

static void
ahci_setup_port(struct ahci_softc *sc, int i)
{
	struct ahci_channel *achp;

	achp = &sc->sc_channels[i];

	AHCI_WRITE(sc, AHCI_P_CLB(i), achp->ahcic_bus_cmdh);
	AHCI_WRITE(sc, AHCI_P_CLBU(i), (uint64_t)achp->ahcic_bus_cmdh>>32);
	AHCI_WRITE(sc, AHCI_P_FB(i), achp->ahcic_bus_rfis);
	AHCI_WRITE(sc, AHCI_P_FBU(i), (uint64_t)achp->ahcic_bus_rfis>>32);
}

static void
ahci_enable_intrs(struct ahci_softc *sc)
{

	/* clear interrupts */
	AHCI_WRITE(sc, AHCI_IS, AHCI_READ(sc, AHCI_IS));
	/* enable interrupts */
	AHCI_WRITE(sc, AHCI_GHC, AHCI_READ(sc, AHCI_GHC) | AHCI_GHC_IE);
}

void
ahci_attach(struct ahci_softc *sc)
{
	uint32_t ahci_rev;
	int i, j, port;
	struct ahci_channel *achp;
	struct ata_channel *chp;
	int error;
	int dmasize;
	char buf[128];
	void *cmdhp;
	void *cmdtblp;

	if (sc->sc_save_init_data) {
		ahci_enable(sc);

		sc->sc_init_data.cap = AHCI_READ(sc, AHCI_CAP);
		sc->sc_init_data.ports = AHCI_READ(sc, AHCI_PI);
		
		ahci_rev = AHCI_READ(sc, AHCI_VS);
		if (AHCI_VS_MJR(ahci_rev) > 1 ||
		    (AHCI_VS_MJR(ahci_rev) == 1 && AHCI_VS_MNR(ahci_rev) >= 20)) {
			sc->sc_init_data.cap2 = AHCI_READ(sc, AHCI_CAP2);
		} else {
			sc->sc_init_data.cap2 = 0;
		}
		if (sc->sc_init_data.ports == 0) {
			sc->sc_init_data.ports = sc->sc_ahci_ports;
		}
	}

	if (ahci_reset(sc) != 0)
		return;

	sc->sc_ahci_cap = AHCI_READ(sc, AHCI_CAP);
	if (sc->sc_ahci_quirks & AHCI_QUIRK_BADPMP) {
		aprint_verbose_dev(sc->sc_atac.atac_dev,
		    "ignoring broken port multiplier support\n");
		sc->sc_ahci_cap &= ~AHCI_CAP_SPM;
	}
	sc->sc_atac.atac_nchannels = (sc->sc_ahci_cap & AHCI_CAP_NPMASK) + 1;
	sc->sc_ncmds = ((sc->sc_ahci_cap & AHCI_CAP_NCS) >> 8) + 1;
	ahci_rev = AHCI_READ(sc, AHCI_VS);
	snprintb(buf, sizeof(buf), "\177\020"
			/* "f\000\005NP\0" */
			"b\005SXS\0"
			"b\006EMS\0"
			"b\007CCCS\0"
			/* "f\010\005NCS\0" */
			"b\015PSC\0"
			"b\016SSC\0"
			"b\017PMD\0"
			"b\020FBSS\0"
			"b\021SPM\0"
			"b\022SAM\0"
			"b\023SNZO\0"
			"f\024\003ISS\0"
			"=\001Gen1\0"
			"=\002Gen2\0"
			"=\003Gen3\0"
			"b\030SCLO\0"
			"b\031SAL\0"
			"b\032SALP\0"
			"b\033SSS\0"
			"b\034SMPS\0"
			"b\035SSNTF\0"
			"b\036SNCQ\0"
			"b\037S64A\0"
			"\0", sc->sc_ahci_cap);
	aprint_normal_dev(sc->sc_atac.atac_dev, "AHCI revision %u.%u"
	    ", %d port%s, %d slot%s, CAP %s\n",
	    AHCI_VS_MJR(ahci_rev), AHCI_VS_MNR(ahci_rev),
	    sc->sc_atac.atac_nchannels,
	    (sc->sc_atac.atac_nchannels == 1 ? "" : "s"), 
	    sc->sc_ncmds, (sc->sc_ncmds == 1 ? "" : "s"), buf);

	sc->sc_atac.atac_cap = ATAC_CAP_DATA16 | ATAC_CAP_DMA | ATAC_CAP_UDMA
		| ((sc->sc_ahci_cap & AHCI_CAP_NCQ) ? ATAC_CAP_NCQ : 0);
	sc->sc_atac.atac_cap |= sc->sc_atac_capflags;
	sc->sc_atac.atac_pio_cap = 4;
	sc->sc_atac.atac_dma_cap = 2;
	sc->sc_atac.atac_udma_cap = 6;
	sc->sc_atac.atac_channels = sc->sc_chanarray;
	sc->sc_atac.atac_probe = ahci_probe_drive;
	sc->sc_atac.atac_bustype_ata = &ahci_ata_bustype;
	sc->sc_atac.atac_set_modes = ahci_setup_channel;
#if NATAPIBUS > 0
	sc->sc_atac.atac_atapibus_attach = ahci_atapibus_attach;
#endif

	dmasize =
	    (AHCI_RFIS_SIZE + AHCI_CMDH_SIZE) * sc->sc_atac.atac_nchannels;
	error = bus_dmamem_alloc(sc->sc_dmat, dmasize, PAGE_SIZE, 0,
	    &sc->sc_cmd_hdr_seg, 1, &sc->sc_cmd_hdr_nseg, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error("%s: unable to allocate command header memory"
		    ", error=%d\n", AHCINAME(sc), error);
		return;
	}
	error = bus_dmamem_map(sc->sc_dmat, &sc->sc_cmd_hdr_seg,
	    sc->sc_cmd_hdr_nseg, dmasize,
	    &cmdhp, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error) {
		aprint_error("%s: unable to map command header memory"
		    ", error=%d\n", AHCINAME(sc), error);
		return;
	}
	error = bus_dmamap_create(sc->sc_dmat, dmasize, 1, dmasize, 0,
	    BUS_DMA_NOWAIT, &sc->sc_cmd_hdrd);
	if (error) {
		aprint_error("%s: unable to create command header map"
		    ", error=%d\n", AHCINAME(sc), error);
		return;
	}
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_cmd_hdrd,
	    cmdhp, dmasize, NULL, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error("%s: unable to load command header map"
		    ", error=%d\n", AHCINAME(sc), error);
		return;
	}
	sc->sc_cmd_hdr = cmdhp;

	ahci_enable_intrs(sc);

	if (sc->sc_ahci_ports == 0) {
		sc->sc_ahci_ports = AHCI_READ(sc, AHCI_PI);
		AHCIDEBUG_PRINT(("active ports %#x\n", sc->sc_ahci_ports),
		    DEBUG_PROBE);
	}
	for (i = 0, port = 0; i < AHCI_MAX_PORTS; i++) {
		if ((sc->sc_ahci_ports & (1U << i)) == 0)
			continue;
		if (port >= sc->sc_atac.atac_nchannels) {
			aprint_error("%s: more ports than announced\n",
			    AHCINAME(sc));
			break;
		}
		achp = &sc->sc_channels[i];
		chp = &achp->ata_channel;
		sc->sc_chanarray[i] = chp;
		chp->ch_channel = i;
		chp->ch_atac = &sc->sc_atac;
		chp->ch_queue = ata_queue_alloc(sc->sc_ncmds);
		if (chp->ch_queue == NULL) {
			aprint_error("%s port %d: can't allocate memory for "
			    "command queue", AHCINAME(sc), i);
			break;
		}
		dmasize = AHCI_CMDTBL_SIZE * sc->sc_ncmds;
		error = bus_dmamem_alloc(sc->sc_dmat, dmasize, PAGE_SIZE, 0,
		    &achp->ahcic_cmd_tbl_seg, 1, &achp->ahcic_cmd_tbl_nseg,
		    BUS_DMA_NOWAIT);
		if (error) {
			aprint_error("%s: unable to allocate command table "
			    "memory, error=%d\n", AHCINAME(sc), error);
			break;
		}
		error = bus_dmamem_map(sc->sc_dmat, &achp->ahcic_cmd_tbl_seg,
		    achp->ahcic_cmd_tbl_nseg, dmasize,
		    &cmdtblp, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
		if (error) {
			aprint_error("%s: unable to map command table memory"
			    ", error=%d\n", AHCINAME(sc), error);
			break;
		}
		error = bus_dmamap_create(sc->sc_dmat, dmasize, 1, dmasize, 0,
		    BUS_DMA_NOWAIT, &achp->ahcic_cmd_tbld);
		if (error) {
			aprint_error("%s: unable to create command table map"
			    ", error=%d\n", AHCINAME(sc), error);
			break;
		}
		error = bus_dmamap_load(sc->sc_dmat, achp->ahcic_cmd_tbld,
		    cmdtblp, dmasize, NULL, BUS_DMA_NOWAIT);
		if (error) {
			aprint_error("%s: unable to load command table map"
			    ", error=%d\n", AHCINAME(sc), error);
			break;
		}
		achp->ahcic_cmdh  = (struct ahci_cmd_header *)
		    ((char *)cmdhp + AHCI_CMDH_SIZE * port);
		achp->ahcic_bus_cmdh = sc->sc_cmd_hdrd->dm_segs[0].ds_addr +
		    AHCI_CMDH_SIZE * port;
		achp->ahcic_rfis = (struct ahci_r_fis *)
		    ((char *)cmdhp + 
		     AHCI_CMDH_SIZE * sc->sc_atac.atac_nchannels + 
		     AHCI_RFIS_SIZE * port);
		achp->ahcic_bus_rfis = sc->sc_cmd_hdrd->dm_segs[0].ds_addr +
		     AHCI_CMDH_SIZE * sc->sc_atac.atac_nchannels + 
		     AHCI_RFIS_SIZE * port;
		AHCIDEBUG_PRINT(("port %d cmdh %p (0x%" PRIx64 ") "
				         "rfis %p (0x%" PRIx64 ")\n", i,
		   achp->ahcic_cmdh, (uint64_t)achp->ahcic_bus_cmdh,
		   achp->ahcic_rfis, (uint64_t)achp->ahcic_bus_rfis),
		   DEBUG_PROBE);
		    
		for (j = 0; j < sc->sc_ncmds; j++) {
			achp->ahcic_cmd_tbl[j] = (struct ahci_cmd_tbl *)
			    ((char *)cmdtblp + AHCI_CMDTBL_SIZE * j);
			achp->ahcic_bus_cmd_tbl[j] =
			     achp->ahcic_cmd_tbld->dm_segs[0].ds_addr +
			     AHCI_CMDTBL_SIZE * j;
			achp->ahcic_cmdh[j].cmdh_cmdtba =
			    htole64(achp->ahcic_bus_cmd_tbl[j]);
			AHCIDEBUG_PRINT(("port %d/%d tbl %p (0x%" PRIx64 ")\n", i, j,
			    achp->ahcic_cmd_tbl[j],
			    (uint64_t)achp->ahcic_bus_cmd_tbl[j]), DEBUG_PROBE);
			/* The xfer DMA map */
			error = bus_dmamap_create(sc->sc_dmat, MAXPHYS,
			    AHCI_NPRD, 0x400000 /* 4MB */, 0,
			    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
			    &achp->ahcic_datad[j]);
			if (error) {
				aprint_error("%s: couldn't alloc xfer DMA map, "
				    "error=%d\n", AHCINAME(sc), error);
				goto end;
			}
		}
		ahci_setup_port(sc, i);
		if (bus_space_subregion(sc->sc_ahcit, sc->sc_ahcih,
		    AHCI_P_SSTS(i), 4,  &achp->ahcic_sstatus) != 0) {
			aprint_error("%s: couldn't map channel %d "
			    "sata_status regs\n", AHCINAME(sc), i);
			break;
		}
		if (bus_space_subregion(sc->sc_ahcit, sc->sc_ahcih,
		    AHCI_P_SCTL(i), 4,  &achp->ahcic_scontrol) != 0) {
			aprint_error("%s: couldn't map channel %d "
			    "sata_control regs\n", AHCINAME(sc), i);
			break;
		}
		if (bus_space_subregion(sc->sc_ahcit, sc->sc_ahcih,
		    AHCI_P_SERR(i), 4,  &achp->ahcic_serror) != 0) {
			aprint_error("%s: couldn't map channel %d "
			    "sata_error regs\n", AHCINAME(sc), i);
			break;
		}
		ata_channel_attach(chp);
		port++;
end:
		continue;
	}
}

int
ahci_detach(struct ahci_softc *sc, int flags)
{
	struct atac_softc *atac;
	struct ahci_channel *achp;
	struct ata_channel *chp;
	struct scsipi_adapter *adapt;
	int i, j;
	int error;

	atac = &sc->sc_atac;
	adapt = &atac->atac_atapi_adapter._generic;

	for (i = 0; i < AHCI_MAX_PORTS; i++) {
		achp = &sc->sc_channels[i];
		chp = &achp->ata_channel;

		if ((sc->sc_ahci_ports & (1U << i)) == 0)
			continue;
		if (i >= sc->sc_atac.atac_nchannels) {
			aprint_error("%s: more ports than announced\n",
			    AHCINAME(sc));
			break;
		}

		if (chp->atabus == NULL)
			continue;
		if ((error = config_detach(chp->atabus, flags)) != 0)
			return error;

		for (j = 0; j < sc->sc_ncmds; j++)
			bus_dmamap_destroy(sc->sc_dmat, achp->ahcic_datad[j]);

		bus_dmamap_unload(sc->sc_dmat, achp->ahcic_cmd_tbld);
		bus_dmamap_destroy(sc->sc_dmat, achp->ahcic_cmd_tbld);
		bus_dmamem_unmap(sc->sc_dmat, achp->ahcic_cmd_tbl[0],
		    AHCI_CMDTBL_SIZE * sc->sc_ncmds);
		bus_dmamem_free(sc->sc_dmat, &achp->ahcic_cmd_tbl_seg,
		    achp->ahcic_cmd_tbl_nseg);

		chp->atabus = NULL;

		ata_channel_detach(chp);
	}

	bus_dmamap_unload(sc->sc_dmat, sc->sc_cmd_hdrd);
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cmd_hdrd);
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_cmd_hdr,
	    (AHCI_RFIS_SIZE + AHCI_CMDH_SIZE) * sc->sc_atac.atac_nchannels);
	bus_dmamem_free(sc->sc_dmat, &sc->sc_cmd_hdr_seg, sc->sc_cmd_hdr_nseg);

	if (adapt->adapt_refcnt != 0)
		return EBUSY;

	return 0;
}

void
ahci_resume(struct ahci_softc *sc)
{
	ahci_reset(sc);
	ahci_setup_ports(sc);
	ahci_reprobe_drives(sc);
	ahci_enable_intrs(sc);
}

int
ahci_intr(void *v)
{
	struct ahci_softc *sc = v;
	uint32_t is;
	int i, r = 0;

	while ((is = AHCI_READ(sc, AHCI_IS))) {
		AHCIDEBUG_PRINT(("%s ahci_intr 0x%x\n", AHCINAME(sc), is),
		    DEBUG_INTR);
		r = 1;
		AHCI_WRITE(sc, AHCI_IS, is);
		for (i = 0; i < AHCI_MAX_PORTS; i++)
			if (is & (1U << i))
				ahci_intr_port(sc, &sc->sc_channels[i]);
	}
	return r;
}

static void
ahci_intr_port(struct ahci_softc *sc, struct ahci_channel *achp)
{
	uint32_t is, tfd, sact;
	struct ata_channel *chp = &achp->ata_channel;
	struct ata_xfer *xfer;
	int slot = -1;
	bool recover = false;

	is = AHCI_READ(sc, AHCI_P_IS(chp->ch_channel));
	AHCI_WRITE(sc, AHCI_P_IS(chp->ch_channel), is);

	AHCIDEBUG_PRINT((
	    "ahci_intr_port %s port %d is 0x%x CI 0x%x SACT 0x%x TFD 0x%x\n",
	    AHCINAME(sc),
	    chp->ch_channel, is,
	    AHCI_READ(sc, AHCI_P_CI(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_SACT(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_TFD(chp->ch_channel))),
	    DEBUG_INTR);

	if ((chp->ch_flags & ATACH_NCQ) == 0) {
		/* Non-NCQ operation */
		sact = AHCI_READ(sc, AHCI_P_CI(chp->ch_channel));
	} else {
		/* NCQ operation */
		sact = AHCI_READ(sc, AHCI_P_SACT(chp->ch_channel));
	}

	/* Handle errors */
	if (is & (AHCI_P_IX_TFES | AHCI_P_IX_HBFS | AHCI_P_IX_HBDS |
	    AHCI_P_IX_IFS | AHCI_P_IX_OFS | AHCI_P_IX_UFS)) {
		/* Fatal errors */
		if (is & AHCI_P_IX_TFES) {
			tfd = AHCI_READ(sc, AHCI_P_TFD(chp->ch_channel));

			if ((chp->ch_flags & ATACH_NCQ) == 0) {
				/* Slot valid only for Non-NCQ operation */
				slot = (AHCI_READ(sc,
				    AHCI_P_CMD(chp->ch_channel))
				    & AHCI_P_CMD_CCS_MASK)
				    >> AHCI_P_CMD_CCS_SHIFT;
			}

			AHCIDEBUG_PRINT((
			    "%s port %d: TFE: sact 0x%x is 0x%x tfd 0x%x\n",
			    AHCINAME(sc), chp->ch_channel, sact, is, tfd),
			    DEBUG_INTR);
		} else {
			/* mark an error, and set BSY */
			tfd = (WDCE_ABRT << AHCI_P_TFD_ERR_SHIFT) |
			    WDCS_ERR | WDCS_BSY;
		}

		if (is & AHCI_P_IX_IFS) {
			AHCIDEBUG_PRINT(("%s port %d: SERR 0x%x\n",
			    AHCINAME(sc), chp->ch_channel,
			    AHCI_READ(sc, AHCI_P_SERR(chp->ch_channel))),
			    DEBUG_INTR);
		}

		if (!achp->ahcic_recovering)
			recover = true;
	} else if (is & (AHCI_P_IX_DHRS|AHCI_P_IX_SDBS)) {
		tfd = AHCI_READ(sc, AHCI_P_TFD(chp->ch_channel));

		/* D2H Register FIS or Set Device Bits */
		if ((tfd & WDCS_ERR) != 0) {
			if (!achp->ahcic_recovering)
				recover = true;

			AHCIDEBUG_PRINT(("%s port %d: transfer aborted 0x%x\n",
			    AHCINAME(sc), chp->ch_channel, tfd), DEBUG_INTR);

		}
	} else {
		tfd = 0;
	}

	if (__predict_false(recover))
		ata_channel_freeze(chp);

	if (slot >= 0) {
		if ((achp->ahcic_cmds_active & __BIT(slot)) != 0 &&
		    (sact & __BIT(slot)) == 0) {
			xfer = ata_queue_hwslot_to_xfer(chp, slot);
			xfer->c_intr(chp, xfer, tfd);
		}
	} else {
		/*
		 * For NCQ, HBA halts processing when error is notified,
		 * and any further D2H FISes are ignored until the error
		 * condition is cleared. Hence if a command is inactive,
		 * it means it actually already finished successfully.
		 * Note: active slots can change as c_intr() callback
		 * can activate another command(s), so must only process
		 * commands active before we start processing.
		 */
		uint32_t aslots = achp->ahcic_cmds_active;

		for (slot=0; slot < sc->sc_ncmds; slot++) {
			if ((aslots & __BIT(slot)) != 0 &&
			    (sact & __BIT(slot)) == 0) {
				xfer = ata_queue_hwslot_to_xfer(chp, slot);
				xfer->c_intr(chp, xfer, tfd);
			}
		}
	}

	if (__predict_false(recover)) {
		ata_channel_thaw(chp);
		ahci_channel_recover(sc, chp, tfd);
	}
}

static void
ahci_reset_drive(struct ata_drive_datas *drvp, int flags, uint32_t *sigp)
{
	struct ata_channel *chp = drvp->chnl_softc;
	struct ahci_softc *sc = (struct ahci_softc *)chp->ch_atac;
	struct ata_xfer *xfer;

	xfer = ata_get_xfer_ext(chp, C_RECOVERY, 0);

	ata_channel_lock(chp);

	AHCI_WRITE(sc, AHCI_GHC,
	    AHCI_READ(sc, AHCI_GHC) & ~AHCI_GHC_IE);
	ahci_channel_stop(sc, chp, flags);
	if (ahci_do_reset_drive(chp, drvp->drive, flags, sigp, xfer) != 0)
		ata_reset_channel(chp, flags);
	AHCI_WRITE(sc, AHCI_GHC, AHCI_READ(sc, AHCI_GHC) | AHCI_GHC_IE);

	ata_channel_unlock(chp);

	ata_free_xfer(chp, xfer);

	return;
}

/* return error code from ata_bio */
static int
ahci_exec_fis(struct ata_channel *chp, int timeout, int flags, int slot)
{
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	struct ahci_softc *sc = (struct ahci_softc *)chp->ch_atac;
	int i;
	uint32_t is;

	/*
	 * Base timeout is specified in ms.
	 * If we are allowed to sleep, wait a tick each round.
	 * Otherwise delay for 10ms on each round.
	 */
	if (flags & AT_WAIT)
		timeout = MAX(1, mstohz(timeout));
	else
		timeout = timeout / 10;

	AHCI_CMDH_SYNC(sc, achp, slot,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	/* start command */
	AHCI_WRITE(sc, AHCI_P_CI(chp->ch_channel), 1U << slot);
	for (i = 0; i < timeout; i++) {
		if ((AHCI_READ(sc, AHCI_P_CI(chp->ch_channel)) & (1U << slot)) ==
		    0)
			return 0;
		is = AHCI_READ(sc, AHCI_P_IS(chp->ch_channel));
		if (is & (AHCI_P_IX_TFES | AHCI_P_IX_HBFS | AHCI_P_IX_HBDS |
		    AHCI_P_IX_IFS |
		    AHCI_P_IX_OFS | AHCI_P_IX_UFS)) {
			if ((is & (AHCI_P_IX_DHRS|AHCI_P_IX_TFES)) ==
			    (AHCI_P_IX_DHRS|AHCI_P_IX_TFES)) {
				/*
				 * we got the D2H FIS anyway,
				 * assume sig is valid.
				 * channel is restarted later
				 */
				return ERROR;
			}
			aprint_debug("%s channel %d: error 0x%x sending FIS\n",
			    AHCINAME(sc), chp->ch_channel, is);
			return ERR_DF;
		}
		ata_delay(chp, 10, "ahcifis", flags);
	}

	aprint_debug("%s channel %d: timeout sending FIS\n",
	    AHCINAME(sc), chp->ch_channel);
	return TIMEOUT;
}

static int
ahci_do_reset_drive(struct ata_channel *chp, int drive, int flags,
    uint32_t *sigp, struct ata_xfer *xfer)
{
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	struct ahci_softc *sc = (struct ahci_softc *)chp->ch_atac;
	struct ahci_cmd_tbl *cmd_tbl;
	struct ahci_cmd_header *cmd_h;
	int i;
	uint32_t sig;

	KASSERT((AHCI_READ(sc, AHCI_P_CMD(chp->ch_channel)) & AHCI_P_CMD_CR) == 0);
	ata_channel_lock_owned(chp);

again:
	/* clear port interrupt register */
	AHCI_WRITE(sc, AHCI_P_IS(chp->ch_channel), 0xffffffff);
	/* clear SErrors and start operations */
	if ((sc->sc_ahci_cap & AHCI_CAP_CLO) == AHCI_CAP_CLO) {
		/*
		 * issue a command list override to clear BSY.
		 * This is needed if there's a PMP with no drive
		 * on port 0
		 */
		ahci_channel_start(sc, chp, flags, 1);
	} else {
		ahci_channel_start(sc, chp, flags, 0);
	}
	if (drive > 0) {
		KASSERT(sc->sc_ahci_cap & AHCI_CAP_SPM);
	}

	if (sc->sc_ahci_quirks & AHCI_QUIRK_SKIP_RESET)
		goto skip_reset;

	/* polled command, assume interrupts are disabled */

	cmd_h = &achp->ahcic_cmdh[xfer->c_slot];
	cmd_tbl = achp->ahcic_cmd_tbl[xfer->c_slot];
	cmd_h->cmdh_flags = htole16(AHCI_CMDH_F_RST | AHCI_CMDH_F_CBSY |
	    RHD_FISLEN / 4 | (drive << AHCI_CMDH_F_PMP_SHIFT));
	cmd_h->cmdh_prdbc = 0;
	memset(cmd_tbl->cmdt_cfis, 0, 64);
	cmd_tbl->cmdt_cfis[fis_type] = RHD_FISTYPE;
	cmd_tbl->cmdt_cfis[rhd_c] = drive;
	cmd_tbl->cmdt_cfis[rhd_control] = WDCTL_RST;
	switch(ahci_exec_fis(chp, 100, flags, xfer->c_slot)) {
	case ERR_DF:
	case TIMEOUT:
		aprint_error("%s channel %d: setting WDCTL_RST failed "
		    "for drive %d\n", AHCINAME(sc), chp->ch_channel, drive);
		if (sigp)
			*sigp = 0xffffffff;
		goto end;
	default:
		break;
	}
	cmd_h->cmdh_flags = htole16(RHD_FISLEN / 4 |
	    (drive << AHCI_CMDH_F_PMP_SHIFT));
	cmd_h->cmdh_prdbc = 0;
	memset(cmd_tbl->cmdt_cfis, 0, 64);
	cmd_tbl->cmdt_cfis[fis_type] = RHD_FISTYPE;
	cmd_tbl->cmdt_cfis[rhd_c] = drive;
	cmd_tbl->cmdt_cfis[rhd_control] = 0;
	switch(ahci_exec_fis(chp, 310, flags, xfer->c_slot)) {
	case ERR_DF:
	case TIMEOUT:
		if ((sc->sc_ahci_quirks & AHCI_QUIRK_BADPMPRESET) != 0 &&
		    drive == PMP_PORT_CTL) {
			/*
			 * some controllers fails to reset when
			 * targeting a PMP but a single drive is attached.
			 * try again with port 0
			 */
			drive = 0;
			ahci_channel_stop(sc, chp, flags);
			goto again;
		}
		aprint_error("%s channel %d: clearing WDCTL_RST failed "
		    "for drive %d\n", AHCINAME(sc), chp->ch_channel, drive);
		if (sigp)
			*sigp = 0xffffffff;
		goto end;
	default:
		break;
	}

skip_reset:
	/*
	 * wait 31s for BSY to clear
	 * This should not be needed, but some controllers clear the
	 * command slot before receiving the D2H FIS ...
	 */
	for (i = 0; i < AHCI_RST_WAIT; i++) {
		sig = AHCI_READ(sc, AHCI_P_TFD(chp->ch_channel));
		if ((__SHIFTOUT(sig, AHCI_P_TFD_ST) & WDCS_BSY) == 0)
			break;
		ata_delay(chp, 10, "ahcid2h", flags);
	}
	if (i == AHCI_RST_WAIT) {
		aprint_error("%s: BSY never cleared, TD 0x%x\n",
		    AHCINAME(sc), sig);
		if (sigp)
			*sigp = 0xffffffff;
		goto end;
	}
	AHCIDEBUG_PRINT(("%s: BSY took %d ms\n", AHCINAME(sc), i * 10),
	    DEBUG_PROBE);
	sig = AHCI_READ(sc, AHCI_P_SIG(chp->ch_channel));
	if (sigp)
		*sigp = sig;
	AHCIDEBUG_PRINT(("%s: port %d: sig=0x%x CMD=0x%x\n",
	    AHCINAME(sc), chp->ch_channel, sig,
	    AHCI_READ(sc, AHCI_P_CMD(chp->ch_channel))), DEBUG_PROBE);
end:
	ahci_channel_stop(sc, chp, flags);
	ata_delay(chp, 500, "ahcirst", flags);
	/* clear port interrupt register */
	AHCI_WRITE(sc, AHCI_P_IS(chp->ch_channel), 0xffffffff);
	ahci_channel_start(sc, chp, flags,
	    (sc->sc_ahci_cap & AHCI_CAP_CLO) ? 1 : 0);
	return 0;
}

static void
ahci_reset_channel(struct ata_channel *chp, int flags)
{
	struct ahci_softc *sc = (struct ahci_softc *)chp->ch_atac;
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	int i, tfd;

	ata_channel_lock(chp);

	ahci_channel_stop(sc, chp, flags);
	if (sata_reset_interface(chp, sc->sc_ahcit, achp->ahcic_scontrol,
	    achp->ahcic_sstatus, flags) != SStatus_DET_DEV) {
		printf("%s: port %d reset failed\n", AHCINAME(sc), chp->ch_channel);
		/* XXX and then ? */
	}
	ata_kill_active(chp, KILL_RESET, flags);
	ata_delay(chp, 500, "ahcirst", flags);
	/* clear port interrupt register */
	AHCI_WRITE(sc, AHCI_P_IS(chp->ch_channel), 0xffffffff);
	/* clear SErrors and start operations */
	ahci_channel_start(sc, chp, flags,
	    (sc->sc_ahci_cap & AHCI_CAP_CLO) ? 1 : 0);
	/* wait 31s for BSY to clear */
	for (i = 0; i <AHCI_RST_WAIT; i++) {
		tfd = AHCI_READ(sc, AHCI_P_TFD(chp->ch_channel));
		if ((AHCI_TFD_ST(tfd) & WDCS_BSY) == 0)
			break;
		ata_delay(chp, 10, "ahcid2h", flags);
	}
	if ((AHCI_TFD_ST(tfd) & WDCS_BSY) != 0)
		aprint_error("%s: BSY never cleared, TD 0x%x\n",
		    AHCINAME(sc), tfd);
	AHCIDEBUG_PRINT(("%s: BSY took %d ms\n", AHCINAME(sc), i * 10),
	    DEBUG_PROBE);
	/* clear port interrupt register */
	AHCI_WRITE(sc, AHCI_P_IS(chp->ch_channel), 0xffffffff);

	ata_channel_unlock(chp);

	return;
}

static int
ahci_ata_addref(struct ata_drive_datas *drvp)
{
	return 0;
}

static void
ahci_ata_delref(struct ata_drive_datas *drvp)
{
	return;
}

static void
ahci_killpending(struct ata_drive_datas *drvp)
{
	return;
}

static void
ahci_probe_drive(struct ata_channel *chp)
{
	struct ahci_softc *sc = (struct ahci_softc *)chp->ch_atac;
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	uint32_t sig;
	struct ata_xfer *xfer;

	xfer = ata_get_xfer_ext(chp, 0, 0);
	if (xfer == NULL) {
		aprint_error_dev(sc->sc_atac.atac_dev,
		    "%s: failed to get xfer port %d\n",
		    __func__, chp->ch_channel);
		return;
        }

	ata_channel_lock(chp);

	/* bring interface up, accept FISs, power up and spin up device */
	AHCI_WRITE(sc, AHCI_P_CMD(chp->ch_channel),
	    AHCI_P_CMD_ICC_AC | AHCI_P_CMD_FRE |
	    AHCI_P_CMD_POD | AHCI_P_CMD_SUD);
	/* reset the PHY and bring online */
	switch (sata_reset_interface(chp, sc->sc_ahcit, achp->ahcic_scontrol,
	    achp->ahcic_sstatus, AT_WAIT)) {
	case SStatus_DET_DEV:
		ata_delay(chp, 500, "ahcidv", AT_WAIT);
		if (sc->sc_ahci_cap & AHCI_CAP_SPM) {
			ahci_do_reset_drive(chp, PMP_PORT_CTL, AT_WAIT, &sig,
			    xfer);
		} else {
			ahci_do_reset_drive(chp, 0, AT_WAIT, &sig, xfer);
		}
		sata_interpret_sig(chp, 0, sig);
		/* if we have a PMP attached, inform the controller */
		if (chp->ch_ndrives > PMP_PORT_CTL &&
		    chp->ch_drive[PMP_PORT_CTL].drive_type == ATA_DRIVET_PM) {
			AHCI_WRITE(sc, AHCI_P_CMD(chp->ch_channel),
			    AHCI_READ(sc, AHCI_P_CMD(chp->ch_channel)) |
			    AHCI_P_CMD_PMA);
		}
		/* clear port interrupt register */
		AHCI_WRITE(sc, AHCI_P_IS(chp->ch_channel), 0xffffffff);
		
		/* and enable interrupts */
		AHCI_WRITE(sc, AHCI_P_IE(chp->ch_channel),
		    AHCI_P_IX_TFES | AHCI_P_IX_HBFS | AHCI_P_IX_HBDS |
		    AHCI_P_IX_IFS |
		    AHCI_P_IX_OFS | AHCI_P_IX_DPS | AHCI_P_IX_UFS |
		    AHCI_P_IX_PSS | AHCI_P_IX_DHRS | AHCI_P_IX_SDBS);
		/* wait 500ms before actually starting operations */
		ata_delay(chp, 500, "ahciprb", AT_WAIT);
		break;

	default:
		break;
	}
	ata_channel_unlock(chp);
}

static void
ahci_setup_channel(struct ata_channel *chp)
{
	return;
}

static int
ahci_exec_command(struct ata_drive_datas *drvp, struct ata_xfer *xfer)
{
	struct ata_channel *chp = drvp->chnl_softc;
	struct ata_command *ata_c = &xfer->c_ata_c;
	int ret;
	int s;

	AHCIDEBUG_PRINT(("ahci_exec_command port %d CI 0x%x\n",
	    chp->ch_channel,
	    AHCI_READ(AHCI_CH2SC(chp), AHCI_P_CI(chp->ch_channel))),
	    DEBUG_XFERS);
	if (ata_c->flags & AT_POLL)
		xfer->c_flags |= C_POLL;
	if (ata_c->flags & AT_WAIT)
		xfer->c_flags |= C_WAIT;
	xfer->c_drive = drvp->drive;
	xfer->c_databuf = ata_c->data;
	xfer->c_bcount = ata_c->bcount;
	xfer->c_start = ahci_cmd_start;
	xfer->c_poll = ahci_cmd_poll;
	xfer->c_abort = ahci_cmd_abort;
	xfer->c_intr = ahci_cmd_complete;
	xfer->c_kill_xfer = ahci_cmd_kill_xfer;
	s = splbio();
	ata_exec_xfer(chp, xfer);
#ifdef DIAGNOSTIC
	if ((ata_c->flags & AT_POLL) != 0 &&
	    (ata_c->flags & AT_DONE) == 0)
		panic("ahci_exec_command: polled command not done");
#endif
	if (ata_c->flags & AT_DONE) {
		ret = ATACMD_COMPLETE;
	} else {
		if (ata_c->flags & AT_WAIT) {
			ata_channel_lock(chp);
			if ((ata_c->flags & AT_DONE) == 0) {
				ata_wait_xfer(chp, xfer);
				KASSERT((ata_c->flags & AT_DONE) != 0);
			}
			ata_channel_unlock(chp);
			ret = ATACMD_COMPLETE;
		} else {
			ret = ATACMD_QUEUED;
		}
	}
	splx(s);
	return ret;
}

static int
ahci_cmd_start(struct ata_channel *chp, struct ata_xfer *xfer)
{
	struct ahci_softc *sc = AHCI_CH2SC(chp);
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	struct ata_command *ata_c = &xfer->c_ata_c;
	int slot = xfer->c_slot;
	struct ahci_cmd_tbl *cmd_tbl;
	struct ahci_cmd_header *cmd_h;

	AHCIDEBUG_PRINT(("ahci_cmd_start CI 0x%x timo %d\n slot %d",
	    AHCI_READ(sc, AHCI_P_CI(chp->ch_channel)),
	    ata_c->timeout, slot),
	    DEBUG_XFERS);

	ata_channel_lock_owned(chp);
	KASSERT((achp->ahcic_cmds_active & (1U << slot)) == 0);

	cmd_tbl = achp->ahcic_cmd_tbl[slot];
	AHCIDEBUG_PRINT(("%s port %d tbl %p\n", AHCINAME(sc), chp->ch_channel,
	      cmd_tbl), DEBUG_XFERS);

	satafis_rhd_construct_cmd(ata_c, cmd_tbl->cmdt_cfis);
	cmd_tbl->cmdt_cfis[rhd_c] |= xfer->c_drive;

	cmd_h = &achp->ahcic_cmdh[slot];
	AHCIDEBUG_PRINT(("%s port %d header %p\n", AHCINAME(sc),
	    chp->ch_channel, cmd_h), DEBUG_XFERS);
	if (ahci_dma_setup(chp, slot,
	    (ata_c->flags & (AT_READ|AT_WRITE) && ata_c->bcount > 0) ?
	    ata_c->data : NULL,
	    ata_c->bcount,
	    (ata_c->flags & AT_READ) ? BUS_DMA_READ : BUS_DMA_WRITE)) {
		ata_c->flags |= AT_DF;
		return ATASTART_ABORT;
	}
	cmd_h->cmdh_flags = htole16(
	    ((ata_c->flags & AT_WRITE) ? AHCI_CMDH_F_WR : 0) |
	    RHD_FISLEN / 4 | (xfer->c_drive << AHCI_CMDH_F_PMP_SHIFT));
	cmd_h->cmdh_prdbc = 0;
	AHCI_CMDH_SYNC(sc, achp, slot,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	if (ata_c->flags & AT_POLL) {
		/* polled command, disable interrupts */
		AHCI_WRITE(sc, AHCI_GHC,
		    AHCI_READ(sc, AHCI_GHC) & ~AHCI_GHC_IE);
	}
	/* start command */
	AHCI_WRITE(sc, AHCI_P_CI(chp->ch_channel), 1U << slot);
	/* and says we started this command */
	achp->ahcic_cmds_active |= 1U << slot;

	if ((ata_c->flags & AT_POLL) == 0) {
		callout_reset(&xfer->c_timo_callout, mstohz(ata_c->timeout),
		    ata_timeout, xfer);
		return ATASTART_STARTED;
	} else
		return ATASTART_POLL;
}

static void
ahci_cmd_poll(struct ata_channel *chp, struct ata_xfer *xfer)
{
	struct ahci_softc *sc = AHCI_CH2SC(chp);
	struct ahci_channel *achp = (struct ahci_channel *)chp;

	ata_channel_lock(chp);

	/*
	 * Polled command. 
	 */
	for (int i = 0; i < xfer->c_ata_c.timeout / 10; i++) {
		if (xfer->c_ata_c.flags & AT_DONE)
			break;
		ata_channel_unlock(chp);
		ahci_intr_port(sc, achp);
		ata_channel_lock(chp);
		ata_delay(chp, 10, "ahcipl", xfer->c_ata_c.flags);
	}
	AHCIDEBUG_PRINT(("%s port %d poll end GHC 0x%x IS 0x%x list 0x%x%x fis 0x%x%x CMD 0x%x CI 0x%x\n", AHCINAME(sc), chp->ch_channel, 
	    AHCI_READ(sc, AHCI_GHC), AHCI_READ(sc, AHCI_IS),
	    AHCI_READ(sc, AHCI_P_CLBU(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_CLB(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_FBU(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_FB(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_CMD(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_CI(chp->ch_channel))),
	    DEBUG_XFERS);

	ata_channel_unlock(chp);

	if ((xfer->c_ata_c.flags & AT_DONE) == 0) {
		xfer->c_ata_c.flags |= AT_TIMEOU;
		xfer->c_intr(chp, xfer, 0);
	}
	/* reenable interrupts */
	AHCI_WRITE(sc, AHCI_GHC, AHCI_READ(sc, AHCI_GHC) | AHCI_GHC_IE);
}

static void
ahci_cmd_abort(struct ata_channel *chp, struct ata_xfer *xfer)
{
	ahci_cmd_complete(chp, xfer, 0);
}

static void
ahci_cmd_kill_xfer(struct ata_channel *chp, struct ata_xfer *xfer, int reason)
{
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	struct ata_command *ata_c = &xfer->c_ata_c;
	bool deactivate = true;

	AHCIDEBUG_PRINT(("ahci_cmd_kill_xfer channel %d\n", chp->ch_channel),
	    DEBUG_FUNCS);

	switch (reason) {
	case KILL_GONE_INACTIVE:
		deactivate = false;
		/* FALLTHROUGH */
	case KILL_GONE:
		ata_c->flags |= AT_GONE;
		break;
	case KILL_RESET:
		ata_c->flags |= AT_RESET;
		break;
	case KILL_REQUEUE:
		panic("%s: not supposed to be requeued\n", __func__);
		break;
	default:
		printf("ahci_cmd_kill_xfer: unknown reason %d\n", reason);
		panic("ahci_cmd_kill_xfer");
	}

	if (deactivate) {
		KASSERT((achp->ahcic_cmds_active & (1U << xfer->c_slot)) != 0);
		achp->ahcic_cmds_active &= ~(1U << xfer->c_slot);
		ata_deactivate_xfer(chp, xfer);
	}

	ahci_cmd_done_end(chp, xfer);
}

static int
ahci_cmd_complete(struct ata_channel *chp, struct ata_xfer *xfer, int tfd)
{
	struct ata_command *ata_c = &xfer->c_ata_c;
	struct ahci_channel *achp = (struct ahci_channel *)chp;

	AHCIDEBUG_PRINT(("ahci_cmd_complete channel %d CMD 0x%x CI 0x%x\n",
	    chp->ch_channel,
	    AHCI_READ(AHCI_CH2SC(chp), AHCI_P_CMD(chp->ch_channel)),
	    AHCI_READ(AHCI_CH2SC(chp), AHCI_P_CI(chp->ch_channel))),
	    DEBUG_FUNCS);

	if (ata_waitdrain_xfer_check(chp, xfer))
		return 0;

	KASSERT((achp->ahcic_cmds_active & (1U << xfer->c_slot)) != 0);
	achp->ahcic_cmds_active &= ~(1U << xfer->c_slot);
	ata_deactivate_xfer(chp, xfer);

	if (xfer->c_flags & C_TIMEOU) {
		ata_c->flags |= AT_TIMEOU;
	}

	if (AHCI_TFD_ST(tfd) & WDCS_BSY) {
		ata_c->flags |= AT_TIMEOU;
	} else if (AHCI_TFD_ST(tfd) & WDCS_ERR) {
		ata_c->r_error = AHCI_TFD_ERR(tfd);
		ata_c->flags |= AT_ERROR;
	}

	if (ata_c->flags & AT_READREG)
		satafis_rdh_cmd_readreg(ata_c, achp->ahcic_rfis->rfis_rfis);

	ahci_cmd_done(chp, xfer);
	return 0;
}

static void
ahci_cmd_done(struct ata_channel *chp, struct ata_xfer *xfer)
{
	struct ahci_softc *sc = (struct ahci_softc *)chp->ch_atac;
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	struct ata_command *ata_c = &xfer->c_ata_c;
	uint16_t *idwordbuf;
	int flags = ata_c->flags;
	int i;

	AHCIDEBUG_PRINT(("ahci_cmd_done channel %d flags %#x/%#x\n",
	    chp->ch_channel, xfer->c_flags, ata_c->flags), DEBUG_FUNCS);

	if (ata_c->flags & (AT_READ|AT_WRITE) && ata_c->bcount > 0) {
		bus_dmamap_t map = achp->ahcic_datad[xfer->c_slot];
		bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
		    (ata_c->flags & AT_READ) ? BUS_DMASYNC_POSTREAD :
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, map);
	}

	AHCI_CMDH_SYNC(sc, achp, xfer->c_slot,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	/* ata(4) expects IDENTIFY data to be in host endianess */
	if (ata_c->r_command == WDCC_IDENTIFY ||
	    ata_c->r_command == ATAPI_IDENTIFY_DEVICE) {
		idwordbuf = xfer->c_databuf;
		for (i = 0; i < (xfer->c_bcount / sizeof(*idwordbuf)); i++) {
			idwordbuf[i] = le16toh(idwordbuf[i]);
		}
	}

	if (achp->ahcic_cmdh[xfer->c_slot].cmdh_prdbc)
		ata_c->flags |= AT_XFDONE;
	ahci_cmd_done_end(chp, xfer);
	if ((flags & (AT_TIMEOU|AT_ERROR)) == 0)
		atastart(chp);
}

static void
ahci_cmd_done_end(struct ata_channel *chp, struct ata_xfer *xfer)
{
	struct ata_command *ata_c = &xfer->c_ata_c;

	ata_channel_lock(chp);

	ata_c->flags |= AT_DONE;

	if (ata_c->flags & AT_WAIT)
		ata_wake_xfer(chp, xfer);

	ata_channel_unlock(chp);
	return;
}

static int
ahci_ata_bio(struct ata_drive_datas *drvp, struct ata_xfer *xfer)
{
	struct ata_channel *chp = drvp->chnl_softc;
	struct ata_bio *ata_bio = &xfer->c_bio;

	AHCIDEBUG_PRINT(("ahci_ata_bio port %d CI 0x%x\n",
	    chp->ch_channel,
	    AHCI_READ(AHCI_CH2SC(chp), AHCI_P_CI(chp->ch_channel))),
	    DEBUG_XFERS);
	if (ata_bio->flags & ATA_POLL)
		xfer->c_flags |= C_POLL;
	xfer->c_drive = drvp->drive;
	xfer->c_databuf = ata_bio->databuf;
	xfer->c_bcount = ata_bio->bcount;
	xfer->c_start = ahci_bio_start;
	xfer->c_poll = ahci_bio_poll;
	xfer->c_abort = ahci_bio_abort;
	xfer->c_intr = ahci_bio_complete;
	xfer->c_kill_xfer = ahci_bio_kill_xfer;
	ata_exec_xfer(chp, xfer);
	return (ata_bio->flags & ATA_ITSDONE) ? ATACMD_COMPLETE : ATACMD_QUEUED;
}

static int
ahci_bio_start(struct ata_channel *chp, struct ata_xfer *xfer)
{
	struct ahci_softc *sc = (struct ahci_softc *)chp->ch_atac;
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	struct ata_bio *ata_bio = &xfer->c_bio;
	struct ahci_cmd_tbl *cmd_tbl;
	struct ahci_cmd_header *cmd_h;

	AHCIDEBUG_PRINT(("ahci_bio_start CI 0x%x\n",
	    AHCI_READ(sc, AHCI_P_CI(chp->ch_channel))), DEBUG_XFERS);

	ata_channel_lock_owned(chp);

	cmd_tbl = achp->ahcic_cmd_tbl[xfer->c_slot];
	AHCIDEBUG_PRINT(("%s port %d tbl %p\n", AHCINAME(sc), chp->ch_channel,
	      cmd_tbl), DEBUG_XFERS);

	satafis_rhd_construct_bio(xfer, cmd_tbl->cmdt_cfis);
	cmd_tbl->cmdt_cfis[rhd_c] |= xfer->c_drive;

	cmd_h = &achp->ahcic_cmdh[xfer->c_slot];
	AHCIDEBUG_PRINT(("%s port %d header %p\n", AHCINAME(sc),
	    chp->ch_channel, cmd_h), DEBUG_XFERS);
	if (ahci_dma_setup(chp, xfer->c_slot, ata_bio->databuf, ata_bio->bcount,
	    (ata_bio->flags & ATA_READ) ? BUS_DMA_READ : BUS_DMA_WRITE)) {
		ata_bio->error = ERR_DMA;
		ata_bio->r_error = 0;
		return ATASTART_ABORT;
	}
	cmd_h->cmdh_flags = htole16(
	    ((ata_bio->flags & ATA_READ) ? 0 :  AHCI_CMDH_F_WR) |
	    RHD_FISLEN / 4 | (xfer->c_drive << AHCI_CMDH_F_PMP_SHIFT));
	cmd_h->cmdh_prdbc = 0;
	AHCI_CMDH_SYNC(sc, achp, xfer->c_slot,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	if (xfer->c_flags & C_POLL) {
		/* polled command, disable interrupts */
		AHCI_WRITE(sc, AHCI_GHC,
		    AHCI_READ(sc, AHCI_GHC) & ~AHCI_GHC_IE);
	}
	if (xfer->c_flags & C_NCQ)
		AHCI_WRITE(sc, AHCI_P_SACT(chp->ch_channel), 1U << xfer->c_slot);
	/* start command */
	AHCI_WRITE(sc, AHCI_P_CI(chp->ch_channel), 1U << xfer->c_slot);
	/* and says we started this command */
	achp->ahcic_cmds_active |= 1U << xfer->c_slot;

	if ((xfer->c_flags & C_POLL) == 0) {
		callout_reset(&xfer->c_timo_callout, mstohz(ATA_DELAY),
		    ata_timeout, xfer);
		return ATASTART_STARTED;
	} else
		return ATASTART_POLL;
}

static void
ahci_bio_poll(struct ata_channel *chp, struct ata_xfer *xfer)
{
	struct ahci_softc *sc = (struct ahci_softc *)chp->ch_atac;
	struct ahci_channel *achp = (struct ahci_channel *)chp;

	/*
	 * Polled command. 
	 */
	for (int i = 0; i < ATA_DELAY * 10; i++) {
		if (xfer->c_bio.flags & ATA_ITSDONE)
			break;
		ahci_intr_port(sc, achp);
		delay(100);
	}
	AHCIDEBUG_PRINT(("%s port %d poll end GHC 0x%x IS 0x%x list 0x%x%x fis 0x%x%x CMD 0x%x CI 0x%x\n", AHCINAME(sc), chp->ch_channel, 
	    AHCI_READ(sc, AHCI_GHC), AHCI_READ(sc, AHCI_IS),
	    AHCI_READ(sc, AHCI_P_CLBU(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_CLB(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_FBU(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_FB(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_CMD(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_CI(chp->ch_channel))),
	    DEBUG_XFERS);
	if ((xfer->c_bio.flags & ATA_ITSDONE) == 0) {
		xfer->c_bio.error = TIMEOUT;
		xfer->c_intr(chp, xfer, 0);
	}
	/* reenable interrupts */
	AHCI_WRITE(sc, AHCI_GHC, AHCI_READ(sc, AHCI_GHC) | AHCI_GHC_IE);
}

static void
ahci_bio_abort(struct ata_channel *chp, struct ata_xfer *xfer)
{
	ahci_bio_complete(chp, xfer, 0);
}

static void
ahci_bio_kill_xfer(struct ata_channel *chp, struct ata_xfer *xfer, int reason)
{
	int drive = xfer->c_drive;
	struct ata_bio *ata_bio = &xfer->c_bio;
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	bool deactivate = true;

	AHCIDEBUG_PRINT(("ahci_bio_kill_xfer channel %d\n", chp->ch_channel),
	    DEBUG_FUNCS);

	ata_bio->flags |= ATA_ITSDONE;
	switch (reason) {
	case KILL_GONE_INACTIVE:
		deactivate = false;
		/* FALLTHROUGH */
	case KILL_GONE:
		ata_bio->error = ERR_NODEV;
		break;
	case KILL_RESET:
		ata_bio->error = ERR_RESET;
		break;
	case KILL_REQUEUE:
		ata_bio->error = REQUEUE;
		break;
	default:
		printf("ahci_bio_kill_xfer: unknown reason %d\n", reason);
		panic("ahci_bio_kill_xfer");
	}
	ata_bio->r_error = WDCE_ABRT;

	if (deactivate) {
		KASSERT((achp->ahcic_cmds_active & (1U << xfer->c_slot)) != 0);
		achp->ahcic_cmds_active &= ~(1U << xfer->c_slot);
		ata_deactivate_xfer(chp, xfer);
	}

	(*chp->ch_drive[drive].drv_done)(chp->ch_drive[drive].drv_softc, xfer);
}

static int
ahci_bio_complete(struct ata_channel *chp, struct ata_xfer *xfer, int tfd)
{
	struct ata_bio *ata_bio = &xfer->c_bio;
	int drive = xfer->c_drive;
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	struct ahci_softc *sc = (struct ahci_softc *)chp->ch_atac;

	AHCIDEBUG_PRINT(("ahci_bio_complete channel %d\n", chp->ch_channel),
	    DEBUG_FUNCS);

	if (ata_waitdrain_xfer_check(chp, xfer))
		return 0;

	KASSERT((achp->ahcic_cmds_active & (1U << xfer->c_slot)) != 0);
	achp->ahcic_cmds_active &= ~(1U << xfer->c_slot);
	ata_deactivate_xfer(chp, xfer);

	if (xfer->c_flags & C_TIMEOU) {
		ata_bio->error = TIMEOUT;
	}

	bus_dmamap_sync(sc->sc_dmat, achp->ahcic_datad[xfer->c_slot], 0,
	    achp->ahcic_datad[xfer->c_slot]->dm_mapsize,
	    (ata_bio->flags & ATA_READ) ? BUS_DMASYNC_POSTREAD :
	    BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, achp->ahcic_datad[xfer->c_slot]);

	ata_bio->flags |= ATA_ITSDONE;
	if (AHCI_TFD_ERR(tfd) & WDCS_DWF) { 
		ata_bio->error = ERR_DF;
	} else if (AHCI_TFD_ST(tfd) & WDCS_ERR) {
		ata_bio->error = ERROR;
		ata_bio->r_error = AHCI_TFD_ERR(tfd);
	} else if (AHCI_TFD_ST(tfd) & WDCS_CORR)
		ata_bio->flags |= ATA_CORR;

	AHCI_CMDH_SYNC(sc, achp, xfer->c_slot,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	AHCIDEBUG_PRINT(("ahci_bio_complete bcount %ld",
	    ata_bio->bcount), DEBUG_XFERS);
	/* 
	 * If it was a write, complete data buffer may have been transfered
	 * before error detection; in this case don't use cmdh_prdbc
	 * as it won't reflect what was written to media. Assume nothing
	 * was transfered and leave bcount as-is.
	 * For queued commands, PRD Byte Count should not be used, and is
	 * not required to be valid; in that case underflow is always illegal.
	 */
	if ((xfer->c_flags & C_NCQ) != 0) {
		if (ata_bio->error == NOERROR)
			ata_bio->bcount = 0;
	} else {
		if ((ata_bio->flags & ATA_READ) || ata_bio->error == NOERROR)
			ata_bio->bcount -=
			    le32toh(achp->ahcic_cmdh[xfer->c_slot].cmdh_prdbc);
	}
	AHCIDEBUG_PRINT((" now %ld\n", ata_bio->bcount), DEBUG_XFERS);
	(*chp->ch_drive[drive].drv_done)(chp->ch_drive[drive].drv_softc, xfer);
	if ((AHCI_TFD_ST(tfd) & WDCS_ERR) == 0)
		atastart(chp);
	return 0;
}

static void
ahci_channel_stop(struct ahci_softc *sc, struct ata_channel *chp, int flags)
{
	int i;
	/* stop channel */
	AHCI_WRITE(sc, AHCI_P_CMD(chp->ch_channel),
	    AHCI_READ(sc, AHCI_P_CMD(chp->ch_channel)) & ~AHCI_P_CMD_ST);
	/* wait 1s for channel to stop */
	for (i = 0; i <100; i++) {
		if ((AHCI_READ(sc, AHCI_P_CMD(chp->ch_channel)) & AHCI_P_CMD_CR)
		    == 0)
			break;
		ata_delay(chp, 10, "ahcistop", flags);
	}
	if (AHCI_READ(sc, AHCI_P_CMD(chp->ch_channel)) & AHCI_P_CMD_CR) {
		printf("%s: channel wouldn't stop\n", AHCINAME(sc));
		/* XXX controller reset ? */
		return;
	}

	if (sc->sc_channel_stop)
		sc->sc_channel_stop(sc, chp);
}

static void
ahci_channel_start(struct ahci_softc *sc, struct ata_channel *chp,
    int flags, int clo)
{
	int i;
	uint32_t p_cmd;
	/* clear error */
	AHCI_WRITE(sc, AHCI_P_SERR(chp->ch_channel),
	    AHCI_READ(sc, AHCI_P_SERR(chp->ch_channel)));

	if (clo) {
		/* issue command list override */
		KASSERT(sc->sc_ahci_cap & AHCI_CAP_CLO);
		AHCI_WRITE(sc, AHCI_P_CMD(chp->ch_channel),
		    AHCI_READ(sc, AHCI_P_CMD(chp->ch_channel)) | AHCI_P_CMD_CLO);
		/* wait 1s for AHCI_CAP_CLO to clear */
		for (i = 0; i <100; i++) {
			if ((AHCI_READ(sc, AHCI_P_CMD(chp->ch_channel)) &
			    AHCI_P_CMD_CLO) == 0)
				break;
			ata_delay(chp, 10, "ahciclo", flags);
		}
		if (AHCI_READ(sc, AHCI_P_CMD(chp->ch_channel)) & AHCI_P_CMD_CLO) {
			printf("%s: channel wouldn't CLO\n", AHCINAME(sc));
			/* XXX controller reset ? */
			return;
		}
	}

	if (sc->sc_channel_start)
		sc->sc_channel_start(sc, chp);

	/* and start controller */
	p_cmd = AHCI_P_CMD_ICC_AC | AHCI_P_CMD_POD | AHCI_P_CMD_SUD |
	    AHCI_P_CMD_FRE | AHCI_P_CMD_ST;
	if (chp->ch_ndrives > PMP_PORT_CTL &&
	    chp->ch_drive[PMP_PORT_CTL].drive_type == ATA_DRIVET_PM) {
		p_cmd |= AHCI_P_CMD_PMA;
	}
	AHCI_WRITE(sc, AHCI_P_CMD(chp->ch_channel), p_cmd);
}

static void
ahci_hold(struct ahci_channel *achp)
{
	achp->ahcic_cmds_hold |= achp->ahcic_cmds_active;
	achp->ahcic_cmds_active = 0;
}

static void
ahci_unhold(struct ahci_channel *achp)
{
	achp->ahcic_cmds_active = achp->ahcic_cmds_hold;
	achp->ahcic_cmds_hold = 0;
}

/* Recover channel after command failure */
void
ahci_channel_recover(struct ahci_softc *sc, struct ata_channel *chp, int tfd)
{
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	struct ata_drive_datas *drvp;
	uint8_t slot, eslot, st, err;
	int drive = -1, error;
	struct ata_xfer *xfer;
	bool reset = false;

	KASSERT(!achp->ahcic_recovering);

	achp->ahcic_recovering = true;

	/*
	 * Read FBS to get the drive which caused the error, if PM is in use.
	 * According to AHCI 1.3 spec, this register is available regardless
	 * if FIS-based switching (FBSS) feature is supported, or disabled.
	 * If FIS-based switching is not in use, it merely maintains single
	 * pair of DRQ/BSY state, but it is enough since in that case we
	 * never issue commands for more than one device at the time anyway.
	 * XXX untested
	 */
	if (chp->ch_ndrives > PMP_PORT_CTL) {
		uint32_t fbs = AHCI_READ(sc, AHCI_P_FBS(chp->ch_channel));
		if (fbs & AHCI_P_FBS_SDE) {
			drive = (fbs & AHCI_P_FBS_DWE) >> AHCI_P_FBS_DWE_SHIFT;

			/*
			 * Tell HBA to reset PM port X (value in DWE) state,
			 * and resume processing commands for other ports.
			 */
			fbs |= AHCI_P_FBS_DEC;
			AHCI_WRITE(sc, AHCI_P_FBS(chp->ch_channel), fbs);
			for (int i = 0; i < 1000; i++) {
				fbs = AHCI_READ(sc,
				    AHCI_P_FBS(chp->ch_channel));
				if ((fbs & AHCI_P_FBS_DEC) == 0)
					break;
				DELAY(1000);
			}
			if ((fbs & AHCI_P_FBS_DEC) != 0) {
				/* follow non-device specific recovery */
				drive = -1;
				reset = true;
			}
		} else {
			/* not device specific, reset channel */
			drive = -1;
			reset = true;
		}
	} else
		drive = 0;

	drvp = &chp->ch_drive[drive];

	/*
	 * If BSY or DRQ bits are set, must execute COMRESET to return
	 * device to idle state. If drive is idle, it's enough to just
	 * reset CMD.ST, it's not necessary to do software reset.
	 * After resetting CMD.ST, need to execute READ LOG EXT for NCQ
	 * to unblock device processing if COMRESET was not done.
	 */
	if (reset || (AHCI_TFD_ST(tfd) & (WDCS_BSY|WDCS_DRQ)) != 0)
		goto reset;

	KASSERT(drive >= 0);
	ahci_channel_stop(sc, chp, AT_POLL);
	ahci_channel_start(sc, chp, AT_POLL,
   	    (sc->sc_ahci_cap & AHCI_CAP_CLO) ? 1 : 0);

	ahci_hold(achp);

	/*
	 * When running NCQ commands, READ LOG EXT is necessary to clear the
	 * error condition and unblock the device.
	 */
	error = ata_read_log_ext_ncq(drvp, AT_POLL, &eslot, &st, &err);

	ahci_unhold(achp);

	switch (error) {
	case 0:
		/* Error out the particular NCQ xfer, then requeue the others */
		if ((achp->ahcic_cmds_active & (1U << eslot)) != 0) {
			xfer = ata_queue_hwslot_to_xfer(chp, eslot);
			xfer->c_flags |= C_RECOVERED;
			xfer->c_intr(chp, xfer,
			    (err << AHCI_P_TFD_ERR_SHIFT) | st);
		}
		break;

	case EOPNOTSUPP:
		/*
		 * Non-NCQ command error, just find the slot and end with
		 * the error.
		 */
		for (slot = 0; slot < sc->sc_ncmds; slot++) {
			if ((achp->ahcic_cmds_active & (1U << slot)) != 0) {
				xfer = ata_queue_hwslot_to_xfer(chp, slot);
				xfer->c_intr(chp, xfer, tfd);
			}
		}
		break;

	case EAGAIN:
		/*
		 * Failed to get resources to run the recovery command, must
		 * reset the drive. This will also kill all still outstanding
		 * transfers.
		 */
reset:
		ahci_reset_channel(chp, AT_POLL);
		goto out;
		/* NOTREACHED */

	default:
		/*
		 * The command to get the slot failed. Kill outstanding
		 * commands for the same drive only. No need to reset
		 * the drive, it's unblocked nevertheless.
		 */
		break;
	}

	/* Requeue all unfinished commands for same drive as failed command */ 
	for (slot = 0; slot < sc->sc_ncmds; slot++) {
		if ((achp->ahcic_cmds_active & (1U << slot)) == 0)
			continue;

		xfer = ata_queue_hwslot_to_xfer(chp, slot);
		if (drive != xfer->c_drive)
			continue;

		xfer->c_kill_xfer(chp, xfer,
		    (error == 0) ? KILL_REQUEUE : KILL_RESET);
	}

out:
	/* Drive unblocked, back to normal operation */
	achp->ahcic_recovering = false;
	atastart(chp);
}

static int
ahci_dma_setup(struct ata_channel *chp, int slot, void *data,
    size_t count, int op)
{
	int error, seg;
	struct ahci_softc *sc = (struct ahci_softc *)chp->ch_atac;
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	struct ahci_cmd_tbl *cmd_tbl;
	struct ahci_cmd_header *cmd_h;

	cmd_h = &achp->ahcic_cmdh[slot];
	cmd_tbl = achp->ahcic_cmd_tbl[slot];

	if (data == NULL) {
		cmd_h->cmdh_prdtl = 0;
		goto end;
	}

	error = bus_dmamap_load(sc->sc_dmat, achp->ahcic_datad[slot],
	    data, count, NULL,
	    BUS_DMA_NOWAIT | BUS_DMA_STREAMING | op);
	if (error) {
		printf("%s port %d: failed to load xfer: %d\n",
		    AHCINAME(sc), chp->ch_channel, error);
		return error;
	}
	bus_dmamap_sync(sc->sc_dmat, achp->ahcic_datad[slot], 0,
	    achp->ahcic_datad[slot]->dm_mapsize, 
	    (op == BUS_DMA_READ) ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
	for (seg = 0; seg <  achp->ahcic_datad[slot]->dm_nsegs; seg++) {
		cmd_tbl->cmdt_prd[seg].prd_dba = htole64(
		     achp->ahcic_datad[slot]->dm_segs[seg].ds_addr);
		cmd_tbl->cmdt_prd[seg].prd_dbc = htole32(
		    achp->ahcic_datad[slot]->dm_segs[seg].ds_len - 1);
	}
	cmd_tbl->cmdt_prd[seg - 1].prd_dbc |= htole32(AHCI_PRD_DBC_IPC);
	cmd_h->cmdh_prdtl = htole16(achp->ahcic_datad[slot]->dm_nsegs);
end:
	AHCI_CMDTBL_SYNC(sc, achp, slot, BUS_DMASYNC_PREWRITE);
	return 0;
}

#if NATAPIBUS > 0
static void
ahci_atapibus_attach(struct atabus_softc * ata_sc)
{
	struct ata_channel *chp = ata_sc->sc_chan;
	struct atac_softc *atac = chp->ch_atac;
	struct scsipi_adapter *adapt = &atac->atac_atapi_adapter._generic;
	struct scsipi_channel *chan = &chp->ch_atapi_channel;
	/*
	 * Fill in the scsipi_adapter.
	 */
	adapt->adapt_dev = atac->atac_dev;
	adapt->adapt_nchannels = atac->atac_nchannels;
	adapt->adapt_request = ahci_atapi_scsipi_request;
	adapt->adapt_minphys = ahci_atapi_minphys;
	atac->atac_atapi_adapter.atapi_probe_device = ahci_atapi_probe_device;

	/*
	 * Fill in the scsipi_channel.
	 */
	memset(chan, 0, sizeof(*chan));
	chan->chan_adapter = adapt;
	chan->chan_bustype = &ahci_atapi_bustype;
	chan->chan_channel = chp->ch_channel;
	chan->chan_flags = SCSIPI_CHAN_OPENINGS;
	chan->chan_openings = 1;
	chan->chan_max_periph = 1;
	chan->chan_ntargets = 1;
	chan->chan_nluns = 1;
	chp->atapibus = config_found_ia(ata_sc->sc_dev, "atapi", chan,
		atapiprint);
}

static void
ahci_atapi_minphys(struct buf *bp)
{
	if (bp->b_bcount > MAXPHYS)
		bp->b_bcount = MAXPHYS;
	minphys(bp);
}

/*
 * Kill off all pending xfers for a periph.
 *
 * Must be called at splbio().
 */
static void
ahci_atapi_kill_pending(struct scsipi_periph *periph)
{
	struct atac_softc *atac =
	    device_private(periph->periph_channel->chan_adapter->adapt_dev);
	struct ata_channel *chp =
	    atac->atac_channels[periph->periph_channel->chan_channel];

	ata_kill_pending(&chp->ch_drive[periph->periph_target]);
}

static void
ahci_atapi_scsipi_request(struct scsipi_channel *chan,
    scsipi_adapter_req_t req, void *arg)
{
	struct scsipi_adapter *adapt = chan->chan_adapter;
	struct scsipi_periph *periph;
	struct scsipi_xfer *sc_xfer;
	struct ahci_softc *sc = device_private(adapt->adapt_dev);
	struct atac_softc *atac = &sc->sc_atac;
	struct ata_xfer *xfer;
	int channel = chan->chan_channel;
	int drive, s;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		sc_xfer = arg;
		periph = sc_xfer->xs_periph;
		drive = periph->periph_target;
		if (!device_is_active(atac->atac_dev)) {
			sc_xfer->error = XS_DRIVER_STUFFUP;
			scsipi_done(sc_xfer);
			return;
		}
		xfer = ata_get_xfer_ext(atac->atac_channels[channel], 0, 0);
		if (xfer == NULL) {
			sc_xfer->error = XS_RESOURCE_SHORTAGE;
			scsipi_done(sc_xfer);
			return;
		}

		if (sc_xfer->xs_control & XS_CTL_POLL)
			xfer->c_flags |= C_POLL;
		xfer->c_drive = drive;
		xfer->c_flags |= C_ATAPI;
		xfer->c_scsipi = sc_xfer;
		xfer->c_databuf = sc_xfer->data;
		xfer->c_bcount = sc_xfer->datalen;
		xfer->c_start = ahci_atapi_start;
		xfer->c_poll = ahci_atapi_poll;
		xfer->c_abort = ahci_atapi_abort;
		xfer->c_intr = ahci_atapi_complete;
		xfer->c_kill_xfer = ahci_atapi_kill_xfer;
		xfer->c_dscpoll = 0;
		s = splbio();
		ata_exec_xfer(atac->atac_channels[channel], xfer);
#ifdef DIAGNOSTIC
		if ((sc_xfer->xs_control & XS_CTL_POLL) != 0 &&
		    (sc_xfer->xs_status & XS_STS_DONE) == 0)
			panic("ahci_atapi_scsipi_request: polled command "
			    "not done");
#endif
		splx(s);
		return;
	default:
		/* Not supported, nothing to do. */
		;
	}
}

static int
ahci_atapi_start(struct ata_channel *chp, struct ata_xfer *xfer)
{
	struct ahci_softc *sc = (struct ahci_softc *)chp->ch_atac;
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	struct scsipi_xfer *sc_xfer = xfer->c_scsipi;
	struct ahci_cmd_tbl *cmd_tbl;
	struct ahci_cmd_header *cmd_h;

	AHCIDEBUG_PRINT(("ahci_atapi_start CI 0x%x\n",
	    AHCI_READ(sc, AHCI_P_CI(chp->ch_channel))), DEBUG_XFERS);

	ata_channel_lock_owned(chp);

	cmd_tbl = achp->ahcic_cmd_tbl[xfer->c_slot];
	AHCIDEBUG_PRINT(("%s port %d tbl %p\n", AHCINAME(sc), chp->ch_channel,
	      cmd_tbl), DEBUG_XFERS);

	satafis_rhd_construct_atapi(xfer, cmd_tbl->cmdt_cfis);
	cmd_tbl->cmdt_cfis[rhd_c] |= xfer->c_drive;
	memset(&cmd_tbl->cmdt_acmd, 0, sizeof(cmd_tbl->cmdt_acmd));
	memcpy(cmd_tbl->cmdt_acmd, sc_xfer->cmd, sc_xfer->cmdlen);

	cmd_h = &achp->ahcic_cmdh[xfer->c_slot];
	AHCIDEBUG_PRINT(("%s port %d header %p\n", AHCINAME(sc),
	    chp->ch_channel, cmd_h), DEBUG_XFERS);
	if (ahci_dma_setup(chp, xfer->c_slot,
	    sc_xfer->datalen ? sc_xfer->data : NULL,
	    sc_xfer->datalen,
	    (sc_xfer->xs_control & XS_CTL_DATA_IN) ?
	    BUS_DMA_READ : BUS_DMA_WRITE)) {
		sc_xfer->error = XS_DRIVER_STUFFUP;
		return ATASTART_ABORT;
	}
	cmd_h->cmdh_flags = htole16(
	    ((sc_xfer->xs_control & XS_CTL_DATA_OUT) ? AHCI_CMDH_F_WR : 0) |
	    RHD_FISLEN / 4 | AHCI_CMDH_F_A |
	    (xfer->c_drive << AHCI_CMDH_F_PMP_SHIFT));
	cmd_h->cmdh_prdbc = 0;
	AHCI_CMDH_SYNC(sc, achp, xfer->c_slot,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	if (xfer->c_flags & C_POLL) {
		/* polled command, disable interrupts */
		AHCI_WRITE(sc, AHCI_GHC,
		    AHCI_READ(sc, AHCI_GHC) & ~AHCI_GHC_IE);
	}
	/* start command */
	AHCI_WRITE(sc, AHCI_P_CI(chp->ch_channel), 1U << xfer->c_slot);
	/* and says we started this command */
	achp->ahcic_cmds_active |= 1U << xfer->c_slot;

	if ((xfer->c_flags & C_POLL) == 0) {
		callout_reset(&xfer->c_timo_callout, mstohz(sc_xfer->timeout),
		    ata_timeout, xfer);
		return ATASTART_STARTED;
	} else
		return ATASTART_POLL;
}

static void
ahci_atapi_poll(struct ata_channel *chp, struct ata_xfer *xfer)
{
	struct ahci_softc *sc = (struct ahci_softc *)chp->ch_atac;
	struct ahci_channel *achp = (struct ahci_channel *)chp;

	/*
	 * Polled command. 
	 */
	for (int i = 0; i < ATA_DELAY / 10; i++) {
		if (xfer->c_scsipi->xs_status & XS_STS_DONE)
			break;
		ahci_intr_port(sc, achp);
		delay(10000);
	}
	AHCIDEBUG_PRINT(("%s port %d poll end GHC 0x%x IS 0x%x list 0x%x%x fis 0x%x%x CMD 0x%x CI 0x%x\n", AHCINAME(sc), chp->ch_channel, 
	    AHCI_READ(sc, AHCI_GHC), AHCI_READ(sc, AHCI_IS),
	    AHCI_READ(sc, AHCI_P_CLBU(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_CLB(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_FBU(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_FB(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_CMD(chp->ch_channel)),
	    AHCI_READ(sc, AHCI_P_CI(chp->ch_channel))),
	    DEBUG_XFERS);
	if ((xfer->c_scsipi->xs_status & XS_STS_DONE) == 0) {
		xfer->c_scsipi->error = XS_TIMEOUT;
		xfer->c_intr(chp, xfer, 0);
	}
	/* reenable interrupts */
	AHCI_WRITE(sc, AHCI_GHC, AHCI_READ(sc, AHCI_GHC) | AHCI_GHC_IE);
}

static void
ahci_atapi_abort(struct ata_channel *chp, struct ata_xfer *xfer)
{
	ahci_atapi_complete(chp, xfer, 0);
}

static int
ahci_atapi_complete(struct ata_channel *chp, struct ata_xfer *xfer, int tfd)
{
	struct scsipi_xfer *sc_xfer = xfer->c_scsipi;
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	struct ahci_softc *sc = (struct ahci_softc *)chp->ch_atac;

	AHCIDEBUG_PRINT(("ahci_atapi_complete channel %d\n", chp->ch_channel),
	    DEBUG_FUNCS);

	if (ata_waitdrain_xfer_check(chp, xfer))
		return 0;

	KASSERT((achp->ahcic_cmds_active & (1U << xfer->c_slot)) != 0);
	achp->ahcic_cmds_active &= ~(1U << xfer->c_slot);
	ata_deactivate_xfer(chp, xfer);

	if (xfer->c_flags & C_TIMEOU) {
		sc_xfer->error = XS_TIMEOUT;
	}

	if (xfer->c_bcount > 0) {
		bus_dmamap_sync(sc->sc_dmat, achp->ahcic_datad[xfer->c_slot], 0,
		    achp->ahcic_datad[xfer->c_slot]->dm_mapsize,
		    (sc_xfer->xs_control & XS_CTL_DATA_IN) ?
		    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, achp->ahcic_datad[xfer->c_slot]);
	}

	AHCI_CMDH_SYNC(sc, achp, xfer->c_slot,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	sc_xfer->resid = sc_xfer->datalen;
	sc_xfer->resid -= le32toh(achp->ahcic_cmdh[xfer->c_slot].cmdh_prdbc);
	AHCIDEBUG_PRINT(("ahci_atapi_complete datalen %d resid %d\n",
	    sc_xfer->datalen, sc_xfer->resid), DEBUG_XFERS);
	if (AHCI_TFD_ST(tfd) & WDCS_ERR && 
	    ((sc_xfer->xs_control & XS_CTL_REQSENSE) == 0 ||
	    sc_xfer->resid == sc_xfer->datalen)) {
		sc_xfer->error = XS_SHORTSENSE;
		sc_xfer->sense.atapi_sense = AHCI_TFD_ERR(tfd);
		if ((sc_xfer->xs_periph->periph_quirks &
		    PQUIRK_NOSENSE) == 0) {
			/* ask scsipi to send a REQUEST_SENSE */
			sc_xfer->error = XS_BUSY;
			sc_xfer->status = SCSI_CHECK;
		}
	} 
	ata_free_xfer(chp, xfer);
	scsipi_done(sc_xfer);
	if ((AHCI_TFD_ST(tfd) & WDCS_ERR) == 0)
		atastart(chp);
	return 0;
}

static void
ahci_atapi_kill_xfer(struct ata_channel *chp, struct ata_xfer *xfer, int reason)
{
	struct scsipi_xfer *sc_xfer = xfer->c_scsipi;
	struct ahci_channel *achp = (struct ahci_channel *)chp;
	bool deactivate = true;

	/* remove this command from xfer queue */
	switch (reason) {
	case KILL_GONE_INACTIVE:
		deactivate = false;
		/* FALLTHROUGH */
	case KILL_GONE:
		sc_xfer->error = XS_DRIVER_STUFFUP;
		break;
	case KILL_RESET:
		sc_xfer->error = XS_RESET;
		break;
	case KILL_REQUEUE:
		sc_xfer->error = XS_REQUEUE;
		break;
	default:
		printf("ahci_ata_atapi_kill_xfer: unknown reason %d\n", reason);
		panic("ahci_ata_atapi_kill_xfer");
	}

	if (deactivate) {
		KASSERT((achp->ahcic_cmds_active & (1U << xfer->c_slot)) != 0);
		achp->ahcic_cmds_active &= ~(1U << xfer->c_slot);
		ata_deactivate_xfer(chp, xfer);
	}

	ata_free_xfer(chp, xfer);
	scsipi_done(sc_xfer);
}

static void
ahci_atapi_probe_device(struct atapibus_softc *sc, int target)
{
	struct scsipi_channel *chan = sc->sc_channel;
	struct scsipi_periph *periph;
	struct ataparams ids;
	struct ataparams *id = &ids;
	struct ahci_softc *ahcic =
	    device_private(chan->chan_adapter->adapt_dev);
	struct atac_softc *atac = &ahcic->sc_atac;
	struct ata_channel *chp = atac->atac_channels[chan->chan_channel];
	struct ata_drive_datas *drvp = &chp->ch_drive[target];
	struct scsipibus_attach_args sa;
	char serial_number[21], model[41], firmware_revision[9];
	int s;

	/* skip if already attached */
	if (scsipi_lookup_periph(chan, target, 0) != NULL)
		return;

	/* if no ATAPI device detected at attach time, skip */
	if (drvp->drive_type != ATA_DRIVET_ATAPI) {
		AHCIDEBUG_PRINT(("ahci_atapi_probe_device: drive %d "
		    "not present\n", target), DEBUG_PROBE);
		return;
	}

	/* Some ATAPI devices need a bit more time after software reset. */
	delay(5000);
	if (ata_get_params(drvp,  AT_WAIT, id) == 0) {
#ifdef ATAPI_DEBUG_PROBE
		printf("%s drive %d: cmdsz 0x%x drqtype 0x%x\n",
		    AHCINAME(ahcic), target,
		    id->atap_config & ATAPI_CFG_CMD_MASK,
		    id->atap_config & ATAPI_CFG_DRQ_MASK);
#endif
		periph = scsipi_alloc_periph(M_NOWAIT);
		if (periph == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "unable to allocate periph for drive %d\n",
			    target);
			return;
		}
		periph->periph_dev = NULL;
		periph->periph_channel = chan;
		periph->periph_switch = &atapi_probe_periphsw;
		periph->periph_target = target;
		periph->periph_lun = 0;
		periph->periph_quirks = PQUIRK_ONLYBIG;

#ifdef SCSIPI_DEBUG
		if (SCSIPI_DEBUG_TYPE == SCSIPI_BUSTYPE_ATAPI &&
		    SCSIPI_DEBUG_TARGET == target)
			periph->periph_dbflags |= SCSIPI_DEBUG_FLAGS;
#endif
		periph->periph_type = ATAPI_CFG_TYPE(id->atap_config);
		if (id->atap_config & ATAPI_CFG_REMOV)
			periph->periph_flags |= PERIPH_REMOVABLE;
		if (periph->periph_type == T_SEQUENTIAL) {
			s = splbio();
			drvp->drive_flags |= ATA_DRIVE_ATAPIDSCW;
			splx(s);
		}

		sa.sa_periph = periph;
		sa.sa_inqbuf.type =  ATAPI_CFG_TYPE(id->atap_config);
		sa.sa_inqbuf.removable = id->atap_config & ATAPI_CFG_REMOV ?
		    T_REMOV : T_FIXED;
		strnvisx(model, sizeof(model), id->atap_model, 40,
		    VIS_TRIM|VIS_SAFE|VIS_OCTAL);
		strnvisx(serial_number, sizeof(serial_number), id->atap_serial,
		    20, VIS_TRIM|VIS_SAFE|VIS_OCTAL);
		strnvisx(firmware_revision, sizeof(firmware_revision),
		    id->atap_revision, 8, VIS_TRIM|VIS_SAFE|VIS_OCTAL);
		sa.sa_inqbuf.vendor = model;
		sa.sa_inqbuf.product = serial_number;
		sa.sa_inqbuf.revision = firmware_revision;

		/*
		 * Determine the operating mode capabilities of the device.
		 */
		if ((id->atap_config & ATAPI_CFG_CMD_MASK) == ATAPI_CFG_CMD_16)
			periph->periph_cap |= PERIPH_CAP_CMD16;
		/* XXX This is gross. */
		periph->periph_cap |= (id->atap_config & ATAPI_CFG_DRQ_MASK);

		drvp->drv_softc = atapi_probe_device(sc, target, periph, &sa);

		if (drvp->drv_softc)
			ata_probe_caps(drvp);
		else {
			s = splbio();
			drvp->drive_type = ATA_DRIVET_NONE;
			splx(s);
		}
	} else {
		AHCIDEBUG_PRINT(("ahci_atapi_get_params: ATAPI_IDENTIFY_DEVICE "
		    "failed for drive %s:%d:%d\n",
		    AHCINAME(ahcic), chp->ch_channel, target), DEBUG_PROBE);
		s = splbio();
		drvp->drive_type = ATA_DRIVET_NONE;
		splx(s);
	}
}
#endif /* NATAPIBUS */
