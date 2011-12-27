/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__KERNEL_RCSID(1, "$NetBSD: rmixl_i2c_pci.c,v 1.1.2.2 2011/12/27 19:57:18 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#include <mips/rmi/rmixl_i2creg.h>
#include <mips/rmi/rmixl_i2cvar.h>

#include "locators.h"

#ifdef DEBUG
int xli2c_debug = 0;
#define	DPRINTF(x, ...)	do { if (xli2c_debug) printf(x, ## __VA_ARGS__); } while (0)
#else
#define	DPRINTF(x)
#endif

static int xli2c_pci_match(device_t, cfdata_t, void *);
static void xli2c_pci_attach(device_t, device_t, void *);

static int  xli2c_acquire_bus(void *, int);
static void xli2c_release_bus(void *, int);
static int  xli2c_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
		void *, size_t, int);
#if 0
static int  xli2c_intr(void *);
#endif
static int  xli2c_wait_for_command(struct xli2c_softc *, uint8_t);

static inline uint8_t
xli2c_read_status(struct xli2c_softc *sc)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, RMIXLP_I2C_STATUS);
}

static inline uint8_t
xli2c_read_control(struct xli2c_softc *sc)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, RMIXLP_I2C_CONTROL);
}

static inline uint8_t
xli2c_read_data_byte(struct xli2c_softc *sc)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, RMIXLP_I2C_RECEIVE);
}

static inline void
xli2c_write_control(struct xli2c_softc *sc, uint32_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, RMIXLP_I2C_CONTROL, v);
}

static inline void
xli2c_write_command(struct xli2c_softc *sc, uint32_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, RMIXLP_I2C_COMMAND, v);
}

static inline void
xli2c_write_prescale(struct xli2c_softc *sc, uint16_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh,
	    RMIXLP_I2C_CLOCK_PRESCALE_LOW, (v >> 0) & 0xff);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh,
	    RMIXLP_I2C_CLOCK_PRESCALE_HIGH, (v >> 8) & 0xff);
}

static inline void
xli2c_write_data_byte(struct xli2c_softc *sc, uint8_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, RMIXLP_I2C_TRANSMIT, v);
}

CFATTACH_DECL_NEW(xli2c_pci, sizeof(struct xli2c_softc),
    xli2c_pci_match, xli2c_pci_attach, 0, 0);

static int
xli2c_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args * const pa = aux;

	if (pa->pa_id == PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_I2C))
		return 1;

        return 0;
}

static void
xli2c_pci_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct pci_attach_args * const pa = aux;
	struct xli2c_softc * const sc = device_private(self);
	struct i2cbus_attach_args iba;

	sc->sc_dev = self;
	sc->sc_bst = &rcp->rc_pci_ecfg_eb_memt;

	/*
	 * Why isn't this accessible via a BAR?
	 */
	if (bus_space_subregion(sc->sc_bst, rcp->rc_pci_ecfg_eb_memh,
		    pa->pa_tag | 0x100, 0, &sc->sc_bsh)) {
		aprint_error(": can't map registers\n");
		return;
	}

	aprint_normal(": XLP I2C Controller\n");

	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_i2c.ic_acquire_bus = xli2c_acquire_bus,
	sc->sc_i2c.ic_release_bus = xli2c_release_bus,
	sc->sc_i2c.ic_exec = xli2c_exec,
	sc->sc_i2c.ic_cookie = sc;

	xli2c_write_control(sc, 0);	/* reset before changing anything */
	xli2c_write_command(sc, RMIXLP_I2C_COMMAND_IACK);
	/* MMM MAGIC */
	xli2c_write_prescale(sc, rmixl_i2c_calc_prescale(133333333, 100000));

#if 0
	pci_intr_handle_t pcih;

	pci_intr_map(pa, &pcih);

	if (pci_intr_establish(pa->pa_pc, pcih, IPL_VM, xli2c_intr, sc) == NULL) {
		aprint_error_dev(self, "failed to establish interrupt\n");
	} else {
		const char * const intrstr = pci_intr_string(pa->pa_pc, pcih);
		aprint_normal_dev(self, "interrupting at %s\n", intrstr);
	}
#endif

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;
	config_found_ia(self, "i2cbus", &iba, iicbus_print);
}

