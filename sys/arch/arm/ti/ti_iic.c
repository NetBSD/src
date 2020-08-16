/* $NetBSD: ti_iic.c,v 1.7 2020/08/16 03:48:59 riastradh Exp $ */

/*
 * Copyright (c) 2013 Manuel Bouyer.  All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 2012 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ti_iic.c,v 1.7 2020/08/16 03:48:59 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <dev/i2c/i2cvar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/ti/ti_prcm.h>
#include <arm/ti/ti_iicreg.h>

#ifndef OMAP2_I2C_SLAVE_ADDR
#define OMAP2_I2C_SLAVE_ADDR	0x01
#endif

#define OMAP2_I2C_FIFOBYTES(fd)	(8 << (fd))

#ifdef I2CDEBUG
#define DPRINTF(args)	printf args
#else
#define DPRINTF(args)
#endif

enum ti_iic_type {
	TI_IIC_OMAP3,
	TI_IIC_OMAP4,
	TI_NTYPES
};

enum {
	I2C_SYSC,
	I2C_IRQSTATUS_RAW,
	I2C_IRQSTATUS,
	I2C_IRQENABLE,		/* OMAP3 */
	I2C_IRQENABLE_SET,	/* OMAP4 */
	I2C_IRQENABLE_CLR,	/* OMAP4 */
	I2C_SYSS,
	I2C_BUF,
	I2C_CNT,
	I2C_DATA,
	I2C_CON,
	I2C_OA,
	I2C_SA,
	I2C_PSC,
	I2C_SCLL,
	I2C_SCLH,
	I2C_BUFSTAT,
	TI_NREGS
};

static const u_int ti_iic_regmap[TI_NTYPES][TI_NREGS] = {
	[TI_IIC_OMAP3] = {
		[I2C_SYSC] = 0x20,
		[I2C_IRQSTATUS_RAW] = 0x08,
		[I2C_IRQSTATUS] = 0x08,
		[I2C_IRQENABLE] = 0x04,
		[I2C_SYSS] = 0x10,
		[I2C_BUF] = 0x14,
		[I2C_CNT] = 0x18,
		[I2C_DATA] = 0x1c,
		[I2C_CON] = 0x24,
		[I2C_OA] = 0x28,
		[I2C_SA] = 0x2c,
		[I2C_PSC] = 0x30,
		[I2C_SCLL] = 0x34,
		[I2C_SCLH] = 0x38,
		[I2C_BUFSTAT] = 0x40,
	},
	[TI_IIC_OMAP4] = {
		[I2C_SYSC] = 0x10,
		[I2C_IRQSTATUS_RAW] = 0x24,
		[I2C_IRQSTATUS] = 0x28,
		[I2C_IRQENABLE_SET] = 0x2c,
		[I2C_IRQENABLE_CLR] = 0x30,
		[I2C_SYSS] = 0x90,
		[I2C_BUF] = 0x94,
		[I2C_CNT] = 0x98,
		[I2C_DATA] = 0x9c,
		[I2C_CON] = 0xa4,
		[I2C_OA] = 0xa8,
		[I2C_SA] = 0xac,
		[I2C_PSC] = 0xb0,
		[I2C_SCLL] = 0xb4,
		[I2C_SCLH] = 0xb8,
		[I2C_BUFSTAT] = 0xc0,
	},
};

static const struct of_compat_data compat_data[] = {
	/* compatible		type */
	{ "ti,omap3-i2c",	TI_IIC_OMAP3 },
	{ "ti,omap4-i2c",	TI_IIC_OMAP4 },
	{ NULL }
};

/* operation in progress */
typedef enum {
	TI_I2CREAD,
	TI_I2CWRITE,
	TI_I2CDONE,
	TI_I2CERROR
} ti_i2cop_t;

struct ti_iic_softc {
	device_t		sc_dev;
	struct i2c_controller	sc_ic;
	kmutex_t		sc_lock;
	device_t		sc_i2cdev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	enum ti_iic_type	sc_type;

