/*	$NetBSD: aac.c,v 1.26.6.1 2006/04/22 11:38:54 simonb Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * Copyright (c) 2001 Scott Long
 * Copyright (c) 2001 Adaptec, Inc.
 * Copyright (c) 2000 Michael Smith
 * Copyright (c) 2000 BSDi
 * Copyright (c) 2000 Niklas Hallqvist
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
 */

/*
 * Driver for the Adaptec 'FSA' family of PCI/SCSI RAID adapters.
 *
 * TODO:
 *
 * o Management interface.
 * o Look again at some of the portability issues.
 * o Handle various AIFs (e.g., notification that a container is going away).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aac.c,v 1.26.6.1 2006/04/22 11:38:54 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <uvm/uvm_extern.h>

#include <dev/ic/aacreg.h>
#include <dev/ic/aacvar.h>
#include <dev/ic/aac_tables.h>

#include "locators.h"

static int	aac_check_firmware(struct aac_softc *);
static void	aac_describe_controller(struct aac_softc *);
static int	aac_dequeue_fib(struct aac_softc *, int, u_int32_t *,
				struct aac_fib **);
static int	aac_enqueue_fib(struct aac_softc *, int, struct aac_fib *);
static void	aac_host_command(struct aac_softc *);
static void	aac_host_response(struct aac_softc *);
static int	aac_init(struct aac_softc *);
static int	aac_print(void *, const char *);
static void	aac_shutdown(void *);
static void	aac_startup(struct aac_softc *);
static int	aac_sync_command(struct aac_softc *, u_int32_t, u_int32_t,
				 u_int32_t, u_int32_t, u_int32_t, u_int32_t *);
static int	aac_sync_fib(struct aac_softc *, u_int32_t, u_int32_t, void *,
			     u_int16_t, void *, u_int16_t *);

#ifdef AAC_DEBUG
static void	aac_print_fib(struct aac_softc *, struct aac_fib *, const char *);
#endif

/*
 * Adapter-space FIB queue manipulation.
 *
 * Note that the queue implementation here is a little funky; neither the PI or
 * CI will ever be zero.  This behaviour is a controller feature.
 */
static struct {
	int	size;
	int	notify;
} const aac_qinfo[] = {
	{ AAC_HOST_NORM_CMD_ENTRIES, AAC_DB_COMMAND_NOT_FULL },
	{ AAC_HOST_HIGH_CMD_ENTRIES, 0 },
	{ AAC_ADAP_NORM_CMD_ENTRIES, AAC_DB_COMMAND_READY },
	{ AAC_ADAP_HIGH_CMD_ENTRIES, 0 },
	{ AAC_HOST_NORM_RESP_ENTRIES, AAC_DB_RESPONSE_NOT_FULL },
	{ AAC_HOST_HIGH_RESP_ENTRIES, 0 },
	{ AAC_ADAP_NORM_RESP_ENTRIES, AAC_DB_RESPONSE_READY },
	{ AAC_ADAP_HIGH_RESP_ENTRIES, 0 }
};

#ifdef AAC_DEBUG
int	aac_debug = AAC_DEBUG;
#endif

static void	*aac_sdh;

extern struct	cfdriver aac_cd;

int
aac_attach(struct aac_softc *sc)
{
	struct aac_attach_args aaca;
	int nsegs, i, rv, state, size;
	struct aac_ccb *ac;
	struct aac_fib *fib;
	bus_addr_t fibpa;
	int locs[AACCF_NLOCS];

	SIMPLEQ_INIT(&sc->sc_ccb_free);
	SIMPLEQ_INIT(&sc->sc_ccb_queue);
	SIMPLEQ_INIT(&sc->sc_ccb_complete);

	/*
	 * Disable interrupts before we do anything.
	 */
	AAC_MASK_INTERRUPTS(sc);

	/*
	 * Initialise the adapter.
	 */
	if (aac_check_firmware(sc))
		return (EINVAL);

	if ((rv = aac_init(sc)) != 0)
		return (rv);
	aac_startup(sc);

	/*
	 * Print a little information about the controller.
	 */
	aac_describe_controller(sc);

	/*
	 * Initialize the ccbs.
	 */
	sc->sc_ccbs = malloc(sizeof(*ac) * AAC_NCCBS, M_DEVBUF,
	    M_NOWAIT | M_ZERO);
	if (sc->sc_ccbs == NULL) {
		aprint_error("%s: memory allocation failure\n",
		    sc->sc_dv.dv_xname);
		return (ENOMEM);
	}
	state = 0;
	size = sizeof(*fib) * AAC_NCCBS;

	if ((rv = bus_dmamap_create(sc->sc_dmat, size, 1, size,
	    0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &sc->sc_fibs_dmamap)) != 0) {
		aprint_error("%s: cannot create fibs dmamap\n",
		    sc->sc_dv.dv_xname);
		goto bail_out;
	}
	state++;
	if ((rv = bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0,
	    &sc->sc_fibs_seg, 1, &nsegs, BUS_DMA_NOWAIT)) != 0) {
		aprint_error("%s: can't allocate fibs structure\n",
		    sc->sc_dv.dv_xname);
		goto bail_out;
	}
	state++;
	if ((rv = bus_dmamem_map(sc->sc_dmat, &sc->sc_fibs_seg, nsegs, size,
	    (caddr_t *)&sc->sc_fibs, 0)) != 0) {
		aprint_error("%s: can't map fibs structure\n",
		    sc->sc_dv.dv_xname);
		goto bail_out;
	}
	state++;
	if ((rv = bus_dmamap_load(sc->sc_dmat, sc->sc_fibs_dmamap, sc->sc_fibs,
	    size, NULL, BUS_DMA_NOWAIT)) != 0) {
		aprint_error("%s: cannot load fibs dmamap\n",
		    sc->sc_dv.dv_xname);
		goto bail_out;
	}
	state++;

	memset(sc->sc_fibs, 0, size);
	fibpa = sc->sc_fibs_seg.ds_addr;
	fib = sc->sc_fibs;

	for (i = 0, ac = sc->sc_ccbs; i < AAC_NCCBS; i++, ac++) {
		rv = bus_dmamap_create(sc->sc_dmat, AAC_MAX_XFER,
		    AAC_MAX_SGENTRIES, AAC_MAX_XFER, 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &ac->ac_dmamap_xfer);
		if (rv) {
			while (--ac >= sc->sc_ccbs)
				bus_dmamap_destroy(sc->sc_dmat,
				    ac->ac_dmamap_xfer);
			aprint_error("%s: cannot create ccb dmamap (%d)",
			    sc->sc_dv.dv_xname, rv);
			goto bail_out;
		}

		ac->ac_fib = fib++;
		ac->ac_fibphys = fibpa;
		fibpa += sizeof(*fib);
		aac_ccb_free(sc, ac);
	}

	/*
	 * Attach devices.
	 */
	for (i = 0; i < AAC_MAX_CONTAINERS; i++) {
		if (!sc->sc_hdr[i].hd_present)
			continue;
		aaca.aaca_unit = i;

		locs[AACCF_UNIT] = i;

		config_found_sm_loc(&sc->sc_dv, "aac", locs, &aaca,
				    aac_print, config_stdsubmatch);
	}

	/*
	 * Enable interrupts, and register our shutdown hook.
	 */
	sc->sc_flags |= AAC_ONLINE;
	AAC_UNMASK_INTERRUPTS(sc);
	if (aac_sdh != NULL)
		shutdownhook_establish(aac_shutdown, NULL);
	return (0);

 bail_out:
 	bus_dmamap_unload(sc->sc_dmat, sc->sc_common_dmamap);
	bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_common,
	    sizeof(*sc->sc_common));
	bus_dmamem_free(sc->sc_dmat, &sc->sc_common_seg, 1);
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_common_dmamap);

 	if (state > 3)
 		bus_dmamap_unload(sc->sc_dmat, sc->sc_fibs_dmamap);
	if (state > 2)
		bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_fibs, size);
	if (state > 1)
		bus_dmamem_free(sc->sc_dmat, &sc->sc_fibs_seg, 1);
	if (state > 0)
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_fibs_dmamap);

	free(sc->sc_ccbs, M_DEVBUF);
	return (rv);
}

