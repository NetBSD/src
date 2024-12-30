/* $NetBSD: qcomspmi.c,v 1.1 2024/12/30 12:31:10 jmcneill Exp $ */
/*	$OpenBSD: qcspmi.c,v 1.6 2024/08/14 10:54:58 mglocker Exp $	*/
/*
 * Copyright (c) 2022 Patrick Wildt <patrick@blueri.se>
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
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/acpi/acpivar.h>

/* SPMI commands */
#define SPMI_CMD_EXT_WRITEL	0x30
#define SPMI_CMD_EXT_READL	0x38

/* Core registers. */
#define SPMI_VERSION		0x00
#define  SPMI_VERSION_V2_MIN		0x20010000
#define  SPMI_VERSION_V3_MIN		0x30000000
#define  SPMI_VERSION_V5_MIN		0x50000000
#define  SPMI_VERSION_V7_MIN		0x70000000
#define SPMI_ARB_APID_MAP(sc, x)	((sc)->sc_arb_apid_map + (x) * 0x4)
#define  SPMI_ARB_APID_MAP_PPID_MASK	0xfff
#define  SPMI_ARB_APID_MAP_PPID_SHIFT	8
#define  SPMI_ARB_APID_MAP_IRQ_OWNER	(1 << 14)

/* Channel registers. */
#define SPMI_CHAN_OFF(sc, x)	((sc)->sc_chan_stride * (x))
#define SPMI_OBSV_OFF(sc, x, y)	\
	((sc)->sc_obsv_ee_stride * (x) + (sc)->sc_obsv_apid_stride * (y))
#define SPMI_COMMAND		0x00
#define  SPMI_COMMAND_OP_EXT_WRITEL	(0 << 27)
#define  SPMI_COMMAND_OP_EXT_READL	(1 << 27)
#define  SPMI_COMMAND_OP_EXT_WRITE	(2 << 27)
#define  SPMI_COMMAND_OP_RESET		(3 << 27)
#define  SPMI_COMMAND_OP_SLEEP		(4 << 27)
#define  SPMI_COMMAND_OP_SHUTDOWN	(5 << 27)
#define  SPMI_COMMAND_OP_WAKEUP		(6 << 27)
#define  SPMI_COMMAND_OP_AUTHENTICATE	(7 << 27)
#define  SPMI_COMMAND_OP_MSTR_READ	(8 << 27)
#define  SPMI_COMMAND_OP_MSTR_WRITE	(9 << 27)
#define  SPMI_COMMAND_OP_EXT_READ	(13 << 27)
#define  SPMI_COMMAND_OP_WRITE		(14 << 27)
#define  SPMI_COMMAND_OP_READ		(15 << 27)
#define  SPMI_COMMAND_OP_ZERO_WRITE	(16 << 27)
#define  SPMI_COMMAND_ADDR(x)		(((x) & 0xff) << 4)
#define  SPMI_COMMAND_LEN(x)		(((x) & 0x7) << 0)
#define SPMI_CONFIG		0x04
#define SPMI_STATUS		0x08
#define  SPMI_STATUS_DONE		(1 << 0)
#define  SPMI_STATUS_FAILURE		(1 << 1)
#define  SPMI_STATUS_DENIED		(1 << 2)
#define  SPMI_STATUS_DROPPED		(1 << 3)
#define SPMI_WDATA0		0x10
#define SPMI_WDATA1		0x14
#define SPMI_RDATA0		0x18
#define SPMI_RDATA1		0x1c
#define SPMI_ACC_ENABLE		0x100
#define  SPMI_ACC_ENABLE_BIT		(1 << 0)
#define SPMI_IRQ_STATUS		0x104
#define SPMI_IRQ_CLEAR		0x108

/* Intr registers */
#define SPMI_OWNER_ACC_STATUS(sc, x, y)	\
	((sc)->sc_chan_stride * (x) + 0x4 * (y))

