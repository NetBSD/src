/*	$NetBSD: nfsmb.c,v 1.3.6.2 2007/07/28 12:31:51 kiyohara Exp $	*/
/*
 * Copyright (c) 2007 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nfsmb.c,v 1.3.6.2 2007/07/28 12:31:51 kiyohara Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/proc.h>

#include <machine/bus.h>

#include <dev/i2c/i2cvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/nfsmbreg.h>


struct nfsmbc_attach_args {
	int nfsmb_num;
	bus_space_tag_t nfsmb_iot;
	int nfsmb_addr;
};

struct nfsmb_softc;
struct nfsmbc_softc {
	struct device sc_dev;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_tag;
	struct pci_attach_args *sc_pa;

	bus_space_tag_t sc_iot;
	struct device *sc_nfsmb[2];
};

struct nfsmb_softc {
	struct device sc_dev;
	int sc_num;
	struct device *sc_nfsmbc;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	struct i2c_controller sc_i2c;	/* i2c controller info */
	struct lock sc_lock;
};


static int nfsmbc_match(struct device *, struct cfdata *, void *);
static void nfsmbc_attach(struct device *, struct device *, void *);
static int nfsmbc_print(void *, const char *);

static int nfsmb_match(struct device *, struct cfdata *, void *);
static void nfsmb_attach(struct device *, struct device *, void *);
static int nfsmb_acquire_bus(void *, int);
static void nfsmb_release_bus(void *, int);
static int nfsmb_exec(
    void *, i2c_op_t, i2c_addr_t, const void *, size_t, void *, size_t, int);
static int nfsmb_check_done(struct nfsmb_softc *);
static int
    nfsmb_send_1(struct nfsmb_softc *, uint8_t, i2c_addr_t, i2c_op_t, int);
static int nfsmb_write_1(
    struct nfsmb_softc *, uint8_t, uint8_t, i2c_addr_t, i2c_op_t, int);
static int nfsmb_write_2(
    struct nfsmb_softc *, uint8_t, uint16_t, i2c_addr_t, i2c_op_t, int);
static int nfsmb_receive_1(struct nfsmb_softc *, i2c_addr_t, i2c_op_t, int);
static int
    nfsmb_read_1(struct nfsmb_softc *, uint8_t, i2c_addr_t, i2c_op_t, int);
static int
    nfsmb_read_2(struct nfsmb_softc *, uint8_t, i2c_addr_t, i2c_op_t, int);


CFATTACH_DECL(nfsmbc, sizeof(struct nfsmbc_softc),
    nfsmbc_match, nfsmbc_attach, NULL, NULL);

static int
nfsmbc_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_NVIDIA) {
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_NVIDIA_NFORCE2_SMBUS:
		case PCI_PRODUCT_NVIDIA_NFORCE2_400_SMBUS:
		case PCI_PRODUCT_NVIDIA_NFORCE3_SMBUS:
		case PCI_PRODUCT_NVIDIA_NFORCE3_250_SMBUS:
		case PCI_PRODUCT_NVIDIA_NFORCE4_SMBUS:
		case PCI_PRODUCT_NVIDIA_MCP04_SMBUS:
			return 1;
		}
	}

	return 0;
}

static void
nfsmbc_attach(struct device *parent, struct device *self, void *aux)
{
	struct nfsmbc_softc *sc = (struct nfsmbc_softc *) self;
	struct pci_attach_args *pa = aux;
	struct nfsmbc_attach_args nfsmbca;
	pcireg_t reg;
	char devinfo[256];

	aprint_naive("\n");
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;
	sc->sc_pa = pa;
	sc->sc_iot = pa->pa_iot;

	nfsmbca.nfsmb_iot = sc->sc_iot;

	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, NFORCE_SMB1);
	nfsmbca.nfsmb_num = 1;
	nfsmbca.nfsmb_addr = NFORCE_SMBBASE(reg);
	sc->sc_nfsmb[0] = config_found(&sc->sc_dev, &nfsmbca, nfsmbc_print);

	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, NFORCE_SMB2);
	nfsmbca.nfsmb_num = 2;
	nfsmbca.nfsmb_addr = NFORCE_SMBBASE(reg);
	sc->sc_nfsmb[1] = config_found(&sc->sc_dev, &nfsmbca, nfsmbc_print);
}