/*
 * Print autoconfiguration message for a sub-device.
 */
static int
aac_print(void *aux, const char *pnp)
{
	struct aac_attach_args *aaca;

	aaca = aux;

	if (pnp != NULL)
		aprint_normal("block device at %s", pnp);
	aprint_normal(" unit %d", aaca->aaca_unit);
	return (UNCONF);
}

/*
 * Look up a text description of a numeric error code and return a pointer to
 * same.
 */
const char *
aac_describe_code(const struct aac_code_lookup *table, u_int32_t code)
{
	int i;

	for (i = 0; table[i].string != NULL; i++)
		if (table[i].code == code)
			return (table[i].string);

	return (table[i + 1].string);
}

/*
 * bitmask_snprintf(9) format string for the adapter options.
 */
static const char *optfmt = 
    "\20\1SNAPSHOT\2CLUSTERS\3WCACHE\4DATA64\5HOSTTIME\6RAID50"
    "\7WINDOW4GB"
    "\10SCSIUPGD\11SOFTERR\12NORECOND\13SGMAP64\14ALARM\15NONDASD";

static void
aac_describe_controller(struct aac_softc *sc)
{
	u_int8_t fmtbuf[256];
	u_int8_t tbuf[AAC_FIB_DATASIZE];
	u_int16_t bufsize;
	struct aac_adapter_info *info;
	u_int8_t arg;

	arg = 0;
	if (aac_sync_fib(sc, RequestAdapterInfo, 0, &arg, sizeof(arg), &tbuf,
	    &bufsize)) {
		aprint_error("%s: RequestAdapterInfo failed\n",
		    sc->sc_dv.dv_xname);
		return;
	}
	if (bufsize != sizeof(*info)) {
		aprint_error("%s: "
		    "RequestAdapterInfo returned wrong data size (%d != %zu)\n",
		    sc->sc_dv.dv_xname, bufsize, sizeof(*info));
		return;
	}
	info = (struct aac_adapter_info *)&tbuf[0];

	aprint_normal("%s: %s at %dMHz, %dMB mem (%dMB cache), %s\n",
	    sc->sc_dv.dv_xname,
	    aac_describe_code(aac_cpu_variant, le32toh(info->CpuVariant)),
	    le32toh(info->ClockSpeed),
	    le32toh(info->TotalMem) / (1024 * 1024),
	    le32toh(info->BufferMem) / (1024 * 1024),
	    aac_describe_code(aac_battery_platform,
			      le32toh(info->batteryPlatform)));

	aprint_verbose("%s: Kernel %d.%d-%d [Build %d], ",
	    sc->sc_dv.dv_xname,
	    info->KernelRevision.external.comp.major,
	    info->KernelRevision.external.comp.minor,
	    info->KernelRevision.external.comp.dash,
	    info->KernelRevision.buildNumber);

	aprint_verbose("Monitor %d.%d-%d [Build %d], S/N %6X\n",
	    info->MonitorRevision.external.comp.major,
	    info->MonitorRevision.external.comp.minor,
	    info->MonitorRevision.external.comp.dash,
	    info->MonitorRevision.buildNumber,
	    ((u_int32_t)info->SerialNumber & 0xffffff));

	aprint_verbose("%s: Controller supports: %s\n",
	    sc->sc_dv.dv_xname,
	    bitmask_snprintf(sc->sc_supported_options, optfmt, fmtbuf,
			     sizeof(fmtbuf)));

	/* Save the kernel revision structure for later use. */
	sc->sc_revision = info->KernelRevision;
}

/*
 * Retrieve the firmware version numbers.  Dell PERC2/QC cards with firmware
 * version 1.x are not compatible with this driver.
 */
static int
aac_check_firmware(struct aac_softc *sc)
{
	u_int32_t major, minor, opts;

	if ((sc->sc_quirks & AAC_QUIRK_PERC2QC) != 0) {
		if (aac_sync_command(sc, AAC_MONKER_GETKERNVER, 0, 0, 0, 0,
		    NULL)) {
			aprint_error("%s: error reading firmware version\n",
			    sc->sc_dv.dv_xname);
			return (1);
		}

		/* These numbers are stored as ASCII! */
		major = (AAC_GET_MAILBOX(sc, 1) & 0xff) - 0x30;
		minor = (AAC_GET_MAILBOX(sc, 2) & 0xff) - 0x30;
		if (major == 1) {
			aprint_error(
			    "%s: firmware version %d.%d not supported.\n",
			    sc->sc_dv.dv_xname, major, minor);
			return (1);
		}
	}

	if (aac_sync_command(sc, AAC_MONKER_GETINFO, 0, 0, 0, 0, NULL)) {
		aprint_error("%s: GETINFO failed\n", sc->sc_dv.dv_xname);
		return (1);
	}
	opts = AAC_GET_MAILBOX(sc, 1);
	sc->sc_supported_options = opts;

	/* XXX -- Enable 64-bit sglists if we can */

	return (0);
}

