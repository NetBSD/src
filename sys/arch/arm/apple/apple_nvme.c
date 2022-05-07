/*	$NetBSD: apple_nvme.c,v 1.1 2022/05/07 08:20:03 skrll Exp $	*/
/*	$OpenBSD: aplns.c,v 1.5 2021/08/29 11:23:29 kettenis Exp $ */

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
 * Copyright (c) 2014, 2021 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apple_nvme.c,v 1.1 2022/05/07 08:20:03 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/intr.h>

#include <dev/ic/nvmereg.h>
#include <dev/ic/nvmevar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/apple/apple_rtkit.h>

int apple_nvme_mpsafe = 1;

#define NVME_IO_Q 1

#define ANS_CPU_CTRL		0x0044
#define ANS_CPU_CTRL_RUN		__BIT(4)

#define ANS_MAX_PEND_CMDS_CTRL	0x01210
#define  ANS_MAX_QUEUE_DEPTH		64
#define ANS_BOOT_STATUS		0x01300
#define  ANS_BOOT_STATUS_OK		0xde71ce55

#define ANS_MODESEL_REG		0x01304
#define ANS_UNKNOWN_CTRL	0x24008
#define  ANS_PRP_NULL_CHECK		__BIT(11)
#define ANS_LINEAR_SQ_CTRL	0x24908
#define  ANS_LINEAR_SQ_CTRL_EN		__BIT(0)
#define ANS_LINEAR_ASQ_DB	0x2490c
#define ANS_LINEAR_IOSQ_DB	0x24910

#define ANS_NVMMU_NUM		0x28100
#define ANS_NVMMU_BASE_ASQ	0x28108
#define ANS_NVMMU_BASE_IOSQ	0x28110
#define ANS_NVMMU_TCB_INVAL	0x28118
#define ANS_NVMMU_TCB_STAT	0x28120

#define ANS_NVMMU_TCB_SIZE	0x4000
#define ANS_NVMMU_TCB_PITCH	0x80

struct ans_nvmmu_tcb {
	uint8_t		tcb_opcode;
	uint8_t		tcb_flags;
#define ANS_NVMMU_TCB_WRITE		__BIT(0)
#define ANS_NVMMU_TCB_READ		__BIT(1)
	uint8_t		tcb_cid;
	uint8_t		tcb_pad0[1];

	uint32_t	tcb_prpl_len;
	uint8_t		tcb_pad1[16];

	uint64_t	tcb_prp[2];
};

void	nvme_ans_enable(struct nvme_softc *);

int	nvme_ans_q_alloc(struct nvme_softc *, struct nvme_queue *);
void	nvme_ans_q_free(struct nvme_softc *, struct nvme_queue *);

uint32_t
	nvme_ans_sq_enter(struct nvme_softc *, struct nvme_queue *,
			  struct nvme_ccb *);
void	nvme_ans_sq_leave(struct nvme_softc *,
			  struct nvme_queue *, struct nvme_ccb *);

void	nvme_ans_cq_done(struct nvme_softc *,
			 struct nvme_queue *, struct nvme_ccb *);

static const struct nvme_ops nvme_ans_ops = {
	.op_enable		= nvme_ans_enable,

	.op_q_alloc		= nvme_ans_q_alloc,
	.op_q_free		= nvme_ans_q_free,

	.op_sq_enter		= nvme_ans_sq_enter,
	.op_sq_leave		= nvme_ans_sq_leave,
	.op_sq_enter_locked	= nvme_ans_sq_enter,
	.op_sq_leave_locked	= nvme_ans_sq_leave,

	.op_cq_done		= nvme_ans_cq_done,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "apple,nvme-m1" },
	{ .compat = "apple,nvme-ans2" },
	DEVICE_COMPAT_EOL
};

struct apple_nvme_softc {
	struct nvme_softc asc_nvme;
	int asc_phandle;

	bus_space_tag_t	asc_iot;
	bus_space_handle_t asc_ioh;
	bus_size_t asc_size;

