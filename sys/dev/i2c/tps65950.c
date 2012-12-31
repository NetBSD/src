/* $NetBSD: tps65950.c,v 1.1 2012/12/31 13:26:42 jmcneill Exp $ */

/*-
 * Copyright (c) 2012 Jared D. McNeill <jmcneill@invisible.ca>
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
 * TI TPS65950 OMAP Power Management and System Companion Device
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tps65950.c,v 1.1 2012/12/31 13:26:42 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>

#include <dev/i2c/i2cvar.h>

/* I2C Bus Addressing */
#define	TPS65950_ADDR_ID1		0x48	/* GP */
#define TPS65950_ADDR_ID2		0x49	/* GP */
#define TPS65950_ADDR_ID3		0x4a	/* GP */
#define TPS65950_ADDR_ID4		0x4b	/* GP */
#define TPS65950_ADDR_ID5		0x12	/* SmartReflex */

#define TPS65950_ID2_IDCODE_7_0		0x85
#define TPS65950_ID2_IDCODE_15_8	0x86
#define TPS65950_ID2_IDCODE_23_16	0x87
#define TPS65950_ID2_IDCODE_31_24	0x88
#define TPS65950_ID2_UNLOCK_TEST_REG	0x97
#define TPS65950_ID2_UNLOCK_TEST_REG_MAGIC 0x49

#define TPS65950_LED_BASE		0xee

#define	TPS65950_ID3_REG_LED		(TPS65950_LED_BASE + 0)
#define	TPS65950_ID3_REG_LED_LEDAON	__BIT(0)
#define	TPS65950_ID3_REG_LED_LEDBON	__BIT(1)
#define TPS65950_ID3_REG_LED_LEDAPWM	__BIT(4)
#define TPS65950_ID3_REG_LED_LEDBPWM	__BIT(5)

struct tps65950_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	struct sysctllog *sc_sysctllog;
};

static int	tps65950_match(device_t, cfdata_t, void *);
static void	tps65950_attach(device_t, device_t, void *);

static int	tps65950_read_1(struct tps65950_softc *, uint8_t, uint8_t *);
static int	tps65950_write_1(struct tps65950_softc *, uint8_t, uint8_t);

static void	tps65950_sysctl_attach(struct tps65950_softc *);

CFATTACH_DECL_NEW(tps65950pm, sizeof(struct tps65950_softc),
    tps65950_match, tps65950_attach, NULL, NULL);

static int
tps65950_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;

	switch (ia->ia_addr) {
	case TPS65950_ADDR_ID1:
	case TPS65950_ADDR_ID2:
	case TPS65950_ADDR_ID3:
	case TPS65950_ADDR_ID4:
	case TPS65950_ADDR_ID5:
		return 1;
	default:
		return 0;
	}
}

static void
tps65950_attach(device_t parent, device_t self, void *aux)
{
	struct tps65950_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	uint8_t buf[4];
	uint32_t idcode;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	aprint_naive("\n");

	switch (sc->sc_addr) {
	case TPS65950_ADDR_ID2:
		memset(buf, 0, sizeof(buf));
		tps65950_write_1(sc, TPS65950_ID2_UNLOCK_TEST_REG,
		    TPS65950_ID2_UNLOCK_TEST_REG_MAGIC);
		tps65950_read_1(sc, TPS65950_ID2_IDCODE_7_0, &buf[0]);
		tps65950_read_1(sc, TPS65950_ID2_IDCODE_15_8, &buf[1]);
		tps65950_read_1(sc, TPS65950_ID2_IDCODE_23_16, &buf[2]);
		tps65950_read_1(sc, TPS65950_ID2_IDCODE_31_24, &buf[3]);
		idcode = (buf[0] << 0) | (buf[1] << 8) |
			 (buf[2] << 16) | (buf[3] << 24);
		aprint_normal(": IDCODE %08X\n", idcode);
		break;
	case TPS65950_ADDR_ID3:
		aprint_normal(": group %02x\n", sc->sc_addr);
		tps65950_sysctl_attach(sc);
		break;
	default:
		aprint_normal(": group %02x\n", sc->sc_addr);
		break;
	}
}

static int
tps65950_read_1(struct tps65950_softc *sc, uint8_t reg, uint8_t *val)
{
	return iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &reg, sizeof(reg), val, sizeof(*val), 0);
}

static int
tps65950_write_1(struct tps65950_softc *sc, uint8_t reg, uint8_t val)
{
	uint8_t data[2] = { reg, val };
	return iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    NULL, 0, data, sizeof(data), 0);
}

static int
tps65950_sysctl_leda(SYSCTLFN_ARGS)
{
	struct tps65950_softc *sc;
	struct sysctlnode node;
	uint8_t val;
	u_int leda;
	int error;

	node = *rnode;
	sc = node.sysctl_data;
	error = tps65950_read_1(sc, TPS65950_ID3_REG_LED, &val);
	if (error)
		return error;
	leda = (val & TPS65950_ID3_REG_LED_LEDAON) ? 1 : 0;
	node.sysctl_data = &leda;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (leda)
		val |= (TPS65950_ID3_REG_LED_LEDAON|TPS65950_ID3_REG_LED_LEDAPWM);
	else
		val &= ~(TPS65950_ID3_REG_LED_LEDAON|TPS65950_ID3_REG_LED_LEDAPWM);

	return tps65950_write_1(sc, TPS65950_ID3_REG_LED, val);
}

static int
tps65950_sysctl_ledb(SYSCTLFN_ARGS)
{
	struct tps65950_softc *sc;
	struct sysctlnode node;
	uint8_t val;
	u_int ledb;
	int error;

	node = *rnode;
	sc = node.sysctl_data;
	error = tps65950_read_1(sc, TPS65950_ID3_REG_LED, &val);
	if (error)
		return error;
	ledb = (val & TPS65950_ID3_REG_LED_LEDBON) ? 1 : 0;
	node.sysctl_data = &ledb;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (ledb)
		val |= (TPS65950_ID3_REG_LED_LEDBON|TPS65950_ID3_REG_LED_LEDBPWM);
	else
		val &= ~(TPS65950_ID3_REG_LED_LEDBON|TPS65950_ID3_REG_LED_LEDBPWM);

	return tps65950_write_1(sc, TPS65950_ID3_REG_LED, val);
}

static void
tps65950_sysctl_attach(struct tps65950_softc *sc)
{
	struct sysctllog **log = &sc->sc_sysctllog;
	const struct sysctlnode *rnode, *cnode;
	int error;

	error = sysctl_createv(log, 0, NULL, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "hw", NULL, NULL, 0, NULL, 0, CTL_HW, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "tps65950", SYSCTL_DESCR("tps65950 control"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "leda",
	    SYSCTL_DESCR("LEDA enable"), tps65950_sysctl_leda, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "ledb",
	    SYSCTL_DESCR("LEDB enable"), tps65950_sysctl_ledb, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;
}