static int
aac_init(struct aac_softc *sc)
{
	int nsegs, i, rv, state, norm, high;
	struct aac_adapter_init	*ip;
	u_int32_t code, qoff;

	state = 0;

	/*
	 * First wait for the adapter to come ready.
	 */
	for (i = 0; i < AAC_BOOT_TIMEOUT * 1000; i++) {
		code = AAC_GET_FWSTATUS(sc);
		if ((code & AAC_SELF_TEST_FAILED) != 0) {
			aprint_error("%s: FATAL: selftest failed\n",
			    sc->sc_dv.dv_xname);
			return (ENXIO);
		}
		if ((code & AAC_KERNEL_PANIC) != 0) {
			aprint_error("%s: FATAL: controller kernel panic\n",
			    sc->sc_dv.dv_xname);
			return (ENXIO);
		}
		if ((code & AAC_UP_AND_RUNNING) != 0)
			break;
		DELAY(1000);
	}
	if (i == AAC_BOOT_TIMEOUT * 1000) {
		aprint_error(
		    "%s: FATAL: controller not coming ready, status %x\n",
		    sc->sc_dv.dv_xname, code);
		return (ENXIO);
	}

	if ((rv = bus_dmamap_create(sc->sc_dmat, sizeof(*sc->sc_common), 1,
	    sizeof(*sc->sc_common), 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
	    &sc->sc_common_dmamap)) != 0) {
		aprint_error("%s: cannot create common dmamap\n",
		    sc->sc_dv.dv_xname);
		return (rv);
	}
	if ((rv = bus_dmamem_alloc(sc->sc_dmat, sizeof(*sc->sc_common),
	    PAGE_SIZE, 0, &sc->sc_common_seg, 1, &nsegs,
	    BUS_DMA_NOWAIT)) != 0) {
		aprint_error("%s: can't allocate common structure\n",
		    sc->sc_dv.dv_xname);
		goto bail_out;
	}
	state++;
	if ((rv = bus_dmamem_map(sc->sc_dmat, &sc->sc_common_seg, nsegs,
	    sizeof(*sc->sc_common), (caddr_t *)&sc->sc_common, 0)) != 0) {
		aprint_error("%s: can't map common structure\n",
		    sc->sc_dv.dv_xname);
		goto bail_out;
	}
	state++;
	if ((rv = bus_dmamap_load(sc->sc_dmat, sc->sc_common_dmamap,
	    sc->sc_common, sizeof(*sc->sc_common), NULL,
	    BUS_DMA_NOWAIT)) != 0) {
		aprint_error("%s: cannot load common dmamap\n",
		    sc->sc_dv.dv_xname);
		goto bail_out;
	}
	state++;

	memset(sc->sc_common, 0, sizeof(*sc->sc_common));

	/*
	 * Fill in the init structure.  This tells the adapter about the
	 * physical location of various important shared data structures.
	 */
	ip = &sc->sc_common->ac_init;
	ip->InitStructRevision = htole32(AAC_INIT_STRUCT_REVISION);

	ip->AdapterFibsPhysicalAddress = htole32(sc->sc_common_seg.ds_addr +
	    offsetof(struct aac_common, ac_fibs));
	ip->AdapterFibsVirtualAddress =
	    (void *)(intptr_t) htole32(&sc->sc_common->ac_fibs[0]);
	ip->AdapterFibsSize =
	    htole32(AAC_ADAPTER_FIBS * sizeof(struct aac_fib));
	ip->AdapterFibAlign = htole32(sizeof(struct aac_fib));

	ip->PrintfBufferAddress = htole32(sc->sc_common_seg.ds_addr +
	    offsetof(struct aac_common, ac_printf));
	ip->PrintfBufferSize = htole32(AAC_PRINTF_BUFSIZE);

	ip->HostPhysMemPages = 0;	/* not used? */
	ip->HostElapsedSeconds = 0;	/* reset later if invalid */

	/*
	 * Initialise FIB queues.  Note that it appears that the layout of
	 * the indexes and the segmentation of the entries is mandated by
	 * the adapter, which is only told about the base of the queue index
	 * fields.
	 *
	 * The initial values of the indices are assumed to inform the
	 * adapter of the sizes of the respective queues.
	 *
	 * The Linux driver uses a much more complex scheme whereby several
	 * header records are kept for each queue.  We use a couple of
	 * generic list manipulation functions which 'know' the size of each
	 * list by virtue of a table.
	 */
	qoff = offsetof(struct aac_common, ac_qbuf) + AAC_QUEUE_ALIGN;
	qoff &= ~(AAC_QUEUE_ALIGN - 1);
	sc->sc_queues = (struct aac_queue_table *)((uintptr_t)sc->sc_common + qoff);
	ip->CommHeaderAddress = htole32(sc->sc_common_seg.ds_addr +
	    ((caddr_t)sc->sc_queues - (caddr_t)sc->sc_common));
	memset(sc->sc_queues, 0, sizeof(struct aac_queue_table));

	norm = htole32(AAC_HOST_NORM_CMD_ENTRIES);
	high = htole32(AAC_HOST_HIGH_CMD_ENTRIES);

	sc->sc_queues->qt_qindex[AAC_HOST_NORM_CMD_QUEUE][AAC_PRODUCER_INDEX] =
	    norm;
	sc->sc_queues->qt_qindex[AAC_HOST_NORM_CMD_QUEUE][AAC_CONSUMER_INDEX] =
	    norm;
	sc->sc_queues->qt_qindex[AAC_HOST_HIGH_CMD_QUEUE][AAC_PRODUCER_INDEX] =
	    high;
	sc->sc_queues->qt_qindex[AAC_HOST_HIGH_CMD_QUEUE][AAC_CONSUMER_INDEX] =
	    high;

	norm = htole32(AAC_ADAP_NORM_CMD_ENTRIES);
	high = htole32(AAC_ADAP_HIGH_CMD_ENTRIES);

	sc->sc_queues->qt_qindex[AAC_ADAP_NORM_CMD_QUEUE][AAC_PRODUCER_INDEX] =
	    norm;
	sc->sc_queues->qt_qindex[AAC_ADAP_NORM_CMD_QUEUE][AAC_CONSUMER_INDEX] =
	    norm;
	sc->sc_queues->qt_qindex[AAC_ADAP_HIGH_CMD_QUEUE][AAC_PRODUCER_INDEX] =
	    high;
	sc->sc_queues->qt_qindex[AAC_ADAP_HIGH_CMD_QUEUE][AAC_CONSUMER_INDEX] =
	    high;

	norm = htole32(AAC_HOST_NORM_RESP_ENTRIES);
	high = htole32(AAC_HOST_HIGH_RESP_ENTRIES);

	sc->sc_queues->
	    qt_qindex[AAC_HOST_NORM_RESP_QUEUE][AAC_PRODUCER_INDEX] = norm;
	sc->sc_queues->
	    qt_qindex[AAC_HOST_NORM_RESP_QUEUE][AAC_CONSUMER_INDEX] = norm;
	sc->sc_queues->
	    qt_qindex[AAC_HOST_HIGH_RESP_QUEUE][AAC_PRODUCER_INDEX] = high;
	sc->sc_queues->
	    qt_qindex[AAC_HOST_HIGH_RESP_QUEUE][AAC_CONSUMER_INDEX] = high;

	norm = htole32(AAC_ADAP_NORM_RESP_ENTRIES);
	high = htole32(AAC_ADAP_HIGH_RESP_ENTRIES);

	sc->sc_queues->
	    qt_qindex[AAC_ADAP_NORM_RESP_QUEUE][AAC_PRODUCER_INDEX] = norm;
	sc->sc_queues->
	    qt_qindex[AAC_ADAP_NORM_RESP_QUEUE][AAC_CONSUMER_INDEX] = norm;
	sc->sc_queues->
	    qt_qindex[AAC_ADAP_HIGH_RESP_QUEUE][AAC_PRODUCER_INDEX] = high;
	sc->sc_queues->
	    qt_qindex[AAC_ADAP_HIGH_RESP_QUEUE][AAC_CONSUMER_INDEX] = high;

	sc->sc_qentries[AAC_HOST_NORM_CMD_QUEUE] =
	    &sc->sc_queues->qt_HostNormCmdQueue[0];
	sc->sc_qentries[AAC_HOST_HIGH_CMD_QUEUE] =
	    &sc->sc_queues->qt_HostHighCmdQueue[0];
	sc->sc_qentries[AAC_ADAP_NORM_CMD_QUEUE] =
	    &sc->sc_queues->qt_AdapNormCmdQueue[0];
	sc->sc_qentries[AAC_ADAP_HIGH_CMD_QUEUE] =
	    &sc->sc_queues->qt_AdapHighCmdQueue[0];
	sc->sc_qentries[AAC_HOST_NORM_RESP_QUEUE] =
	    &sc->sc_queues->qt_HostNormRespQueue[0];
	sc->sc_qentries[AAC_HOST_HIGH_RESP_QUEUE] =
	    &sc->sc_queues->qt_HostHighRespQueue[0];
	sc->sc_qentries[AAC_ADAP_NORM_RESP_QUEUE] =
	    &sc->sc_queues->qt_AdapNormRespQueue[0];
	sc->sc_qentries[AAC_ADAP_HIGH_RESP_QUEUE] =
	    &sc->sc_queues->qt_AdapHighRespQueue[0];

	/*
	 * Do controller-type-specific initialisation
	 */
	switch (sc->sc_hwif) {
	case AAC_HWIF_I960RX:
		AAC_SETREG4(sc, AAC_RX_ODBR, ~0);
		break;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_common_dmamap, 0,
	    sizeof(*sc->sc_common),
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	/*
	 * Give the init structure to the controller.
	 */
	if (aac_sync_command(sc, AAC_MONKER_INITSTRUCT,
	    sc->sc_common_seg.ds_addr + offsetof(struct aac_common, ac_init),
	    0, 0, 0, NULL)) {
		aprint_error("%s: error establishing init structure\n",
		    sc->sc_dv.dv_xname);
		rv = EIO;
		goto bail_out;
	}

	return (0);

 bail_out:
 	if (state > 2)
 		bus_dmamap_unload(sc->sc_dmat, sc->sc_common_dmamap);
	if (state > 1)
		bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_common,
		    sizeof(*sc->sc_common));
	if (state > 0)
		bus_dmamem_free(sc->sc_dmat, &sc->sc_common_seg, 1);
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_common_dmamap);

	return (rv);
}

