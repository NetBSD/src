/* $NetBSD: zynq_xadc.c,v 1.1 2022/11/11 20:31:30 jmcneill Exp $ */

/*-
 * Copyright (c) 2022 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Xilinx 7 series ADC ("XADC")
 *
 * Documentation can be found on the Xilinx web site:
 *  - Zynq-7000 SoC Technical Reference Manual UG585 (v1.13)
 *  - XADC User Guide 3 UG480 (v1.11)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zynq_xadc.c,v 1.1 2022/11/11 20:31:30 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bitops.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/fdt/fdtvar.h>

/* PS-XADC interface registers */
#define	XADCIF_CFG		0x00
#define	 CFG_ENABLE		__BIT(31)
#define	 CFG_WEDGE		__BIT(13)
#define	 CFG_REDGE		__BIT(12)
#define	 CFG_TCKRATE		__BITS(9,8)
#define	 CFG_TCKRATE_DIV4	1
#define	XADCIF_INT_STS		0x04
#define	XADCIF_INT_MASK		0x08
#define	XADCIF_MSTS		0x0c
#define	 MSTS_CFIFOE		__BIT(10)
#define	XADCIF_CMDFIFO		0x10
#define	XADCIF_RDFIFO		0x14
#define	XADCIF_MCTL		0x18

/* XADC registers */
#define	XADC_STATUS_TEMP	0x00
#define	XADC_STATUS_VCCINT	0x01
#define	XADC_STATUS_VCCAUX	0x02
#define	XADC_STATUS_VPVN	0x03
#define	XADC_STATUS_VREFP	0x04
#define	XADC_STATUS_VREFN	0x05
#define	XADC_STATUS_VCCBRAM	0x06
#define	XADC_STATUS_VCCPINT	0x0d
#define	XADC_STATUS_VCCPAUX	0x0e
#define	XADC_STATUS_VCCO_DDR	0x0f
#define	XADC_STATUS_FLAG	0x3f
#define	 FLAG_OT		__BIT(3)
#define	XADC_CONF(n)		(0x40 + (n))
#define	 CONF1_SEQ		__BITS(15,12)
#define	 CONF1_SEQ_CONT		2
#define	XADC_SEQ(n)		(0x48 + (n))
#define	 SEQ0_CALIB		__BIT(0)
#define	 SEQ0_ALL		__BITS(5,14)
#define	 SEQ4_VREFN		__BIT(13)

/* XADC commands */
#define	XADC_COMMAND_CMD	__BITS(29,26)
#define	XADC_COMMAND_DRP_ADDR	__BITS(25,16)
#define	XADC_COMMAND_DRP_DATA	__BITS(15,0)
#define	XADC_COMMAND(cmd, addr, data)			\
	(__SHIFTIN(cmd, XADC_COMMAND_CMD) |		\
	 __SHIFTIN(addr, XADC_COMMAND_DRP_ADDR) |	\
	 __SHIFTIN(data, XADC_COMMAND_DRP_DATA))
#define	XADC_CMD_NOP	0
#define	XADC_CMD_READ	1
#define	XADC_CMD_WRITE	2

enum {
	XADC_SENSOR_TEMP,
	XADC_SENSOR_VCCINT,
	XADC_SENSOR_VCCAUX,
	XADC_SENSOR_VPVN,
	XADC_SENSOR_VREFP,
	XADC_SENSOR_VREFN,
	XADC_SENSOR_VCCBRAM,
	XADC_SENSOR_VCCPINT,
	XADC_SENSOR_VCCPAUX,
	XADC_SENSOR_VCCO_DDR,
	XADC_NSENSOR
};

static const struct {
	const char *name;
	uint32_t units;
	uint16_t reg;
} zynq_xadc_sensors[] = {
	[XADC_SENSOR_TEMP] = { "temperature", ENVSYS_STEMP, XADC_STATUS_TEMP },
	[XADC_SENSOR_VCCINT] = { "vccint", ENVSYS_SVOLTS_DC, XADC_STATUS_VCCINT },
	[XADC_SENSOR_VCCAUX] = { "vccaux", ENVSYS_SVOLTS_DC, XADC_STATUS_VCCAUX },
	[XADC_SENSOR_VPVN] = { "vp/vn", ENVSYS_SVOLTS_DC, XADC_STATUS_VPVN },
	[XADC_SENSOR_VREFP] = { "vrefp", ENVSYS_SVOLTS_DC, XADC_STATUS_VREFP },
	[XADC_SENSOR_VREFN] = { "vrefn", ENVSYS_SVOLTS_DC, XADC_STATUS_VREFN },
	[XADC_SENSOR_VCCBRAM] = { "vccbram", ENVSYS_SVOLTS_DC, XADC_STATUS_VCCBRAM },
	[XADC_SENSOR_VCCPINT] = { "vccpint", ENVSYS_SVOLTS_DC, XADC_STATUS_VCCPINT },
	[XADC_SENSOR_VCCPAUX] = { "vccpaux", ENVSYS_SVOLTS_DC, XADC_STATUS_VCCPAUX },
	[XADC_SENSOR_VCCO_DDR] = { "vcco_ddr", ENVSYS_SVOLTS_DC, XADC_STATUS_VCCO_DDR },
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "xlnx,zynq-xadc-1.00.a" },
	DEVICE_COMPAT_EOL
};

struct zynq_xadc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_lock;

	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[XADC_NSENSOR];
};

#define RD4(sc, reg) 		\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val) 	\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	zynq_xadc_match(device_t, cfdata_t, void *);
static void	zynq_xadc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(zynqxadc, sizeof(struct zynq_xadc_softc),
	zynq_xadc_match, zynq_xadc_attach, NULL, NULL);