/* Config registers */
#define SPMI_OWNERSHIP_TABLE(sc, x)	((sc)->sc_ownership_table + (x) * 0x4)
#define  SPMI_OWNERSHIP_TABLE_OWNER(x)	((x) & 0x7)

/* Misc */
#define SPMI_MAX_PERIPH		1024
#define SPMI_MAX_PPID		4096
#define SPMI_PPID_TO_APID_VALID	(1U << 15)
#define SPMI_PPID_TO_APID_MASK	(0x7fff)

/* Intr commands */
#define INTR_RT_STS		0x10
#define INTR_SET_TYPE		0x11
#define INTR_POLARITY_HIGH	0x12
#define INTR_POLARITY_LOW	0x13
#define INTR_LATCHED_CLR	0x14
#define INTR_EN_SET		0x15
#define INTR_EN_CLR		0x16
#define INTR_LATCHED_STS	0x18

#define HREAD4(sc, obj, reg)						\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, 			\
			 (sc)->sc_data->regs[obj] + (reg))
#define HWRITE4(sc, obj, reg, val)					\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, 			\
			  (sc)->sc_data->regs[obj] + (reg), (val))

#define QCSPMI_REG_CORE		0
#define QCSPMI_REG_CHNLS	1
#define QCSPMI_REG_OBSRVR	2
#define QCSPMI_REG_INTR		3
#define QCSPMI_REG_CNFG		4
#define QCSPMI_REG_MAX		5

struct qcspmi_apid {
	uint16_t		ppid;
	uint8_t			write_ee;
	uint8_t			irq_ee;
};

struct qcspmi_data {
	bus_size_t		regs[QCSPMI_REG_MAX];
	int			ee;
};

struct qcspmi_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	const struct qcspmi_data *sc_data;

	int			sc_ee;

	struct qcspmi_apid	sc_apid[SPMI_MAX_PERIPH];
	uint16_t		sc_ppid_to_apid[SPMI_MAX_PPID];
	uint16_t		sc_max_periph;
	bus_size_t		sc_chan_stride;
	bus_size_t		sc_obsv_ee_stride;
	bus_size_t		sc_obsv_apid_stride;
	bus_size_t		sc_arb_apid_map;
	bus_size_t		sc_ownership_table;
};

static int	qcspmi_match(device_t, cfdata_t, void *);
static void	qcspmi_attach(device_t, device_t, void *);

int	qcspmi_cmd_read(struct qcspmi_softc *, uint8_t, uint8_t,
	    uint16_t, void *, size_t);
int	qcspmi_cmd_write(struct qcspmi_softc *, uint8_t, uint8_t, uint16_t,
	    const void *, size_t);

CFATTACH_DECL_NEW(qcomspmi, sizeof(struct qcspmi_softc),
    qcspmi_match, qcspmi_attach, NULL, NULL);

static const struct qcspmi_data qcspmi_x1e_data = {
	.ee = 0,
	.regs = {
		[QCSPMI_REG_CORE]   = 0x0,
		[QCSPMI_REG_CHNLS]  = 0x100000,
		[QCSPMI_REG_OBSRVR] = 0x40000,
		[QCSPMI_REG_INTR]   = 0xc0000,
		[QCSPMI_REG_CNFG]   = 0x2d000,
	},
};

static const struct device_compatible_entry compat_data[] = {
        { .compat = "QCOM0C0B", .data = &qcspmi_x1e_data },
        DEVICE_COMPAT_EOL
};

static int
qcspmi_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

