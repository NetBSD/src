/*	$NetBSD: ichsmb.c,v 1.3.2.2 2007/08/05 23:05:03 xtraeme Exp $	*/
/*	$OpenBSD: ichiic.c,v 1.18 2007/05/03 09:36:26 dlg Exp $	*/

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
 * Intel ICH SMBus controller driver.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/proc.h>

#include <machine/bus.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/pci/ichreg.h>

#include <dev/i2c/i2cvar.h>

#ifdef ICHIIC_DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

#define ICHIIC_DELAY	100
#define ICHIIC_TIMEOUT	1

struct ichsmb_softc {
	struct device		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	void *			sc_ih;
	int			sc_poll;

	struct i2c_controller	sc_i2c_tag;
	struct lock		sc_i2c_lock;
	struct {
		i2c_op_t     op;
		void *       buf;
		size_t       len;
		int          flags;
		volatile int error;
	}			sc_i2c_xfer;
};

static int	ichsmb_match(struct device *, struct cfdata *, void *);
static void	ichsmb_attach(struct device *, struct device *, void *);

static int	ichsmb_i2c_acquire_bus(void *, int);
static void	ichsmb_i2c_release_bus(void *, int);
static int	ichsmb_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
		    size_t, void *, size_t, int);

static int	ichsmb_intr(void *);


CFATTACH_DECL(ichsmb, sizeof(struct ichsmb_softc),
    ichsmb_match, ichsmb_attach, NULL, NULL);


static int
ichsmb_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL) {
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_6300ESB_SMB:
		case PCI_PRODUCT_INTEL_63XXESB_SMB:
		case PCI_PRODUCT_INTEL_82801AA_SMB:
		case PCI_PRODUCT_INTEL_82801AB_SMB:
		case PCI_PRODUCT_INTEL_82801BA_SMB:
		case PCI_PRODUCT_INTEL_82801CA_SMB:
		case PCI_PRODUCT_INTEL_82801DB_SMB:
		case PCI_PRODUCT_INTEL_82801E_SMB:
		case PCI_PRODUCT_INTEL_82801EB_SMB:
		case PCI_PRODUCT_INTEL_82801FB_SMB:
		case PCI_PRODUCT_INTEL_82801G_SMB:
		case PCI_PRODUCT_INTEL_82801H_SMB:
			return 1;
		}
	}
	return 0;
}

static void
ichsmb_attach(struct device *parent, struct device *self, void *aux)
{
	struct ichsmb_softc *sc = (struct ichsmb_softc *)self;
	struct pci_attach_args *pa = aux;
	struct i2cbus_attach_args iba;
	pcireg_t conf;
	bus_size_t iosize;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	char devinfo[256];

	aprint_naive("\n");
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	/* Read configuration */
	conf = pci_conf_read(pa->pa_pc, pa->pa_tag, ICH_SMB_HOSTC);
	DPRINTF(("%s: conf 0x%08x", sc->sc_dev.dv_xname, conf));

	if ((conf & ICH_SMB_HOSTC_HSTEN) == 0) {
		aprint_error("%s: SMBus disabled\n", sc->sc_dev.dv_xname);
		return;
	}

	/* Map I/O space */
	if (pci_mapreg_map(pa, ICH_SMB_BASE, PCI_MAPREG_TYPE_IO, 0,
	    &sc->sc_iot, &sc->sc_ioh, NULL, &iosize)) {
		aprint_error("%s: can't map I/O space\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_poll = 1;
	if (conf & ICH_SMB_HOSTC_SMIEN) {
		/* No PCI IRQ */
		aprint_normal("%s: SMI\n", sc->sc_dev.dv_xname);
	} else {
		/* Install interrupt handler */
		if (pci_intr_map(pa, &ih) == 0) {
			intrstr = pci_intr_string(pa->pa_pc, ih);
			sc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO,
			    ichsmb_intr, sc);
			if (sc->sc_ih != NULL) {
				aprint_normal("%s: interrupting at %s\n",
				    sc->sc_dev.dv_xname, intrstr);
				sc->sc_poll = 0;
			}
		}
		if (sc->sc_poll)
			aprint_normal("%s: polling\n", sc->sc_dev.dv_xname);
	}

	/* Attach I2C bus */
	lockinit(&sc->sc_i2c_lock, PZERO, "smblk", 0, 0);
	sc->sc_i2c_tag.ic_cookie = sc;
	sc->sc_i2c_tag.ic_acquire_bus = ichsmb_i2c_acquire_bus;
	sc->sc_i2c_tag.ic_release_bus = ichsmb_i2c_release_bus;
	sc->sc_i2c_tag.ic_exec = ichsmb_i2c_exec;

	bzero(&iba, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c_tag;
	config_found(self, &iba, iicbus_print);

	return;
}

static int
ichsmb_i2c_acquire_bus(void *cookie, int flags)
{
	struct ichsmb_softc *sc = cookie;

	if (cold || sc->sc_poll || (flags & I2C_F_POLL))
		return (0);

	return (lockmgr(&sc->sc_i2c_lock, LK_EXCLUSIVE, NULL));
}

static void
ichsmb_i2c_release_bus(void *cookie, int flags)
{
	struct ichsmb_softc *sc = cookie;

	if (cold || sc->sc_poll || (flags & I2C_F_POLL))
		return;

	lockmgr(&sc->sc_i2c_lock, LK_RELEASE, NULL);
}

static int
ichsmb_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct ichsmb_softc *sc = cookie;
	const uint8_t *b;
	uint8_t ctl = 0, st;
	int retries;

	DPRINTF(("%s: exec: op %d, addr 0x%02x, cmdlen %zu, len %d, "
	    "flags 0x%02x\n", sc->sc_dev.dv_xname, op, addr, cmdlen,
	    len, flags));

	/* Wait for bus to be idle */
	for (retries = 100; retries > 0; retries--) {
		st = bus_space_read_1(sc->sc_iot, sc->sc_ioh, ICH_SMB_HS);
		if (!(st & ICH_SMB_HS_BUSY))
			break;
		DELAY(ICHIIC_DELAY);
	}
	DPRINTF(("%s: exec: st 0x%02x\n", sc->sc_dev.dv_xname, st));
	if (st & ICH_SMB_HS_BUSY)
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
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, ICH_SMB_TXSLVA,
	    ICH_SMB_TXSLVA_ADDR(addr) |
	    (I2C_OP_READ_P(op) ? ICH_SMB_TXSLVA_READ : 0));

	b = (const uint8_t *)cmdbuf;
	if (cmdlen > 0)
		/* Set command byte */
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, ICH_SMB_HCMD, b[0]);

	if (I2C_OP_WRITE_P(op)) {
		/* Write data */
		b = buf;
		if (len > 0)
			bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			    ICH_SMB_HD0, b[0]);
		if (len > 1)
			bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			    ICH_SMB_HD1, b[1]);
	}

	/* Set SMBus command */
	if (len == 0)
		ctl = ICH_SMB_HC_CMD_BYTE;
	else if (len == 1)
		ctl = ICH_SMB_HC_CMD_BDATA;
	else if (len == 2)
		ctl = ICH_SMB_HC_CMD_WDATA;

	if ((flags & I2C_F_POLL) == 0)
		ctl |= ICH_SMB_HC_INTREN;

	/* Start transaction */
	ctl |= ICH_SMB_HC_START;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, ICH_SMB_HC, ctl);

	if (flags & I2C_F_POLL) {
		/* Poll for completion */
		DELAY(ICHIIC_DELAY);
		for (retries = 1000; retries > 0; retries--) {
			st = bus_space_read_1(sc->sc_iot, sc->sc_ioh,
			    ICH_SMB_HS);
			if ((st & ICH_SMB_HS_BUSY) == 0)
				break;
			DELAY(ICHIIC_DELAY);
		}
		if (st & ICH_SMB_HS_BUSY)
			goto timeout;
		ichsmb_intr(sc);
	} else {
		/* Wait for interrupt */
		if (tsleep(sc, PRIBIO, "iicexec", ICHIIC_TIMEOUT * hz))
			goto timeout;
	}

	if (sc->sc_i2c_xfer.error)
		return (1);

	return (0);

