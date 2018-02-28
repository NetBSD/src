/* $NetBSD: imcsmb.c,v 1.19 2018/02/28 09:25:02 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Goyette
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

/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Authors: Joe Kloss; Ravi Pokala (rpokala@freebsd.org)
 *
 * Copyright (c) 2017-2018 Panasas
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* 
 * Driver for the SMBus controllers in Intel's Integrated Memory Controllers
 * in certain CPUs.  A more detailed description of this device is present
 * in imc.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imcsmb.c,v 1.19 2018/02/28 09:25:02 pgoyette Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/endian.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/bus.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/i2c/i2cvar.h>

#include "imcsmb_reg.h"
#include "imcsmb_var.h"

/* Device methods */
static int  imcsmb_probe(device_t, cfdata_t, void *);
static void imcsmb_attach(device_t, device_t, void *);
static int  imcsmb_detach(device_t, int flags);
static int  imcsmb_rescan(device_t, const char *, const int *);
static void imcsmb_chdet(device_t, device_t);

CFATTACH_DECL3_NEW(imcsmb, sizeof(struct imcsmb_softc),
    imcsmb_probe, imcsmb_attach, imcsmb_detach, NULL, imcsmb_rescan,
    imcsmb_chdet, 0);

/* Bus access control methods */
static int imcsmb_acquire_bus(void *cookie, int flags);
static void imcsmb_release_bus(void *cookie, int flags);

/* SMBus methods */
static int imcsmb_exec(void *cookie, i2c_op_t, i2c_addr_t, const void *,
    size_t, void *, size_t, int);

/**
 * device_attach() method. Set up the softc, including getting the set of the
 * parent imcsmb_pci's registers that we will use. Create the smbus(4) device,
 * which any SMBus slave device drivers will connect to. Probe and attach
 * anything which might be downstream.
 *
 * @author rpokala
 *
 * @param[in,out] dev
 *      Device being attached.
 */

static void
imcsmb_attach(device_t parent, device_t self, void *aux)
{
	struct imcsmb_softc *sc = device_private(self);
	struct imc_attach_args *imca = aux;

	aprint_naive("\n");
	aprint_normal("SMBus controller\n");

	/* Initialize private state */
	sc->sc_dev = self;
	sc->sc_regs = imca->ia_regs;
	sc->sc_pci_tag = imca->ia_pci_tag;
	sc->sc_pci_chipset_tag = imca->ia_pci_chipset_tag;
	mutex_init(&sc->sc_i2c_mutex, MUTEX_DEFAULT, IPL_NONE);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	imcsmb_rescan(self, "i2cbus", 0);
}

static int
imcsmb_rescan(device_t self, const char *ifattr, const int *flags)
{
	struct imcsmb_softc *sc = device_private(self);
	struct i2cbus_attach_args iba;

	if (!ifattr_match(ifattr, "i2cbus"))
		return 0;

	/* Create the i2cbus child */
	if (sc->sc_smbus != NULL)
		return 0;

	sc->sc_i2c_tag.ic_cookie = sc;
	sc->sc_i2c_tag.ic_acquire_bus = imcsmb_acquire_bus;
	sc->sc_i2c_tag.ic_release_bus = imcsmb_release_bus;
	sc->sc_i2c_tag.ic_exec = imcsmb_exec;

	memset(&iba, 0, sizeof(iba));
	iba.iba_type = I2C_TYPE_SMBUS;
	iba.iba_tag = &sc->sc_i2c_tag;
	sc->sc_smbus = config_found_ia(self, ifattr, &iba, iicbus_print);

	if (sc->sc_smbus == NULL) {
		aprint_normal_dev(self, "no child found\n");
		return ENXIO;
	}

	return 0;
}

static void
imcsmb_chdet(device_t self, device_t child)
{
	struct imcsmb_softc *sc = device_private(self);

	if (child == sc->sc_smbus)
		sc->sc_smbus = NULL;
	else KASSERT(child == NULL);
}

/**
 * device_detach() method. attach() didn't do any allocations, so there's
 * nothing special needed
 */
static int
imcsmb_detach(device_t self, int flags)
{
	int error;
	struct imcsmb_softc *sc = device_private(self);

	if (sc->sc_smbus != NULL) {
		error = config_detach(sc->sc_smbus, flags);
		if (error)
			return error;
	}

	pmf_device_deregister(self);
	mutex_destroy(&sc->sc_i2c_mutex);
	return 0;
}

