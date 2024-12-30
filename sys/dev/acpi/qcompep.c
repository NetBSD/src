/* $NetBSD: qcompep.c,v 1.1 2024/12/30 12:31:10 jmcneill Exp $ */
/*	$OpenBSD: qcaoss.c,v 1.1 2023/05/23 14:10:27 patrick Exp $	*/
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
#include <dev/acpi/qcompep.h>
#include <dev/acpi/qcomipcc.h>

#define AOSS_DESC_MAGIC			0x0
#define AOSS_DESC_VERSION		0x4
#define AOSS_DESC_FEATURES		0x8
#define AOSS_DESC_UCORE_LINK_STATE	0xc
#define AOSS_DESC_UCORE_LINK_STATE_ACK	0x10
#define AOSS_DESC_UCORE_CH_STATE	0x14
#define AOSS_DESC_UCORE_CH_STATE_ACK	0x18
#define AOSS_DESC_UCORE_MBOX_SIZE	0x1c
#define AOSS_DESC_UCORE_MBOX_OFFSET	0x20
#define AOSS_DESC_MCORE_LINK_STATE	0x24
#define AOSS_DESC_MCORE_LINK_STATE_ACK	0x28
#define AOSS_DESC_MCORE_CH_STATE	0x2c
#define AOSS_DESC_MCORE_CH_STATE_ACK	0x30
#define AOSS_DESC_MCORE_MBOX_SIZE	0x34
#define AOSS_DESC_MCORE_MBOX_OFFSET	0x38

#define AOSS_MAGIC			0x4d41494c
#define AOSS_VERSION			1

#define AOSS_STATE_UP			(0xffffU << 0)
#define AOSS_STATE_DOWN			(0xffffU << 16)

#define AOSSREAD4(sc, reg)						\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_aoss_ioh, (reg))
#define AOSSWRITE4(sc, reg, val)					\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_aoss_ioh, (reg), (val))

struct qcpep_data {
	bus_addr_t		aoss_base;
	bus_size_t		aoss_size;
	uint32_t		aoss_client_id;
	uint32_t		aoss_signal_id;
};

struct qcpep_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;

	const struct qcpep_data	*sc_data;

	bus_space_handle_t	sc_aoss_ioh;
	size_t			sc_aoss_offset;
	size_t			sc_aoss_size;
	void *			sc_aoss_ipcc;
};

struct qcpep_softc *qcpep_sc;

static const struct qcpep_data qcpep_x1e_data = {
	.aoss_base = 0x0c300000,
	.aoss_size = 0x400,
	.aoss_client_id = 0,	/* IPCC_CLIENT_AOP */
	.aoss_signal_id = 0,	/* IPCC_MPROC_SIGNAL_GLINK_QMP */
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "QCOM0C17",		.data = &qcpep_x1e_data },
	DEVICE_COMPAT_EOL
};

static int	qcpep_match(device_t, cfdata_t, void *);
static void	qcpep_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(qcompep, sizeof(struct qcpep_softc),
    qcpep_match, qcpep_attach, NULL, NULL);

