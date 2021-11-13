/* $NetBSD: rk_i2c.c,v 1.12 2021/11/13 01:08:15 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: rk_i2c.c,v 1.12 2021/11/13 01:08:15 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kmem.h>

#include <dev/i2c/i2cvar.h>

#include <dev/fdt/fdtvar.h>

#define	RKI2C_CON		0x000
#define	 RKI2C_CON_ACT2NAK	__BIT(6)
#define	 RKI2C_CON_ACK		__BIT(5)
#define	 RKI2C_CON_STOP		__BIT(4)
#define	 RKI2C_CON_START	__BIT(3)
#define	 RKI2C_CON_I2C_MODE	__BITS(2,1)
#define	  RKI2C_CON_I2C_MODE_TX		0
#define	  RKI2C_CON_I2C_MODE_RTX	1
#define	  RKI2C_CON_I2C_MODE_RX		2
#define	  RKI2C_CON_I2C_MODE_RRX	3
#define	 RKI2C_CON_I2C_EN	__BIT(0)

#define	RKI2C_CLKDIV		0x004
#define	 RKI2C_CLKDIV_CLKDIVH	__BITS(31,16)
#define	 RKI2C_CLKDIV_CLKDIVL	__BITS(15,0)

#define	RKI2C_MRXADDR		0x008
#define	 RKI2C_MRXADDR_ADDHVLD	__BIT(26)
#define	 RKI2C_MRXADDR_ADDMVLD	__BIT(25)
#define	 RKI2C_MRXADDR_ADDLVLD	__BIT(24)
#define	 RKI2C_MRXADDR_SADDR	__BITS(23,0)

#define	RKI2C_MRXRADDR		0x00c
#define	 RKI2C_MRXRADDR_ADDHVLD	__BIT(26)
#define	 RKI2C_MRXRADDR_ADDMVLD	__BIT(25)
#define	 RKI2C_MRXRADDR_ADDLVLD	__BIT(24)
#define	 RKI2C_MRXRADDR_SADDR	__BITS(23,0)

#define	RKI2C_MTXCNT		0x010
#define	 RKI2C_MTXCNT_MTXCNT	__BITS(5,0)

#define	RKI2C_MRXCNT		0x014
#define	 RKI2C_MRXCNT_MRXCNT	__BITS(5,0)

#define	RKI2C_IEN		0x018
#define	 RKI2C_IEN_NAKRCVIEN	__BIT(6)
#define	 RKI2C_IEN_STOPIEN	__BIT(5)
#define	 RKI2C_IEN_STARTIEN	__BIT(4)
#define	 RKI2C_IEN_MBRFIEN	__BIT(3)
#define	 RKI2C_IEN_MBTFIEN	__BIT(2)
#define	 RKI2C_IEN_BRFIEN	__BIT(1)
#define	 RKI2C_IEN_BTFIEN	__BIT(0)

#define	RKI2C_IPD		0x01c
#define	 RKI2C_IPD_NAKRCVIPD	__BIT(6)
#define	 RKI2C_IPD_STOPIPD	__BIT(5)
#define	 RKI2C_IPD_STARTIPD	__BIT(4)
#define	 RKI2C_IPD_MBRFIPD	__BIT(3)
#define	 RKI2C_IPD_MBTFIPD	__BIT(2)
#define	 RKI2C_IPD_BRFIPD	__BIT(1)
#define	 RKI2C_IPD_BTFIPD	__BIT(0)

#define	RKI2C_FCNT		0x020
#define	 RKI2C_FCNT_FCNT	__BITS(5,0)

#define	RKI2C_TXDATA(n)		(0x100 + (n) * 4)
#define	RKI2C_RXDATA(n)		(0x200 + (n) * 4)

/* Compat data flags */
#define	RKI2C_HAS_PCLK		__BIT(0)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3288-i2c",	.value = 0 },
	{ .compat = "rockchip,rk3399-i2c",	.value = RKI2C_HAS_PCLK },
	DEVICE_COMPAT_EOL
};

struct rk_i2c_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	struct clk		*sc_sclk;
	struct clk		*sc_pclk;

	u_int			sc_clkfreq;

	struct i2c_controller	sc_ic;
};

