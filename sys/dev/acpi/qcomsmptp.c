/* $NetBSD: qcomsmptp.c,v 1.1 2024/12/30 12:31:10 jmcneill Exp $ */
/*	$OpenBSD: qcsmptp.c,v 1.2 2023/07/04 14:32:21 patrick Exp $	*/
/*
 * Copyright (c) 2023 Patrick Wildt <patrick@blueri.se>
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/qcomipcc.h>
#include <dev/acpi/qcomsmem.h>
#include <dev/acpi/qcomsmptp.h>

#define SMP2P_MAX_ENTRY		16
#define SMP2P_MAX_ENTRY_NAME	16

struct qcsmptp_smem_item {
	uint32_t magic;
#define SMP2P_MAGIC			0x504d5324
	uint8_t version;
#define SMP2P_VERSION			1
	unsigned features:24;
#define SMP2P_FEATURE_SSR_ACK		(1 << 0)
	uint16_t local_pid;
	uint16_t remote_pid;
	uint16_t total_entries;
	uint16_t valid_entries;
	uint32_t flags;
#define SMP2P_FLAGS_RESTART_DONE	(1 << 0)
#define SMP2P_FLAGS_RESTART_ACK		(1 << 1)

	struct {
		uint8_t name[SMP2P_MAX_ENTRY_NAME];
		uint32_t value;
	} entries[SMP2P_MAX_ENTRY];
} __packed;

struct qcsmptp_intrhand {
	TAILQ_ENTRY(qcsmptp_intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	void *ih_ic;
	int ih_pin;
	int ih_enabled;
};

struct qcsmptp_interrupt_controller {
	TAILQ_HEAD(,qcsmptp_intrhand) ic_intrq;
	struct qcsmptp_softc *ic_sc;
};

struct qcsmptp_entry {
	TAILQ_ENTRY(qcsmptp_entry) e_q;
	const char *e_name;
	uint32_t *e_value;
	uint32_t e_last_value;
	struct qcsmptp_interrupt_controller *e_ic;
};

struct qcsmptp_softc {
	device_t		sc_dev;
	void			*sc_ih;

	uint16_t		sc_local_pid;
	uint16_t		sc_remote_pid;
	uint32_t		sc_smem_id[2];

	struct qcsmptp_smem_item *sc_in;
	struct qcsmptp_smem_item *sc_out;

	TAILQ_HEAD(,qcsmptp_entry) sc_inboundq;
	TAILQ_HEAD(,qcsmptp_entry) sc_outboundq;

	int			sc_negotiated;
	int			sc_ssr_ack_enabled;
	int			sc_ssr_ack;

	uint16_t		sc_valid_entries;

	void			*sc_ipcc;
};

static struct qcsmptp_interrupt_controller *qcsmptp_ic = NULL;

static int	qcsmptp_match(device_t, cfdata_t, void *);
static void	qcsmptp_attach(device_t, device_t, void *);

static int	qcsmptp_intr(void *);

CFATTACH_DECL_NEW(qcomsmptp, sizeof(struct qcsmptp_softc),
    qcsmptp_match, qcsmptp_attach, NULL, NULL);

#define IPCC_CLIENT_LPASS	3
#define IPCC_MPROC_SIGNAL_SMP2P	2

#define QCSMPTP_X1E_LOCAL_PID	0
#define QCSMPTP_X1E_REMOTE_PID	2
#define QCSMPTP_X1E_SMEM_ID0	443
#define QCSMPTP_X1E_SMEM_ID1	429

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "QCOM0C5C" },
	DEVICE_COMPAT_EOL
};

static int
qcsmptp_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
qcsmptp_attach(device_t parent, device_t self, void *aux)
{
	struct qcsmptp_softc *sc = device_private(self);
	struct qcsmptp_interrupt_controller *ic;
	struct qcsmptp_entry *e;

	sc->sc_dev = self;

	TAILQ_INIT(&sc->sc_inboundq);
	TAILQ_INIT(&sc->sc_outboundq);

	sc->sc_ih = qcipcc_intr_establish(IPCC_CLIENT_LPASS,
	    IPCC_MPROC_SIGNAL_SMP2P, IPL_VM, qcsmptp_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error(": can't establish interrupt\n");
		return;
	}

	sc->sc_local_pid = QCSMPTP_X1E_LOCAL_PID;
	sc->sc_remote_pid = QCSMPTP_X1E_REMOTE_PID;
	sc->sc_smem_id[0] = QCSMPTP_X1E_SMEM_ID0;
	sc->sc_smem_id[1] = QCSMPTP_X1E_SMEM_ID1;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_ipcc = qcipcc_channel(IPCC_CLIENT_LPASS,
	    IPCC_MPROC_SIGNAL_SMP2P);
	if (sc->sc_ipcc == NULL) {
		aprint_error_dev(self, "can't get mailbox\n");
		return;
	}

	if (qcsmem_alloc(sc->sc_remote_pid, sc->sc_smem_id[0],
	    sizeof(*sc->sc_in)) != 0) {
		aprint_error_dev(self, "can't alloc smp2p item\n");
		return;
	}

	sc->sc_in = qcsmem_get(sc->sc_remote_pid, sc->sc_smem_id[0], NULL);
	if (sc->sc_in == NULL) {
		aprint_error_dev(self, "can't get smp2p item\n");
		return;
	}

	if (qcsmem_alloc(sc->sc_remote_pid, sc->sc_smem_id[1],
	    sizeof(*sc->sc_out)) != 0) {
		aprint_error_dev(self, "can't alloc smp2p item\n");
		return;
	}

	sc->sc_out = qcsmem_get(sc->sc_remote_pid, sc->sc_smem_id[1], NULL);
	if (sc->sc_out == NULL) {
		aprint_error_dev(self, "can't get smp2p item\n");
		return;
	}

	qcsmem_memset(sc->sc_out, 0, sizeof(*sc->sc_out));
	sc->sc_out->magic = SMP2P_MAGIC;
	sc->sc_out->local_pid = sc->sc_local_pid;
	sc->sc_out->remote_pid = sc->sc_remote_pid;
	sc->sc_out->total_entries = SMP2P_MAX_ENTRY;
	sc->sc_out->features = SMP2P_FEATURE_SSR_ACK;
	membar_sync();
	sc->sc_out->version = SMP2P_VERSION;
	qcipcc_send(sc->sc_ipcc);

	e = kmem_zalloc(sizeof(*e), KM_SLEEP);
	e->e_name = "master-kernel";
	e->e_value = &sc->sc_out->entries[sc->sc_out->valid_entries].value;
	sc->sc_out->valid_entries++;
	TAILQ_INSERT_TAIL(&sc->sc_outboundq, e, e_q);
	/* TODO: provide as smem state */

	e = kmem_zalloc(sizeof(*e), KM_SLEEP);
	e->e_name = "slave-kernel";
	ic = kmem_zalloc(sizeof(*ic), KM_SLEEP);
	TAILQ_INIT(&ic->ic_intrq);
	ic->ic_sc = sc;
	e->e_ic = ic;
	TAILQ_INSERT_TAIL(&sc->sc_inboundq, e, e_q);

	qcsmptp_ic = ic;
}