void
qcspmi_attach(device_t parent, device_t self, void *aux)
{
	struct acpi_attach_args *aa = aux;
	struct qcspmi_softc *sc = device_private(self);
	struct qcspmi_apid *apid, *last_apid;
	struct acpi_resources res;
        struct acpi_mem *mem;
	uint32_t val, ppid, irq_own;
	ACPI_STATUS rv;
	int error, i;

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

	sc->sc_dev = self;
	sc->sc_data = acpi_compatible_lookup(aa, compat_data)->data;
	sc->sc_iot = aa->aa_memt;
	error = bus_space_map(sc->sc_iot, mem->ar_base, mem->ar_length, 0,
	    &sc->sc_ioh);
	if (error != 0) {
		aprint_error_dev(self, "couldn't map registers\n");
		goto done;
	}

	/* Support only version 5 and 7 for now */
	val = HREAD4(sc, QCSPMI_REG_CORE, SPMI_VERSION);
	if (val < SPMI_VERSION_V5_MIN) {
		printf(": unsupported version 0x%08x\n", val);
		return;
	}

	if (val < SPMI_VERSION_V7_MIN) {
		sc->sc_max_periph = 512;
		sc->sc_chan_stride = 0x10000;
		sc->sc_obsv_ee_stride = 0x10000;
		sc->sc_obsv_apid_stride = 0x00080;
		sc->sc_arb_apid_map = 0x00900;
		sc->sc_ownership_table = 0x00700;
	} else {
		sc->sc_max_periph = 1024;
		sc->sc_chan_stride = 0x01000;
		sc->sc_obsv_ee_stride = 0x08000;
		sc->sc_obsv_apid_stride = 0x00020;
		sc->sc_arb_apid_map = 0x02000;
		sc->sc_ownership_table = 0x00000;
	}

	KASSERT(sc->sc_max_periph <= SPMI_MAX_PERIPH);

	sc->sc_ee = sc->sc_data->ee;

	for (i = 0; i < sc->sc_max_periph; i++) {
		val = HREAD4(sc, QCSPMI_REG_CORE, SPMI_ARB_APID_MAP(sc, i));
		if (!val)
			continue;
		ppid = (val >> SPMI_ARB_APID_MAP_PPID_SHIFT) &
		    SPMI_ARB_APID_MAP_PPID_MASK;
		irq_own = val & SPMI_ARB_APID_MAP_IRQ_OWNER;
		val = HREAD4(sc, QCSPMI_REG_CNFG, SPMI_OWNERSHIP_TABLE(sc, i));
		apid = &sc->sc_apid[i];
		apid->write_ee = SPMI_OWNERSHIP_TABLE_OWNER(val);
		apid->irq_ee = 0xff;
		if (irq_own)
			apid->irq_ee = apid->write_ee;
		last_apid = &sc->sc_apid[sc->sc_ppid_to_apid[ppid] &
		    SPMI_PPID_TO_APID_MASK];
		if (!(sc->sc_ppid_to_apid[ppid] & SPMI_PPID_TO_APID_VALID) ||
		    apid->write_ee == sc->sc_ee) {
			sc->sc_ppid_to_apid[ppid] = SPMI_PPID_TO_APID_VALID | i;
		} else if ((sc->sc_ppid_to_apid[ppid] &
		    SPMI_PPID_TO_APID_VALID) && irq_own &&
		    last_apid->write_ee == sc->sc_ee) {
			last_apid->irq_ee = apid->irq_ee;
		}
	}

done:
	acpi_resource_cleanup(&res);
}