/**
 * device_probe() method. All the actual probing was done by the imc
 * parent, so just report success.
 *
 * @author Joe Kloss
 *
 * @param[in,out] dev
 *      Device being probed.
 */
static int
imcsmb_probe(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

static int
imcsmb_acquire_bus(void *cookie, int flags)
{
	struct imcsmb_softc *sc = cookie;

	if (cold)
		return 0;

	mutex_enter(&sc->sc_i2c_mutex);

	imc_callback(sc, IMC_BIOS_DISABLE);

	return 0;
}

static void
imcsmb_release_bus(void *cookie, int flags)
{
	struct imcsmb_softc *sc = cookie;

	if (cold)
		return;

	imc_callback(sc, IMC_BIOS_ENABLE);

	mutex_exit(&sc->sc_i2c_mutex);
}

static int
imcsmb_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *cmdbuf,
    size_t cmdlen, void *buf, size_t len, int flags)
{
	struct imcsmb_softc *sc = cookie;
	int i;
	int rc = 0;
	uint32_t cmd_val;
	uint32_t cntl_val;
	uint32_t orig_cntl_val;
	uint32_t stat_val;
	uint16_t *word;
	uint16_t lword;
	uint8_t *byte;
	uint8_t lbyte;
	uint8_t cmd;

	if ((cmdlen != 1) || (len > 2) || (len < 1))
		return EINVAL;

	byte = (uint8_t *) buf;
	word = (uint16_t *) buf;
	lbyte = *byte;
	lword = *word;

	/* We modify the value of the control register; save the original, so
	 * we can restore it later
	 */
	orig_cntl_val = pci_conf_read(sc->sc_pci_chipset_tag, sc->sc_pci_tag,
	    sc->sc_regs->smb_cntl);

	cntl_val = orig_cntl_val;

	/*
	 * Set up the SMBCNTL register
	 */

	/* [31:28] Clear the existing value of the DTI bits, then set them to
	 * the four high bits of the slave address.
	 */
	cntl_val &= ~IMCSMB_CNTL_DTI_MASK;
	cntl_val |= ((uint32_t) addr & 0x78) << 25;

	/* [27:27] Set the CLK_OVERRIDE bit, to enable normal operation */
	cntl_val |= IMCSMB_CNTL_CLK_OVERRIDE;

	/* [26:26] Clear the WRITE_DISABLE bit; the datasheet says this isn't
	 * necessary, but empirically, it is.
	 */
	cntl_val &= ~IMCSMB_CNTL_WRITE_DISABLE_BIT;

	/* [9:9] Clear the POLL_EN bit, to stop the hardware TSOD polling. */
	cntl_val &= ~IMCSMB_CNTL_POLL_EN;

	/*
	 * Set up the SMBCMD register
	 */

	/* [31:31] Set the TRIGGER bit; when this gets written, the controller
	 * will issue the command.
	 */
	cmd_val = IMCSMB_CMD_TRIGGER_BIT;

	/* [29:29] For word operations, set the WORD_ACCESS bit. */
	if (len == 2) {
		cmd_val |= IMCSMB_CMD_WORD_ACCESS;
	}

	/* [27:27] For write operations, set the WRITE bit. */
	if (I2C_OP_WRITE_P(op)) {
		cmd_val |= IMCSMB_CMD_WRITE_BIT;
	}

	/* [26:24] The three non-DTI, non-R/W bits of the slave address. */
	cmd_val |= (uint32_t) ((addr & 0x7) << 24);

	/* [23:16] The command (offset in the case of an EEPROM, or register in
	 * the case of TSOD or NVDIMM controller).
	 */
	cmd = *((const uint8_t *) cmdbuf);
	cmd_val |= (uint32_t) (cmd << 16);

	/* [15:0] The data to be written for a write operation. */
	if (I2C_OP_WRITE_P(op)) {
		if (len == 2) {
			/* The datasheet says the controller uses different
			 * endianness for word operations on I2C vs SMBus!
			 *      I2C: [15:8] = MSB; [7:0] = LSB
			 *      SMB: [15:8] = LSB; [7:0] = MSB
			 * As a practical matter, this controller is very
			 * specifically for use with DIMMs, the SPD (and
			 * NVDIMM controllers) are only accessed as bytes,
			 * the temperature sensor is only accessed as words, and
			 * the temperature sensors are I2C. Thus, byte-swap the
			 * word.
			 */
			lword = htobe16(*(uint16_t *)buf);
		} else {
			/* For byte operations, the data goes in the LSB, and
			 * the MSB is a don't care.
			 */
			lword = *(uint8_t *)buf;
		}
		cmd_val |= lword;
	}

	/* Write the updated value to the control register first, to disable
	 * the hardware TSOD polling.
	 */
	pci_conf_write(sc->sc_pci_chipset_tag, sc->sc_pci_tag,
	    sc->sc_regs->smb_cntl, cntl_val);

	/* Poll on the BUSY bit in the status register until clear, or timeout.
	 * We just cleared the auto-poll bit, so we need to make sure the device
	 * is idle before issuing a command. We can safely timeout after 35 ms,
	 * as this is the maximum time the SMBus spec allows for a transaction.
	 */
	for (i = 4; i != 0; i--) {
		stat_val = pci_conf_read(sc->sc_pci_chipset_tag,
		    sc->sc_pci_tag, sc->sc_regs->smb_stat);
		if (! (stat_val & IMCSMB_STATUS_BUSY_BIT)) {
			break;
		}
		delay(100);	/* wait 10ms */
	}

	if (i == 0) {
		aprint_debug_dev(sc->sc_dev,
		    "transfer: timeout waiting for device to settle\n");
	}

	/* Now that polling has stopped, we can write the command register. This
	 * starts the SMBus command.
	 */
	pci_conf_write(sc->sc_pci_chipset_tag, sc->sc_pci_tag,
	    sc->sc_regs->smb_cmd, cmd_val);

	/* Wait for WRITE_DATA_DONE/READ_DATA_VALID to be set, or timeout and
	 * fail. We wait up to 35ms.
	 */
	for (i = 35000; i != 0; i -= 10)
	{
		delay(10);
		stat_val = pci_conf_read(sc->sc_pci_chipset_tag,
		    sc->sc_pci_tag, sc->sc_regs->smb_stat);
		/*
		 * For a write, the bits holding the data contain the data
		 * being written. You would think that would cause the
		 * READ_DATA_VALID bit to be cleared, because the data bits
		 * no longer contain valid data from the most recent read
		 * operation. While that would be logical, that's not the
		 * case here: READ_DATA_VALID is only cleared when starting
		 * a read operation, and WRITE_DATA_DONE is only cleared
		 * when starting a write operation.
		 */
		if (I2C_OP_WRITE_P(op)) {
			if (stat_val & IMCSMB_STATUS_WRITE_DATA_DONE) {
				break;
			}
		} else {
			if (stat_val & IMCSMB_STATUS_READ_DATA_VALID) {
				break;
			}
		}
	}
	if (i == 0) {
		rc = ETIMEDOUT;
		aprint_debug_dev(sc->sc_dev, "transfer timeout\n");
		goto out;
	}

	/* It is generally the case that this bit indicates non-ACK, but it
	 * could also indicate other bus errors. There's no way to tell the
	 * difference.
	 */
	if (stat_val & IMCSMB_STATUS_BUS_ERROR_BIT) {
		/* While it is not documented, empirically, SPD page-change
		 * commands (writes with DTI = 0x30) always complete with the
		 * error bit set. So, ignore it in those cases.
		 */
		if ((addr & 0x78) != 0x30) {
			rc = ENODEV;
			goto out;
		}
	}

	/* For a read operation, copy the data out */
	if (I2C_OP_READ_P(op)) {
		if (len == 2) {
			/* The data is returned in bits [15:0]; as discussed
			 * above, byte-swap.
			 */
			lword = (uint16_t) (stat_val & 0xffff);
			lword = htobe16(lword);
			*(uint16_t *)buf = lword;
		} else {
			/* The data is returned in bits [7:0] */
			lbyte = (uint8_t) (stat_val & 0xff);
			*(uint8_t *)buf = lbyte;
		}
	}

out:
	/* Restore the original value of the control register. */
	pci_conf_write(sc->sc_pci_chipset_tag, sc->sc_pci_tag,
	    sc->sc_regs->smb_cntl, orig_cntl_val);
	return rc;
};

MODULE(MODULE_CLASS_DRIVER, imcsmb, "imc,iic");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
imcsmb_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

#ifdef _MODULE
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = config_init_component(cfdriver_ioconf_imcsmb,
		    cfattach_ioconf_imcsmb, cfdata_ioconf_imcsmb);
		break;
	case MODULE_CMD_FINI:
		error = config_fini_component(cfdriver_ioconf_imcsmb,
		    cfattach_ioconf_imcsmb, cfdata_ioconf_imcsmb);
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}
