/* $NetBSD: qcomiic.c,v 1.4 2025/09/15 15:18:42 thorpej Exp $ */

/*	$OpenBSD: qciic.c,v 1.7 2024/10/02 21:21:32 kettenis Exp $	*/
/*
 * Copyright (c) 2022 Mark Kettenis <kettenis@openbsd.org>
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
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>
#include <dev/acpi/acpi_i2c.h>

#include <dev/i2c/i2cvar.h>

/* Registers */
#define GENI_I2C_TX_TRANS_LEN		0x26c
#define GENI_I2C_RX_TRANS_LEN		0x270
#define GENI_M_CMD0			0x600
#define  GENI_M_CMD0_OPCODE_I2C_WRITE	(0x1 << 27)
#define  GENI_M_CMD0_OPCODE_I2C_READ	(0x2 << 27)
#define  GENI_M_CMD0_SLV_ADDR_SHIFT	9
#define  GENI_M_CMD0_STOP_STRETCH	(1 << 2)
#define GENI_M_IRQ_STATUS		0x610
#define GENI_M_IRQ_CLEAR		0x618
#define  GENI_M_IRQ_CMD_DONE		(1 << 0)
#define GENI_TX_FIFO			0x700
#define GENI_RX_FIFO			0x780
#define GENI_TX_FIFO_STATUS		0x800
#define GENI_RX_FIFO_STATUS		0x804
#define  GENI_RX_FIFO_STATUS_WC(val)	((val) & 0xffffff)

#define HREAD4(sc, reg)							\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

struct qciic_softc {
	device_t		sc_dev;
	struct acpi_devnode	*sc_acpi;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	device_t		sc_iic;

	struct i2c_controller	sc_ic;
};

static int	qciic_acpi_match(device_t, cfdata_t, void *);
static void	qciic_acpi_attach(device_t, device_t, void *);
static int	qciic_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
		    void *, size_t, int);

CFATTACH_DECL_NEW(qcomiic, sizeof(struct qciic_softc),
    qciic_acpi_match, qciic_acpi_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "QCOM0610" },
	{ .compat = "QCOM0811" },
	{ .compat = "QCOM0C10" },
	DEVICE_COMPAT_EOL
};

static int
qciic_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
qciic_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct qciic_softc * const sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_STATUS rv;
	int error;

	sc->sc_dev = self;
	sc->sc_acpi = aa->aa_node;
	sc->sc_iot = aa->aa_memt;

	rv = acpi_resource_parse(sc->sc_dev, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv)) {
		return;
	}

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		aprint_error_dev(self, "couldn't find mem resource\n");
		goto done;
	}

	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		aprint_error_dev(self, "couldn't find irq resource\n");
		goto done;
	}

	error = bus_space_map(sc->sc_iot, mem->ar_base, mem->ar_length, 0,
	    &sc->sc_ioh);
	if (error != 0) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	iic_tag_init(&sc->sc_ic);
	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_exec = qciic_exec;

	acpi_i2c_register(aa->aa_node, self, &sc->sc_ic);

	iicbus_attach(self, &sc->sc_ic);

done:
	acpi_resource_cleanup(&res);
}

static int
qciic_wait(struct qciic_softc *sc, uint32_t bits)
{
	uint32_t stat;
	int timo;

	for (timo = 50000; timo > 0; timo--) {
		stat = HREAD4(sc, GENI_M_IRQ_STATUS);
		if (stat & bits)
			break;
		delay(10);
	}
	if (timo == 0)
		return ETIMEDOUT;

	return 0;
}

static int
qciic_read(struct qciic_softc *sc, uint8_t *buf, size_t len)
{
	uint32_t stat, word;
	int timo, i;

	word = 0;
	for (i = 0; i < len; i++) {
		if ((i % 4) == 0) {
			for (timo = 50000; timo > 0; timo--) {
				stat = HREAD4(sc, GENI_RX_FIFO_STATUS);
				if (GENI_RX_FIFO_STATUS_WC(stat) > 0)
					break;
				delay(10);
			}
			if (timo == 0)
				return ETIMEDOUT;
			word = HREAD4(sc, GENI_RX_FIFO);
		}
		buf[i] = word >> ((i % 4) * 8);
	}

	return 0;
}

static int
qciic_write(struct qciic_softc *sc, const uint8_t *buf, size_t len)
{
	uint32_t stat, word;
	int timo, i;

	word = 0;
	for (i = 0; i < len; i++) {
		word |= buf[i] << ((i % 4) * 8);
		if ((i % 4) == 3 || i == (len - 1)) {
			for (timo = 50000; timo > 0; timo--) {
				stat = HREAD4(sc, GENI_TX_FIFO_STATUS);
				if (stat < 16)
					break;
				delay(10);
			}
			if (timo == 0)
				return ETIMEDOUT;
			HWRITE4(sc, GENI_TX_FIFO, word);
			word = 0;
		}
	}

	return 0;
}

static int
qciic_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *cmd,
    size_t cmdlen, void *buf, size_t buflen, int flags)
{
	struct qciic_softc *sc = cookie;
	uint32_t m_cmd, m_param, stat;
	int error;

	m_param = addr << GENI_M_CMD0_SLV_ADDR_SHIFT;
	m_param |= GENI_M_CMD0_STOP_STRETCH;

	if (buflen == 0 && I2C_OP_STOP_P(op))
		m_param &= ~GENI_M_CMD0_STOP_STRETCH;

	if (cmdlen > 0) {
		stat = HREAD4(sc, GENI_M_IRQ_STATUS);
		HWRITE4(sc, GENI_M_IRQ_CLEAR, stat);
		HWRITE4(sc, GENI_I2C_TX_TRANS_LEN, cmdlen);
		m_cmd = GENI_M_CMD0_OPCODE_I2C_WRITE | m_param;
		HWRITE4(sc, GENI_M_CMD0, m_cmd);

		error = qciic_write(sc, cmd, cmdlen);
		if (error)
			return error;

		error = qciic_wait(sc, GENI_M_IRQ_CMD_DONE);
		if (error)
			return error;
	}

	if (buflen == 0)
		return 0;

	if (I2C_OP_STOP_P(op))
		m_param &= ~GENI_M_CMD0_STOP_STRETCH;

	if (I2C_OP_READ_P(op)) {
		stat = HREAD4(sc, GENI_M_IRQ_STATUS);
		HWRITE4(sc, GENI_M_IRQ_CLEAR, stat);
		HWRITE4(sc, GENI_I2C_RX_TRANS_LEN, buflen);
		m_cmd = GENI_M_CMD0_OPCODE_I2C_READ | m_param;
		HWRITE4(sc, GENI_M_CMD0, m_cmd);

		error = qciic_read(sc, buf, buflen);
		if (error)
			return error;

		error = qciic_wait(sc, GENI_M_IRQ_CMD_DONE);
		if (error)
			return error;
	} else {
		stat = HREAD4(sc, GENI_M_IRQ_STATUS);
		HWRITE4(sc, GENI_M_IRQ_CLEAR, stat);
		HWRITE4(sc, GENI_I2C_TX_TRANS_LEN, buflen);
		m_cmd = GENI_M_CMD0_OPCODE_I2C_WRITE | m_param;
		HWRITE4(sc, GENI_M_CMD0, m_cmd);

		error = qciic_write(sc, buf, buflen);
		if (error)
			return error;

		error = qciic_wait(sc, GENI_M_IRQ_CMD_DONE);
		if (error)
			return error;
	}

	return 0;
}
