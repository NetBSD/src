/* $NetBSD: piixpm.c,v 1.20 2008/02/05 06:52:26 simonb Exp $ */
/*	$OpenBSD: piixpm.c,v 1.20 2006/02/27 08:25:02 grange Exp $	*/

/*
 * Copyright (c) 2005, 2006 Alexander Yurchenko <grange@openbsd.org>
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

/*
 * Intel PIIX and compatible Power Management controller driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: piixpm.c,v 1.20 2008/02/05 06:52:26 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/rwlock.h>
#include <sys/proc.h>

#include <sys/bus.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/pci/piixpmreg.h>

#include <dev/i2c/i2cvar.h>

#include <dev/ic/acpipmtimer.h>

#ifdef PIIXPM_DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

#define PIIXPM_DELAY	200
#define PIIXPM_TIMEOUT	1

struct piixpm_softc {
	struct device		sc_dev;

	bus_space_tag_t		sc_smb_iot;
	bus_space_handle_t	sc_smb_ioh;
	void *			sc_smb_ih;
	int			sc_poll;

	bus_space_tag_t		sc_pm_iot;
	bus_space_handle_t	sc_pm_ioh;

	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_pcitag;

	struct i2c_controller	sc_i2c_tag;
	krwlock_t		sc_i2c_rwlock;
	struct {
		i2c_op_t     op;
		void *      buf;
		size_t       len;
		int          flags;
		volatile int error;
	}			sc_i2c_xfer;

	pcireg_t		sc_devact[2];
};

int	piixpm_match(struct device *, struct cfdata *, void *);
void	piixpm_attach(struct device *, struct device *, void *);

static bool	piixpm_suspend(device_t);
static bool	piixpm_resume(device_t);

int	piixpm_i2c_acquire_bus(void *, int);
void	piixpm_i2c_release_bus(void *, int);
int	piixpm_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
	    void *, size_t, int);

int	piixpm_intr(void *);

CFATTACH_DECL(piixpm, sizeof(struct piixpm_softc),
    piixpm_match, piixpm_attach, NULL, NULL);

int
piixpm_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct pci_attach_args *pa;

	pa = (struct pci_attach_args *)aux;
	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_INTEL:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_82371AB_PMC:
		case PCI_PRODUCT_INTEL_82440MX_PMC:
			return 1;
		}
		break;
	case PCI_VENDOR_ATI:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_ATI_SB200_SMB:
		case PCI_PRODUCT_ATI_SB300_SMB:
		case PCI_PRODUCT_ATI_SB400_SMB:
			return 1;
		}
		break;
	case PCI_VENDOR_SERVERWORKS:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_SERVERWORKS_OSB4:
		case PCI_PRODUCT_SERVERWORKS_CSB5:
		case PCI_PRODUCT_SERVERWORKS_CSB6:
		case PCI_PRODUCT_SERVERWORKS_HT1000SB:
			return 1;
		}
	}

	return 0;
}

void
piixpm_attach(struct device *parent, struct device *self, void *aux)
{
	struct piixpm_softc *sc = (struct piixpm_softc *)self;
	struct pci_attach_args *pa = aux;
	struct i2cbus_attach_args iba;
	pcireg_t base, conf;
	pcireg_t pmmisc;
	pci_intr_handle_t ih;
	char devinfo[256];
	const char *intrstr = NULL;

	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;

	aprint_naive("\n");

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal("\n%s: %s (rev. 0x%02x)",
		      device_xname(self), devinfo, PCI_REVISION(pa->pa_class));

	if (!pmf_device_register(self, piixpm_suspend, piixpm_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/* Read configuration */
	conf = pci_conf_read(pa->pa_pc, pa->pa_tag, PIIX_SMB_HOSTC);
	DPRINTF((": conf 0x%x", conf));

	if ((PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL) ||
	    (PCI_PRODUCT(pa->pa_id) != PCI_PRODUCT_INTEL_82371AB_PMC))
		goto nopowermanagement;

	/* check whether I/O access to PM regs is enabled */
	pmmisc = pci_conf_read(pa->pa_pc, pa->pa_tag, PIIX_PMREGMISC);
	if (!(pmmisc & 1))
		goto nopowermanagement;

	sc->sc_pm_iot = pa->pa_iot;
	/* Map I/O space */
	base = pci_conf_read(pa->pa_pc, pa->pa_tag, PIIX_PM_BASE);
	if (bus_space_map(sc->sc_pm_iot, PCI_MAPREG_IO_ADDR(base),
	    PIIX_PM_SIZE, 0, &sc->sc_pm_ioh)) {
		aprint_error("%s: can't map power management I/O space\n",
		    sc->sc_dev.dv_xname);
		goto nopowermanagement;
	}

	/*
	 * Revision 0 and 1 are PIIX4, 2 is PIIX4E, 3 is PIIX4M.
	 * PIIX4 and PIIX4E have a bug in the timer latch, see Errata #20
	 * in the "Specification update" (document #297738).
	 */
	acpipmtimer_attach(&sc->sc_dev, sc->sc_pm_iot, sc->sc_pm_ioh,
			   PIIX_PM_PMTMR,
		(PCI_REVISION(pa->pa_class) < 3) ? ACPIPMT_BADLATCH : 0 );