static int
xli2c_acquire_bus(void *v, int flags)
{
	struct xli2c_softc * const sc = v;

	mutex_enter(&sc->sc_buslock);

	/* enable the I2C module */
	xli2c_write_control(sc, RMIXLP_I2C_CONTROL_EN);

	return 0;
}

static void
xli2c_release_bus(void *v, int flags)
{
	struct xli2c_softc * const sc = v;

	/* disable the I2C module */
	xli2c_write_control(sc, 0);

	mutex_exit(&sc->sc_buslock);
}

#if 0
static int
xli2c_intr(void *v)
{
	struct xli2c_softc * const sc = v;

	panic("%s(%p) called!", __func__, sc);

	return 0;
}
#endif

/* send a command and busy wait for the byte data transfer to complete */
static int
xli2c_wait_for_command(struct xli2c_softc *sc, uint8_t command)
{
	uint32_t status;
	int error = 0;
	u_int timo;

	/*
	 * Issue the command.
	 */
	xli2c_write_command(sc, command);

	for (timo = 1000; --timo; )  {
		status = xli2c_read_status(sc);
		if ((status & RMIXLP_I2C_STATUS_TIP) == 0)
			break;
		DELAY(10);
	}

	if (timo == 0) {
		DPRINTF("%s: timeout (status=%#x, command=%#x)\n",
		    __func__, status, command);
		error = ETIMEDOUT;
	}

	/*
	 * NO_RX_ACK is only valid when transmitting.
	 */
	if ((command & RMIXLP_I2C_COMMAND_WR)
	    && (status & RMIXLP_I2C_STATUS_NO_RX_ACK)) {
		DPRINTF("%s: missing rx ack (status=%#x, command=%#x): "
		    "spin=%u\n", __func__, status, command, 1000 - timo);
		error = EIO;
	}
	return error;
}