	struct rtkit_state *asc_rtkit;

	size_t asc_nintrs;
	void *asc_ihs;
};

void
nvme_ans_enable(struct nvme_softc *sc)
{
	nvme_write4(sc, ANS_NVMMU_NUM,
	    (ANS_NVMMU_TCB_SIZE / ANS_NVMMU_TCB_PITCH) - 1);
	nvme_write4(sc, ANS_MODESEL_REG, 0);
}


int
nvme_ans_q_alloc(struct nvme_softc *sc, struct nvme_queue *q)
{
	bus_size_t db, base;

	KASSERT(q->q_entries <= (ANS_NVMMU_TCB_SIZE / ANS_NVMMU_TCB_PITCH));

	q->q_nvmmu_dmamem = nvme_dmamem_alloc(sc, ANS_NVMMU_TCB_SIZE);
	if (q->q_nvmmu_dmamem == NULL)
		return -1;

	memset(NVME_DMA_KVA(q->q_nvmmu_dmamem), 0,
	    NVME_DMA_LEN(q->q_nvmmu_dmamem));

	switch (q->q_id) {
	case NVME_IO_Q:
		db = ANS_LINEAR_IOSQ_DB;
		base = ANS_NVMMU_BASE_IOSQ;
		break;
	case NVME_ADMIN_Q:
		db = ANS_LINEAR_ASQ_DB;
		base = ANS_NVMMU_BASE_ASQ;
		break;
	default:
		panic("unsupported queue id %u", q->q_id);
		/* NOTREACHED */
	}

	q->q_sqtdbl = db;

	nvme_dmamem_sync(sc, q->q_nvmmu_dmamem, BUS_DMASYNC_PREWRITE);
	nvme_write8(sc, base, NVME_DMA_DVA(q->q_nvmmu_dmamem));

	return 0;
}

void
nvme_ans_q_free(struct nvme_softc *sc,
    struct nvme_queue *q)
{
	nvme_dmamem_sync(sc, q->q_nvmmu_dmamem, BUS_DMASYNC_POSTWRITE);
	nvme_dmamem_free(sc, q->q_nvmmu_dmamem);
}

uint32_t
nvme_ans_sq_enter(struct nvme_softc *sc,
    struct nvme_queue *q, struct nvme_ccb *ccb)
{
	return ccb->ccb_id;
}

static inline struct ans_nvmmu_tcb *
nvme_ans_tcb(struct nvme_queue *q, unsigned int qid)
{
	uint8_t *ptr = NVME_DMA_KVA(q->q_nvmmu_dmamem);
	ptr += qid * ANS_NVMMU_TCB_PITCH;

	return (struct ans_nvmmu_tcb *)ptr;
}

void
nvme_ans_sq_leave(struct nvme_softc *sc,
    struct nvme_queue *q, struct nvme_ccb *ccb)
{
	unsigned int id = ccb->ccb_id;
	struct ans_nvmmu_tcb *tcb = nvme_ans_tcb(q, id);
	struct nvme_sqe_io *sqe = NVME_DMA_KVA(q->q_sq_dmamem);
	sqe += id;

	bus_dmamap_sync(sc->sc_dmat, NVME_DMA_MAP(q->q_nvmmu_dmamem),
	    ANS_NVMMU_TCB_PITCH * id, sizeof(*tcb), BUS_DMASYNC_POSTWRITE);

	memset(tcb, 0, sizeof(*tcb));
	tcb->tcb_opcode = sqe->opcode;
	tcb->tcb_flags = ANS_NVMMU_TCB_WRITE | ANS_NVMMU_TCB_READ;
	tcb->tcb_cid = id;
	tcb->tcb_prpl_len = sqe->nlb;
	tcb->tcb_prp[0] = sqe->entry.prp[0];
	tcb->tcb_prp[1] = sqe->entry.prp[1];