	void			*sc_ih;
	kmutex_t		sc_mtx;
	kcondvar_t		sc_cv;
	ti_i2cop_t		sc_op;
	int			sc_opflags;
	int			sc_buflen;
	int			sc_bufidx;
	char			*sc_buf;

	bool			sc_busy;

	int			sc_rxthres;
	int			sc_txthres;
};

#define I2C_READ_REG(sc, reg)		\
	bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, ti_iic_regmap[(sc)->sc_type][(reg)])
#define I2C_READ_DATA(sc)		\
	bus_space_read_1((sc)->sc_iot, (sc)->sc_ioh, ti_iic_regmap[(sc)->sc_type][I2C_DATA])
#define I2C_WRITE_REG(sc, reg, val)	\
	bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, ti_iic_regmap[(sc)->sc_type][(reg)], (val))
#define I2C_WRITE_DATA(sc, val)		\
	bus_space_write_1((sc)->sc_iot, (sc)->sc_ioh, ti_iic_regmap[(sc)->sc_type][I2C_DATA], (val))

static int	ti_iic_match(device_t, cfdata_t, void *);
static void	ti_iic_attach(device_t, device_t, void *);

static int	ti_iic_intr(void *);

static int	ti_iic_acquire_bus(void *, int);
static void	ti_iic_release_bus(void *, int);
static int	ti_iic_exec(void *, i2c_op_t, i2c_addr_t, const void *,
			       size_t, void *, size_t, int);

static int	ti_iic_reset(struct ti_iic_softc *);
static int	ti_iic_op(struct ti_iic_softc *, i2c_addr_t, ti_i2cop_t,
			       uint8_t *, size_t, int);
static void	ti_iic_handle_intr(struct ti_iic_softc *, uint32_t);
static void	ti_iic_do_read(struct ti_iic_softc *, uint32_t);
static void	ti_iic_do_write(struct ti_iic_softc *, uint32_t);

static int	ti_iic_wait(struct ti_iic_softc *, uint16_t, uint16_t, int);
static uint32_t	ti_iic_stat(struct ti_iic_softc *, uint32_t);
static int	ti_iic_flush(struct ti_iic_softc *);

static i2c_tag_t ti_iic_get_tag(device_t);

static const struct fdtbus_i2c_controller_func ti_iic_funcs = {
	.get_tag = ti_iic_get_tag,
};

CFATTACH_DECL_NEW(ti_iic, sizeof(struct ti_iic_softc),
    ti_iic_match, ti_iic_attach, NULL, NULL);

static int
ti_iic_match(device_t parent, cfdata_t match, void *opaque)
{
	struct fdt_attach_args * const faa = opaque;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
ti_iic_attach(device_t parent, device_t self, void *opaque)
{
	struct ti_iic_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = opaque;
	const int phandle = faa->faa_phandle;
	int fifodepth, fifo;
	const char *modname;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": couldn't decode interrupt\n");
		return;
	}

	if (ti_prcm_enable_hwmod(phandle, 0) != 0) {
		aprint_error(": couldn't enable module\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_NET);
	cv_init(&sc->sc_cv, "tiiic");
	iic_tag_init(&sc->sc_ic);
	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = ti_iic_acquire_bus;
	sc->sc_ic.ic_release_bus = ti_iic_release_bus;
	sc->sc_ic.ic_exec = ti_iic_exec;

	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_type = of_search_compatible(phandle, compat_data)->data;

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_NET, 0,
	    ti_iic_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error(": couldn't establish interrupt\n");
		return;
	}

	modname = fdtbus_get_string(phandle, "ti,hwmods");
	if (modname == NULL)
		modname = fdtbus_get_string(OF_parent(phandle), "ti,hwmods");

	fifodepth = I2C_BUFSTAT_FIFODEPTH(I2C_READ_REG(sc, I2C_BUFSTAT));
	fifo = OMAP2_I2C_FIFOBYTES(fifodepth);
	sc->sc_rxthres = sc->sc_txthres = fifo >> 1;

	aprint_naive("\n");
	if (modname != NULL)
		aprint_normal(": I2C controller (%s), %d-bytes FIFO\n", modname, fifo);
	else
		aprint_normal(": I2C controller (i2c@%" PRIxBUSADDR "), %d-bytes FIFO\n",
		    addr, fifo);

	ti_iic_reset(sc);
	ti_iic_flush(sc);

	fdtbus_register_i2c_controller(self, phandle, &ti_iic_funcs);

	fdtbus_attach_i2cbus(self, phandle, &sc->sc_ic, iicbus_print);
}