int
qcspmi_cmd_read(struct qcspmi_softc *sc, uint8_t sid, uint8_t cmd,
    uint16_t addr, void *buf, size_t len)
{
	uint8_t *cbuf = buf;
	uint32_t reg;
	uint16_t apid, ppid;
	int bc = len - 1;
	int i;

	if (len == 0 || len > 8)
		return EINVAL;

	/* TODO: support more types */
	if (cmd != SPMI_CMD_EXT_READL)
		return EINVAL;

	ppid = (sid << 8) | (addr >> 8);
	if (!(sc->sc_ppid_to_apid[ppid] & SPMI_PPID_TO_APID_VALID))
		return ENXIO;
	apid = sc->sc_ppid_to_apid[ppid] & SPMI_PPID_TO_APID_MASK;

	HWRITE4(sc, QCSPMI_REG_OBSRVR,
	    SPMI_OBSV_OFF(sc, sc->sc_ee, apid) + SPMI_COMMAND,
	    SPMI_COMMAND_OP_EXT_READL | SPMI_COMMAND_ADDR(addr) |
	    SPMI_COMMAND_LEN(bc));

	for (i = 1000; i > 0; i--) {
		reg = HREAD4(sc, QCSPMI_REG_OBSRVR,
		    SPMI_OBSV_OFF(sc, sc->sc_ee, apid) + SPMI_STATUS);
		if (reg & SPMI_STATUS_DONE)
			break;
		if (reg & SPMI_STATUS_FAILURE) {
			printf(": transaction failed\n");
			return EIO;
		}
		if (reg & SPMI_STATUS_DENIED) {
			printf(": transaction denied\n");
			return EIO;
		}
		if (reg & SPMI_STATUS_DROPPED) {
			printf(": transaction dropped\n");
			return EIO;
		}
	}
	if (i == 0) {
		printf("\n");
		return ETIMEDOUT;
	}

	if (len > 0) {
		reg = HREAD4(sc, QCSPMI_REG_OBSRVR,
		    SPMI_OBSV_OFF(sc, sc->sc_ee, apid) + SPMI_RDATA0);
		memcpy(cbuf, &reg, MIN(len, 4));
		cbuf += MIN(len, 4);
		len -= MIN(len, 4);
	}
	if (len > 0) {
		reg = HREAD4(sc, QCSPMI_REG_OBSRVR,
		    SPMI_OBSV_OFF(sc, sc->sc_ee, apid) + SPMI_RDATA1);
		memcpy(cbuf, &reg, MIN(len, 4));
		cbuf += MIN(len, 4);
		len -= MIN(len, 4);
	}

	return 0;
}

int
qcspmi_cmd_write(struct qcspmi_softc *sc, uint8_t sid, uint8_t cmd,
    uint16_t addr, const void *buf, size_t len)
{
	const uint8_t *cbuf = buf;
	uint32_t reg;
	uint16_t apid, ppid;
	int bc = len - 1;
	int i;

	if (len == 0 || len > 8)
		return EINVAL;

	/* TODO: support more types */
	if (cmd != SPMI_CMD_EXT_WRITEL)
		return EINVAL;

	ppid = (sid << 8) | (addr >> 8);
	if (!(sc->sc_ppid_to_apid[ppid] & SPMI_PPID_TO_APID_VALID))
		return ENXIO;
	apid = sc->sc_ppid_to_apid[ppid] & SPMI_PPID_TO_APID_MASK;

	if (sc->sc_apid[apid].write_ee != sc->sc_ee)
		return EPERM;

	if (len > 0) {
		memcpy(&reg, cbuf, MIN(len, 4));
		HWRITE4(sc, QCSPMI_REG_CHNLS, SPMI_CHAN_OFF(sc, apid) +
		    SPMI_WDATA0, reg);
		cbuf += MIN(len, 4);
		len -= MIN(len, 4);
	}
	if (len > 0) {
		memcpy(&reg, cbuf, MIN(len, 4));
		HWRITE4(sc, QCSPMI_REG_CHNLS, SPMI_CHAN_OFF(sc, apid) +
		    SPMI_WDATA1, reg);
		cbuf += MIN(len, 4);
		len -= MIN(len, 4);
	}

	HWRITE4(sc, QCSPMI_REG_CHNLS, SPMI_CHAN_OFF(sc, apid) + SPMI_COMMAND,
	    SPMI_COMMAND_OP_EXT_WRITEL | SPMI_COMMAND_ADDR(addr) |
	    SPMI_COMMAND_LEN(bc));

	for (i = 1000; i > 0; i--) {
		reg = HREAD4(sc, QCSPMI_REG_CHNLS, SPMI_CHAN_OFF(sc, apid) +
		    SPMI_STATUS);
		if (reg & SPMI_STATUS_DONE)
			break;
	}
	if (i == 0)
		return ETIMEDOUT;

	if (reg & SPMI_STATUS_FAILURE ||
	    reg & SPMI_STATUS_DENIED ||
	    reg & SPMI_STATUS_DROPPED)
		return EIO;

	return 0;
}