	bus_dmamap_sync(sc->sc_dmat, NVME_DMA_MAP(q->q_nvmmu_dmamem),
	    ANS_NVMMU_TCB_PITCH * id, sizeof(*tcb), BUS_DMASYNC_PREWRITE);
	nvme_write4(sc, q->q_sqtdbl, id);
}

void
nvme_ans_cq_done(struct nvme_softc *sc,
    struct nvme_queue *q, struct nvme_ccb *ccb)
{
	unsigned int id = ccb->ccb_id;
	struct ans_nvmmu_tcb *tcb = nvme_ans_tcb(q, id);

	bus_dmamap_sync(sc->sc_dmat, NVME_DMA_MAP(q->q_nvmmu_dmamem),
	    ANS_NVMMU_TCB_PITCH * id, sizeof(*tcb), BUS_DMASYNC_POSTWRITE);
	memset(tcb, 0, sizeof(*tcb));
	bus_dmamap_sync(sc->sc_dmat, NVME_DMA_MAP(q->q_nvmmu_dmamem),
	    ANS_NVMMU_TCB_PITCH * id, sizeof(*tcb), BUS_DMASYNC_PREWRITE);

	nvme_write4(sc, ANS_NVMMU_TCB_INVAL, id);
	uint32_t stat = nvme_read4(sc, ANS_NVMMU_TCB_STAT);
	if (stat != 0) {
		printf("%s: nvmmu tcp stat is non-zero: 0x%08x\n",
		    device_xname(sc->sc_dev), stat);
	}
}


static int
apple_nvme_intr_establish(struct nvme_softc *sc, uint16_t qid,
    struct nvme_queue *q)
{
	struct apple_nvme_softc * const asc =
	    container_of(sc, struct apple_nvme_softc, asc_nvme);
	const int phandle = asc->asc_phandle;
	char intr_xname[INTRDEVNAMEBUF];
	char intrstr[128];
	const device_t self = sc->sc_dev;

	KASSERT(sc->sc_use_mq || qid == NVME_ADMIN_Q);
	KASSERT(sc->sc_ih[qid] == NULL);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": couldn't decode interrupt\n");
		return 1;
	}
	sc->sc_ih[qid] = fdtbus_intr_establish_xname(phandle, 0, IPL_BIO,
	    FDT_INTR_MPSAFE, nvme_intr, sc, device_xname(sc->sc_dev));
	if (sc->sc_ih[qid] == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return 1;
	}

	/* establish also the software interrupt */
	sc->sc_softih[qid] = softint_establish(
	    SOFTINT_BIO|(apple_nvme_mpsafe ? SOFTINT_MPSAFE : 0),
	    nvme_softintr_intx, q);
	if (sc->sc_softih[qid] == NULL) {
		fdtbus_intr_disestablish(phandle, sc->sc_ih[qid]);
		sc->sc_ih[qid] = NULL;

		aprint_error_dev(sc->sc_dev,
		    "unable to establish %s soft interrupt\n",
		    intr_xname);
		return 1;
	}

	if (!sc->sc_use_mq) {
		aprint_normal_dev(sc->sc_dev, "interrupting on %s\n", intrstr);
	} else if (qid == NVME_ADMIN_Q) {
		aprint_normal_dev(sc->sc_dev,
		    "for admin queue interrupting on %s\n", intrstr);
	} else {
		aprint_normal_dev(sc->sc_dev,
		    "for io queue %d interrupting on %s\n", qid, intrstr);
	}
	return 0;
}

static int
apple_nvme_intr_disestablish(struct nvme_softc *sc, uint16_t qid)
{
	struct apple_nvme_softc * const asc =
	   container_of(sc, struct apple_nvme_softc, asc_nvme);

	KASSERT(sc->sc_use_mq || qid == NVME_ADMIN_Q);
	KASSERT(sc->sc_ih[qid] != NULL);

	if (sc->sc_softih) {
		softint_disestablish(sc->sc_softih[qid]);
		sc->sc_softih[qid] = NULL;
	}

	fdtbus_intr_disestablish(asc->asc_phandle, sc->sc_ih[qid]);
	sc->sc_ih[qid] = NULL;

	return 0;
}