static int
ti_iic_intr(void *arg)
{
	struct ti_iic_softc *sc = arg;
	uint32_t stat;

	mutex_enter(&sc->sc_mtx);
	DPRINTF(("ti_iic_intr opflags=%#x\n", sc->sc_opflags));
	if ((sc->sc_opflags & I2C_F_POLL) == 0) {
		stat = I2C_READ_REG(sc, I2C_IRQSTATUS);
		DPRINTF(("ti_iic_intr pre handle sc->sc_op eq %#x\n", sc->sc_op));
		ti_iic_handle_intr(sc, stat);
		I2C_WRITE_REG(sc, I2C_IRQSTATUS, stat);
		if (sc->sc_op == TI_I2CERROR || sc->sc_op == TI_I2CDONE) {
			DPRINTF(("ti_iic_intr post handle sc->sc_op %#x\n", sc->sc_op));
			cv_broadcast(&sc->sc_cv);
		}
	}
	mutex_exit(&sc->sc_mtx);
	DPRINTF(("ti_iic_intr status 0x%x\n", stat));
	return 1;
}

static int
ti_iic_acquire_bus(void *opaque, int flags)
{
	struct ti_iic_softc *sc = opaque;

	mutex_enter(&sc->sc_lock);
	while (sc->sc_busy)
		cv_wait(&sc->sc_cv, &sc->sc_lock);
	sc->sc_busy = true;
	mutex_exit(&sc->sc_lock);

	return 0;
}

static void
ti_iic_release_bus(void *opaque, int flags)
{
	struct ti_iic_softc *sc = opaque;

	mutex_enter(&sc->sc_lock);
	sc->sc_busy = false;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);
}

static int
ti_iic_exec(void *opaque, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct ti_iic_softc *sc = opaque;
	int err;

	DPRINTF(("ti_iic_exec: op 0x%x cmdlen %zd len %zd flags 0x%x\n",
	    op, cmdlen, len, flags));

	if (cmdlen > 0) {
		err = ti_iic_op(sc, addr, TI_I2CWRITE,
		    __UNCONST(cmdbuf), cmdlen,
		    (I2C_OP_READ_P(op) ? 0 : I2C_F_STOP) | flags);
		if (err)
			goto done;
	}

	if (I2C_OP_STOP_P(op))
		flags |= I2C_F_STOP;

	/*
	 * I2C controller doesn't allow for zero-byte transfers.
	 */
	if (len == 0) {
		err = EINVAL;
		goto done;
	}

	if (I2C_OP_READ_P(op)) {
		err = ti_iic_op(sc, addr, TI_I2CREAD, buf, len, flags);
	} else {
		err = ti_iic_op(sc, addr, TI_I2CWRITE, buf, len, flags);
	}

done:
	if (err)
		ti_iic_reset(sc);

	ti_iic_flush(sc);

	DPRINTF(("ti_iic_exec: done %d\n", err));
	return err;
}

