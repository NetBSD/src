/* $NetBS$ */
/*	$OpenBSD: qcipcc.c,v 1.2 2023/05/19 20:54:55 patrick Exp $	*/
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
#include <dev/acpi/acpi_intr.h>
#include <dev/acpi/qcomipcc.h>

#define IPCC_SEND_ID			0x0c
#define IPCC_RECV_ID			0x10
#define IPCC_RECV_SIGNAL_ENABLE		0x14
#define IPCC_RECV_SIGNAL_DISABLE	0x18
#define IPCC_RECV_SIGNAL_CLEAR		0x1c

#define IPCC_SIGNAL_ID_MASK		__BITS(15,0)
#define IPCC_CLIENT_ID_MASK		__BITS(31,16)

#define HREAD4(sc, reg)							\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

struct qcipcc_intrhand {
	TAILQ_ENTRY(qcipcc_intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	void *ih_sc;
	uint32_t ih_client_id;
	uint32_t ih_signal_id;
};

struct qcipcc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	void			*sc_ih;

	TAILQ_HEAD(,qcipcc_intrhand) sc_intrq;
};

static struct qcipcc_softc *qcipcc = NULL;

struct qcipcc_channel {
	struct qcipcc_softc	*ch_sc;
	uint32_t		ch_client_id;
	uint32_t		ch_signal_id;
};

#define QCIPCC_8380_BASE	0x00408000
#define QCIPCC_SIZE		0x1000

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "QCOM06C2",		.value = QCIPCC_8380_BASE },
	DEVICE_COMPAT_EOL
};

static int	qcipcc_match(device_t, cfdata_t, void *);
static void	qcipcc_attach(device_t, device_t, void *);
static int	qcipcc_intr(void *);

CFATTACH_DECL_NEW(qcomipcc, sizeof(struct qcipcc_softc),
    qcipcc_match, qcipcc_attach, NULL, NULL);

static int
qcipcc_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
qcipcc_attach(device_t parent, device_t self, void *aux)
{
	struct qcipcc_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	ACPI_HANDLE hdl;
	struct acpi_resources res;
	struct acpi_irq *irq;
	bus_addr_t base_addr;
	ACPI_STATUS rv;

	if (qcipcc != NULL) {
		aprint_error(": already attached!\n");
		return;
	}

	hdl = aa->aa_node->ad_handle;
	rv = acpi_resource_parse(self, hdl, "_CRS", &res,
	    &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv)) {
		return;
	}

	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		aprint_error_dev(self, "couldn't find irq resource\n");
		goto done;
	}

	base_addr = acpi_compatible_lookup(aa, compat_data)->value;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_memt;
	if (bus_space_map(sc->sc_iot, base_addr, QCIPCC_SIZE,
	    BUS_SPACE_MAP_NONPOSTED, &sc->sc_ioh) != 0) {
		aprint_error_dev(self, "couldn't map registers\n");
		goto done;
	}

	TAILQ_INIT(&sc->sc_intrq);

	sc->sc_ih = acpi_intr_establish(self,
	    (uint64_t)(uintptr_t)hdl,
	    IPL_VM, false, qcipcc_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self,
		    "couldn't establish interrupt\n");
		goto done;
	}

	qcipcc = sc;

done:
	acpi_resource_cleanup(&res);
}

static int
qcipcc_intr(void *arg)
{
	struct qcipcc_softc *sc = arg;
	struct qcipcc_intrhand *ih;
	uint16_t client_id, signal_id;
	uint32_t reg;
	int handled = 0;

	while ((reg = HREAD4(sc, IPCC_RECV_ID)) != ~0) {
		HWRITE4(sc, IPCC_RECV_SIGNAL_CLEAR, reg);

		client_id = __SHIFTOUT(reg, IPCC_CLIENT_ID_MASK);
		signal_id = __SHIFTOUT(reg, IPCC_SIGNAL_ID_MASK);

		TAILQ_FOREACH(ih, &sc->sc_intrq, ih_q) {
			if (ih->ih_client_id != client_id ||
			    ih->ih_signal_id != signal_id)
				continue;
			ih->ih_func(ih->ih_arg);
			handled = 1;
		}
	}

	return handled;
}

void *
qcipcc_intr_establish(uint16_t client_id, uint16_t signal_id, int ipl,
    int (*func)(void *), void *arg)
{
	struct qcipcc_softc *sc = qcipcc;
	struct qcipcc_intrhand *ih;

	if (sc == NULL) {
		return NULL;
	}

	ih = kmem_zalloc(sizeof(*ih), KM_SLEEP);
	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_sc = sc;
	ih->ih_client_id = client_id;
	ih->ih_signal_id = signal_id;
	TAILQ_INSERT_TAIL(&sc->sc_intrq, ih, ih_q);

	qcipcc_intr_enable(ih);

	return ih;
}

void
qcipcc_intr_disestablish(void *cookie)
{
	struct qcipcc_intrhand *ih = cookie;
	struct qcipcc_softc *sc = ih->ih_sc;

	qcipcc_intr_disable(ih);

	TAILQ_REMOVE(&sc->sc_intrq, ih, ih_q);
	kmem_free(ih, sizeof(*ih));
}

void
qcipcc_intr_enable(void *cookie)
{
	struct qcipcc_intrhand *ih = cookie;
	struct qcipcc_softc *sc = ih->ih_sc;

	HWRITE4(sc, IPCC_RECV_SIGNAL_ENABLE,
	    __SHIFTIN(ih->ih_client_id, IPCC_CLIENT_ID_MASK) |
	    __SHIFTIN(ih->ih_signal_id, IPCC_SIGNAL_ID_MASK));
}

void
qcipcc_intr_disable(void *cookie)
{
	struct qcipcc_intrhand *ih = cookie;
	struct qcipcc_softc *sc = ih->ih_sc;

	HWRITE4(sc, IPCC_RECV_SIGNAL_DISABLE,
	    __SHIFTIN(ih->ih_client_id, IPCC_CLIENT_ID_MASK) |
	    __SHIFTIN(ih->ih_signal_id, IPCC_SIGNAL_ID_MASK));
}

void *
qcipcc_channel(uint16_t client_id, uint16_t signal_id)
{
	struct qcipcc_softc *sc = qcipcc;
	struct qcipcc_channel *ch;

	if (qcipcc == NULL) {
		return NULL;
	}

	ch = kmem_zalloc(sizeof(*ch), KM_SLEEP);
	ch->ch_sc = sc;
	ch->ch_client_id = client_id;
	ch->ch_signal_id = signal_id;

	return ch;
}

int
qcipcc_send(void *cookie)
{
	struct qcipcc_channel *ch = cookie;
	struct qcipcc_softc *sc = ch->ch_sc;

	HWRITE4(sc, IPCC_SEND_ID,
	    __SHIFTIN(ch->ch_client_id, IPCC_CLIENT_ID_MASK) |
	    __SHIFTIN(ch->ch_signal_id, IPCC_SIGNAL_ID_MASK));

	return 0;
}