static int
apple_nvme_setup_intr(struct fdt_attach_args * const faa,
    struct apple_nvme_softc * const asc)
{
	struct nvme_softc * const sc = &asc->asc_nvme;

	asc->asc_nintrs = 1;
	asc->asc_phandle = faa->faa_phandle;

	sc->sc_use_mq = asc->asc_nintrs > 1;
	sc->sc_nq = 1;	/* sc_use_mq */

	return 0;
}


static int
apple_nvme_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
apple_nvme_attach(device_t parent, device_t self, void *aux)
{
	struct apple_nvme_softc * const asc = device_private(self);
	struct nvme_softc *sc = &asc->asc_nvme;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr, ans_addr;
	bus_size_t size, ans_size;
	uint32_t ctrl, status;


	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get NVME registers\n");
		return;
	}

	if (fdtbus_get_reg(phandle, 1, &ans_addr, &ans_size) != 0) {
		aprint_error(": couldn't get ANS registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = asc->asc_iot = faa->faa_bst;
	sc->sc_ios = size;

	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map NVME registers\n");
		return;
	}

	if (bus_space_map(asc->asc_iot, ans_addr, ans_size, 0, &asc->asc_ioh) != 0) {
		aprint_error(": couldn't map ANS registers\n");
		goto fail_ansmap;
	}

	sc->sc_dmat = faa->faa_dmat;
	sc->sc_ops = &nvme_ans_ops;
	aprint_naive("\n");
	aprint_normal(": Apple NVME\n");

	apple_nvme_setup_intr(faa, asc);

	sc->sc_intr_establish = apple_nvme_intr_establish;
	sc->sc_intr_disestablish = apple_nvme_intr_disestablish;

	sc->sc_ih = kmem_zalloc(sizeof(*sc->sc_ih) * asc->asc_nintrs, KM_SLEEP);
	sc->sc_softih = kmem_zalloc(sizeof(*sc->sc_softih) * asc->asc_nintrs,
	    KM_SLEEP);

	asc->asc_rtkit = rtkit_init(phandle, NULL);
	if (asc->asc_rtkit == NULL) {
		aprint_error("can't map mailbox channel\n");
		goto fail_rtkit;
	}

	ctrl = bus_space_read_4(asc->asc_iot, asc->asc_ioh, ANS_CPU_CTRL);
	bus_space_write_4(asc->asc_iot, asc->asc_ioh, ANS_CPU_CTRL,
	    ctrl | ANS_CPU_CTRL_RUN);

	status = bus_space_read_4(sc->sc_iot, sc->sc_ioh, ANS_BOOT_STATUS);
	if (status != ANS_BOOT_STATUS_OK)
		rtkit_boot(asc->asc_rtkit);

	status = bus_space_read_4(sc->sc_iot, sc->sc_ioh, ANS_BOOT_STATUS);
	if (status != ANS_BOOT_STATUS_OK) {
		aprint_error("firmware not ready\n");
		goto fail_ansnotready;
	}

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ANS_LINEAR_SQ_CTRL,
	    ANS_LINEAR_SQ_CTRL_EN);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ANS_MAX_PEND_CMDS_CTRL,
	    (ANS_MAX_QUEUE_DEPTH << 16) | ANS_MAX_QUEUE_DEPTH);

	ctrl = bus_space_read_4(sc->sc_iot, sc->sc_ioh, ANS_UNKNOWN_CTRL);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ANS_UNKNOWN_CTRL,
	    ctrl & ~ANS_PRP_NULL_CHECK);

	if (nvme_attach(sc) != 0) {
		/* error printed by nvme_attach() */
		return;
	}

	SET(sc->sc_flags, NVME_F_ATTACHED);
	return;
fail_ansnotready:

fail_rtkit:

fail_ansmap:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
}

CFATTACH_DECL_NEW(apple_nvme, sizeof(struct apple_nvme_softc),
	apple_nvme_match, apple_nvme_attach, NULL, NULL);