/*
 * Probe for containers, create disks.
 */
static void
aac_startup(struct aac_softc *sc)
{
	struct aac_mntinfo mi;
	struct aac_mntinforesponse mir;
	struct aac_drive *hd;
	u_int16_t rsize;
	int i;

	/*
	 * Loop over possible containers.
	 */
	hd = sc->sc_hdr;

	for (i = 0; i < AAC_MAX_CONTAINERS; i++, hd++) {
		/*
		 * Request information on this container.
		 */
		memset(&mi, 0, sizeof(mi));
		mi.Command = htole32(VM_NameServe);
		mi.MntType = htole32(FT_FILESYS);
		mi.MntCount = htole32(i);
		if (aac_sync_fib(sc, ContainerCommand, 0, &mi, sizeof(mi), &mir,
		    &rsize)) {
			aprint_error("%s: error probing container %d\n",
			    sc->sc_dv.dv_xname, i);
			continue;
		}
		if (rsize != sizeof(mir)) {
			aprint_error("%s: container info response wrong size "
			    "(%d should be %zu)\n",
			    sc->sc_dv.dv_xname, rsize, sizeof(mir));
			continue;
		}

		/*
		 * Check container volume type for validity.  Note that many
		 * of the possible types may never show up.
		 */
		if (le32toh(mir.Status) != ST_OK ||
		    le32toh(mir.MntTable[0].VolType) == CT_NONE)
			continue;

		hd->hd_present = 1;
		hd->hd_size = le32toh(mir.MntTable[0].Capacity);
		hd->hd_devtype = le32toh(mir.MntTable[0].VolType);
		hd->hd_size &= ~0x1f;
		sc->sc_nunits++;
	}
}