#define	RD4(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static void
rk_i2c_init(struct rk_i2c_softc *sc)
{
	int div, divl, divh;
	u_int rate;

	/*
	 * SCL frequency is calculated by the following formula:
	 *
	 * SCL Divisor = 8 * (CLKDIVL + 1 + CLKDIVH + 1)
	 * SCL = PCLK / SCLK Divisor
	 */

	rate = clk_get_rate(sc->sc_sclk);
	div = howmany(rate, sc->sc_clkfreq * 8) - 2;
	if (div >= 0) {
		divl = div / 2;
		if (div % 2 == 0)
			divh = divl;
		else
			divh = howmany(div, 2);
	} else {
		divl = divh = 0;
	}

	WR4(sc, RKI2C_CLKDIV,
	    __SHIFTIN(divh, RKI2C_CLKDIV_CLKDIVH) |
	    __SHIFTIN(divl, RKI2C_CLKDIV_CLKDIVL));

	/*
	 * Disable the module until we are ready to use it.
	 */
	WR4(sc, RKI2C_CON, 0);
	WR4(sc, RKI2C_IEN, 0);
	WR4(sc, RKI2C_IPD, RD4(sc, RKI2C_IPD));
}

static int
rk_i2c_wait(struct rk_i2c_softc *sc, uint32_t mask)
{
	u_int timeo = 100000;
	uint32_t val;

	const uint32_t ipdmask = mask | RKI2C_IPD_NAKRCVIPD;
	do {
		val = RD4(sc, RKI2C_IPD);
		if (val & ipdmask)
			break;
		delay(1);
	} while (--timeo > 0);

	WR4(sc, RKI2C_IPD, val & ipdmask);

	if ((val & RKI2C_IPD_NAKRCVIPD) != 0)
		return EIO;
	if ((val & mask) != 0)
		return 0;

	return ETIMEDOUT;
}

static int
rk_i2c_start(struct rk_i2c_softc *sc)
{
	uint32_t con;
	int error;

	/* Send start */
	con = RD4(sc, RKI2C_CON);
	con |= RKI2C_CON_START;
	WR4(sc, RKI2C_CON, con);

	if ((error = rk_i2c_wait(sc, RKI2C_IPD_STARTIPD)) != 0)
		return error;

	con &= ~RKI2C_CON_START;
	WR4(sc, RKI2C_CON, con);

	return 0;
}

static int
rk_i2c_stop(struct rk_i2c_softc *sc)
{
	uint32_t con;
	int error;

	/* Send start */
	con = RD4(sc, RKI2C_CON);
	con |= RKI2C_CON_STOP;
	WR4(sc, RKI2C_CON, con);

	if ((error = rk_i2c_wait(sc, RKI2C_IPD_STOPIPD)) != 0)
		return error;

	con &= ~RKI2C_CON_STOP;
	WR4(sc, RKI2C_CON, con);

	return 0;
}

static int
rk_i2c_write(struct rk_i2c_softc *sc, i2c_addr_t addr, const uint8_t *cmd,
    size_t cmdlen, const uint8_t *buf, size_t buflen, int flags, bool send_start)
{
	union {
		uint8_t data8[32];
		uint32_t data32[8];
	} txdata;
	uint32_t con;
	u_int mode;
	int error;
	size_t len;

	len = cmdlen + buflen;
	if (len > 31)
		return EINVAL;

	mode = RKI2C_CON_I2C_MODE_TX;
	con = RKI2C_CON_I2C_EN | __SHIFTIN(mode, RKI2C_CON_I2C_MODE);
	WR4(sc, RKI2C_CON, con);

	if (send_start && (error = rk_i2c_start(sc)) != 0)
		return error;

	/* Transmit data. Slave address goes in the lower 8 bits of TXDATA0 */
	txdata.data8[0] = addr << 1;
	memcpy(&txdata.data8[1], cmd, cmdlen);
	memcpy(&txdata.data8[1 + cmdlen], buf, buflen);
#if _BYTE_ORDER == _BIG_ENDIAN
	for (int i = 0; i < howmany(len + 1, 4); i++)
		LE32TOH(txdata.data32[i]);
#endif
	bus_space_write_region_4(sc->sc_bst, sc->sc_bsh, RKI2C_TXDATA(0),
	    txdata.data32, howmany(len + 1, 4));
	WR4(sc, RKI2C_MTXCNT, __SHIFTIN(len + 1, RKI2C_MTXCNT_MTXCNT));

	if ((error = rk_i2c_wait(sc, RKI2C_IPD_MBTFIPD)) != 0)
		return error;

	return 0;
}