static int
ti_iic_reset(struct ti_iic_softc *sc)
{
	uint32_t psc, scll, sclh;
	int i;

	DPRINTF(("ti_iic_reset\n"));

	/* Disable */
	I2C_WRITE_REG(sc, I2C_CON, 0);
	/* Soft reset */
	I2C_WRITE_REG(sc, I2C_SYSC, I2C_SYSC_SRST);
	delay(1000);
	/* enable so that we can check for reset complete */
	I2C_WRITE_REG(sc, I2C_CON, I2C_CON_EN);
	delay(1000);
	for (i = 0; i < 1000; i++) { /* 1s delay for reset */
		if (I2C_READ_REG(sc, I2C_SYSS) & I2C_SYSS_RDONE)
			break;
	}
	/* Disable again */
	I2C_WRITE_REG(sc, I2C_CON, 0);
	delay(50000);

	if (i >= 1000) {
		aprint_error_dev(sc->sc_dev, ": couldn't reset module\n");
		return 1;
	}


	/* XXX standard speed only */
	if (sc->sc_type == TI_IIC_OMAP3) {
		psc = (96000000 / 19200000) - 1;
		scll = sclh = (19200000 / (2 * 100000)) - 6;
	} else {
		psc = 3;
		scll = 53;
		sclh = 55;
	}

	/* Clocks */
	I2C_WRITE_REG(sc, I2C_PSC, psc);
	I2C_WRITE_REG(sc, I2C_SCLL, scll);
	I2C_WRITE_REG(sc, I2C_SCLH, sclh);

	/* Own I2C address */
	I2C_WRITE_REG(sc, I2C_OA, OMAP2_I2C_SLAVE_ADDR);

	/* 5 bytes fifo */
	I2C_WRITE_REG(sc, I2C_BUF,
	    I2C_BUF_RXTRSH(sc->sc_rxthres) | I2C_BUF_TXTRSH(sc->sc_txthres));

	/* Enable */
	I2C_WRITE_REG(sc, I2C_CON, I2C_CON_EN);

	return 0;
}

static int
ti_iic_op(struct ti_iic_softc *sc, i2c_addr_t addr, ti_i2cop_t op,
    uint8_t *buf, size_t buflen, int flags)
{
	uint16_t con, stat, mask;
	int err, retry;

	KASSERT(op == TI_I2CREAD || op == TI_I2CWRITE);
	DPRINTF(("ti_iic_op: addr %#x op %#x buf %p buflen %#x flags %#x\n",
	    addr, op, buf, (unsigned int) buflen, flags));

	mask = I2C_IRQSTATUS_ARDY | I2C_IRQSTATUS_NACK | I2C_IRQSTATUS_AL;
	if (op == TI_I2CREAD) {
		mask |= I2C_IRQSTATUS_RDR | I2C_IRQSTATUS_RRDY;
	} else {
		mask |= I2C_IRQSTATUS_XDR | I2C_IRQSTATUS_XRDY;
	}

	err = ti_iic_wait(sc, I2C_IRQSTATUS_BB, 0, flags);
	if (err) {
		DPRINTF(("ti_iic_op: wait error %d\n", err));
		return err;
	}

	con = I2C_CON_EN;
	con |= I2C_CON_MST;
	con |= I2C_CON_STT;
	if (flags & I2C_F_STOP)
		con |= I2C_CON_STP;
	if (addr & ~0x7f)
		con |= I2C_CON_XSA;
	if (op == TI_I2CWRITE)
		con |= I2C_CON_TRX;

	mutex_enter(&sc->sc_mtx);
	sc->sc_op = op;
	sc->sc_opflags = flags;
	sc->sc_buf = buf;
	sc->sc_buflen = buflen;
	sc->sc_bufidx = 0;

	I2C_WRITE_REG(sc, I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_STP);
	DPRINTF(("ti_iic_op: op %d con 0x%x ", op, con));
	I2C_WRITE_REG(sc, I2C_CNT, buflen);
	I2C_WRITE_REG(sc, I2C_SA, (addr & I2C_SA_MASK));
	DPRINTF(("SA 0x%x len %d\n", I2C_READ_REG(sc, I2C_SA), I2C_READ_REG(sc, I2C_CNT)));

	if ((flags & I2C_F_POLL) == 0 || sc->sc_type == TI_IIC_OMAP3) {
		/* clear any pending interrupt */
		I2C_WRITE_REG(sc, I2C_IRQSTATUS,
		    I2C_READ_REG(sc, I2C_IRQSTATUS));
		/* and enable */
		if (sc->sc_type == TI_IIC_OMAP4) {
			I2C_WRITE_REG(sc, I2C_IRQENABLE_SET, mask);
		} else {
			I2C_WRITE_REG(sc, I2C_IRQENABLE, mask);
		}
	}
	/* start transfer */
	I2C_WRITE_REG(sc, I2C_CON, con);

	if ((flags & I2C_F_POLL) == 0) {
		/* and wait for completion */
		DPRINTF(("ti_iic_op waiting, op %#x\n", sc->sc_op));
		while (sc->sc_op == op) {
			if (cv_timedwait(&sc->sc_cv, &sc->sc_mtx,
			    mstohz(5000)) == EWOULDBLOCK) {
				/* timeout */
				op = TI_I2CERROR;
			}
		}
		DPRINTF(("ti_iic_op waiting done, op %#x\n", sc->sc_op));

		/* disable interrupts */
		if (sc->sc_type == TI_IIC_OMAP4) {
			I2C_WRITE_REG(sc, I2C_IRQENABLE_CLR, 0xffff);
		} else {
			I2C_WRITE_REG(sc, I2C_IRQENABLE, 0);
		}
	} else {
		/* poll for completion */
		DPRINTF(("ti_iic_op polling, op %x\n", sc->sc_op));
		while (sc->sc_op == op) {
			stat = ti_iic_stat(sc, mask);
			DPRINTF(("ti_iic_op stat 0x%x\n", stat));
			if (stat == 0) {
				/* timeout */
				sc->sc_op = TI_I2CERROR;
			} else {
				ti_iic_handle_intr(sc, stat);
			}
			I2C_WRITE_REG(sc, I2C_IRQSTATUS, stat);
		}
		DPRINTF(("ti_iic_op polling done, op now %x\n", sc->sc_op));
	}
	mutex_exit(&sc->sc_mtx);
	retry = 10000;
	I2C_WRITE_REG(sc, I2C_CON, 0);
	while (I2C_READ_REG(sc, I2C_CON) & I2C_CON_MST) {
		delay(100);
		if (--retry == 0)
			break;
	}
	return (sc->sc_op == TI_I2CDONE) ? 0 : EIO;
}