nopowermanagement:
	if ((conf & PIIX_SMB_HOSTC_HSTEN) == 0) {
		aprint_normal("%s: SMBus disabled\n", sc->sc_dev.dv_xname);
		return;
	}

	/* Map I/O space */
	sc->sc_smb_iot = pa->pa_iot;
	base = pci_conf_read(pa->pa_pc, pa->pa_tag, PIIX_SMB_BASE) & 0xffff;
	if (bus_space_map(sc->sc_smb_iot, PCI_MAPREG_IO_ADDR(base),
	    PIIX_SMB_SIZE, 0, &sc->sc_smb_ioh)) {
		aprint_error("%s: can't map smbus I/O space\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_poll = 1;
	if ((conf & PIIX_SMB_HOSTC_INTMASK) == PIIX_SMB_HOSTC_SMI) {
		/* No PCI IRQ */
		aprint_normal("%s: interrupting at SMI", sc->sc_dev.dv_xname);
	} else if ((conf & PIIX_SMB_HOSTC_INTMASK) == PIIX_SMB_HOSTC_IRQ) {
		/* Install interrupt handler */
		if (pci_intr_map(pa, &ih) == 0) {
			intrstr = pci_intr_string(pa->pa_pc, ih);
			sc->sc_smb_ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO,
			    piixpm_intr, sc);
			if (sc->sc_smb_ih != NULL) {
				aprint_normal("%s: interrupting at %s",
				    sc->sc_dev.dv_xname, intrstr);
				sc->sc_poll = 0;
			}
		}
		if (sc->sc_poll)
			aprint_normal("%s: polling", sc->sc_dev.dv_xname);
	}

	aprint_normal("\n");

	/* Attach I2C bus */
	rw_init(&sc->sc_i2c_rwlock);
	sc->sc_i2c_tag.ic_cookie = sc;
	sc->sc_i2c_tag.ic_acquire_bus = piixpm_i2c_acquire_bus;
	sc->sc_i2c_tag.ic_release_bus = piixpm_i2c_release_bus;
	sc->sc_i2c_tag.ic_exec = piixpm_i2c_exec;

	bzero(&iba, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c_tag;
	config_found_ia(self, "i2cbus", &iba, iicbus_print);

	return;
}

static bool
piixpm_suspend(device_t dv)
{
	struct piixpm_softc *sc = device_private(dv);

	sc->sc_devact[0] = pci_conf_read(sc->sc_pc, sc->sc_pcitag,
	    PIIX_DEVACTA);
	sc->sc_devact[1] = pci_conf_read(sc->sc_pc, sc->sc_pcitag,
	    PIIX_DEVACTB);

	return true;
}

static bool
piixpm_resume(device_t dv)
{
	struct piixpm_softc *sc = device_private(dv);

	pci_conf_write(sc->sc_pc, sc->sc_pcitag, PIIX_DEVACTA,
	    sc->sc_devact[0]);
	pci_conf_write(sc->sc_pc, sc->sc_pcitag, PIIX_DEVACTB,
	    sc->sc_devact[1]);

	return true;
}

int
piixpm_i2c_acquire_bus(void *cookie, int flags)
{
	struct piixpm_softc *sc = cookie;

	if (cold || sc->sc_poll || (flags & I2C_F_POLL))
		return (0);

	rw_enter(&sc->sc_i2c_rwlock, RW_WRITER);
	return 0;
}

void
piixpm_i2c_release_bus(void *cookie, int flags)
{
	struct piixpm_softc *sc = cookie;

	if (cold || sc->sc_poll || (flags & I2C_F_POLL))
		return;

	rw_exit(&sc->sc_i2c_rwlock);
}

int
piixpm_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct piixpm_softc *sc = cookie;
	const u_int8_t *b;
	u_int8_t ctl = 0, st;
	int retries;

	DPRINTF(("%s: exec: op %d, addr 0x%x, cmdlen %d, len %d, flags 0x%x\n",
	    sc->sc_dev.dv_xname, op, addr, cmdlen, len, flags));

	/* Wait for bus to be idle */
	for (retries = 100; retries > 0; retries--) {
		st = bus_space_read_1(sc->sc_smb_iot, sc->sc_smb_ioh,
		    PIIX_SMB_HS);
		if (!(st & PIIX_SMB_HS_BUSY))
			break;
		DELAY(PIIXPM_DELAY);
	}
	DPRINTF(("%s: exec: st 0x%d\n", sc->sc_dev.dv_xname, st & 0xff));
	if (st & PIIX_SMB_HS_BUSY)
		return (1);

	if (cold || sc->sc_poll)
		flags |= I2C_F_POLL;

	if (!I2C_OP_STOP_P(op) || cmdlen > 1 || len > 2)
		return (1);

	/* Setup transfer */
	sc->sc_i2c_xfer.op = op;
	sc->sc_i2c_xfer.buf = buf;
	sc->sc_i2c_xfer.len = len;
	sc->sc_i2c_xfer.flags = flags;
	sc->sc_i2c_xfer.error = 0;

	/* Set slave address and transfer direction */
	bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_TXSLVA,
	    PIIX_SMB_TXSLVA_ADDR(addr) |
	    (I2C_OP_READ_P(op) ? PIIX_SMB_TXSLVA_READ : 0));

	b = cmdbuf;
	if (cmdlen > 0)
		/* Set command byte */
		bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh,
		    PIIX_SMB_HCMD, b[0]);

	if (I2C_OP_WRITE_P(op)) {
		/* Write data */
		b = buf;
		if (len > 0)
			bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh,
			    PIIX_SMB_HD0, b[0]);
		if (len > 1)
			bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh,
			    PIIX_SMB_HD1, b[1]);
	}

	/* Set SMBus command */
	if (len == 0)
		ctl = PIIX_SMB_HC_CMD_BYTE;
	else if (len == 1)
		ctl = PIIX_SMB_HC_CMD_BDATA;
	else if (len == 2)
		ctl = PIIX_SMB_HC_CMD_WDATA;

	if ((flags & I2C_F_POLL) == 0)
		ctl |= PIIX_SMB_HC_INTREN;

	/* Start transaction */
	ctl |= PIIX_SMB_HC_START;
	bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HC, ctl);

	if (flags & I2C_F_POLL) {
		/* Poll for completion */
		DELAY(PIIXPM_DELAY);
		for (retries = 1000; retries > 0; retries--) {
			st = bus_space_read_1(sc->sc_smb_iot, sc->sc_smb_ioh,
			    PIIX_SMB_HS);
			if ((st & PIIX_SMB_HS_BUSY) == 0)
				break;
			DELAY(PIIXPM_DELAY);
		}
		if (st & PIIX_SMB_HS_BUSY)
			goto timeout;
		piixpm_intr(sc);
	} else {
		/* Wait for interrupt */
		if (tsleep(sc, PRIBIO, "iicexec", PIIXPM_TIMEOUT * hz))
			goto timeout;
	}

	if (sc->sc_i2c_xfer.error)
		return (1);

	return (0);