static void
aac_shutdown(void *cookie)
{
	struct aac_softc *sc;
	struct aac_close_command cc;
	u_int32_t i;

	for (i = 0; i < aac_cd.cd_ndevs; i++) {
		if ((sc = device_lookup(&aac_cd, i)) == NULL)
			continue;
		if ((sc->sc_flags & AAC_ONLINE) == 0)
			continue;

		AAC_MASK_INTERRUPTS(sc);

		/*
		 * Send a Container shutdown followed by a HostShutdown FIB
		 * to the controller to convince it that we don't want to
		 * talk to it anymore.  We've been closed and all I/O
		 * completed already
		 */
		memset(&cc, 0, sizeof(cc));
		cc.Command = htole32(VM_CloseAll);
		cc.ContainerId = 0xffffffff;
		if (aac_sync_fib(sc, ContainerCommand, 0, &cc, sizeof(cc),
		    NULL, NULL)) {
			printf("%s: unable to halt controller\n",
			    sc->sc_dv.dv_xname);
			continue;
		}

		/*
		 * Note that issuing this command to the controller makes it
		 * shut down but also keeps it from coming back up without a
		 * reset of the PCI bus.
		 */
		if (aac_sync_fib(sc, FsaHostShutdown, AAC_FIBSTATE_SHUTDOWN,
		    &i, sizeof(i), NULL, NULL))
			printf("%s: unable to halt controller\n",
			    sc->sc_dv.dv_xname);

		sc->sc_flags &= ~AAC_ONLINE;
	}
}

/*
 * Take an interrupt.
 */
int
aac_intr(void *cookie)
{
	struct aac_softc *sc;
	u_int16_t reason;
	int claimed;

	sc = cookie;
	claimed = 0;

	AAC_DPRINTF(AAC_D_INTR, ("aac_intr(%p) ", sc));

	reason = AAC_GET_ISTATUS(sc);
	AAC_DPRINTF(AAC_D_INTR, ("istatus 0x%04x ", reason));

	/*
	 * Controller wants to talk to the log.  XXX Should we defer this?
	 */
	if ((reason & AAC_DB_PRINTF) != 0) {
		if (sc->sc_common->ac_printf[0] != '\0') {
			printf("%s: WARNING: adapter logged message:\n",
			    sc->sc_dv.dv_xname);
			printf("%s:     %.*s", sc->sc_dv.dv_xname,
			    AAC_PRINTF_BUFSIZE, sc->sc_common->ac_printf);
			sc->sc_common->ac_printf[0] = '\0';
		}
		AAC_CLEAR_ISTATUS(sc, AAC_DB_PRINTF);
		AAC_QNOTIFY(sc, AAC_DB_PRINTF);
		claimed = 1;
	}

	/*
	 * Controller has a message for us?
	 */
	if ((reason & AAC_DB_COMMAND_READY) != 0) {
		aac_host_command(sc);
		AAC_CLEAR_ISTATUS(sc, AAC_DB_COMMAND_READY);
		claimed = 1;
	}

	/*
	 * Controller has a response for us?
	 */
	if ((reason & AAC_DB_RESPONSE_READY) != 0) {
		aac_host_response(sc);
		AAC_CLEAR_ISTATUS(sc, AAC_DB_RESPONSE_READY);
		claimed = 1;
	}

	/*
	 * Spurious interrupts that we don't use - reset the mask and clear
	 * the interrupts.
	 */
	if ((reason & (AAC_DB_SYNC_COMMAND | AAC_DB_COMMAND_NOT_FULL |
            AAC_DB_RESPONSE_NOT_FULL)) != 0) {
		AAC_UNMASK_INTERRUPTS(sc);
		AAC_CLEAR_ISTATUS(sc, AAC_DB_SYNC_COMMAND |
		    AAC_DB_COMMAND_NOT_FULL | AAC_DB_RESPONSE_NOT_FULL);
		claimed = 1;
	}

	return (claimed);
}

/*
 * Handle notification of one or more FIBs coming from the controller.
 */
static void
aac_host_command(struct aac_softc *sc)
{
	struct aac_fib *fib;
	u_int32_t fib_size;

	for (;;) {
		if (aac_dequeue_fib(sc, AAC_HOST_NORM_CMD_QUEUE, &fib_size,
		    &fib))
			break;	/* nothing to do */

		bus_dmamap_sync(sc->sc_dmat, sc->sc_common_dmamap,
		    (caddr_t)fib - (caddr_t)sc->sc_common, sizeof(*fib),
		    BUS_DMASYNC_POSTREAD);

		switch (le16toh(fib->Header.Command)) {
		case AifRequest:
#ifdef notyet
			aac_handle_aif(sc,
			    (struct aac_aif_command *)&fib->data[0]);
#endif
			break;
		default:
			printf("%s: unknown command from controller\n",
			    sc->sc_dv.dv_xname);
			AAC_PRINT_FIB(sc, fib);
			break;
		}

		bus_dmamap_sync(sc->sc_dmat, sc->sc_common_dmamap,
		    (caddr_t)fib - (caddr_t)sc->sc_common, sizeof(*fib),
		    BUS_DMASYNC_PREREAD);

		/* XXX reply to FIBs requesting responses ?? */
		/* XXX how do we return these FIBs to the controller? */
	}
}

/*
 * Handle notification of one or more FIBs completed by the controller
 */
