/* $NetBSD: qcompep.c,v 1.2 2025/01/08 22:58:05 jmcneill Exp $ */
/*	$OpenBSD: qcaoss.c,v 1.1 2023/05/23 14:10:27 patrick Exp $	*/
/*      $OpenBSD: qccpucp.c,v 1.1 2024/11/16 21:17:54 tobhe Exp $       */
/*
 * Copyright (c) 2023 Patrick Wildt <patrick@blueri.se>
 * Copyright (c) 2024 Tobias Heider <tobhe@openbsd.org>
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

#include <dev/ic/scmi.h>

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

#define CPUCP_REG_CMD(i)	(0x104 + ((i) * 8))
#define CPUCP_MASK_CMD		0xffffffffffffffffULL
#define CPUCP_REG_RX_MAP	0x4000
#define CPUCP_REG_RX_STAT	0x4400
#define CPUCP_REG_RX_CLEAR	0x4800
#define CPUCP_REG_RX_EN		0x4C00

#define RXREAD8(sc, reg)						\
	(bus_space_read_8((sc)->sc_iot, (sc)->sc_cpucp_rx_ioh, (reg)))
#define RXWRITE8(sc, reg, val)						\
	bus_space_write_8((sc)->sc_iot, (sc)->sc_cpucp_rx_ioh, (reg), (val))

#define TXWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_cpucp_tx_ioh, (reg), (val))


struct qcpep_data {
	bus_addr_t		aoss_base;
	bus_size_t		aoss_size;
	uint32_t		aoss_client_id;
	uint32_t		aoss_signal_id;
	bus_addr_t		cpucp_rx_base;
	bus_size_t		cpucp_rx_size;
	bus_addr_t		cpucp_tx_base;
	bus_size_t		cpucp_tx_size;
	bus_addr_t		cpucp_shmem_base;
	bus_size_t		cpucp_shmem_size;
};

struct qcpep_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;

	const struct qcpep_data	*sc_data;

	bus_space_handle_t	sc_aoss_ioh;
	size_t			sc_aoss_offset;
	size_t			sc_aoss_size;
	void *			sc_aoss_ipcc;

	bus_space_handle_t	sc_cpucp_rx_ioh;
	bus_space_handle_t	sc_cpucp_tx_ioh;

	struct scmi_softc	sc_scmi;
};

struct qcpep_softc *qcpep_sc;

static const struct qcpep_data qcpep_x1e_data = {
	.aoss_base = 0x0c300000,
	.aoss_size = 0x400,
	.aoss_client_id = 0,	/* IPCC_CLIENT_AOP */
	.aoss_signal_id = 0,	/* IPCC_MPROC_SIGNAL_GLINK_QMP */
	.cpucp_rx_base = 0x17430000,
	.cpucp_rx_size = 0x10000,
	.cpucp_tx_base = 0x18830000,
	.cpucp_tx_size = 0x10000,
	.cpucp_shmem_base = 0x18b4e000,
	.cpucp_shmem_size = 0x400,
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
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	struct acpi_resources res;
	uint8_t *scmi_shmem;
	ACPI_STATUS rv;
	int i, last_pkg;;

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
		aprint_error_dev(self, "couldn't map aoss registers\n");
		return;
	}
	if (bus_space_map(sc->sc_iot, sc->sc_data->cpucp_rx_base,
	    sc->sc_data->cpucp_rx_size, BUS_SPACE_MAP_NONPOSTED,
	    &sc->sc_cpucp_rx_ioh)) {
		aprint_error_dev(self, "couldn't map cpucp rx registers\n");
		return;
	}
	if (bus_space_map(sc->sc_iot, sc->sc_data->cpucp_tx_base,
	    sc->sc_data->cpucp_tx_size, BUS_SPACE_MAP_NONPOSTED,
	    &sc->sc_cpucp_tx_ioh)) {
		aprint_error_dev(self, "couldn't map cpucp tx registers\n");
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

	RXWRITE8(sc, CPUCP_REG_RX_EN, 0);
	RXWRITE8(sc, CPUCP_REG_RX_CLEAR, 0);
	RXWRITE8(sc, CPUCP_REG_RX_MAP, 0);
	RXWRITE8(sc, CPUCP_REG_RX_MAP, CPUCP_MASK_CMD);

	qcpep_sc = sc;

	/* SCMI setup */
	scmi_shmem = AcpiOsMapMemory(sc->sc_data->cpucp_shmem_base,
	    sc->sc_data->cpucp_shmem_size);
	if (scmi_shmem == NULL) {
		aprint_error_dev(self, "couldn't map SCMI shared memory\n");
		return;
	}

	sc->sc_scmi.sc_dev = self;
	sc->sc_scmi.sc_iot = sc->sc_iot;
	sc->sc_scmi.sc_shmem_tx = (struct scmi_shmem *)(scmi_shmem + 0x000);
	sc->sc_scmi.sc_shmem_rx = (struct scmi_shmem *)(scmi_shmem + 0x200);
	sc->sc_scmi.sc_mbox_tx = qccpucp_channel(0);
	sc->sc_scmi.sc_mbox_tx_send = qccpucp_send;
	sc->sc_scmi.sc_mbox_rx = qccpucp_channel(2);
	sc->sc_scmi.sc_mbox_rx_send = qccpucp_send;
	/* Build performance domain to CPU map. */
	sc->sc_scmi.sc_perf_ndmap = 0;
	last_pkg = -1;
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_package_id != last_pkg) {
			sc->sc_scmi.sc_perf_ndmap++;
			last_pkg = ci->ci_package_id;
		}
	}
	sc->sc_scmi.sc_perf_dmap = kmem_zalloc(
	    sizeof(*sc->sc_scmi.sc_perf_dmap) * sc->sc_scmi.sc_perf_ndmap,
	    KM_SLEEP);
	last_pkg = -1;
	i = 0;
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_package_id != last_pkg) {
			sc->sc_scmi.sc_perf_dmap[i].pm_domain = i;
			sc->sc_scmi.sc_perf_dmap[i].pm_ci = ci;
			last_pkg = ci->ci_package_id;
			i++;
		}
	}
	if (scmi_init_mbox(&sc->sc_scmi) != 0) {
		aprint_error_dev(self, "couldn't setup SCMI\n");
		return;
	}
	scmi_attach_perf(&sc->sc_scmi);
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

void *
qccpucp_channel(u_int id)
{
	struct qcpep_softc *sc = qcpep_sc;
	uint64_t val;

	if (sc == NULL || id > 2) {
		return NULL;
	}

	val = RXREAD8(sc, CPUCP_REG_RX_EN);
	val |= (1 << id);
	RXWRITE8(sc, CPUCP_REG_RX_EN, val);
	
	return (void *)(uintptr_t)(id + 1);
}

int
qccpucp_send(void *cookie)
{
	struct qcpep_softc *sc = qcpep_sc;
	uintptr_t id = (uintptr_t)cookie - 1;

	TXWRITE4(sc, CPUCP_REG_CMD(id), 0);
	
	return 0;
}