timeout:
	/*
	 * Transfer timeout. Kill the transaction and clear status bits.
	 */
	aprint_error("%s: timeout, status 0x%x\n", sc->sc_dev.dv_xname, st);
	bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HC,
	    PIIX_SMB_HC_KILL);
	DELAY(PIIXPM_DELAY);
	st = bus_space_read_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HS);
	if ((st & PIIX_SMB_HS_FAILED) == 0)
		aprint_error("%s: transaction abort failed, status 0x%x\n",
		    sc->sc_dev.dv_xname, st);
	bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HS, st);
	return (1);
}

int
piixpm_intr(void *arg)
{
	struct piixpm_softc *sc = arg;
	u_int8_t st;
	u_int8_t *b;
	size_t len;

	/* Read status */
	st = bus_space_read_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HS);
	if ((st & PIIX_SMB_HS_BUSY) != 0 || (st & (PIIX_SMB_HS_INTR |
	    PIIX_SMB_HS_DEVERR | PIIX_SMB_HS_BUSERR |
	    PIIX_SMB_HS_FAILED)) == 0)
		/* Interrupt was not for us */
		return (0);

	DPRINTF(("%s: intr st 0x%d\n", sc->sc_dev.dv_xname, st & 0xff));

	/* Clear status bits */
	bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HS, st);

	/* Check for errors */
	if (st & (PIIX_SMB_HS_DEVERR | PIIX_SMB_HS_BUSERR |
	    PIIX_SMB_HS_FAILED)) {
		sc->sc_i2c_xfer.error = 1;
		goto done;
	}

	if (st & PIIX_SMB_HS_INTR) {
		if (I2C_OP_WRITE_P(sc->sc_i2c_xfer.op))
			goto done;

		/* Read data */
		b = sc->sc_i2c_xfer.buf;
		len = sc->sc_i2c_xfer.len;
		if (len > 0)
			b[0] = bus_space_read_1(sc->sc_smb_iot, sc->sc_smb_ioh,
			    PIIX_SMB_HD0);
		if (len > 1)
			b[1] = bus_space_read_1(sc->sc_smb_iot, sc->sc_smb_ioh,
			    PIIX_SMB_HD1);
	}

done:
	if ((sc->sc_i2c_xfer.flags & I2C_F_POLL) == 0)
		wakeup(sc);
	return (1);
}