static void
ti_iic_handle_intr(struct ti_iic_softc *sc, uint32_t stat)
{
	KASSERT(mutex_owned(&sc->sc_mtx));
	KASSERT(stat != 0);
	DPRINTF(("ti_iic_handle_intr stat %#x\n", stat));

	if (stat &
	    (I2C_IRQSTATUS_NACK|I2C_IRQSTATUS_AL)) {
		sc->sc_op = TI_I2CERROR;
		return;
	}
	if (stat & I2C_IRQSTATUS_ARDY) {
		sc->sc_op = TI_I2CDONE;
		return;
	}
	if (sc->sc_op == TI_I2CREAD)
		ti_iic_do_read(sc, stat);
	else if (sc->sc_op == TI_I2CWRITE)
		ti_iic_do_write(sc, stat);
	else
		return;
}
void
ti_iic_do_read(struct ti_iic_softc *sc, uint32_t stat)
{
	int len = 0;

	KASSERT(mutex_owned(&sc->sc_mtx));
	DPRINTF(("ti_iic_do_read stat %#x\n", stat));
	if (stat & I2C_IRQSTATUS_RDR) {
		len = I2C_READ_REG(sc, I2C_BUFSTAT);
		len = I2C_BUFSTAT_RXSTAT(len);
		DPRINTF(("ti_iic_do_read receive drain len %d left %d\n",
		    len, I2C_READ_REG(sc, I2C_CNT)));
	} else if (stat & I2C_IRQSTATUS_RRDY) {
		len = sc->sc_rxthres + 1;
		DPRINTF(("ti_iic_do_read receive len %d left %d\n",
		    len, I2C_READ_REG(sc, I2C_CNT)));
	}
	for (;
	    sc->sc_bufidx < sc->sc_buflen && len > 0;
	    sc->sc_bufidx++, len--) {
		sc->sc_buf[sc->sc_bufidx] = I2C_READ_DATA(sc);
		DPRINTF(("ti_iic_do_read got b[%d]=0x%x\n", sc->sc_bufidx,
		    sc->sc_buf[sc->sc_bufidx]));
	}
	DPRINTF(("ti_iic_do_read done\n"));
}