timeout:
	/*
	 * Transfer timeout. Kill the transaction and clear status bits.
	 */
	printf("%s: exec: op %d, addr 0x%02x, cmdlen %zd, len %zd, "
	    "flags 0x%02x: timeout, status 0x%02x\n",
	    sc->sc_dev.dv_xname, op, addr, cmdlen, len, flags,
	    st);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, ICH_SMB_HC,
	    ICH_SMB_HC_KILL);
	DELAY(ICHIIC_DELAY);
	st = bus_space_read_1(sc->sc_iot, sc->sc_ioh, ICH_SMB_HS);
	if ((st & ICH_SMB_HS_FAILED) == 0)
		printf("%s: abort failed, status 0x%02x\n",
		    sc->sc_dev.dv_xname, st);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, ICH_SMB_HS, st);
	return (1);
}

static int
ichsmb_intr(void *arg)
{
	struct ichsmb_softc *sc = arg;
	uint8_t st;
	uint8_t *b;
	size_t len;

	/* Read status */
	st = bus_space_read_1(sc->sc_iot, sc->sc_ioh, ICH_SMB_HS);
	if ((st & ICH_SMB_HS_BUSY) != 0 || (st & (ICH_SMB_HS_INTR |
	    ICH_SMB_HS_DEVERR | ICH_SMB_HS_BUSERR | ICH_SMB_HS_FAILED |
	    ICH_SMB_HS_SMBAL | ICH_SMB_HS_BDONE)) == 0)
		/* Interrupt was not for us */
		return (0);

	DPRINTF(("%s: intr st 0x%02x\n", sc->sc_dev.dv_xname, st));

	/* Clear status bits */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, ICH_SMB_HS, st);

	/* Check for errors */
	if (st & (ICH_SMB_HS_DEVERR | ICH_SMB_HS_BUSERR | ICH_SMB_HS_FAILED)) {
		sc->sc_i2c_xfer.error = 1;
		goto done;
	}

	if (st & ICH_SMB_HS_INTR) {
		if (I2C_OP_WRITE_P(sc->sc_i2c_xfer.op))
			goto done;

		/* Read data */
		b = sc->sc_i2c_xfer.buf;
		len = sc->sc_i2c_xfer.len;
		if (len > 0)
			b[0] = bus_space_read_1(sc->sc_iot, sc->sc_ioh,
			    ICH_SMB_HD0);
		if (len > 1)
			b[1] = bus_space_read_1(sc->sc_iot, sc->sc_ioh,
			    ICH_SMB_HD1);
	}

done:
	if ((sc->sc_i2c_xfer.flags & I2C_F_POLL) == 0)
		wakeup(sc);
	return (1);
}