int
xli2c_exec(void *v, i2c_op_t op, i2c_addr_t addr,
	const void *cmdbuf, size_t cmdlen,
	void *databuf, size_t datalen,
	int flags)
{
	struct xli2c_softc * const sc = v;
	uint8_t status;
	uint8_t command;
	int error;

	status = xli2c_read_status(sc);
	command = 0;

#if 0
	DPRINTF("%s(%#x,%#x,%p,%zu,%p,%zu,%#x): status=%#x command=%#x\n",
	    __func__, op, addr, cmdbuf, cmdlen, databuf, datalen, flags,
	    status, command);
#endif

	if ((status & RMIXLP_I2C_STATUS_BUSY) != 0) {
		/* wait for bus becoming available */
		u_int timo = 100;
		do {
			DELAY(10);
		} while (--timo > 0
			 && ((status = xli2c_read_status(sc)) & RMIXLP_I2C_STATUS_BUSY) != 0);

		if (timo == 0) {
			DPRINTF("%s: bus is busy (%#x)\n", __func__, status);
			return ETIMEDOUT;
		}
	}

	/* reset interrupt and arbitration-lost flags (all others are RO) */
	xli2c_write_command(sc, RMIXLP_I2C_COMMAND_IACK);

	/*
	 * send target address and transfer direction
	 */
	uint8_t addr_byte = (addr << 1)
	    | (cmdlen == 0 && I2C_OP_READ_P(op) ? 1 : 0);
	xli2c_write_data_byte(sc, addr_byte);

	/*
	 * Generate start (or restart) condition
	 */
	command = RMIXLP_I2C_COMMAND_WR | RMIXLP_I2C_COMMAND_STA;
	if (cmdlen == 0 && datalen == 0 && I2C_OP_WRITE_P(op)) {
		/*
		 * This would allows a probe of i2c without sending data.
		 * You just get back a rxack if the device exists.
		 */
		command |= RMIXLP_I2C_COMMAND_STO;
	}
	error = xli2c_wait_for_command(sc, command);
	if (error) {
		DPRINTF("%s: error sending address: %d\n", __func__, error);
		if (error == EIO)
			error = ENXIO;
		goto out;
	}

	status = xli2c_read_status(sc);
	if (status & RMIXLP_I2C_STATUS_AL) {
		DPRINTF("%s: lost bus: status=%#x\n", __func__, status);

		/*
		 * Generate start (or restart) condition (e.g. try again).
		 */
		error = xli2c_wait_for_command(sc, command);
		if (error) {
			DPRINTF("%s: error re-sending address: %d\n",
			    __func__, error);
			if (error == EIO)
				error = ENXIO;
			goto out;
		}

		status = xli2c_read_status(sc);
		if (status & RMIXLP_I2C_STATUS_AL) {
			error = EBUSY;
			goto out;
		}
		DPRINTF("%s: reacquired bus (status=%#x ocmmand=%#x)\n",
		    __func__, status, command);
	}

	if (cmdlen > 0) {
		const uint8_t *cmdptr = cmdbuf;
		command = RMIXLP_I2C_COMMAND_WR;

		for (size_t i = 0; i < cmdlen; i++) {
			xli2c_write_data_byte(sc, *cmdptr++);

			/*
			 * If this is the last byte we are sending and we
			 * are supposed to send a STOP, add the stop bit now.
			 */
			if (i + 1 == cmdlen + datalen
			    && I2C_OP_WRITE_P(op) && I2C_OP_STOP_P(op))
				command |= RMIXLP_I2C_COMMAND_STO;

			error = xli2c_wait_for_command(sc, command);
			if (error) {
				DPRINTF("%s: error sending cmd byte %zu: %d\n",
				    __func__, i, error);
				goto out;
			}
		}

		if (I2C_OP_READ_P(op)) {
			/* send target address and read transfer direction */
			addr_byte |= 1;
			xli2c_write_data_byte(sc, addr_byte);

			/*
			 * Restart for the read direction.
			 */
			KASSERT(command == RMIXLP_I2C_COMMAND_WR);
			command |= RMIXLP_I2C_COMMAND_STA;
			error = xli2c_wait_for_command(sc, command);
			if (error) {
				if (error == EIO)
					error = ENXIO;
				goto out;
			}
		}
	}

	if (I2C_OP_READ_P(op)) {
		KASSERT(command & RMIXLP_I2C_COMMAND_WR);
		KASSERT(command & RMIXLP_I2C_COMMAND_STA);
		KASSERT((command & RMIXLP_I2C_COMMAND_ACK) == 0);
		uint8_t *dataptr = databuf;

		command = RMIXLP_I2C_COMMAND_RD;
		for (size_t i = 0; i < datalen; i++) {
			/*
			 * If a master receiver wants to terminate a data
			 * transfer, it must inform the slave transmitter by
			 * not acknowledging the last byte of data.
			 */
			if (i == datalen - 1) {
				command |= RMIXLP_I2C_COMMAND_ACK;
				if (I2C_OP_STOP_P(op))
					command |= RMIXLP_I2C_COMMAND_STO;
			}
			error = xli2c_wait_for_command(sc, command);
			if (error) {
				DPRINTF("%s: error reading byte %zu: %d\n",
				    __func__, i, error);
				goto out;
			}
			*dataptr++ = xli2c_read_data_byte(sc);
		}
		if (datalen == 0) {
			command |= RMIXLP_I2C_COMMAND_ACK;
			if (I2C_OP_STOP_P(op))
				command |= RMIXLP_I2C_COMMAND_STO;

			error = xli2c_wait_for_command(sc, command);
			if (error) {
				DPRINTF("%s: error reading dummy last byte:"
				    "%d\n", __func__, error);
				goto out;
			}
			(void) xli2c_read_data_byte(sc); /* dummy read */
		}
	} else if (datalen > 0) {
		const uint8_t *dataptr = databuf;
		command = RMIXLP_I2C_COMMAND_WR;
		for (size_t i = 0; i < datalen; i++) {
			xli2c_write_data_byte(sc, *dataptr++);
			if (i == datalen - 1)
			error = xli2c_wait_for_command(sc, command);
			if (error) {
				DPRINTF("%s: error sending data byte %zu:"
				    " %d\n", __func__, i, error);
				goto out;
			}
		}
	}

 out:
	/*
	 * If we encountered an error condition or caller wants a STOP,
	 * send a STOP.
	 */
	if (error
	    || (command & RMIXLP_I2C_COMMAND_ACK)
	    || ((command & RMIXLP_I2C_COMMAND_STA) && I2C_OP_STOP_P(op))) {
		command = RMIXLP_I2C_COMMAND_STO;
		xli2c_write_command(sc, command);
		DPRINTF("%s: stopping\n", __func__);
	}

	DPRINTF("%s: exit status=%#x command=%#x: %d\n", __func__,
	    xli2c_read_status(sc), command, error);

	return error;
}