static int
qcsmptp_intr(void *arg)
{
	struct qcsmptp_softc *sc = arg;
	struct qcsmptp_entry *e;
	struct qcsmptp_intrhand *ih;
	uint32_t changed, val;
	int do_ack = 0, i;

	/* Do initial feature negotiation if inbound is new. */
	if (!sc->sc_negotiated) {
		if (sc->sc_in->version != sc->sc_out->version)
			return 1;
		sc->sc_out->features &= sc->sc_in->features;
		if (sc->sc_out->features & SMP2P_FEATURE_SSR_ACK)
			sc->sc_ssr_ack_enabled = 1;
		sc->sc_negotiated = 1;
	}
	if (!sc->sc_negotiated) {
		return 1;
	}

	/* Use ACK mechanism if negotiated. */
	if (sc->sc_ssr_ack_enabled &&
	    !!(sc->sc_in->flags & SMP2P_FLAGS_RESTART_DONE) != sc->sc_ssr_ack)
		do_ack = 1;

	/* Catch up on new inbound entries that got added in the meantime. */
	for (i = sc->sc_valid_entries; i < sc->sc_in->valid_entries; i++) {
		TAILQ_FOREACH(e, &sc->sc_inboundq, e_q) {
			if (strncmp(sc->sc_in->entries[i].name, e->e_name,
			    SMP2P_MAX_ENTRY_NAME) != 0)
				continue;
			e->e_value = &sc->sc_in->entries[i].value;
		}
	}
	sc->sc_valid_entries = i;

	/* For each inbound "interrupt controller". */
	TAILQ_FOREACH(e, &sc->sc_inboundq, e_q) {
		if (e->e_value == NULL)
			continue;
		val = *e->e_value;
		if (val == e->e_last_value)
			continue;
		changed = val ^ e->e_last_value;
		e->e_last_value = val;
		TAILQ_FOREACH(ih, &e->e_ic->ic_intrq, ih_q) {
			if (!ih->ih_enabled)
				continue;
			if ((changed & (1 << ih->ih_pin)) == 0)
				continue;
			ih->ih_func(ih->ih_arg);
		}
	}

	if (do_ack) {
		sc->sc_ssr_ack = !sc->sc_ssr_ack;
		if (sc->sc_ssr_ack)
			sc->sc_out->flags |= SMP2P_FLAGS_RESTART_ACK;
		else
			sc->sc_out->flags &= ~SMP2P_FLAGS_RESTART_ACK;
		membar_sync();
		qcipcc_send(sc->sc_ipcc);
	}

	return 1;
}

void *
qcsmptp_intr_establish(u_int pin, int (*func)(void *), void *arg)
{
	struct qcsmptp_interrupt_controller *ic = qcsmptp_ic;
	struct qcsmptp_intrhand *ih;

	if (ic == NULL) {
		return NULL;
	}

	ih = kmem_zalloc(sizeof(*ih), KM_SLEEP);
	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_ic = ic;
	ih->ih_pin = pin;
	TAILQ_INSERT_TAIL(&ic->ic_intrq, ih, ih_q);

	qcsmptp_intr_enable(ih);

	return ih;
}

void
qcsmptp_intr_disestablish(void *cookie)
{
	struct qcsmptp_intrhand *ih = cookie;
	struct qcsmptp_interrupt_controller *ic = ih->ih_ic;

	qcsmptp_intr_disable(ih);

	TAILQ_REMOVE(&ic->ic_intrq, ih, ih_q);
	kmem_free(ih, sizeof(*ih));
}

void
qcsmptp_intr_enable(void *cookie)
{
	struct qcsmptp_intrhand *ih = cookie;

	ih->ih_enabled = 1;
}

void
qcsmptp_intr_disable(void *cookie)
{
	struct qcsmptp_intrhand *ih = cookie;

	ih->ih_enabled = 0;
}