static int
nfsmbc_print(void *aux, const char *pnp)
{
	struct nfsmbc_attach_args *nfsmbcap = aux;

	if (pnp)
		aprint_normal("nfsmb SMBus %d at %s",
		    nfsmbcap->nfsmb_num, pnp);
	else
		aprint_normal(" SMBus %d", nfsmbcap->nfsmb_num);
	return UNCONF;
}


CFATTACH_DECL(nfsmb, sizeof(struct nfsmb_softc),
    nfsmb_match, nfsmb_attach, NULL, NULL);

static int
nfsmb_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct nfsmbc_attach_args *nfsmbcap = aux;

	if (nfsmbcap->nfsmb_num == 1 || nfsmbcap->nfsmb_num == 2)
		return 1;
	return 0;
}

static void
nfsmb_attach(struct device *parent, struct device *self, void *aux)
{
	struct nfsmb_softc *sc = (struct nfsmb_softc *) self;
	struct nfsmbc_attach_args *nfsmbcap = aux;
	struct i2cbus_attach_args iba;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_nfsmbc = parent;
	sc->sc_num = nfsmbcap->nfsmb_num;
	sc->sc_iot = nfsmbcap->nfsmb_iot;

	/* register with iic */
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = nfsmb_acquire_bus;
	sc->sc_i2c.ic_release_bus = nfsmb_release_bus;
	sc->sc_i2c.ic_send_start = NULL;
	sc->sc_i2c.ic_send_stop = NULL;
	sc->sc_i2c.ic_initiate_xfer = NULL;
	sc->sc_i2c.ic_read_byte = NULL;
	sc->sc_i2c.ic_write_byte = NULL;
	sc->sc_i2c.ic_exec = nfsmb_exec;

	lockinit(&sc->sc_lock, PZERO, "nfsmb", 0, 0);

	if (bus_space_map(sc->sc_iot, nfsmbcap->nfsmb_addr, NFORCE_SMBSIZE, 0,
	    &sc->sc_ioh) != 0) {
		aprint_error("%s: failed to map SMBus space\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	iba.iba_tag = &sc->sc_i2c;
	(void) config_found_ia(&sc->sc_dev, "i2cbus", &iba, iicbus_print);
}

static int
nfsmb_acquire_bus(void *cookie, int flags)
{
	struct nfsmb_softc *sc = cookie;
	int err;

	err = lockmgr(&sc->sc_lock, LK_EXCLUSIVE, NULL);

	return err;
}

static void
nfsmb_release_bus(void *cookie, int flags)
{
	struct nfsmb_softc *sc = cookie;

	lockmgr(&sc->sc_lock, LK_RELEASE, NULL);

	return;
}

static int
nfsmb_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *cmd,
	   size_t cmdlen, void *vbuf, size_t buflen, int flags)
{
	struct nfsmb_softc *sc  = (struct nfsmb_softc *)cookie;
	uint8_t *p = vbuf;
	int rv;

	if (I2C_OP_READ_P(op) && (cmdlen == 0) && (buflen == 1)) {
		rv = nfsmb_receive_1(sc, addr, op, flags);
		if (rv == -1)
			return -1;
		*p = (uint8_t)rv;
		return 0;
	}

	if ((I2C_OP_READ_P(op)) && (cmdlen == 1) && (buflen == 1)) {
		rv = nfsmb_read_1(sc, *(const uint8_t*)cmd, addr, op, flags);
		if (rv == -1)
			return -1;
		*p = (uint8_t)rv;
		return 0;
	}
	if ((I2C_OP_READ_P(op)) && (cmdlen == 1) && (buflen == 2)) {
		rv = nfsmb_read_2(sc, *(const uint8_t*)cmd, addr, op, flags);
		if (rv == -1)
			return -1;
		*p = (uint8_t)rv;
		return 0;
	}


	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 0) && (buflen == 1))
		return nfsmb_send_1(sc, *(uint8_t*)vbuf, addr, op, flags);

	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 1) && (buflen == 1))
		return nfsmb_write_1(sc, *(const uint8_t*)cmd, *(uint8_t*)vbuf,
		    addr, op, flags);

	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 1) && (buflen == 2))
		return nfsmb_write_2(sc,
		    *(const uint8_t*)cmd, *((uint16_t *)vbuf), addr, op, flags);

	return (-1);
}

static int
nfsmb_check_done(struct nfsmb_softc *sc)
{
	int us;
	uint8_t stat;

	us = 10 * 1000;	/* XXXX: wait maximum 10 msec */
	do {
		delay(10);
		us -= 10;
		if (us <= 0)
			return -1;
	} while (bus_space_read_1(sc->sc_iot, sc->sc_ioh,
	    NFORCE_SMB_PROTOCOL) != 0);

	stat = bus_space_read_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_STATUS);
	if ((stat & NFORCE_SMB_STATUS_DONE) &&
	    !(stat & NFORCE_SMB_STATUS_STATUS))
		return 0;
	return -1;
}