static void
zynq_xadc_write(struct zynq_xadc_softc *sc, uint16_t reg,
    uint16_t data)
{
	int retry = 10000;

	/*
	 * Write sequence is:
	 *
	 * 1. Prepare write command
	 * 2. Write data to Command FIFO
	 * 3. Wait until the Command FIFO becomes empty
	 */

	WR4(sc, XADCIF_CMDFIFO, XADC_COMMAND(XADC_CMD_WRITE, reg, data));
	while (--retry > 0) {
		if ((RD4(sc, XADCIF_MSTS) & MSTS_CFIFOE) != 0) {
			break;
		}
		delay(10);
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "command FIFO timeout (write)\n");
	}
	/*
	 * Every write to Command FIFO shifts data into Read FIFO, so
	 * drain that after the command completes.
	 */
	RD4(sc, XADCIF_RDFIFO);
}

static uint16_t
zynq_xadc_read(struct zynq_xadc_softc *sc, uint16_t reg)
{
	int retry = 10000;
	uint32_t val;

	/*
	 * Read sequence is:
	 *
	 * 1. Prepare read command
	 * 2. Write data to Command FIFO
	 * 3. Wait until the Command FIFO becomes empty
	 * 4. Read dummy data from the Read Data FIFO
	 * 5. Prepare nop command
	 * 6. Write data to Command FIFO
	 * 7. Read the Read Data FIFO
	 */

	WR4(sc, XADCIF_CMDFIFO, XADC_COMMAND(XADC_CMD_READ, reg, 0));
	WR4(sc, XADCIF_CMDFIFO, XADC_COMMAND(XADC_CMD_NOP, 0, 0));
	while (--retry > 0) {
		if ((RD4(sc, XADCIF_MSTS) & MSTS_CFIFOE) != 0) {
			break;
		}
		delay(10);
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "command FIFO timeout (read)\n");
		return 0xffff;
	}
	val = RD4(sc, XADCIF_RDFIFO);
	val = RD4(sc, XADCIF_RDFIFO);

	return val & 0xffff;
}

static void
zynq_xadc_init(struct zynq_xadc_softc *sc, struct clk *clk)
{
	uint32_t val;

	/* Enable the PS-XADC interface */
	val = RD4(sc, XADCIF_CFG);
	val |= CFG_ENABLE;
	val &= ~CFG_TCKRATE;
	val |= __SHIFTIN(CFG_TCKRATE_DIV4, CFG_TCKRATE);
	val |= CFG_WEDGE | CFG_REDGE;
	WR4(sc, XADCIF_CFG, val);
	WR4(sc, XADCIF_MCTL, 0);

	/* Turn on continuous sampling for all ADC channels we monitor */
	zynq_xadc_write(sc, XADC_SEQ(0), SEQ0_CALIB | SEQ0_ALL);
	zynq_xadc_write(sc, XADC_SEQ(4), SEQ4_VREFN);
	zynq_xadc_write(sc, XADC_CONF(0), 0);
	zynq_xadc_write(sc, XADC_CONF(1),
	    __SHIFTIN(CONF1_SEQ_CONT, CONF1_SEQ));

}

static void
zynq_xadc_sensors_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct zynq_xadc_softc *sc = sme->sme_cookie;
	union {
		uint16_t u16;
		int16_t s16;
	} val;
	int64_t temp;

	
	val.u16 = zynq_xadc_read(sc, zynq_xadc_sensors[edata->sensor].reg);
	if (edata->units == ENVSYS_STEMP) {
		if (val.u16 == 0) {
			edata->state = ENVSYS_SINVALID;
		} else {
        		temp = ((int64_t)(val.u16 >> 4) * 503975) / 4096;
			edata->value_cur = 1000 * temp;
			edata->state = ENVSYS_SVALID;
		}

		val.u16 = zynq_xadc_read(sc, XADC_STATUS_FLAG);
		if ((val.u16 & FLAG_OT) != 0) {
			edata->state = ENVSYS_SCRITOVER;
		}
	} else {
		KASSERT(edata->units == ENVSYS_SVOLTS_DC);
		switch (edata->sensor) {
		case XADC_SENSOR_VPVN:
			edata->value_cur = (((val.u16 >> 4) * 1000) / 4096) * 1000;
			break;
		case XADC_SENSOR_VREFN:
			edata->value_cur = (((val.s16 >> 4) * 3000) / 4096) * 1000;
			break;
		default:
			edata->value_cur = (((val.u16 >> 4) * 3000) / 4096) * 1000;
			break;
		}
		edata->state = ENVSYS_SVALID;
	}
}

static int
zynq_xadc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
zynq_xadc_attach(device_t parent, device_t self, void *aux)
{
	struct zynq_xadc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	u_int n;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	clk = fdtbus_clock_get_index(phandle, 0);
	if (clk == NULL || clk_enable(clk) != 0) {
		aprint_error(": couldn't enable clock\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	aprint_naive("\n");
	aprint_normal(": ADC\n");

	zynq_xadc_init(sc, clk);

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = zynq_xadc_sensors_refresh;

	for (n = 0; n < XADC_NSENSOR; n++) {
		sc->sc_sensor[n].units = zynq_xadc_sensors[n].units;
		sc->sc_sensor[n].state = ENVSYS_SINVALID;
		sc->sc_sensor[n].flags = ENVSYS_FHAS_ENTROPY;
		if (zynq_xadc_sensors[n].units == ENVSYS_STEMP) {
			sc->sc_sensor[n].flags |= ENVSYS_FMONCRITICAL;
		}
		strncpy(sc->sc_sensor[n].desc, zynq_xadc_sensors[n].name,
		    sizeof(sc->sc_sensor[n].desc));
		sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[n]);
	}

	sysmon_envsys_register(sc->sc_sme);
}
