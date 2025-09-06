/*-
 * Copyright (c) 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: (BSD-2-Clause OR GPL-2.0)
 */

/*

These files are provided under a dual BSD-2 Clause/GPLv2 license. When
using or redistributing this file, you may do so under either license.

BSD-2 Clause License

Copyright (c) 2018 VMware, Inc.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

GPL License Summary

Copyright (c) 2018 VMware, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
The full GNU General Public License is included in this distribution
in the file called LICENSE.GPL.

*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pvscsi.c,v 1.5 2025/09/06 02:56:52 riastradh Exp $");

#include <sys/param.h>

#include <sys/buf.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/paravirt_membar.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/scsi_disk.h>

#include "pvscsi.h"

#define	PVSCSI_DEFAULT_NUM_PAGES_REQ_RING	8
#define	PVSCSI_SENSE_LENGTH			256

#define PVSCSI_MAXPHYS				MAXPHYS
#define PVSCSI_MAXPHYS_SEGS			((PVSCSI_MAXPHYS / PAGE_SIZE) + 1)

#define PVSCSI_CMD_PER_LUN 64
#define PVSCSI_MAX_LUN 8
#define PVSCSI_MAX_TARGET 16

//#define PVSCSI_DEBUG_LOGGING

#ifdef PVSCSI_DEBUG_LOGGING
#define	DEBUG_PRINTF(level, dev, fmt, ...)				\
	do {								\
		if (pvscsi_log_level >= (level)) {			\
			aprint_normal_dev((dev), (fmt), ##__VA_ARGS__);	\
		}							\
	} while(0)
#else
#define DEBUG_PRINTF(level, dev, fmt, ...)
#endif /* PVSCSI_DEBUG_LOGGING */

struct pvscsi_softc;
struct pvscsi_hcb;
struct pvscsi_dma;

#define VMWARE_PVSCSI_DEVSTR	"VMware Paravirtual SCSI Controller"

static inline uint32_t pvscsi_reg_read(struct pvscsi_softc *sc,
    uint32_t offset);
static inline void pvscsi_reg_write(struct pvscsi_softc *sc, uint32_t offset,
    uint32_t val);
static inline uint32_t pvscsi_read_intr_status(struct pvscsi_softc *sc);
static inline void pvscsi_write_intr_status(struct pvscsi_softc *sc,
    uint32_t val);
static inline void pvscsi_intr_enable(struct pvscsi_softc *sc);
static inline void pvscsi_intr_disable(struct pvscsi_softc *sc);
static void pvscsi_kick_io(struct pvscsi_softc *sc, uint8_t cdb0);
static void pvscsi_write_cmd(struct pvscsi_softc *sc, uint32_t cmd, void *data,
    uint32_t len);
static uint32_t pvscsi_get_max_targets(struct pvscsi_softc *sc);
static int pvscsi_setup_req_call(struct pvscsi_softc *sc, uint32_t enable);
static void pvscsi_setup_rings(struct pvscsi_softc *sc);
static void pvscsi_setup_msg_ring(struct pvscsi_softc *sc);
static int pvscsi_hw_supports_msg(struct pvscsi_softc *sc);

static void pvscsi_timeout(void *arg);
static void pvscsi_adapter_reset(struct pvscsi_softc *sc);
static void pvscsi_bus_reset(struct pvscsi_softc *sc);
static void pvscsi_device_reset(struct pvscsi_softc *sc, uint32_t target);
static void pvscsi_abort(struct pvscsi_softc *sc, uint32_t target,
    struct pvscsi_hcb *hcb);

static void pvscsi_process_completion(struct pvscsi_softc *sc,
    struct pvscsi_ring_cmp_desc *e);
static void pvscsi_process_cmp_ring(struct pvscsi_softc *sc);
static void pvscsi_process_msg(struct pvscsi_softc *sc,
    struct pvscsi_ring_msg_desc *e);
static void pvscsi_process_msg_ring(struct pvscsi_softc *sc);

static void pvscsi_intr_locked(struct pvscsi_softc *sc);
static int pvscsi_intr(void *xsc);

static void pvscsi_scsipi_request(struct scsipi_channel *,
    scsipi_adapter_req_t, void *);

static inline uint64_t pvscsi_hcb_to_context(struct pvscsi_softc *sc,
    struct pvscsi_hcb *hcb);
static inline struct pvscsi_hcb *pvscsi_context_to_hcb(struct pvscsi_softc *sc,
    uint64_t context);
static struct pvscsi_hcb * pvscsi_hcb_get(struct pvscsi_softc *sc);
static void pvscsi_hcb_put(struct pvscsi_softc *sc, struct pvscsi_hcb *hcb);

static void pvscsi_dma_free(struct pvscsi_softc *sc, struct pvscsi_dma *dma);
static int pvscsi_dma_alloc(struct pvscsi_softc *sc, struct pvscsi_dma *dma,
    bus_size_t size, bus_size_t alignment);
static int pvscsi_dma_alloc_ppns(struct pvscsi_softc *sc,
    struct pvscsi_dma *dma, uint64_t *ppn_list, uint32_t num_pages);
static void pvscsi_dma_free_per_hcb(struct pvscsi_softc *sc,
    uint32_t hcbs_allocated);
static int pvscsi_dma_alloc_per_hcb(struct pvscsi_softc *sc);
static void pvscsi_free_rings(struct pvscsi_softc *sc);
static int pvscsi_allocate_rings(struct pvscsi_softc *sc);
static void pvscsi_free_interrupts(struct pvscsi_softc *sc);
static int pvscsi_setup_interrupts(struct pvscsi_softc *sc, const struct pci_attach_args *);
static void pvscsi_free_all(struct pvscsi_softc *sc);

static void pvscsi_attach(device_t, device_t, void *);
static int pvscsi_detach(device_t, int);
static int pvscsi_probe(device_t, cfdata_t, void *);

#define pvscsi_get_tunable(_sc, _name, _value)	(_value)

#ifdef PVSCSI_DEBUG_LOGGING
static int pvscsi_log_level = 1;
#endif