static int
rk_i2c_read(struct rk_i2c_softc *sc, i2c_addr_t addr,
    const uint8_t *cmd, size_t cmdlen, uint8_t *buf,
    size_t buflen, int flags, bool send_start, bool last_ack)
{
	uint32_t rxdata[8];
	uint32_t con, mrxaddr, mrxraddr;
	u_int mode;
	int error, n;

	if (buflen > 32)
		return EINVAL;
	if (cmdlen > 3)
		return EINVAL;

	mode = send_start ? RKI2C_CON_I2C_MODE_RTX : RKI2C_CON_I2C_MODE_RX;
	con = RKI2C_CON_I2C_EN | __SHIFTIN(mode, RKI2C_CON_I2C_MODE);
	WR4(sc, RKI2C_CON, con);

	if (send_start && (error = rk_i2c_start(sc)) != 0)
		return error;

	if (send_start) {
		mrxaddr = __SHIFTIN((addr << 1) | 1, RKI2C_MRXADDR_SADDR) |
		    RKI2C_MRXADDR_ADDLVLD;
		WR4(sc, RKI2C_MRXADDR, mrxaddr);
		for (n = 0, mrxraddr = 0; n < cmdlen; n++) {
			mrxraddr |= cmd[n] << (n * 8);
			mrxraddr |= (RKI2C_MRXRADDR_ADDLVLD << n);
		}
		WR4(sc, RKI2C_MRXRADDR, mrxraddr);
	}

	if (last_ack) {
		con |= RKI2C_CON_ACK;
	}
	WR4(sc, RKI2C_CON, con);

	/* Receive data. Slave address goes in the lower 8 bits of MRXADDR */
	WR4(sc, RKI2C_MRXCNT, __SHIFTIN(buflen, RKI2C_MRXCNT_MRXCNT));
	if ((error = rk_i2c_wait(sc, RKI2C_IPD_MBRFIPD)) != 0)
		return error;

#if 0
	bus_space_read_region_4(sc->sc_bst, sc->sc_bsh, RKI2C_RXDATA(0),
	    rxdata, howmany(buflen, 4));
#else
	for (n = 0; n < roundup(buflen, 4); n += 4)
		rxdata[n/4] = RD4(sc, RKI2C_RXDATA(n/4));
#endif

#if _BYTE_ORDER == _BIG_ENDIAN
	for (int i = 0; i < howmany(buflen, 4); i++)
		HTOLE32(rxdata[i]);
#endif

	memcpy(buf, rxdata, buflen);

	return 0;
}

static int
rk_i2c_exec(void *priv, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t buflen, int flags)
{
	struct rk_i2c_softc * const sc = priv;
	bool send_start = true;
	int error;

	if (I2C_OP_READ_P(op)) {
		uint8_t *databuf = buf;
		while (buflen > 0) {
			const size_t datalen = uimin(buflen, 32);
			const bool last_ack = datalen == buflen;
			error = rk_i2c_read(sc, addr, cmdbuf, cmdlen, databuf, datalen, flags, send_start, last_ack);
			if (error != 0)
				break;
			databuf += datalen;
			buflen -= datalen;
			send_start = false;
			cmdbuf = NULL;
			cmdlen = 0;
		}
	} else {
		error = rk_i2c_write(sc, addr, cmdbuf, cmdlen, buf, buflen, flags, send_start);
	}

	if (error != 0 || I2C_OP_STOP_P(op))
		rk_i2c_stop(sc);

	WR4(sc, RKI2C_CON, 0);
	WR4(sc, RKI2C_IPD, RD4(sc, RKI2C_IPD));

	return error;
}

static int
rk_i2c_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk_i2c_attach(device_t parent, device_t self, void *aux)
{
	struct rk_i2c_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	u_int flags;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	flags = of_compatible_lookup(phandle, compat_data)->value;

	sc->sc_sclk = fdtbus_clock_get(phandle, "i2c");
	if (sc->sc_sclk == NULL || clk_enable(sc->sc_sclk) != 0) {
		aprint_error(": couldn't enable sclk\n");
		return;
	}

	if (ISSET(flags, RKI2C_HAS_PCLK)) {
		sc->sc_pclk = fdtbus_clock_get(phandle, "pclk");
		if (sc->sc_pclk == NULL || clk_enable(sc->sc_pclk) != 0) {
			aprint_error(": couldn't enable pclk\n");
			return;
		}
	}

	if (of_getprop_uint32(phandle, "clock-frequency", &sc->sc_clkfreq))
		sc->sc_clkfreq = 100000;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Rockchip I2C (%u Hz)\n", sc->sc_clkfreq);

	fdtbus_clock_assign(phandle);

	rk_i2c_init(sc);

	iic_tag_init(&sc->sc_ic);
	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_exec = rk_i2c_exec;

	fdtbus_register_i2c_controller(&sc->sc_ic, phandle);

	fdtbus_attach_i2cbus(self, phandle, &sc->sc_ic, iicbus_print);
}

CFATTACH_DECL_NEW(rk_i2c, sizeof(struct rk_i2c_softc),
    rk_i2c_match, rk_i2c_attach, NULL, NULL);