void
ti_iic_do_write(struct ti_iic_softc *sc, uint32_t stat)
{
	int len = 0;

	DPRINTF(("ti_iic_do_write stat %#x\n", stat));
	KASSERT(mutex_owned(&sc->sc_mtx));
	if (stat & I2C_IRQSTATUS_XDR) {
		len = I2C_READ_REG(sc, I2C_BUFSTAT);
		len = I2C_BUFSTAT_TXSTAT(len);
		DPRINTF(("ti_iic_do_write xmit drain len %d left %d\n",
		    len, I2C_READ_REG(sc, I2C_CNT)));
	} else if (stat & I2C_IRQSTATUS_XRDY) {
		len = sc->sc_txthres + 1;
		DPRINTF(("ti_iic_do_write xmit len %d left %d\n",
		    len, I2C_READ_REG(sc, I2C_CNT)));
	}
	for (;
	    sc->sc_bufidx < sc->sc_buflen && len > 0;
	    sc->sc_bufidx++, len--) {
		DPRINTF(("ti_iic_do_write send b[%d]=0x%x\n",
		    sc->sc_bufidx, sc->sc_buf[sc->sc_bufidx]));
		I2C_WRITE_DATA(sc, sc->sc_buf[sc->sc_bufidx]);
	}
	DPRINTF(("ti_iic_do_write done\n"));
}

static int
ti_iic_wait(struct ti_iic_softc *sc, uint16_t mask, uint16_t val, int flags)
{
	int retry = 10;
	uint16_t v;
	DPRINTF(("ti_iic_wait mask %#x val %#x flags %#x\n", mask, val, flags));

	while (((v = I2C_READ_REG(sc, I2C_IRQSTATUS_RAW)) & mask) != val) {
		--retry;
		if (retry == 0) {
			aprint_error_dev(sc->sc_dev, ": wait timeout, "
			    "mask = %#x val = %#x stat = %#x\n",
			    mask, val, v);
			return EBUSY;
		}
		if (flags & I2C_F_POLL) {
			delay(50000);
		} else {
			kpause("tiiic", false, mstohz(50), NULL);
		}
	}
	DPRINTF(("ti_iic_wait done retry %#x\n", retry));

	return 0;
}

static uint32_t
ti_iic_stat(struct ti_iic_softc *sc, uint32_t mask)
{
	uint32_t v;
	int retry = 500;
	DPRINTF(("ti_iic_wait mask %#x\n", mask));
	while (--retry > 0) {
		v = I2C_READ_REG(sc, I2C_IRQSTATUS_RAW) & mask;
		if (v != 0)
			break;
		delay(100);
	}
	DPRINTF(("ti_iic_wait done retry %#x\n", retry));
	return v;
}

static int
ti_iic_flush(struct ti_iic_softc *sc)
{
	DPRINTF(("ti_iic_flush\n"));
#if 0
	int retry = 1000;
	uint16_t v;

	while ((v = I2C_READ_REG(sc, I2C_IRQSTATUS_RAW)) & I2C_IRQSTATUS_RRDY) {
		if (--retry == 0) {
			aprint_error_dev(sc->sc_dev,
			    ": flush timeout, stat = %#x\n", v);
			return EBUSY;
		}
		(void)I2C_READ_DATA(sc);
		delay(1000);
	}
#endif

	I2C_WRITE_REG(sc, I2C_CNT, 0);
	return 0;
}

static i2c_tag_t
ti_iic_get_tag(device_t dev)
{
	struct ti_iic_softc * const sc = device_private(dev);

	return &sc->sc_ic;
}