static int
qcpep_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
qcpep_attach(device_t parent, device_t self, void *aux)
{
	struct qcpep_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	ACPI_STATUS rv;
	int i;

        rv = acpi_resource_parse(self, aa->aa_node->ad_handle,
	    "_CRS", &res, &acpi_resource_parse_ops_default);
        if (ACPI_FAILURE(rv)) {
                return;
        }
	acpi_resource_cleanup(&res);

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_memt;
	sc->sc_data = acpi_compatible_lookup(aa, compat_data)->data;

	if (bus_space_map(sc->sc_iot, sc->sc_data->aoss_base,
	    sc->sc_data->aoss_size, BUS_SPACE_MAP_NONPOSTED, &sc->sc_aoss_ioh)) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	sc->sc_aoss_ipcc = qcipcc_channel(sc->sc_data->aoss_client_id,
					  sc->sc_data->aoss_signal_id);
	if (sc->sc_aoss_ipcc == NULL) {
		aprint_error_dev(self, "couldn't find ipcc mailbox\n");
		return;
	}

	if (AOSSREAD4(sc, AOSS_DESC_MAGIC) != AOSS_MAGIC ||
	    AOSSREAD4(sc, AOSS_DESC_VERSION) != AOSS_VERSION) {
		aprint_error_dev(self, "invalid QMP info\n");
		return;
	}

	sc->sc_aoss_offset = AOSSREAD4(sc, AOSS_DESC_MCORE_MBOX_OFFSET);
	sc->sc_aoss_size = AOSSREAD4(sc, AOSS_DESC_MCORE_MBOX_SIZE);
	if (sc->sc_aoss_size == 0) {
		aprint_error_dev(self, "invalid AOSS mailbox size\n");
		return;
	}

	AOSSWRITE4(sc, AOSS_DESC_UCORE_LINK_STATE_ACK,
	    AOSSREAD4(sc, AOSS_DESC_UCORE_LINK_STATE));

	AOSSWRITE4(sc, AOSS_DESC_MCORE_LINK_STATE, AOSS_STATE_UP);
	qcipcc_send(sc->sc_aoss_ipcc);

	for (i = 1000; i > 0; i--) {
		if (AOSSREAD4(sc, AOSS_DESC_MCORE_LINK_STATE_ACK) == AOSS_STATE_UP)
			break;
		delay(1000);
	}
	if (i == 0) {
		aprint_error_dev(self, "didn't get link state ack\n");
		return;
	}

	AOSSWRITE4(sc, AOSS_DESC_MCORE_CH_STATE, AOSS_STATE_UP);
	qcipcc_send(sc->sc_aoss_ipcc);

	for (i = 1000; i > 0; i--) {
		if (AOSSREAD4(sc, AOSS_DESC_UCORE_CH_STATE) == AOSS_STATE_UP)
			break;
		delay(1000);
	}
	if (i == 0) {
		aprint_error_dev(self, "didn't get open channel\n");
		return;
	}

	AOSSWRITE4(sc, AOSS_DESC_UCORE_CH_STATE_ACK, AOSS_STATE_UP);
	qcipcc_send(sc->sc_aoss_ipcc);

	for (i = 1000; i > 0; i--) {
		if (AOSSREAD4(sc, AOSS_DESC_MCORE_CH_STATE_ACK) == AOSS_STATE_UP)
			break;
		delay(1000);
	}
	if (i == 0) {
		aprint_error_dev(self, "didn't get channel ack\n");
		return;
	}

	qcpep_sc = sc;
}

int
qcaoss_send(char *data, size_t len)
{
	struct qcpep_softc *sc = qcpep_sc;
	uint32_t reg;
	int i;

	if (sc == NULL)
		return ENXIO;

	if (data == NULL || sizeof(uint32_t) + len > sc->sc_aoss_size ||
	    (len % sizeof(uint32_t)) != 0)
		return EINVAL;

	/* Write data first, needs to be 32-bit access. */
	for (i = 0; i < len; i += 4) {
		memcpy(&reg, data + i, sizeof(reg));
		AOSSWRITE4(sc, sc->sc_aoss_offset + sizeof(uint32_t) + i, reg);
	}

	/* Commit transaction by writing length. */
	AOSSWRITE4(sc, sc->sc_aoss_offset, len);

	/* Assert it's stored and inform peer. */
	if (AOSSREAD4(sc, sc->sc_aoss_offset) != len) {
		device_printf(sc->sc_dev,
		    "aoss message readback failed\n");
	}
	qcipcc_send(sc->sc_aoss_ipcc);

	for (i = 1000; i > 0; i--) {
		if (AOSSREAD4(sc, sc->sc_aoss_offset) == 0)
			break;
		delay(1000);
	}
	if (i == 0) {
		device_printf(sc->sc_dev, "timeout sending message\n");
		AOSSWRITE4(sc, sc->sc_aoss_offset, 0);
		return ETIMEDOUT;
	}

	return 0;
}