static void
aac_host_response(struct aac_softc *sc)
{
	struct aac_ccb *ac;
	struct aac_fib *fib;
	u_int32_t fib_size;

	/*
	 * Look for completed FIBs on our queue.
	 */
	for (;;) {
		if (aac_dequeue_fib(sc, AAC_HOST_NORM_RESP_QUEUE, &fib_size,
		    &fib))
			break;	/* nothing to do */

		bus_dmamap_sync(sc->sc_dmat, sc->sc_fibs_dmamap,
		    (caddr_t)fib - (caddr_t)sc->sc_fibs, sizeof(*fib),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

		if ((fib->Header.SenderData & 0x80000000) == 0) {
			/* Not valid; not sent by us. */
			AAC_PRINT_FIB(sc, fib);
		} else {
			ac = (struct aac_ccb *)((caddr_t)sc->sc_ccbs +
			    (fib->Header.SenderData & 0x7fffffff));
			fib->Header.SenderData = 0;
			SIMPLEQ_INSERT_TAIL(&sc->sc_ccb_complete, ac, ac_chain);
		}
	}

	/*
	 * Deal with any completed commands.
	 */
	while ((ac = SIMPLEQ_FIRST(&sc->sc_ccb_complete)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_ccb_complete, ac_chain);
		ac->ac_flags |= AAC_CCB_COMPLETED;

		if (ac->ac_intr != NULL)
			(*ac->ac_intr)(ac);
	}

	/*
	 * Try to submit more commands.
	 */
	if (! SIMPLEQ_EMPTY(&sc->sc_ccb_queue))
		aac_ccb_enqueue(sc, NULL);
}

/*
 * Send a synchronous command to the controller and wait for a result.
 */
static int
aac_sync_command(struct aac_softc *sc, u_int32_t command, u_int32_t arg0,
		 u_int32_t arg1, u_int32_t arg2, u_int32_t arg3, u_int32_t *sp)
{
	int i;
	u_int32_t status;
	int s;

	s = splbio();

	/* Populate the mailbox. */
	AAC_SET_MAILBOX(sc, command, arg0, arg1, arg2, arg3);

	/* Ensure the sync command doorbell flag is cleared. */
	AAC_CLEAR_ISTATUS(sc, AAC_DB_SYNC_COMMAND);

	/* ... then set it to signal the adapter. */
	AAC_QNOTIFY(sc, AAC_DB_SYNC_COMMAND);
	DELAY(AAC_SYNC_DELAY);

	/* Spin waiting for the command to complete. */
	for (i = 0; i < AAC_IMMEDIATE_TIMEOUT * 1000; i++) {
		if (AAC_GET_ISTATUS(sc) & AAC_DB_SYNC_COMMAND)
			break;
		DELAY(1000);
	}
	if (i == AAC_IMMEDIATE_TIMEOUT * 1000) {
		splx(s);
		return (EIO);
	}

	/* Clear the completion flag. */
	AAC_CLEAR_ISTATUS(sc, AAC_DB_SYNC_COMMAND);

	/* Get the command status. */
	status = AAC_GET_MAILBOXSTATUS(sc);
	splx(s);
	if (sp != NULL)
		*sp = status;

	return (0);	/* XXX Check command return status? */
}

/*
 * Send a synchronous FIB to the controller and wait for a result.
 */
static int
aac_sync_fib(struct aac_softc *sc, u_int32_t command, u_int32_t xferstate,
	     void *data, u_int16_t datasize, void *result,
	     u_int16_t *resultsize)
{
	struct aac_fib *fib;
	u_int32_t fibpa, status;

	fib = &sc->sc_common->ac_sync_fib;
	fibpa = sc->sc_common_seg.ds_addr +
	    offsetof(struct aac_common, ac_sync_fib);

	if (datasize > AAC_FIB_DATASIZE)
		return (EINVAL);

	/*
	 * Set up the sync FIB.
	 */
	fib->Header.XferState = htole32(AAC_FIBSTATE_HOSTOWNED |
	    AAC_FIBSTATE_INITIALISED | AAC_FIBSTATE_EMPTY | xferstate);
	fib->Header.Command = htole16(command);
	fib->Header.StructType = AAC_FIBTYPE_TFIB;
	fib->Header.Size = htole16(sizeof(*fib) + datasize);
	fib->Header.SenderSize = htole16(sizeof(*fib));
	fib->Header.SenderFibAddress = 0; /* htole32((u_int32_t)fib);	* XXX */
	fib->Header.ReceiverFibAddress = htole32(fibpa);

	/*
	 * Copy in data.
	 */
	if (data != NULL) {
		memcpy(fib->data, data, datasize);
		fib->Header.XferState |=
		    htole32(AAC_FIBSTATE_FROMHOST | AAC_FIBSTATE_NORM);
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_common_dmamap,
	    (caddr_t)fib - (caddr_t)sc->sc_common, sizeof(*fib),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/*
	 * Give the FIB to the controller, wait for a response.
	 */
	if (aac_sync_command(sc, AAC_MONKER_SYNCFIB, fibpa, 0, 0, 0, &status))
		return (EIO);
	if (status != 1) {
		printf("%s: syncfib command %04x status %08x\n",
			sc->sc_dv.dv_xname, command, status);
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_common_dmamap,
	    (caddr_t)fib - (caddr_t)sc->sc_common, sizeof(*fib),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

	/*
	 * Copy out the result
	 */
	if (result != NULL) {
		*resultsize = le16toh(fib->Header.Size) - sizeof(fib->Header);
		memcpy(result, fib->data, *resultsize);
	}

	return (0);
}

struct aac_ccb *
aac_ccb_alloc(struct aac_softc *sc, int flags)
{
	struct aac_ccb *ac;
	int s;

	AAC_DPRINTF(AAC_D_QUEUE, ("aac_ccb_alloc(%p, 0x%x) ", sc, flags));

	s = splbio();
	ac = SIMPLEQ_FIRST(&sc->sc_ccb_free);
#ifdef DIAGNOSTIC
	if (ac == NULL)
		panic("aac_ccb_get: no free CCBS");
#endif
	SIMPLEQ_REMOVE_HEAD(&sc->sc_ccb_free, ac_chain);
	splx(s);

	ac->ac_flags = flags;
	return (ac);
}

void
aac_ccb_free(struct aac_softc *sc, struct aac_ccb *ac)
{
	int s;

	AAC_DPRINTF(AAC_D_QUEUE, ("aac_ccb_free(%p, %p) ", sc, ac));

	ac->ac_flags = 0;
	ac->ac_intr = NULL;
	ac->ac_fib->Header.XferState = htole32(AAC_FIBSTATE_EMPTY);
	ac->ac_fib->Header.StructType = AAC_FIBTYPE_TFIB;
	ac->ac_fib->Header.Flags = 0;
	ac->ac_fib->Header.SenderSize = htole16(sizeof(*ac->ac_fib));

#ifdef AAC_DEBUG
	/*
	 * These are duplicated in aac_ccb_submit() to cover the case where
	 * an intermediate stage may have destroyed them.  They're left
	 * initialised here for debugging purposes only.
	 */
	ac->ac_fib->Header.SenderFibAddress = htole32((u_int32_t)(intptr_t/*XXX LP64*/)ac->ac_fib);
	ac->ac_fib->Header.ReceiverFibAddress = htole32(ac->ac_fibphys);
#endif

	s = splbio();
	SIMPLEQ_INSERT_HEAD(&sc->sc_ccb_free, ac, ac_chain);
	splx(s);
}

int
aac_ccb_map(struct aac_softc *sc, struct aac_ccb *ac)
{
	int error;

	AAC_DPRINTF(AAC_D_QUEUE, ("aac_ccb_map(%p, %p) ", sc, ac));

#ifdef DIAGNOSTIC
	if ((ac->ac_flags & AAC_CCB_MAPPED) != 0)
		panic("aac_ccb_map: already mapped");
#endif

	error = bus_dmamap_load(sc->sc_dmat, ac->ac_dmamap_xfer, ac->ac_data,
	    ac->ac_datalen, NULL, BUS_DMA_NOWAIT | BUS_DMA_STREAMING |
	    ((ac->ac_flags & AAC_CCB_DATA_IN) ? BUS_DMA_READ : BUS_DMA_WRITE));
	if (error) {
		printf("%s: aac_ccb_map: ", sc->sc_dv.dv_xname);
		if (error == EFBIG)
			printf("more than %d DMA segs\n", AAC_MAX_SGENTRIES);
		else
			printf("error %d loading DMA map\n", error);
		return (error);
	}

	bus_dmamap_sync(sc->sc_dmat, ac->ac_dmamap_xfer, 0, ac->ac_datalen,
	    (ac->ac_flags & AAC_CCB_DATA_IN) ? BUS_DMASYNC_PREREAD :
	    BUS_DMASYNC_PREWRITE);

#ifdef DIAGNOSTIC
	ac->ac_flags |= AAC_CCB_MAPPED;
#endif
	return (0);
}

void
aac_ccb_unmap(struct aac_softc *sc, struct aac_ccb *ac)
{

	AAC_DPRINTF(AAC_D_QUEUE, ("aac_ccb_unmap(%p, %p) ", sc, ac));

#ifdef DIAGNOSTIC
	if ((ac->ac_flags & AAC_CCB_MAPPED) == 0)
		panic("aac_ccb_unmap: not mapped");
#endif

	bus_dmamap_sync(sc->sc_dmat, ac->ac_dmamap_xfer, 0, ac->ac_datalen,
	    (ac->ac_flags & AAC_CCB_DATA_IN) ? BUS_DMASYNC_POSTREAD :
	    BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, ac->ac_dmamap_xfer);

#ifdef DIAGNOSTIC
	ac->ac_flags &= ~AAC_CCB_MAPPED;
#endif
}

void
aac_ccb_enqueue(struct aac_softc *sc, struct aac_ccb *ac)
{
	int s;

	AAC_DPRINTF(AAC_D_QUEUE, ("aac_ccb_enqueue(%p, %p) ", sc, ac));

	s = splbio();

	if (ac != NULL)
		SIMPLEQ_INSERT_TAIL(&sc->sc_ccb_queue, ac, ac_chain);

	while ((ac = SIMPLEQ_FIRST(&sc->sc_ccb_queue)) != NULL) {
		if (aac_ccb_submit(sc, ac))
			break;
		SIMPLEQ_REMOVE_HEAD(&sc->sc_ccb_queue, ac_chain);
	}

	splx(s);
}

int
aac_ccb_submit(struct aac_softc *sc, struct aac_ccb *ac)
{

	AAC_DPRINTF(AAC_D_QUEUE, ("aac_ccb_submit(%p, %p) ", sc, ac));

	/* Fix up the address values. */
	ac->ac_fib->Header.SenderFibAddress = htole32((u_int32_t)(intptr_t/*XXX LP64*/)ac->ac_fib);
	ac->ac_fib->Header.ReceiverFibAddress = htole32(ac->ac_fibphys);

	/* Save a pointer to the command for speedy reverse-lookup. */
	ac->ac_fib->Header.SenderData =
	    (u_int32_t)((caddr_t)ac - (caddr_t)sc->sc_ccbs) | 0x80000000;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_fibs_dmamap,
	    (caddr_t)ac->ac_fib - (caddr_t)sc->sc_fibs, sizeof(*ac->ac_fib),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/* Put the FIB on the outbound queue. */
	return (aac_enqueue_fib(sc, AAC_ADAP_NORM_CMD_QUEUE, ac->ac_fib));
}

int
aac_ccb_poll(struct aac_softc *sc, struct aac_ccb *ac, int timo)
{
	int rv, s;

	AAC_DPRINTF(AAC_D_QUEUE, ("aac_ccb_poll(%p, %p, %d) ", sc, ac, timo));

	s = splbio();

	if ((rv = aac_ccb_submit(sc, ac)) != 0) {
		splx(s);
		return (rv);
	}

	for (timo *= 1000; timo != 0; timo--) {
		aac_intr(sc);
		if ((ac->ac_flags & AAC_CCB_COMPLETED) != 0)
			break;
		DELAY(100);
	}

	splx(s);
	return (timo == 0);
}

/*
 * Atomically insert an entry into the nominated queue, returns 0 on success
 * or EBUSY if the queue is full.
 *
 * XXX Note that it would be more efficient to defer notifying the
 * controller in the case where we may be inserting several entries in rapid
 * succession, but implementing this usefully is difficult.
 */
static int
aac_enqueue_fib(struct aac_softc *sc, int queue, struct aac_fib *fib)
{
	u_int32_t fib_size, fib_addr, pi, ci;

	fib_size = le16toh(fib->Header.Size);
	fib_addr = le32toh(fib->Header.ReceiverFibAddress);

	bus_dmamap_sync(sc->sc_dmat, sc->sc_common_dmamap,
	    (caddr_t)sc->sc_common->ac_qbuf - (caddr_t)sc->sc_common,
	    sizeof(sc->sc_common->ac_qbuf),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

	/* Get the producer/consumer indices.  */
	pi = le32toh(sc->sc_queues->qt_qindex[queue][AAC_PRODUCER_INDEX]);
	ci = le32toh(sc->sc_queues->qt_qindex[queue][AAC_CONSUMER_INDEX]);

	/* Wrap the queue? */
	if (pi >= aac_qinfo[queue].size)
		pi = 0;

	/* Check for queue full. */
	if ((pi + 1) == ci)
		return (EAGAIN);

	/* Populate queue entry. */
	(sc->sc_qentries[queue] + pi)->aq_fib_size = htole32(fib_size);
	(sc->sc_qentries[queue] + pi)->aq_fib_addr = htole32(fib_addr);

	/* Update producer index. */
	sc->sc_queues->qt_qindex[queue][AAC_PRODUCER_INDEX] = htole32(pi + 1);

	bus_dmamap_sync(sc->sc_dmat, sc->sc_common_dmamap,
	    (caddr_t)sc->sc_common->ac_qbuf - (caddr_t)sc->sc_common,
	    sizeof(sc->sc_common->ac_qbuf),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/* Notify the adapter if we know how. */
	if (aac_qinfo[queue].notify != 0)
		AAC_QNOTIFY(sc, aac_qinfo[queue].notify);

	return (0);
}

/*
 * Atomically remove one entry from the nominated queue, returns 0 on success
 * or ENOENT if the queue is empty.
 */
static int
aac_dequeue_fib(struct aac_softc *sc, int queue, u_int32_t *fib_size,
		struct aac_fib **fib_addr)
{
	u_int32_t pi, ci;
	int notify;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_common_dmamap,
	    (caddr_t)sc->sc_common->ac_qbuf - (caddr_t)sc->sc_common,
	    sizeof(sc->sc_common->ac_qbuf),
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

	/* Get the producer/consumer indices. */
	pi = le32toh(sc->sc_queues->qt_qindex[queue][AAC_PRODUCER_INDEX]);
	ci = le32toh(sc->sc_queues->qt_qindex[queue][AAC_CONSUMER_INDEX]);

	/* Check for queue empty. */
	if (ci == pi)
		return (ENOENT);

	notify = 0;
	if (ci == pi + 1)
		notify = 1;

	/* Wrap the queue? */
	if (ci >= aac_qinfo[queue].size)
		ci = 0;

	/* Fetch the entry. */
	*fib_size = le32toh((sc->sc_qentries[queue] + ci)->aq_fib_size);
	*fib_addr = (void *)(intptr_t) le32toh(
	    (sc->sc_qentries[queue] + ci)->aq_fib_addr);

	/* Update consumer index. */
	sc->sc_queues->qt_qindex[queue][AAC_CONSUMER_INDEX] = ci + 1;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_common_dmamap,
	    (caddr_t)sc->sc_common->ac_qbuf - (caddr_t)sc->sc_common,
	    sizeof(sc->sc_common->ac_qbuf),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/* If we have made the queue un-full, notify the adapter. */
	if (notify && (aac_qinfo[queue].notify != 0))
		AAC_QNOTIFY(sc, aac_qinfo[queue].notify);

	return (0);
}

#ifdef AAC_DEBUG
/*
 * Print a FIB
 */
static void
aac_print_fib(struct aac_softc *sc, struct aac_fib *fib, const char *caller)
{
	struct aac_blockread *br;
	struct aac_blockwrite *bw;
	struct aac_sg_table *sg;
	char tbuf[512];
	int i;

	printf("%s: FIB @ %p\n", caller, fib);
	bitmask_snprintf(le32toh(fib->Header.XferState),
	    "\20"
	    "\1HOSTOWNED"
	    "\2ADAPTEROWNED"
	    "\3INITIALISED"
	    "\4EMPTY"
	    "\5FROMPOOL"
	    "\6FROMHOST"
	    "\7FROMADAP"
	    "\10REXPECTED"
	    "\11RNOTEXPECTED"
	    "\12DONEADAP"
	    "\13DONEHOST"
	    "\14HIGH"
	    "\15NORM"
	    "\16ASYNC"
	    "\17PAGEFILEIO"
	    "\20SHUTDOWN"
	    "\21LAZYWRITE"
	    "\22ADAPMICROFIB"
	    "\23BIOSFIB"
	    "\24FAST_RESPONSE"
	    "\25APIFIB\n",
	    tbuf,
	    sizeof(tbuf));

	printf("  XferState       %s\n", tbuf);
	printf("  Command         %d\n", le16toh(fib->Header.Command));
	printf("  StructType      %d\n", fib->Header.StructType);
	printf("  Flags           0x%x\n", fib->Header.Flags);
	printf("  Size            %d\n", le16toh(fib->Header.Size));
	printf("  SenderSize      %d\n", le16toh(fib->Header.SenderSize));
	printf("  SenderAddress   0x%x\n",
	    le32toh(fib->Header.SenderFibAddress));
	printf("  ReceiverAddress 0x%x\n",
	    le32toh(fib->Header.ReceiverFibAddress));
	printf("  SenderData      0x%x\n", fib->Header.SenderData);

	switch (fib->Header.Command) {
	case ContainerCommand: {
		br = (struct aac_blockread *)fib->data;
		bw = (struct aac_blockwrite *)fib->data;
		sg = NULL;

		if (le32toh(br->Command) == VM_CtBlockRead) {
			printf("  BlockRead: container %d  0x%x/%d\n",
			    le32toh(br->ContainerId), le32toh(br->BlockNumber),
			    le32toh(br->ByteCount));
			sg = &br->SgMap;
		}
		if (le32toh(bw->Command) == VM_CtBlockWrite) {
			printf("  BlockWrite: container %d  0x%x/%d (%s)\n",
			    le32toh(bw->ContainerId), le32toh(bw->BlockNumber),
			    le32toh(bw->ByteCount),
			    le32toh(bw->Stable) == CSTABLE ?
			    "stable" : "unstable");
			sg = &bw->SgMap;
		}
		if (sg != NULL) {
			printf("  %d s/g entries\n", le32toh(sg->SgCount));
			for (i = 0; i < le32toh(sg->SgCount); i++)
				printf("  0x%08x/%d\n",
				    le32toh(sg->SgEntry[i].SgAddress),
				    le32toh(sg->SgEntry[i].SgByteCount));
		}
		break;
	}
	default:
		// dump first 32 bytes of fib->data
		printf("  Raw data:");
		for (i = 0; i < 32; i++)
			printf(" %02x", fib->data[i]);
		printf("\n");
		break;
	}
}
#endif /* AAC_DEBUG */