/* ARGSUSED */
static int
nfsmb_send_1(struct nfsmb_softc *sc, uint8_t val, i2c_addr_t addr, i2c_op_t op,
	     int flags)
{
	uint8_t data;

	/* store cmd */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_COMMAND, val);

	/* write smbus slave address to register */
	data = addr << 1;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_ADDRESS, data);

	/* write smbus protocol to register */
	data = I2C_OP_READ_P(op) | NFORCE_SMB_PROTOCOL_BYTE;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_PROTOCOL, data);

	return nfsmb_check_done(sc);
}

/* ARGSUSED */
static int
nfsmb_write_1(struct nfsmb_softc *sc, uint8_t cmd, uint8_t val, i2c_addr_t addr,
	      i2c_op_t op, int flags)
{
	uint8_t data;

	/* store cmd */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_COMMAND, cmd);

	/* store data */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_DATA, val);

	/* write smbus slave address to register */
	data = addr << 1;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_ADDRESS, data);

	/* write smbus protocol to register */
	data = I2C_OP_READ_P(op) | NFORCE_SMB_PROTOCOL_BYTE_DATA;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_PROTOCOL, data);

	return nfsmb_check_done(sc);
}

static int
nfsmb_write_2(struct nfsmb_softc *sc, uint8_t cmd, uint16_t val,
	      i2c_addr_t addr, i2c_op_t op, int flags)
{
	uint8_t data, low, high;

	/* store cmd */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_COMMAND, cmd);

	/* store data */
	low = val;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_DATA, low);
	high = val >> 8;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_DATA, high);

	/* write smbus slave address to register */
	data = addr << 1;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_ADDRESS, data);

	/* write smbus protocol to register */
	data = I2C_OP_READ_P(op) | NFORCE_SMB_PROTOCOL_WORD_DATA;
	if (flags & I2C_F_PEC)
		data |= NFORCE_SMB_PROTOCOL_PEC;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_PROTOCOL, data);

	return nfsmb_check_done(sc);
}

/* ARGSUSED */
static int
nfsmb_receive_1(struct nfsmb_softc *sc, i2c_addr_t addr, i2c_op_t op, int flags)
{
	uint8_t data;

	/* write smbus slave address to register */
	data = addr << 1;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_ADDRESS, data);

	/* write smbus protocol to register */
	data = I2C_OP_READ_P(op) | NFORCE_SMB_PROTOCOL_BYTE;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_PROTOCOL, data);

	/* check for errors */
	if (nfsmb_check_done(sc) < 0)
		return -1;

	/* read data */
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_DATA);
}

/* ARGSUSED */
static int
nfsmb_read_1(struct nfsmb_softc *sc, uint8_t cmd, i2c_addr_t addr, i2c_op_t op,
	     int flags)
{
	uint8_t data;

	/* store cmd */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_COMMAND, cmd);

	/* write smbus slave address to register */
	data = addr << 1;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_ADDRESS, data);

	/* write smbus protocol to register */
	data = I2C_OP_READ_P(op) | NFORCE_SMB_PROTOCOL_BYTE_DATA;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_PROTOCOL, data);

	/* check for errors */
	if (nfsmb_check_done(sc) < 0)
		return (-1);

	/* read data */
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_DATA);
}

static int
nfsmb_read_2(struct nfsmb_softc *sc, uint8_t cmd, i2c_addr_t addr, i2c_op_t op,
	     int flags)
{
	uint8_t data, low, high;

	/* store cmd */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_COMMAND, cmd);

	/* write smbus slave address to register */
	data = addr << 1;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_ADDRESS, data);

	/* write smbus protocol to register */
	data = I2C_OP_READ_P(op) | NFORCE_SMB_PROTOCOL_BYTE_DATA;
	if (flags & I2C_F_PEC)
		data |= NFORCE_SMB_PROTOCOL_PEC;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_PROTOCOL, data);

	/* check for errors */
	if (nfsmb_check_done(sc) < 0)
		return (-1);

	/* read data */
	low = bus_space_read_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_DATA);
	high = bus_space_read_1(sc->sc_iot, sc->sc_ioh, NFORCE_SMB_DATA);
	return low | high << 8;
}