#define TUNABLE_INT(__x, __d)					\
	err = sysctl_createv(clog, 0, &rnode, &cnode,		\
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,	\
	    #__x, SYSCTL_DESCR(__d),				\
	    NULL, 0, &(pvscsi_ ## __x), sizeof(pvscsi_ ## __x), \
	    CTL_CREATE,	CTL_EOL);				\
	if (err)						\
		goto fail;

static int pvscsi_request_ring_pages = 0;
static int pvscsi_use_msg = 1;
static int pvscsi_use_msi = 1;
static int pvscsi_use_msix = 1;
static int pvscsi_use_req_call_threshold = 0;
static int pvscsi_max_queue_depth = 0;

SYSCTL_SETUP(sysctl_hw_pvscsi_setup, "sysctl hw.pvscsi setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "pvscsi",
	    SYSCTL_DESCR("pvscsi global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

#ifdef PVSCSI_DEBUG_LOGGING
	TUNABLE_INT(log_level, "Enable debugging output");
#endif

	TUNABLE_INT(request_ring_pages, "No. of pages for the request ring");
	TUNABLE_INT(use_msg, "Use message passing");
	TUNABLE_INT(use_msi, "Use MSI interrupt");
	TUNABLE_INT(use_msix, "Use MSXI interrupt");
	TUNABLE_INT(use_req_call_threshold, "Use request limit");
	TUNABLE_INT(max_queue_depth, "Maximum size of request queue");

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

struct pvscsi_sg_list {
	struct pvscsi_sg_element sge[PVSCSI_MAX_SG_ENTRIES_PER_SEGMENT];
};

#define	PVSCSI_ABORT_TIMEOUT	2
#define	PVSCSI_RESET_TIMEOUT	10

#define	PVSCSI_HCB_NONE		0
#define	PVSCSI_HCB_ABORT	1
#define	PVSCSI_HCB_DEVICE_RESET	2
#define	PVSCSI_HCB_BUS_RESET	3

struct pvscsi_hcb {
	struct scsipi_xfer 		*xs;
	struct pvscsi_softc		*sc;

	struct pvscsi_ring_req_desc	*e;
	int				 recovery;
	SLIST_ENTRY(pvscsi_hcb)		 links;

	bus_dmamap_t			 dma_map;
	bus_addr_t			 dma_map_offset;
	bus_size_t			 dma_map_size;
	void				*sense_buffer;
	bus_addr_t			 sense_buffer_paddr;
	struct pvscsi_sg_list		*sg_list;
	bus_addr_t			 sg_list_paddr;
	bus_addr_t			 sg_list_offset;
};

struct pvscsi_dma {
	bus_dmamap_t		 map;
	void		        *vaddr;
	bus_addr_t	 	 paddr;
	bus_size_t	 	 size;
	bus_dma_segment_t	 seg[1];
};

struct pvscsi_softc {
	device_t		 dev;
	kmutex_t		 lock;

	device_t		 sc_scsibus_dv;
	struct scsipi_adapter	 sc_adapter;
	struct scsipi_channel 	 sc_channel;

	struct pvscsi_rings_state	*rings_state;
	struct pvscsi_ring_req_desc	*req_ring;
	struct pvscsi_ring_cmp_desc	*cmp_ring;
	struct pvscsi_ring_msg_desc	*msg_ring;
	uint32_t		 hcb_cnt;
	struct pvscsi_hcb	*hcbs;
	SLIST_HEAD(, pvscsi_hcb) free_list;

	bus_dma_tag_t		sc_dmat;
	bus_space_tag_t		sc_memt;
	bus_space_handle_t	sc_memh;
	bus_size_t		sc_mems;

	bool		 use_msg;
	uint32_t	 max_targets;
	int		 mm_rid;
	int		 irq_id;
	int		 use_req_call_threshold;

	pci_chipset_tag_t	 sc_pc;
	pci_intr_handle_t *	 sc_pihp;
	void			*sc_ih;

	uint64_t	rings_state_ppn;
	uint32_t	req_ring_num_pages;
	uint64_t	req_ring_ppn[PVSCSI_MAX_NUM_PAGES_REQ_RING];
	uint32_t	cmp_ring_num_pages;
	uint64_t	cmp_ring_ppn[PVSCSI_MAX_NUM_PAGES_CMP_RING];
	uint32_t	msg_ring_num_pages;
	uint64_t	msg_ring_ppn[PVSCSI_MAX_NUM_PAGES_MSG_RING];

	struct	pvscsi_dma rings_state_dma;
	struct	pvscsi_dma req_ring_dma;
	struct	pvscsi_dma cmp_ring_dma;
	struct	pvscsi_dma msg_ring_dma;

	struct	pvscsi_dma sg_list_dma;
	struct	pvscsi_dma sense_buffer_dma;
};

CFATTACH_DECL3_NEW(pvscsi, sizeof(struct pvscsi_softc),
    pvscsi_probe, pvscsi_attach, pvscsi_detach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

#define	PVSCSI_DMA_SYNC_STATE(sc, dma, structptr, member, ops)		      \
	bus_dmamap_sync((sc)->sc_dmat, (dma)->map,			      \
	    /*offset*/offsetof(__typeof__(*(structptr)), member),	      \
	    /*length*/sizeof((structptr)->member),			      \
	    (ops))

#define	PVSCSI_DMA_SYNC_RING(sc, dma, ring, idx, ops)			      \
	bus_dmamap_sync((sc)->sc_dmat, (dma)->map,			      \
	    /*offset*/sizeof(*(ring)) * (idx),				      \
	    /*length*/sizeof(*(ring)),					      \
	    (ops))

static inline uint32_t
pvscsi_reg_read(struct pvscsi_softc *sc, uint32_t offset)
{

	return (bus_space_read_4(sc->sc_memt, sc->sc_memh, offset));
}

static inline void
pvscsi_reg_write(struct pvscsi_softc *sc, uint32_t offset, uint32_t val)
{

	bus_space_write_4(sc->sc_memt, sc->sc_memh, offset, val);
}

static inline uint32_t
pvscsi_read_intr_status(struct pvscsi_softc *sc)
{

	return (pvscsi_reg_read(sc, PVSCSI_REG_OFFSET_INTR_STATUS));
}

static inline void
pvscsi_write_intr_status(struct pvscsi_softc *sc, uint32_t val)
{

	pvscsi_reg_write(sc, PVSCSI_REG_OFFSET_INTR_STATUS, val);
}

static inline void
pvscsi_intr_enable(struct pvscsi_softc *sc)
{
	uint32_t mask;

	mask = PVSCSI_INTR_CMPL_MASK;
	if (sc->use_msg) {
		mask |= PVSCSI_INTR_MSG_MASK;
	}

	pvscsi_reg_write(sc, PVSCSI_REG_OFFSET_INTR_MASK, mask);
}

static inline void
pvscsi_intr_disable(struct pvscsi_softc *sc)
{

	pvscsi_reg_write(sc, PVSCSI_REG_OFFSET_INTR_MASK, 0);
}

static void
pvscsi_kick_io(struct pvscsi_softc *sc, uint8_t cdb0)
{
	struct pvscsi_dma *s_dma;
	struct pvscsi_rings_state *s;

	DEBUG_PRINTF(2, sc->dev, "%s: cdb0 %#x\n", __func__, cdb0);
	if (cdb0 == SCSI_READ_6_COMMAND  || cdb0 == READ_10  ||
	    cdb0 == READ_12  || cdb0 == READ_16  ||
	    cdb0 == SCSI_WRITE_6_COMMAND || cdb0 == WRITE_10 ||
	    cdb0 == WRITE_12 || cdb0 == WRITE_16) {
		s_dma = &sc->rings_state_dma;
		s = sc->rings_state;

		/*
		 * Ensure the command has been published before we read
		 * req_cons_idx to test whether we need to kick the
		 * host.
		 */
		paravirt_membar_sync();

		PVSCSI_DMA_SYNC_STATE(sc, s_dma, s, req_cons_idx,
		    BUS_DMASYNC_POSTREAD);
		DEBUG_PRINTF(2, sc->dev, "%s req prod %d cons %d\n", __func__,
		    s->req_prod_idx, s->req_cons_idx);
		if (!sc->use_req_call_threshold ||
		    (s->req_prod_idx - s->req_cons_idx) >=
		     s->req_call_threshold) {
			pvscsi_reg_write(sc, PVSCSI_REG_OFFSET_KICK_RW_IO, 0);
			DEBUG_PRINTF(2, sc->dev, "kicked\n");
		} else {
			DEBUG_PRINTF(2, sc->dev, "wtf\n");
		}
		PVSCSI_DMA_SYNC_STATE(sc, s_dma, s, req_cons_idx,
		    BUS_DMASYNC_PREREAD);
	} else {
		s = sc->rings_state;
		/*
		 * XXX req_cons_idx in debug log might be stale, but no
		 * need for DMA sync otherwise in this branch
		 */
		DEBUG_PRINTF(1, sc->dev, "%s req prod %d cons %d not checked\n", __func__,
		    s->req_prod_idx, s->req_cons_idx);

		pvscsi_reg_write(sc, PVSCSI_REG_OFFSET_KICK_NON_RW_IO, 0);
	}
}

static void
pvscsi_write_cmd(struct pvscsi_softc *sc, uint32_t cmd, void *data,
		 uint32_t len)
{
	uint32_t *data_ptr;
	int i;

	KASSERTMSG(len % sizeof(uint32_t) == 0,
		"command size not a multiple of 4");

	data_ptr = data;
	len /= sizeof(uint32_t);

	pvscsi_reg_write(sc, PVSCSI_REG_OFFSET_COMMAND, cmd);
	for (i = 0; i < len; ++i) {
		pvscsi_reg_write(sc, PVSCSI_REG_OFFSET_COMMAND_DATA,
		   data_ptr[i]);
	}
}

static inline uint64_t pvscsi_hcb_to_context(struct pvscsi_softc *sc,
    struct pvscsi_hcb *hcb)
{

	/* Offset by 1 because context must not be 0 */
	return (hcb - sc->hcbs + 1);
}

static inline struct pvscsi_hcb* pvscsi_context_to_hcb(struct pvscsi_softc *sc,
    uint64_t context)
{

	return (sc->hcbs + (context - 1));
}

static struct pvscsi_hcb *
pvscsi_hcb_get(struct pvscsi_softc *sc)
{
	struct pvscsi_hcb *hcb;

	KASSERT(mutex_owned(&sc->lock));

	hcb = SLIST_FIRST(&sc->free_list);
	if (hcb) {
		SLIST_REMOVE_HEAD(&sc->free_list, links);
	}

	return (hcb);
}

static void
pvscsi_hcb_put(struct pvscsi_softc *sc, struct pvscsi_hcb *hcb)
{

	KASSERT(mutex_owned(&sc->lock));
	hcb->xs = NULL;
	hcb->e = NULL;
	hcb->recovery = PVSCSI_HCB_NONE;
	SLIST_INSERT_HEAD(&sc->free_list, hcb, links);
}

static uint32_t
pvscsi_get_max_targets(struct pvscsi_softc *sc)
{
	uint32_t max_targets;

	pvscsi_write_cmd(sc, PVSCSI_CMD_GET_MAX_TARGETS, NULL, 0);

	max_targets = pvscsi_reg_read(sc, PVSCSI_REG_OFFSET_COMMAND_STATUS);

	if (max_targets == ~0) {
		max_targets = 16;
	}

	return (max_targets);
}

static int pvscsi_setup_req_call(struct pvscsi_softc *sc, uint32_t enable)
{
	uint32_t status;
	struct pvscsi_cmd_desc_setup_req_call cmd;

	if (!pvscsi_get_tunable(sc, "pvscsi_use_req_call_threshold",
	    pvscsi_use_req_call_threshold)) {
		return (0);
	}

	pvscsi_reg_write(sc, PVSCSI_REG_OFFSET_COMMAND,
	    PVSCSI_CMD_SETUP_REQCALLTHRESHOLD);
	status = pvscsi_reg_read(sc, PVSCSI_REG_OFFSET_COMMAND_STATUS);

	if (status != -1) {
		memset(&cmd, 0, sizeof(cmd));
		cmd.enable = enable;
		pvscsi_write_cmd(sc, PVSCSI_CMD_SETUP_REQCALLTHRESHOLD,
		    &cmd, sizeof(cmd));
		status = pvscsi_reg_read(sc, PVSCSI_REG_OFFSET_COMMAND_STATUS);

		/*
		 * After setup, sync req_call_threshold before use.
		 * After this point it should be stable, so no need to
		 * sync again during use.
		 */
		PVSCSI_DMA_SYNC_STATE(sc, &sc->rings_state_dma,
		    sc->rings_state, req_call_threshold,
		    BUS_DMASYNC_POSTREAD);

		return (status != 0);
	} else {
		return (0);
	}
}

static void
pvscsi_dma_free(struct pvscsi_softc *sc, struct pvscsi_dma *dma)
{

	bus_dmamap_unload(sc->sc_dmat, dma->map);
	bus_dmamem_unmap(sc->sc_dmat, dma->vaddr, dma->size);
	bus_dmamap_destroy(sc->sc_dmat, dma->map);
	bus_dmamem_free(sc->sc_dmat, dma->seg, __arraycount(dma->seg));

	memset(dma, 0, sizeof(*dma));
}

static int
pvscsi_dma_alloc(struct pvscsi_softc *sc, struct pvscsi_dma *dma,
    bus_size_t size, bus_size_t alignment)
{
	int error;
	int nsegs;

	memset(dma, 0, sizeof(*dma));

	error = bus_dmamem_alloc(sc->sc_dmat, size, alignment, 0, dma->seg,
	    __arraycount(dma->seg), &nsegs, BUS_DMA_WAITOK);
	if (error) {
		aprint_normal_dev(sc->dev, "error allocating dma mem, error %d\n",
		    error);
		goto fail;
	}

	error = bus_dmamem_map(sc->sc_dmat, dma->seg, nsegs, size,
	    &dma->vaddr, BUS_DMA_WAITOK);
	if (error != 0) {
		device_printf(sc->dev, "Failed to map DMA memory\n");
		goto dmamemmap_fail;
	}

	error = bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    BUS_DMA_WAITOK, &dma->map);
	if (error != 0) {
		device_printf(sc->dev, "Failed to create DMA map\n");
		goto dmamapcreate_fail;
	}

	error = bus_dmamap_load(sc->sc_dmat, dma->map, dma->vaddr, size,
	    NULL, BUS_DMA_WAITOK);
	if (error) {
		aprint_normal_dev(sc->dev, "error mapping dma mam, error %d\n",
		    error);
		goto dmamapload_fail;
	}

	dma->paddr = dma->map->dm_segs[0].ds_addr;
	dma->size = size;

	return 0;

dmamapload_fail:
	bus_dmamap_destroy(sc->sc_dmat, dma->map);
dmamapcreate_fail:
	bus_dmamem_unmap(sc->sc_dmat, dma->vaddr, dma->size);
dmamemmap_fail:
	bus_dmamem_free(sc->sc_dmat, dma->seg, __arraycount(dma->seg));
fail:

	return (error);
}

static int
pvscsi_dma_alloc_ppns(struct pvscsi_softc *sc, struct pvscsi_dma *dma,
    uint64_t *ppn_list, uint32_t num_pages)
{
	int error;
	uint32_t i;
	uint64_t ppn;

	error = pvscsi_dma_alloc(sc, dma, num_pages * PAGE_SIZE, PAGE_SIZE);
	if (error) {
		aprint_normal_dev(sc->dev, "Error allocating pages, error %d\n",
		    error);
		return (error);
	}

	memset(dma->vaddr, 0, num_pages * PAGE_SIZE);
	bus_dmamap_sync(sc->sc_dmat, dma->map, 0, num_pages * PAGE_SIZE,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	ppn = dma->paddr >> PAGE_SHIFT;
	for (i = 0; i < num_pages; i++) {
		ppn_list[i] = ppn + i;
	}

	return (0);
}

static void
pvscsi_dma_free_per_hcb(struct pvscsi_softc *sc, uint32_t hcbs_allocated)
{
	int i;
	struct pvscsi_hcb *hcb;

	for (i = 0; i < hcbs_allocated; ++i) {
		hcb = sc->hcbs + i;
		bus_dmamap_destroy(sc->sc_dmat, hcb->dma_map);
	};

	pvscsi_dma_free(sc, &sc->sense_buffer_dma);
	pvscsi_dma_free(sc, &sc->sg_list_dma);
}

static int
pvscsi_dma_alloc_per_hcb(struct pvscsi_softc *sc)
{
	int i;
	int error;
	struct pvscsi_hcb *hcb;

	i = 0;

	error = pvscsi_dma_alloc(sc, &sc->sg_list_dma,
	    sizeof(struct pvscsi_sg_list) * sc->hcb_cnt, 1);
	if (error) {
		aprint_normal_dev(sc->dev,
		    "Error allocation sg list DMA memory, error %d\n", error);
		goto fail;
	}

	error = pvscsi_dma_alloc(sc, &sc->sense_buffer_dma,
				 PVSCSI_SENSE_LENGTH * sc->hcb_cnt, 1);
	if (error) {
		aprint_normal_dev(sc->dev,
		    "Error allocation buffer DMA memory, error %d\n", error);
		goto fail;
	}

	for (i = 0; i < sc->hcb_cnt; ++i) {
		hcb = sc->hcbs + i;

		error = bus_dmamap_create(sc->sc_dmat, PVSCSI_MAXPHYS,
		    PVSCSI_MAXPHYS_SEGS, PVSCSI_MAXPHYS, 0,
		    BUS_DMA_WAITOK, &hcb->dma_map);
		if (error) {
			aprint_normal_dev(sc->dev,
			    "Error creating dma map for hcb %d, error %d\n",
			    i, error);
			goto fail;
		}

		hcb->sc = sc;
		hcb->dma_map_offset = PVSCSI_SENSE_LENGTH * i;
		hcb->dma_map_size = PVSCSI_SENSE_LENGTH;
		hcb->sense_buffer =
		    (void *)((char *)sc->sense_buffer_dma.vaddr +
		    PVSCSI_SENSE_LENGTH * i);
		hcb->sense_buffer_paddr = sc->sense_buffer_dma.paddr +
		    PVSCSI_SENSE_LENGTH * i;

		hcb->sg_list =
		    (struct pvscsi_sg_list *)((char *)sc->sg_list_dma.vaddr +
		    sizeof(struct pvscsi_sg_list) * i);
		hcb->sg_list_paddr =
		    sc->sg_list_dma.paddr + sizeof(struct pvscsi_sg_list) * i;
		hcb->sg_list_offset = sizeof(struct pvscsi_sg_list) * i;
	}

	SLIST_INIT(&sc->free_list);
	for (i = (sc->hcb_cnt - 1); i >= 0; --i) {
		hcb = sc->hcbs + i;
		SLIST_INSERT_HEAD(&sc->free_list, hcb, links);
	}

fail:
	if (error) {
		pvscsi_dma_free_per_hcb(sc, i);
	}

	return (error);
}

static void
pvscsi_free_rings(struct pvscsi_softc *sc)
{

	bus_dmamap_sync(sc->sc_dmat, sc->rings_state_dma.map,
	    0, sc->rings_state_dma.size,
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(sc->sc_dmat, sc->req_ring_dma.map,
	    0, sc->req_ring_dma.size,
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(sc->sc_dmat, sc->cmp_ring_dma.map,
	    0, sc->cmp_ring_dma.size,
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	pvscsi_dma_free(sc, &sc->rings_state_dma);
	pvscsi_dma_free(sc, &sc->req_ring_dma);
	pvscsi_dma_free(sc, &sc->cmp_ring_dma);
	if (sc->use_msg) {
		pvscsi_dma_free(sc, &sc->msg_ring_dma);
	}
}

static int
pvscsi_allocate_rings(struct pvscsi_softc *sc)
{
	int error;

	error = pvscsi_dma_alloc_ppns(sc, &sc->rings_state_dma,
	    &sc->rings_state_ppn, 1);
	if (error) {
		aprint_normal_dev(sc->dev,
		    "Error allocating rings state, error = %d\n", error);
		goto fail;
	}
	sc->rings_state = sc->rings_state_dma.vaddr;

	error = pvscsi_dma_alloc_ppns(sc, &sc->req_ring_dma, sc->req_ring_ppn,
	    sc->req_ring_num_pages);
	if (error) {
		aprint_normal_dev(sc->dev,
		    "Error allocating req ring pages, error = %d\n", error);
		goto fail;
	}
	sc->req_ring = sc->req_ring_dma.vaddr;

	error = pvscsi_dma_alloc_ppns(sc, &sc->cmp_ring_dma, sc->cmp_ring_ppn,
	    sc->cmp_ring_num_pages);
	if (error) {
		aprint_normal_dev(sc->dev,
		    "Error allocating cmp ring pages, error = %d\n", error);
		goto fail;
	}
	sc->cmp_ring = sc->cmp_ring_dma.vaddr;

	sc->msg_ring = NULL;
	if (sc->use_msg) {
		error = pvscsi_dma_alloc_ppns(sc, &sc->msg_ring_dma,
		    sc->msg_ring_ppn, sc->msg_ring_num_pages);
		if (error) {
			aprint_normal_dev(sc->dev,
			    "Error allocating cmp ring pages, error = %d\n",
			    error);
			goto fail;
		}
		sc->msg_ring = sc->msg_ring_dma.vaddr;
	}

fail:
	if (error) {
		pvscsi_free_rings(sc);
	}
	return (error);
}

static void
pvscsi_setup_rings(struct pvscsi_softc *sc)
{
	struct pvscsi_cmd_desc_setup_rings cmd;
	uint32_t i;

	memset(&cmd, 0, sizeof(cmd));

	cmd.rings_state_ppn = sc->rings_state_ppn;

	cmd.req_ring_num_pages = sc->req_ring_num_pages;
	for (i = 0; i < sc->req_ring_num_pages; ++i) {
		cmd.req_ring_ppns[i] = sc->req_ring_ppn[i];
	}

	cmd.cmp_ring_num_pages = sc->cmp_ring_num_pages;
	for (i = 0; i < sc->cmp_ring_num_pages; ++i) {
		cmd.cmp_ring_ppns[i] = sc->cmp_ring_ppn[i];
	}

	pvscsi_write_cmd(sc, PVSCSI_CMD_SETUP_RINGS, &cmd, sizeof(cmd));

	/*
	 * After setup, sync *_num_entries_log2 before use.  After this
	 * point they should be stable, so no need to sync again during
	 * use.
	 */
	PVSCSI_DMA_SYNC_STATE(sc, &sc->rings_state_dma,
	    sc->rings_state, req_num_entries_log2,
	    BUS_DMASYNC_POSTREAD);
	PVSCSI_DMA_SYNC_STATE(sc, &sc->rings_state_dma,
	    sc->rings_state, cmp_num_entries_log2,
	    BUS_DMASYNC_POSTREAD);
}

static int
pvscsi_hw_supports_msg(struct pvscsi_softc *sc)
{
	uint32_t status;

	pvscsi_reg_write(sc, PVSCSI_REG_OFFSET_COMMAND,
	    PVSCSI_CMD_SETUP_MSG_RING);
	status = pvscsi_reg_read(sc, PVSCSI_REG_OFFSET_COMMAND_STATUS);

	return (status != -1);
}

static void
pvscsi_setup_msg_ring(struct pvscsi_softc *sc)
{
	struct pvscsi_cmd_desc_setup_msg_ring cmd;
	uint32_t i;

	KASSERTMSG(sc->use_msg, "msg is not being used");

	memset(&cmd, 0, sizeof(cmd));

	cmd.num_pages = sc->msg_ring_num_pages;
	for (i = 0; i < sc->msg_ring_num_pages; ++i) {
		cmd.ring_ppns[i] = sc->msg_ring_ppn[i];
	}

	pvscsi_write_cmd(sc, PVSCSI_CMD_SETUP_MSG_RING, &cmd, sizeof(cmd));

	/*
	 * After setup, sync msg_num_entries_log2 before use.  After
	 * this point it should be stable, so no need to sync again
	 * during use.
	 */
	PVSCSI_DMA_SYNC_STATE(sc, &sc->rings_state_dma,
	    sc->rings_state, msg_num_entries_log2,
	    BUS_DMASYNC_POSTREAD);
}

static void
pvscsi_adapter_reset(struct pvscsi_softc *sc)
{
	aprint_normal_dev(sc->dev, "Adapter Reset\n");

	pvscsi_write_cmd(sc, PVSCSI_CMD_ADAPTER_RESET, NULL, 0);
#ifdef PVSCSI_DEBUG_LOGGING
	uint32_t val =
#endif
	pvscsi_read_intr_status(sc);

	DEBUG_PRINTF(2, sc->dev, "adapter reset done: %u\n", val);
}

static void
pvscsi_bus_reset(struct pvscsi_softc *sc)
{

	aprint_normal_dev(sc->dev, "Bus Reset\n");

	pvscsi_write_cmd(sc, PVSCSI_CMD_RESET_BUS, NULL, 0);
	pvscsi_process_cmp_ring(sc);

	DEBUG_PRINTF(2, sc->dev, "bus reset done\n");
}

static void
pvscsi_device_reset(struct pvscsi_softc *sc, uint32_t target)
{
	struct pvscsi_cmd_desc_reset_device cmd;

	memset(&cmd, 0, sizeof(cmd));

	cmd.target = target;

	aprint_normal_dev(sc->dev, "Device reset for target %u\n", target);

	pvscsi_write_cmd(sc, PVSCSI_CMD_RESET_DEVICE, &cmd, sizeof cmd);
	pvscsi_process_cmp_ring(sc);

	DEBUG_PRINTF(2, sc->dev, "device reset done\n");
}

static void
pvscsi_abort(struct pvscsi_softc *sc, uint32_t target, struct pvscsi_hcb *hcb)
{
	struct pvscsi_cmd_desc_abort_cmd cmd;
	uint64_t context;

	pvscsi_process_cmp_ring(sc);

	if (hcb != NULL) {
		context = pvscsi_hcb_to_context(sc, hcb);

		memset(&cmd, 0, sizeof cmd);
		cmd.target = target;
		cmd.context = context;

		aprint_normal_dev(sc->dev, "Abort for target %u context %llx\n",
		    target, (unsigned long long)context);

		pvscsi_write_cmd(sc, PVSCSI_CMD_ABORT_CMD, &cmd, sizeof(cmd));
		pvscsi_process_cmp_ring(sc);

		DEBUG_PRINTF(2, sc->dev, "abort done\n");
	} else {
		DEBUG_PRINTF(1, sc->dev,
		    "Target %u hcb %p not found for abort\n", target, hcb);
	}
}

static int
pvscsi_probe(device_t dev, cfdata_t cf, void *aux)
{
	const struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_VMWARE &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_VMWARE_PVSCSI) {
		return 1;
	}
	return 0;
}

static void
pvscsi_timeout(void *arg)
{
	struct pvscsi_hcb *hcb = arg;
	struct scsipi_xfer *xs = hcb->xs;

	if (xs == NULL) {
		/* Already completed */
		return;
	}

	struct pvscsi_softc *sc = hcb->sc;

	mutex_enter(&sc->lock);

	scsipi_printaddr(xs->xs_periph);
	printf("command timeout, CDB: ");
	scsipi_print_cdb(xs->cmd);
	printf("\n");

	switch (hcb->recovery) {
	case PVSCSI_HCB_NONE:
		hcb->recovery = PVSCSI_HCB_ABORT;
		pvscsi_abort(sc, hcb->e->target, hcb);
		callout_reset(&xs->xs_callout,
		    mstohz(PVSCSI_ABORT_TIMEOUT * 1000),
		    pvscsi_timeout, hcb);
		break;
	case PVSCSI_HCB_ABORT:
		hcb->recovery = PVSCSI_HCB_DEVICE_RESET;
		pvscsi_device_reset(sc, hcb->e->target);
		callout_reset(&xs->xs_callout,
		    mstohz(PVSCSI_RESET_TIMEOUT * 1000),
		    pvscsi_timeout, hcb);
		break;
	case PVSCSI_HCB_DEVICE_RESET:
		hcb->recovery = PVSCSI_HCB_BUS_RESET;
		pvscsi_bus_reset(sc);
		callout_reset(&xs->xs_callout,
		    mstohz(PVSCSI_RESET_TIMEOUT * 1000),
		    pvscsi_timeout, hcb);
		break;
	case PVSCSI_HCB_BUS_RESET:
		pvscsi_adapter_reset(sc);
		break;
	};
	mutex_exit(&sc->lock);
}

static void
pvscsi_process_completion(struct pvscsi_softc *sc,
    struct pvscsi_ring_cmp_desc *e)
{
	struct pvscsi_hcb *hcb;
	struct scsipi_xfer *xs;
	uint32_t error = XS_NOERROR;
	uint32_t btstat;
	uint32_t sdstat;
	int op;

	hcb = pvscsi_context_to_hcb(sc, e->context);
	xs = hcb->xs;

	callout_stop(&xs->xs_callout);

	btstat = e->host_status;
	sdstat = e->scsi_status;

	xs->status = sdstat;
	xs->resid = xs->datalen - e->data_len;

	DEBUG_PRINTF(3, sc->dev,
	    "command context %llx btstat %d (%#x) sdstat %d (%#x)\n",
	    (unsigned long long)e->context, btstat, btstat, sdstat, sdstat);

	if ((xs->xs_control & XS_CTL_DATA_IN) == XS_CTL_DATA_IN) {
		op = BUS_DMASYNC_POSTREAD;
	} else {
		op = BUS_DMASYNC_POSTWRITE;
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sense_buffer_dma.map,
	    hcb->dma_map_offset, hcb->dma_map_size, op);

	if (btstat == BTSTAT_SUCCESS && sdstat == SCSI_OK) {
		DEBUG_PRINTF(3, sc->dev,
		    "completing command context %llx success\n",
		    (unsigned long long)e->context);
		xs->resid = 0;
	} else {
		switch (btstat) {
		case BTSTAT_SUCCESS:
		case BTSTAT_LINKED_COMMAND_COMPLETED:
		case BTSTAT_LINKED_COMMAND_COMPLETED_WITH_FLAG:
			switch (sdstat) {
			case SCSI_OK:
				xs->resid = 0;
				error = XS_NOERROR;
				break;
			case SCSI_CHECK:
				error = XS_SENSE;
				xs->resid = 0;

				memset(&xs->sense, 0, sizeof(xs->sense));
				memcpy(&xs->sense, hcb->sense_buffer,
				    MIN(sizeof(xs->sense), e->sense_len));
				break;
			case SCSI_BUSY:
			case SCSI_QUEUE_FULL:
				error = XS_NOERROR;
				break;
			case SCSI_TERMINATED:
// 			case SCSI_STATUS_TASK_ABORTED:
				DEBUG_PRINTF(1, sc->dev,
				    "xs: %p sdstat=0x%x\n", xs, sdstat);
				error = XS_DRIVER_STUFFUP;
				break;
			default:
				DEBUG_PRINTF(1, sc->dev,
				    "xs: %p sdstat=0x%x\n", xs, sdstat);
				error = XS_DRIVER_STUFFUP;
				break;
			}
			break;
		case BTSTAT_SELTIMEO:
			error = XS_SELTIMEOUT;
			break;
		case BTSTAT_DATARUN:
		case BTSTAT_DATA_UNDERRUN:
//			xs->resid = xs->datalen - c->data_len;
			error = XS_NOERROR;
			break;
		case BTSTAT_ABORTQUEUE:
		case BTSTAT_HATIMEOUT:
			error = XS_NOERROR;
			break;
		case BTSTAT_NORESPONSE:
		case BTSTAT_SENTRST:
		case BTSTAT_RECVRST:
		case BTSTAT_BUSRESET:
			error = XS_RESET;
			break;
		case BTSTAT_SCSIPARITY:
			error = XS_DRIVER_STUFFUP;
			DEBUG_PRINTF(1, sc->dev,
			    "xs: %p sdstat=0x%x\n", xs, sdstat);
			break;
		case BTSTAT_BUSFREE:
			error = XS_DRIVER_STUFFUP;
			DEBUG_PRINTF(1, sc->dev,
			    "xs: %p sdstat=0x%x\n", xs, sdstat);
			break;
		case BTSTAT_INVPHASE:
			error = XS_DRIVER_STUFFUP;
			DEBUG_PRINTF(1, sc->dev,
			    "xs: %p sdstat=0x%x\n", xs, sdstat);
			break;
		case BTSTAT_SENSFAILED:
			error = XS_DRIVER_STUFFUP;
			DEBUG_PRINTF(1, sc->dev,
			    "xs: %p sdstat=0x%x\n", xs, sdstat);
			break;
		case BTSTAT_LUNMISMATCH:
		case BTSTAT_TAGREJECT:
		case BTSTAT_DISCONNECT:
		case BTSTAT_BADMSG:
		case BTSTAT_INVPARAM:
			error = XS_DRIVER_STUFFUP;
			DEBUG_PRINTF(1, sc->dev,
			    "xs: %p sdstat=0x%x\n", xs, sdstat);
			break;
		case BTSTAT_HASOFTWARE:
		case BTSTAT_HAHARDWARE:
			error = XS_DRIVER_STUFFUP;
			DEBUG_PRINTF(1, sc->dev,
			    "xs: %p sdstat=0x%x\n", xs, sdstat);
			break;
		default:
			aprint_normal_dev(sc->dev, "unknown hba status: 0x%x\n",
			    btstat);
			error = XS_DRIVER_STUFFUP;
			break;
		}

		DEBUG_PRINTF(3, sc->dev,
		    "completing command context %llx btstat %x sdstat %x - error %x\n",
		    (unsigned long long)e->context, btstat, sdstat, error);
	}

	xs->error = error;
	pvscsi_hcb_put(sc, hcb);

	mutex_exit(&sc->lock);

	scsipi_done(xs);

	mutex_enter(&sc->lock);
}

static void
pvscsi_process_cmp_ring(struct pvscsi_softc *sc)
{
	struct pvscsi_dma *ring_dma;
	struct pvscsi_ring_cmp_desc *ring;
	struct pvscsi_dma *s_dma;
	struct pvscsi_rings_state *s;
	struct pvscsi_ring_cmp_desc *e;
	uint32_t mask;

	KASSERT(mutex_owned(&sc->lock));

	s_dma = &sc->rings_state_dma;
	s = sc->rings_state;
	ring_dma = &sc->cmp_ring_dma;
	ring = sc->cmp_ring;
	mask = MASK(s->cmp_num_entries_log2);

	for (;;) {
		PVSCSI_DMA_SYNC_STATE(sc, s_dma, s, cmp_prod_idx,
		    BUS_DMASYNC_POSTREAD);
		size_t crpidx = s->cmp_prod_idx;
		PVSCSI_DMA_SYNC_STATE(sc, s_dma, s, cmp_prod_idx,
		    BUS_DMASYNC_PREREAD);

		if (s->cmp_cons_idx == crpidx)
			break;

		size_t crcidx = s->cmp_cons_idx & mask;

		PVSCSI_DMA_SYNC_RING(sc, ring_dma, ring, crcidx,
		    BUS_DMASYNC_POSTREAD);

		e = ring + crcidx;

		pvscsi_process_completion(sc, e);

		/*
		 * ensure completion processing reads happen before write to
		 * (increment of) cmp_cons_idx
		 */
		PVSCSI_DMA_SYNC_RING(sc, ring_dma, ring, crcidx,
		    BUS_DMASYNC_PREREAD);

		/*
		 * XXX Not actually sure the `device' does DMA for
		 * s->cmp_cons_idx at all -- qemu doesn't.  If not, we
		 * can skip these DMA syncs.
		 */
		PVSCSI_DMA_SYNC_STATE(sc, s_dma, s, cmp_cons_idx,
		    BUS_DMASYNC_POSTWRITE);
		s->cmp_cons_idx++;
		PVSCSI_DMA_SYNC_STATE(sc, s_dma, s, cmp_cons_idx,
		    BUS_DMASYNC_PREWRITE);
	}
}

static void
pvscsi_process_msg(struct pvscsi_softc *sc, struct pvscsi_ring_msg_desc *e)
{
	struct pvscsi_ring_msg_dev_status_changed *desc;

	switch (e->type) {
	case PVSCSI_MSG_DEV_ADDED:
	case PVSCSI_MSG_DEV_REMOVED: {
		desc = (struct pvscsi_ring_msg_dev_status_changed *)e;
		struct scsibus_softc *ssc = device_private(sc->sc_scsibus_dv);

		aprint_normal_dev(sc->dev, "MSG: device %s at scsi%u:%u:%u\n",
		    desc->type == PVSCSI_MSG_DEV_ADDED ? "addition" : "removal",
		    desc->bus, desc->target, desc->lun[1]);

		if (desc->type == PVSCSI_MSG_DEV_ADDED) {
			if (scsi_probe_bus(ssc,
			    desc->target, desc->lun[1]) != 0) {
				aprint_normal_dev(sc->dev,
				    "Error creating path for dev change.\n");
				break;
			}
		} else {
			if (scsipi_target_detach(ssc->sc_channel,
			    desc->target, desc->lun[1],
			    DETACH_FORCE) != 0) {
				aprint_normal_dev(sc->dev,
				    "Error detaching target %d lun %d\n",
				    desc->target, desc->lun[1]);
			};

		}
	} break;
	default:
		aprint_normal_dev(sc->dev, "Unknown msg type 0x%x\n", e->type);
	};
}

static void
pvscsi_process_msg_ring(struct pvscsi_softc *sc)
{
	struct pvscsi_dma *ring_dma;
	struct pvscsi_ring_msg_desc *ring;
	struct pvscsi_dma *s_dma;
	struct pvscsi_rings_state *s;
	struct pvscsi_ring_msg_desc *e;
	uint32_t mask;

	KASSERT(mutex_owned(&sc->lock));

	s_dma = &sc->rings_state_dma;
	s = sc->rings_state;
	ring_dma = &sc->msg_ring_dma;
	ring = sc->msg_ring;
	mask = MASK(s->msg_num_entries_log2);

	for (;;) {
		PVSCSI_DMA_SYNC_STATE(sc, s_dma, s, msg_prod_idx,
		    BUS_DMASYNC_POSTREAD);
		size_t mpidx = s->msg_prod_idx;	// dma read (device -> cpu)
		PVSCSI_DMA_SYNC_STATE(sc, s_dma, s, msg_prod_idx,
		    BUS_DMASYNC_PREREAD);

		if (s->msg_cons_idx == mpidx)
			break;

		size_t mcidx = s->msg_cons_idx & mask;

		PVSCSI_DMA_SYNC_RING(sc, ring_dma, ring, mcidx,
		    BUS_DMASYNC_POSTREAD);

		e = ring + mcidx;

		pvscsi_process_msg(sc, e);

		/*
		 * ensure message processing reads happen before write to
		 * (increment of) msg_cons_idx
		 */
		PVSCSI_DMA_SYNC_RING(sc, ring_dma, ring, mcidx,
		    BUS_DMASYNC_PREREAD);

		PVSCSI_DMA_SYNC_STATE(sc, s_dma, s, msg_cons_idx,
		    BUS_DMASYNC_POSTWRITE);
		s->msg_cons_idx++;
		PVSCSI_DMA_SYNC_STATE(sc, s_dma, s, msg_cons_idx,
		    BUS_DMASYNC_PREWRITE);
	}
}

static void
pvscsi_intr_locked(struct pvscsi_softc *sc)
{
	uint32_t val;

	KASSERT(mutex_owned(&sc->lock));

	val = pvscsi_read_intr_status(sc);

	if ((val & PVSCSI_INTR_ALL_SUPPORTED) != 0) {
		pvscsi_write_intr_status(sc, val & PVSCSI_INTR_ALL_SUPPORTED);
		pvscsi_process_cmp_ring(sc);
		if (sc->use_msg) {
			pvscsi_process_msg_ring(sc);
		}
	}
}

static int
pvscsi_intr(void *xsc)
{
	struct pvscsi_softc *sc;

	sc = xsc;

	mutex_enter(&sc->lock);
	pvscsi_intr_locked(xsc);
	mutex_exit(&sc->lock);

	return 1;
}

static void
pvscsi_scsipi_request(struct scsipi_channel *chan, scsipi_adapter_req_t
    request, void *arg)
{
	struct pvscsi_softc *sc = device_private(chan->chan_adapter->adapt_dev);

	if (request == ADAPTER_REQ_SET_XFER_MODE) {
		struct scsipi_xfer_mode *xm = arg;

		xm->xm_mode = PERIPH_CAP_TQING;
		xm->xm_period = 0;
		xm->xm_offset = 0;
		scsipi_async_event(chan, ASYNC_EVENT_XFER_MODE, xm);
		return;
	} else if (request != ADAPTER_REQ_RUN_XFER) {
		DEBUG_PRINTF(1, sc->dev, "unhandled %d\n", request);
		return;
	}

	/* request is ADAPTER_REQ_RUN_XFER */
	struct scsipi_xfer *xs = arg;
	struct scsipi_periph *periph = xs->xs_periph;
#ifdef SCSIPI_DEBUG
	periph->periph_dbflags |= SCSIPI_DEBUG_FLAGS;
#endif

	uint32_t req_num_entries_log2;
	struct pvscsi_dma *ring_dma;
	struct pvscsi_ring_req_desc *ring;
	struct pvscsi_ring_req_desc *e;
	struct pvscsi_dma *s_dma;
	struct pvscsi_rings_state *s;
	struct pvscsi_hcb *hcb;

	if (xs->cmdlen < 0 || xs->cmdlen > sizeof(e->cdb)) {
		DEBUG_PRINTF(1, sc->dev, "bad cmdlen %zu > %zu\n",
		    (size_t)xs->cmdlen, sizeof(e->cdb));
		/* not a temporary condition */
		xs->error = XS_DRIVER_STUFFUP;
		scsipi_done(xs);
		return;
	}

	ring_dma = &sc->req_ring_dma;
	ring = sc->req_ring;
	s_dma = &sc->rings_state_dma;
	s = sc->rings_state;

	hcb = NULL;
	req_num_entries_log2 = s->req_num_entries_log2;

	/* Protect against multiple senders */
	mutex_enter(&sc->lock);

	if (s->req_prod_idx - s->cmp_cons_idx >=
	    (1 << req_num_entries_log2)) {
		aprint_normal_dev(sc->dev,
		    "Not enough room on completion ring.\n");
		xs->error = XS_RESOURCE_SHORTAGE;
		goto finish_xs;
	}

	if (xs->cmdlen > sizeof(e->cdb)) {
		DEBUG_PRINTF(1, sc->dev, "cdb length %u too large\n",
		    xs->cmdlen);
		xs->error = XS_DRIVER_STUFFUP;
		goto finish_xs;
	}

	hcb = pvscsi_hcb_get(sc);
	if (hcb == NULL) {
		aprint_normal_dev(sc->dev, "No free hcbs.\n");
		xs->error = XS_RESOURCE_SHORTAGE;
		goto finish_xs;
	}

	hcb->xs = xs;

	const size_t rridx = s->req_prod_idx & MASK(req_num_entries_log2);
	PVSCSI_DMA_SYNC_RING(sc, ring_dma, ring, rridx, BUS_DMASYNC_POSTWRITE);
	e = ring + rridx;

	memset(e, 0, sizeof(*e));
	e->bus = 0;
	e->target = periph->periph_target;
	e->lun[1] = periph->periph_lun;
	e->data_addr = 0;
	e->data_len = xs->datalen;
	e->vcpu_hint = cpu_index(curcpu());
	e->flags = 0;

	e->cdb_len = xs->cmdlen;
	memcpy(e->cdb, xs->cmd, xs->cmdlen);

	e->sense_addr = 0;
	e->sense_len = sizeof(xs->sense);
	if (e->sense_len > 0) {
		e->sense_addr = hcb->sense_buffer_paddr;
	}
	//e->tag = xs->xs_tag_type;
	e->tag = MSG_SIMPLE_Q_TAG;

	switch (xs->xs_control & (XS_CTL_DATA_IN | XS_CTL_DATA_OUT)) {
	case XS_CTL_DATA_IN:
		e->flags |= PVSCSI_FLAG_CMD_DIR_TOHOST;
		break;
	case XS_CTL_DATA_OUT:
		e->flags |= PVSCSI_FLAG_CMD_DIR_TODEVICE;
		break;
	default:
		e->flags |= PVSCSI_FLAG_CMD_DIR_NONE;
		break;
	}

	e->context = pvscsi_hcb_to_context(sc, hcb);
	hcb->e = e;

	DEBUG_PRINTF(3, sc->dev,
	    " queuing command %02x context %llx\n", e->cdb[0],
	    (unsigned long long)e->context);

	int flags;
	flags  = (xs->xs_control & XS_CTL_DATA_IN) ? BUS_DMA_READ : BUS_DMA_WRITE;
	flags |= (xs->xs_control & XS_CTL_NOSLEEP) ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK;

	int error = bus_dmamap_load(sc->sc_dmat, hcb->dma_map,
	    xs->data, xs->datalen, NULL, flags);

	if (error) {
		if (error == ENOMEM || error == EAGAIN) {
			xs->error = XS_RESOURCE_SHORTAGE;
		} else {
			xs->error = XS_DRIVER_STUFFUP;
		}
		DEBUG_PRINTF(1, sc->dev,
		    "xs: %p load error %d data %p len %d",
                    xs, error, xs->data, xs->datalen);
		goto error_load;
	}

	int op = (xs->xs_control & XS_CTL_DATA_IN) ? BUS_DMASYNC_PREREAD :
	    BUS_DMASYNC_PREWRITE;
	int nseg = hcb->dma_map->dm_nsegs;
	bus_dma_segment_t *segs = hcb->dma_map->dm_segs;
	if (nseg != 0) {
		if (nseg > 1) {
			struct pvscsi_sg_element *sge;

			KASSERTMSG(nseg <= PVSCSI_MAX_SG_ENTRIES_PER_SEGMENT,
			    "too many sg segments");

			sge = hcb->sg_list->sge;
			e->flags |= PVSCSI_FLAG_CMD_WITH_SG_LIST;

			for (size_t i = 0; i < nseg; ++i) {
				sge[i].addr = segs[i].ds_addr;
				sge[i].length = segs[i].ds_len;
				sge[i].flags = 0;
			}

			e->data_addr = hcb->sg_list_paddr;

			bus_dmamap_sync(sc->sc_dmat,
			    sc->sg_list_dma.map, hcb->sg_list_offset,
			    sizeof(*sge) * nseg, BUS_DMASYNC_PREWRITE);
		} else {
			e->data_addr = segs->ds_addr;
		}

		bus_dmamap_sync(sc->sc_dmat, hcb->dma_map, 0,
		    xs->datalen, op);
	} else {
		e->data_addr = 0;
	}

	/*
	 * Ensure request record writes happen before write to (increment of)
	 * req_prod_idx.
	 */
	PVSCSI_DMA_SYNC_RING(sc, ring_dma, ring, rridx, BUS_DMASYNC_PREWRITE);

	uint8_t cdb0 = e->cdb[0];

	/* handle timeout */
	if ((xs->xs_control & XS_CTL_POLL) == 0) {
		int timeout = mstohz(xs->timeout);
		/* start expire timer */
		if (timeout == 0)
			timeout = 1;
		callout_reset(&xs->xs_callout, timeout, pvscsi_timeout, hcb);
	}

	PVSCSI_DMA_SYNC_STATE(sc, s_dma, s, req_prod_idx,
	    BUS_DMASYNC_POSTWRITE);
	s->req_prod_idx++;

	/*
	 * Ensure req_prod_idx write (increment) happens before
	 * IO is kicked (via a write).
	 */
	PVSCSI_DMA_SYNC_STATE(sc, s_dma, s, req_prod_idx,
	    BUS_DMASYNC_PREWRITE);

	pvscsi_kick_io(sc, cdb0);
	mutex_exit(&sc->lock);

	return;

error_load:
	pvscsi_hcb_put(sc, hcb);

finish_xs:
	mutex_exit(&sc->lock);
	scsipi_done(xs);
}

static void
pvscsi_free_interrupts(struct pvscsi_softc *sc)
{

	if (sc->sc_ih != NULL) {
		pci_intr_disestablish(sc->sc_pc, sc->sc_ih);
		sc->sc_ih = NULL;
	}
	if (sc->sc_pihp != NULL) {
		pci_intr_release(sc->sc_pc, sc->sc_pihp, 1);
		sc->sc_pihp = NULL;
	}
}

static int
pvscsi_setup_interrupts(struct pvscsi_softc *sc, const struct pci_attach_args *pa)
{
	int use_msix;
	int use_msi;
	int counts[PCI_INTR_TYPE_SIZE];

	for (size_t i = 0; i < PCI_INTR_TYPE_SIZE; i++) {
		counts[i] = 1;
	}

	use_msix = pvscsi_get_tunable(sc, "use_msix", pvscsi_use_msix);
	use_msi = pvscsi_get_tunable(sc, "use_msi", pvscsi_use_msi);

	if (!use_msix) {
		counts[PCI_INTR_TYPE_MSIX] = 0;
	}
	if (!use_msi) {
		counts[PCI_INTR_TYPE_MSI] = 0;
	}

	/* Allocate and establish the interrupt. */
	if (pci_intr_alloc(pa, &sc->sc_pihp, counts, PCI_INTR_TYPE_MSIX)) {
		aprint_error_dev(sc->dev, "can't allocate handler\n");
		goto fail;
	}

	char intrbuf[PCI_INTRSTR_LEN];
	const pci_chipset_tag_t pc = pa->pa_pc;
	char const *intrstr = pci_intr_string(pc, sc->sc_pihp[0], intrbuf,
	    sizeof(intrbuf));

	sc->sc_ih = pci_intr_establish_xname(pc, sc->sc_pihp[0], IPL_BIO,
	    pvscsi_intr, sc, device_xname(sc->dev));
	if (sc->sc_ih == NULL) {
		pci_intr_release(pc, sc->sc_pihp, 1);
		sc->sc_pihp = NULL;
		aprint_error_dev(sc->dev, "couldn't establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		goto fail;
	}
	pci_intr_setattr(pc, sc->sc_pihp, PCI_INTR_MPSAFE, true);

	aprint_normal_dev(sc->dev, "interrupting at %s\n", intrstr);

	return (0);

fail:
	if (sc->sc_ih != NULL) {
		pci_intr_disestablish(sc->sc_pc, sc->sc_ih);
		sc->sc_ih = NULL;
	}
	if (sc->sc_pihp != NULL) {
		pci_intr_release(sc->sc_pc, sc->sc_pihp, 1);
		sc->sc_pihp = NULL;
	}
	if (sc->sc_mems) {
		bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_mems);
		sc->sc_mems = 0;
	}

	return 1;
}

static void
pvscsi_free_all(struct pvscsi_softc *sc)
{

	pvscsi_dma_free_per_hcb(sc, sc->hcb_cnt);

	if (sc->hcbs) {
		kmem_free(sc->hcbs, sc->hcb_cnt * sizeof(*sc->hcbs));
	}

	pvscsi_free_rings(sc);

	pvscsi_free_interrupts(sc);

	if (sc->sc_mems) {
		bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_mems);
		sc->sc_mems = 0;
	}
}

static inline void
pci_enable_busmaster(device_t dev, const pci_chipset_tag_t pc,
    const pcitag_t tag)
{
	pcireg_t pci_cmd_word;

	pci_cmd_word = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	if (!(pci_cmd_word & PCI_COMMAND_MASTER_ENABLE)) {
		pci_cmd_word |= PCI_COMMAND_MASTER_ENABLE;
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, pci_cmd_word);
	}
}

static void
pvscsi_attach(device_t parent, device_t dev, void *aux)
{
	const struct pci_attach_args *pa = aux;
	struct pvscsi_softc *sc;
	int rid;
	int error;
	int max_queue_depth;
	int adapter_queue_size;

	sc = device_private(dev);
	sc->dev = dev;

	struct scsipi_adapter *adapt = &sc->sc_adapter;
	struct scsipi_channel *chan = &sc->sc_channel;

	mutex_init(&sc->lock, MUTEX_DEFAULT, IPL_BIO);

	sc->sc_pc = pa->pa_pc;
	pci_enable_busmaster(dev, pa->pa_pc, pa->pa_tag);

	pci_aprint_devinfo_fancy(pa, "virtual disk controller",
	    VMWARE_PVSCSI_DEVSTR, true);

	/*
	 * Map the device.  All devices support memory-mapped acccess.
	 */
	bool memh_valid;
	bus_space_tag_t memt;
	bus_space_handle_t memh;
	bus_size_t mems;
	pcireg_t regt;

	for (rid = PCI_MAPREG_START; rid < PCI_MAPREG_END; rid += sizeof(regt)) {
		regt = pci_mapreg_type(pa->pa_pc, pa->pa_tag, rid);
		if (PCI_MAPREG_TYPE(regt) == PCI_MAPREG_TYPE_MEM)
			break;
	}

	if (rid >= PCI_MAPREG_END) {
		aprint_error_dev(dev,
		    "unable to locate device registers\n");
	}

	memh_valid = (pci_mapreg_map(pa, rid, regt, 0, &memt, &memh,
	    NULL, &mems) == 0);
	if (!memh_valid) {
		aprint_error_dev(dev,
		    "unable to map device registers\n");
		return;
	}
	sc->sc_memt = memt;
	sc->sc_memh = memh;
	sc->sc_mems = mems;

	if (pci_dma64_available(pa)) {
		sc->sc_dmat = pa->pa_dmat64;
		aprint_verbose_dev(sc->dev, "64-bit DMA\n");
	} else {
		aprint_verbose_dev(sc->dev, "32-bit DMA\n");
		sc->sc_dmat = pa->pa_dmat;
	}

	error = pvscsi_setup_interrupts(sc, pa);
	if (error) {
		aprint_normal_dev(dev, "Interrupt setup failed\n");
		pvscsi_free_all(sc);
		return;
	}

	sc->max_targets = pvscsi_get_max_targets(sc);

	sc->use_msg = pvscsi_get_tunable(sc, "use_msg", pvscsi_use_msg) &&
	    pvscsi_hw_supports_msg(sc);
	sc->msg_ring_num_pages = sc->use_msg ? 1 : 0;

	sc->req_ring_num_pages = pvscsi_get_tunable(sc, "request_ring_pages",
	    pvscsi_request_ring_pages);
	if (sc->req_ring_num_pages <= 0) {
		if (sc->max_targets <= 16) {
			sc->req_ring_num_pages =
			    PVSCSI_DEFAULT_NUM_PAGES_REQ_RING;
		} else {
			sc->req_ring_num_pages = PVSCSI_MAX_NUM_PAGES_REQ_RING;
		}
	} else if (sc->req_ring_num_pages > PVSCSI_MAX_NUM_PAGES_REQ_RING) {
		sc->req_ring_num_pages = PVSCSI_MAX_NUM_PAGES_REQ_RING;
	}
	sc->cmp_ring_num_pages = sc->req_ring_num_pages;

	max_queue_depth = pvscsi_get_tunable(sc, "max_queue_depth",
	    pvscsi_max_queue_depth);

	adapter_queue_size = (sc->req_ring_num_pages * PAGE_SIZE) /
	    sizeof(struct pvscsi_ring_req_desc);
	if (max_queue_depth > 0) {
		adapter_queue_size = MIN(adapter_queue_size, max_queue_depth);
	}
	adapter_queue_size = MIN(adapter_queue_size,
	    PVSCSI_MAX_REQ_QUEUE_DEPTH);

	aprint_normal_dev(sc->dev, "Use Msg: %d\n", sc->use_msg);
	aprint_normal_dev(sc->dev, "Max targets: %d\n", sc->max_targets);
	aprint_normal_dev(sc->dev, "REQ num pages: %d\n", sc->req_ring_num_pages);
	aprint_normal_dev(sc->dev, "CMP num pages: %d\n", sc->cmp_ring_num_pages);
	aprint_normal_dev(sc->dev, "MSG num pages: %d\n", sc->msg_ring_num_pages);
	aprint_normal_dev(sc->dev, "Queue size: %d\n", adapter_queue_size);

	if (pvscsi_allocate_rings(sc)) {
		aprint_normal_dev(dev, "ring allocation failed\n");
		pvscsi_free_all(sc);
		return;
	}

	sc->hcb_cnt = adapter_queue_size;
	sc->hcbs = kmem_zalloc(sc->hcb_cnt * sizeof(*sc->hcbs), KM_SLEEP);

	if (pvscsi_dma_alloc_per_hcb(sc)) {
		aprint_normal_dev(dev, "error allocating per hcb dma memory\n");
		pvscsi_free_all(sc);
		return;
	}

	pvscsi_adapter_reset(sc);

	/*
	 * Fill in the scsipi_adapter.
	 */
	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = sc->dev;
	adapt->adapt_nchannels = 1;
	adapt->adapt_openings = MIN(adapter_queue_size, PVSCSI_CMD_PER_LUN);
	adapt->adapt_max_periph = adapt->adapt_openings;
	adapt->adapt_request = pvscsi_scsipi_request;
	adapt->adapt_minphys = minphys;

	/*
	 * Fill in the scsipi_channel.
	 */
	memset(chan, 0, sizeof(*chan));
	chan->chan_adapter = adapt;
	chan->chan_bustype = &scsi_bustype;
	chan->chan_channel = 0;
	chan->chan_ntargets = MIN(PVSCSI_MAX_TARGET, 16);	/* cap reasonably */
	chan->chan_nluns = MIN(PVSCSI_MAX_LUN, 1024);		/* cap reasonably */
	chan->chan_id = PVSCSI_MAX_TARGET;
	chan->chan_flags = SCSIPI_CHAN_NOSETTLE;

	pvscsi_setup_rings(sc);
	if (sc->use_msg) {
		pvscsi_setup_msg_ring(sc);
	}

	sc->use_req_call_threshold = pvscsi_setup_req_call(sc, 1);

	pvscsi_intr_enable(sc);

	sc->sc_scsibus_dv = config_found(sc->dev, &sc->sc_channel, scsiprint,
	    CFARGS_NONE);

	return;
}

static int
pvscsi_detach(device_t dev, int flags)
{
	struct pvscsi_softc *sc;

	sc = device_private(dev);

	pvscsi_intr_disable(sc);
	pvscsi_adapter_reset(sc);

	pvscsi_free_all(sc);

	mutex_destroy(&sc->lock);

	return (0);
}
