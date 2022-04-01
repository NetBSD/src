/* $NetBSD: piixpm.c,v 1.67 2022/04/01 15:34:34 pgoyette Exp $ */
/*	$OpenBSD: piixpm.c,v 1.39 2013/10/01 20:06:02 sf Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: piixpm.c,v 1.67 2022/04/01 15:34:34 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
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

#define PIIXPM_IS_CSB5(sc)						      \
	(PCI_VENDOR((sc)->sc_id) == PCI_VENDOR_SERVERWORKS &&		      \
	PCI_PRODUCT((sc)->sc_id) == PCI_PRODUCT_SERVERWORKS_CSB5)
#define PIIXPM_DELAY	200
#define PIIXPM_TIMEOUT	1

#define PIIXPM_IS_SB800GRP(sc)						      \
	((PCI_VENDOR((sc)->sc_id) == PCI_VENDOR_ATI) &&			      \
	    ((PCI_PRODUCT((sc)->sc_id) == PCI_PRODUCT_ATI_SB600_SMB) &&	      \
		((sc)->sc_rev >= 0x40)))

#define PIIXPM_IS_HUDSON(sc)						      \
	((PCI_VENDOR((sc)->sc_id) == PCI_VENDOR_AMD) &&			      \
	    (PCI_PRODUCT((sc)->sc_id) == PCI_PRODUCT_AMD_HUDSON_SMB))

#define PIIXPM_IS_KERNCZ(sc)						      \
	((PCI_VENDOR((sc)->sc_id) == PCI_VENDOR_AMD) &&			      \
	    (PCI_PRODUCT((sc)->sc_id) == PCI_PRODUCT_AMD_KERNCZ_SMB))

#define PIIXPM_IS_FCHGRP(sc)	(PIIXPM_IS_HUDSON(sc) || PIIXPM_IS_KERNCZ(sc))

#define PIIX_SB800_TIMEOUT 500

struct piixpm_smbus {
	int			sda;
	int			sda_save;
	struct			piixpm_softc *softc;
};

struct piixpm_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_iot;
#define	sc_pm_iot sc_iot
#define sc_smb_iot sc_iot
	bus_space_handle_t	sc_pm_ioh;
	bus_space_handle_t	sc_sb800_ioh;
	bus_space_handle_t	sc_smb_ioh;
	void *			sc_smb_ih;
	int			sc_poll;
	bool			sc_sb800_selen; /* Use SMBUS0SEL */

	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_pcitag;
	pcireg_t		sc_id;
	pcireg_t		sc_rev;

	int			sc_numbusses;
	device_t		sc_i2c_device[4];
	struct piixpm_smbus	sc_busses[4];
	struct i2c_controller	sc_i2c_tags[4];

	kmutex_t		sc_exec_lock;
	kcondvar_t		sc_exec_wait;

	struct {
		i2c_op_t	op;
		void *		buf;
		size_t		len;
		int		flags;
		int             error;
		bool            done;
	}			sc_i2c_xfer;

	pcireg_t		sc_devact[2];
};

static int	piixpm_match(device_t, cfdata_t, void *);
static void	piixpm_attach(device_t, device_t, void *);
static int	piixpm_rescan(device_t, const char *, const int *);
static void	piixpm_chdet(device_t, device_t);

static bool	piixpm_suspend(device_t, const pmf_qual_t *);
static bool	piixpm_resume(device_t, const pmf_qual_t *);

static int	piixpm_sb800_init(struct piixpm_softc *);
static void	piixpm_csb5_reset(void *);
static int	piixpm_i2c_sb800_acquire_bus(void *, int);
static void	piixpm_i2c_sb800_release_bus(void *, int);
static int	piixpm_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
    size_t, void *, size_t, int);

static int	piixpm_intr(void *);

CFATTACH_DECL3_NEW(piixpm, sizeof(struct piixpm_softc),
    piixpm_match, piixpm_attach, NULL, NULL, piixpm_rescan, piixpm_chdet, 0);

static int
piixpm_match(device_t parent, cfdata_t match, void *aux)
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
		case PCI_PRODUCT_ATI_SB600_SMB:	/* matches SB600/SB700/SB800 */
			return 1;
		}
		break;
	case PCI_VENDOR_SERVERWORKS:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_SERVERWORKS_OSB4:
		case PCI_PRODUCT_SERVERWORKS_CSB5:
		case PCI_PRODUCT_SERVERWORKS_CSB6:
		case PCI_PRODUCT_SERVERWORKS_HT1000SB:
		case PCI_PRODUCT_SERVERWORKS_HT1100SB:
			return 1;
		}
		break;
	case PCI_VENDOR_AMD:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_AMD_HUDSON_SMB:
		case PCI_PRODUCT_AMD_KERNCZ_SMB:
			return 1;
		}
		break;
	}

	return 0;
}

static void
piixpm_attach(device_t parent, device_t self, void *aux)
{
	struct piixpm_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	pcireg_t base, conf;
	pcireg_t pmmisc;
	pci_intr_handle_t ih;
	bool usesmi = false;
	const char *intrstr = NULL;
	int i;
	char intrbuf[PCI_INTRSTR_LEN];

	sc->sc_dev = self;
	sc->sc_iot = pa->pa_iot;
	sc->sc_id = pa->pa_id;
	sc->sc_rev = PCI_REVISION(pa->pa_class);
	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_numbusses = 1;

	pci_aprint_devinfo(pa, NULL);

	mutex_init(&sc->sc_exec_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_exec_wait, device_xname(self));

	if (!pmf_device_register(self, piixpm_suspend, piixpm_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	if ((PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL) ||
	    (PCI_PRODUCT(pa->pa_id) != PCI_PRODUCT_INTEL_82371AB_PMC))
		goto nopowermanagement;

	/* check whether I/O access to PM regs is enabled */
	pmmisc = pci_conf_read(pa->pa_pc, pa->pa_tag, PIIX_PMREGMISC);
	if (!(pmmisc & 1))
		goto nopowermanagement;

	/* Map I/O space */
	base = pci_conf_read(pa->pa_pc, pa->pa_tag, PIIX_PM_BASE);
	if (base == 0 || bus_space_map(sc->sc_pm_iot, PCI_MAPREG_IO_ADDR(base),
	    PIIX_PM_SIZE, 0, &sc->sc_pm_ioh)) {
		aprint_error_dev(self,
		    "can't map power management I/O space\n");
		goto nopowermanagement;
	}

	/*
	 * Revision 0 and 1 are PIIX4, 2 is PIIX4E, 3 is PIIX4M.
	 * PIIX4 and PIIX4E have a bug in the timer latch, see Errata #20
	 * in the "Specification update" (document #297738).
	 */
	acpipmtimer_attach(self, sc->sc_pm_iot, sc->sc_pm_ioh, PIIX_PM_PMTMR,
	    (PCI_REVISION(pa->pa_class) < 3) ? ACPIPMT_BADLATCH : 0);

nopowermanagement:

	/* SB800 rev 0x40+, AMD HUDSON and newer need special initialization */
	if (PIIXPM_IS_FCHGRP(sc) || PIIXPM_IS_SB800GRP(sc)) {
		if (piixpm_sb800_init(sc) == 0) {
			/* Read configuration */
			conf = bus_space_read_1(sc->sc_iot,
			    sc->sc_smb_ioh, SB800_SMB_HOSTC);
			usesmi = ((conf & SB800_SMB_HOSTC_IRQ) == 0);
			goto setintr;
		}
		aprint_normal_dev(self, "SMBus initialization failed\n");
		return;
	}

	/* Read configuration */
	conf = pci_conf_read(pa->pa_pc, pa->pa_tag, PIIX_SMB_HOSTC);
	DPRINTF(("%s: conf 0x%08x\n", device_xname(self), conf));

	if ((conf & PIIX_SMB_HOSTC_HSTEN) == 0) {
		aprint_normal_dev(self, "SMBus disabled\n");
		return;
	}
	usesmi = (conf & PIIX_SMB_HOSTC_INTMASK) == PIIX_SMB_HOSTC_SMI;

	/* Map I/O space */
	base = pci_conf_read(pa->pa_pc, pa->pa_tag, PIIX_SMB_BASE) & 0xffff;
	if (base == 0 ||
	    bus_space_map(sc->sc_smb_iot, PCI_MAPREG_IO_ADDR(base),
	    PIIX_SMB_SIZE, 0, &sc->sc_smb_ioh)) {
		aprint_error_dev(self, "can't map smbus I/O space\n");
		return;
	}

setintr:
	sc->sc_poll = 1;
	aprint_normal_dev(self, "");
	if (usesmi) {
		/* No PCI IRQ */
		aprint_normal("interrupting at SMI, ");
	} else {
		if ((conf & PIIX_SMB_HOSTC_INTMASK) == PIIX_SMB_HOSTC_IRQ) {
			/* Install interrupt handler */
			if (pci_intr_map(pa, &ih) == 0) {
				intrstr = pci_intr_string(pa->pa_pc, ih,
				    intrbuf, sizeof(intrbuf));
				pci_intr_setattr(pa->pa_pc, &ih,
				    PCI_INTR_MPSAFE, true);
				sc->sc_smb_ih = pci_intr_establish_xname(
					pa->pa_pc, ih, IPL_BIO, piixpm_intr,
					sc, device_xname(sc->sc_dev));
				if (sc->sc_smb_ih != NULL) {
					aprint_normal("interrupting at %s",
					    intrstr);
					sc->sc_poll = 0;
				}
			}
		}
		if (sc->sc_poll)
			aprint_normal("polling");
	}

	aprint_normal("\n");

	for (i = 0; i < sc->sc_numbusses; i++)
		sc->sc_i2c_device[i] = NULL;

	piixpm_rescan(self, NULL, NULL);
}

static int
piixpm_iicbus_print(void *aux, const char *pnp)
{
	struct i2cbus_attach_args *iba = aux;
	struct i2c_controller *tag = iba->iba_tag;
	struct piixpm_smbus *bus = tag->ic_cookie;
	struct piixpm_softc *sc = bus->softc;

	iicbus_print(aux, pnp);
	if (sc->sc_numbusses != 0)
		aprint_normal(" port %d", bus->sda);

	return UNCONF;
}

static int
piixpm_rescan(device_t self, const char *ifattr, const int *locators)
{
	struct piixpm_softc *sc = device_private(self);
	struct i2cbus_attach_args iba;
	int i;

	/* Attach I2C bus */

	for (i = 0; i < sc->sc_numbusses; i++) {
		struct i2c_controller *tag = &sc->sc_i2c_tags[i];

		if (sc->sc_i2c_device[i] != NULL)
			continue;
		sc->sc_busses[i].sda = i;
		sc->sc_busses[i].softc = sc;
		iic_tag_init(tag);
		tag->ic_cookie = &sc->sc_busses[i];
		if (PIIXPM_IS_SB800GRP(sc) || PIIXPM_IS_FCHGRP(sc)) {
			tag->ic_acquire_bus = piixpm_i2c_sb800_acquire_bus;
			tag->ic_release_bus = piixpm_i2c_sb800_release_bus;
		} else {
			tag->ic_acquire_bus = NULL;
			tag->ic_release_bus = NULL;
		}
		tag->ic_exec = piixpm_i2c_exec;
		memset(&iba, 0, sizeof(iba));
		iba.iba_tag = tag;
		sc->sc_i2c_device[i] =
		    config_found(self, &iba, piixpm_iicbus_print, CFARGS_NONE);
		if (sc->sc_i2c_device[i] == NULL)
			iic_tag_fini(tag);
	}

	return 0;
}

static void
piixpm_chdet(device_t self, device_t child)
{
	struct piixpm_softc *sc = device_private(self);
	int i;

	for (i = 0; i < sc->sc_numbusses; i++) {
		if (sc->sc_i2c_device[i] == child) {

			struct i2c_controller *tag = &sc->sc_i2c_tags[i];

			iic_tag_fini(tag);
			sc->sc_i2c_device[i] = NULL;
			break;
		}
	}
}


static bool
piixpm_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct piixpm_softc *sc = device_private(dv);

	sc->sc_devact[0] = pci_conf_read(sc->sc_pc, sc->sc_pcitag,
	    PIIX_DEVACTA);
	sc->sc_devact[1] = pci_conf_read(sc->sc_pc, sc->sc_pcitag,
	    PIIX_DEVACTB);

	return true;
}

static bool
piixpm_resume(device_t dv, const pmf_qual_t *qual)
{
	struct piixpm_softc *sc = device_private(dv);

	pci_conf_write(sc->sc_pc, sc->sc_pcitag, PIIX_DEVACTA,
	    sc->sc_devact[0]);
	pci_conf_write(sc->sc_pc, sc->sc_pcitag, PIIX_DEVACTB,
	    sc->sc_devact[1]);

	return true;
}

/*
 * Extract SMBus base address from SB800 Power Management (PM) registers.
 * The PM registers can be accessed either through indirect I/O (CD6/CD7) or
 * direct mapping if AcpiMMioDecodeEn is enabled. Since this function is only
 * called once it uses indirect I/O for simplicity.
 */
static int
piixpm_sb800_init(struct piixpm_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh;	/* indirect I/O handle */
	uint16_t val, base_addr;
	bool enabled;

	if (PIIXPM_IS_KERNCZ(sc) ||
	    (PIIXPM_IS_HUDSON(sc) && (sc->sc_rev >= 0x1f)))
		sc->sc_numbusses = 2;
	else
		sc->sc_numbusses = 4;

	/* Fetch SMB base address */
	if (bus_space_map(iot,
	    SB800_INDIRECTIO_BASE, SB800_INDIRECTIO_SIZE, 0, &ioh)) {
		device_printf(sc->sc_dev, "couldn't map indirect I/O space\n");
		return EBUSY;
	}
	if (PIIXPM_IS_FCHGRP(sc)) {
		bus_space_write_1(iot, ioh, SB800_INDIRECTIO_INDEX,
		    AMDFCH41_PM_DECODE_EN0);
		val = bus_space_read_1(iot, ioh, SB800_INDIRECTIO_DATA);
		enabled = val & AMDFCH41_SMBUS_EN;
		if (!enabled)
			return ENOENT;

		bus_space_write_1(iot, ioh, SB800_INDIRECTIO_INDEX,
		    AMDFCH41_PM_DECODE_EN1);
		val = bus_space_read_1(iot, ioh, SB800_INDIRECTIO_DATA) << 8;
		base_addr = val;
	} else {
		uint8_t data;

		bus_space_write_1(iot, ioh, SB800_INDIRECTIO_INDEX,
		    SB800_PM_SMBUS0EN_LO);
		val = bus_space_read_1(iot, ioh, SB800_INDIRECTIO_DATA);
		enabled = val & SB800_PM_SMBUS0EN_ENABLE;
		if (!enabled)
			return ENOENT;

		bus_space_write_1(iot, ioh, SB800_INDIRECTIO_INDEX,
		    SB800_PM_SMBUS0EN_HI);
		val |= bus_space_read_1(iot, ioh, SB800_INDIRECTIO_DATA) << 8;
		base_addr = val & SB800_PM_SMBUS0EN_BADDR;

		bus_space_write_1(iot, ioh, SB800_INDIRECTIO_INDEX,
		    SB800_PM_SMBUS0SELEN);
		data = bus_space_read_1(iot, ioh, SB800_INDIRECTIO_DATA);
		if ((data & SB800_PM_USE_SMBUS0SEL) != 0)
			sc->sc_sb800_selen = true;
	}

	sc->sc_sb800_ioh = ioh;
	aprint_debug_dev(sc->sc_dev, "SMBus @ 0x%04x\n", base_addr);

	if (bus_space_map(iot, PCI_MAPREG_IO_ADDR(base_addr),
	    SB800_SMB_SIZE, 0, &sc->sc_smb_ioh)) {
		aprint_error_dev(sc->sc_dev, "can't map smbus I/O space\n");
		return EBUSY;
	}

	return 0;
}

static void
piixpm_csb5_reset(void *arg)
{
	struct piixpm_softc *sc = arg;
	pcireg_t base, hostc, pmbase;

	base = pci_conf_read(sc->sc_pc, sc->sc_pcitag, PIIX_SMB_BASE);
	hostc = pci_conf_read(sc->sc_pc, sc->sc_pcitag, PIIX_SMB_HOSTC);

	pmbase = pci_conf_read(sc->sc_pc, sc->sc_pcitag, PIIX_PM_BASE);
	pmbase |= PIIX_PM_BASE_CSB5_RESET;
	pci_conf_write(sc->sc_pc, sc->sc_pcitag, PIIX_PM_BASE, pmbase);
	pmbase &= ~PIIX_PM_BASE_CSB5_RESET;
	pci_conf_write(sc->sc_pc, sc->sc_pcitag, PIIX_PM_BASE, pmbase);

	pci_conf_write(sc->sc_pc, sc->sc_pcitag, PIIX_SMB_BASE, base);
	pci_conf_write(sc->sc_pc, sc->sc_pcitag, PIIX_SMB_HOSTC, hostc);

	(void) tsleep(&sc, PRIBIO, "csb5reset", hz/2);
}

static int
piixpm_i2c_sb800_acquire_bus(void *cookie, int flags)
{
	struct piixpm_smbus *smbus = cookie;
	struct piixpm_softc *sc = smbus->softc;
	uint8_t sctl, old_sda, index, mask, reg;
	int i;

	sctl = bus_space_read_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_SC);
	for (i = 0; i < PIIX_SB800_TIMEOUT; i++) {
		/* Try to acquire the host semaphore */
		sctl &= ~PIIX_SMB_SC_SEMMASK;
		bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_SC,
		    sctl | PIIX_SMB_SC_HOSTSEM);

		sctl = bus_space_read_1(sc->sc_smb_iot, sc->sc_smb_ioh,
		    PIIX_SMB_SC);
		if ((sctl & PIIX_SMB_SC_HOSTSEM) != 0)
			break;

		delay(1000);
	}
	if (i >= PIIX_SB800_TIMEOUT) {
		device_printf(sc->sc_dev,
		    "Failed to acquire the host semaphore\n");
		return -1;
	}

	if (PIIXPM_IS_KERNCZ(sc) ||
	    (PIIXPM_IS_HUDSON(sc) && (sc->sc_rev >= 0x1f))) {
		index = AMDFCH41_PM_PORT_INDEX;
		mask = AMDFCH41_SMBUS_PORTMASK;
	} else if (sc->sc_sb800_selen) {
		index = SB800_PM_SMBUS0SEL;
		mask = SB800_PM_SMBUS0_MASK_E;
	} else {
		index = SB800_PM_SMBUS0EN_LO;
		mask = SB800_PM_SMBUS0_MASK_C;
	}

	bus_space_write_1(sc->sc_iot, sc->sc_sb800_ioh,
	    SB800_INDIRECTIO_INDEX, index);
	reg = bus_space_read_1(sc->sc_iot, sc->sc_sb800_ioh,
	    SB800_INDIRECTIO_DATA);

	old_sda = __SHIFTOUT(reg, mask);
	if (smbus->sda != old_sda) {
		reg &= ~mask;
		reg |= __SHIFTIN(smbus->sda, mask);
		bus_space_write_1(sc->sc_iot, sc->sc_sb800_ioh,
		    SB800_INDIRECTIO_DATA, reg);
	}

	/* Save the old port number */
	smbus->sda_save = old_sda;

	return 0;
}

static void
piixpm_i2c_sb800_release_bus(void *cookie, int flags)
{
	struct piixpm_smbus *smbus = cookie;
	struct piixpm_softc *sc = smbus->softc;
	uint8_t sctl, index, mask, reg;

	if (PIIXPM_IS_KERNCZ(sc) ||
	    (PIIXPM_IS_HUDSON(sc) && (sc->sc_rev >= 0x1f))) {
		index = AMDFCH41_PM_PORT_INDEX;
		mask = AMDFCH41_SMBUS_PORTMASK;
	} else if (sc->sc_sb800_selen) {
		index = SB800_PM_SMBUS0SEL;
		mask = SB800_PM_SMBUS0_MASK_E;
	} else {
		index = SB800_PM_SMBUS0EN_LO;
		mask = SB800_PM_SMBUS0_MASK_C;
	}

	bus_space_write_1(sc->sc_iot, sc->sc_sb800_ioh,
	    SB800_INDIRECTIO_INDEX, index);
	if (smbus->sda != smbus->sda_save) {
		/* Restore the port number */
		reg = bus_space_read_1(sc->sc_iot, sc->sc_sb800_ioh,
		    SB800_INDIRECTIO_DATA);
		reg &= ~mask;
		reg |= __SHIFTIN(smbus->sda_save, mask);
		bus_space_write_1(sc->sc_iot, sc->sc_sb800_ioh,
		    SB800_INDIRECTIO_DATA, reg);
	}

	/* Release the host semaphore */
	sctl = bus_space_read_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_SC);
	sctl &= ~PIIX_SMB_SC_SEMMASK;
	bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_SC,
	    sctl | PIIX_SMB_SC_CLRHOSTSEM);
}

static int
piixpm_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct piixpm_smbus *smbus = cookie;
	struct piixpm_softc *sc = smbus->softc;
	const uint8_t *b;
	uint8_t ctl = 0, st;
	int retries;

	DPRINTF(("%s: exec: op %d, addr 0x%02x, cmdlen %zu, len %zu, "
		"flags 0x%x\n",
		device_xname(sc->sc_dev), op, addr, cmdlen, len, flags));

	mutex_enter(&sc->sc_exec_lock);

	/* Clear status bits */
	bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HS,
	    PIIX_SMB_HS_INTR | PIIX_SMB_HS_DEVERR |
	    PIIX_SMB_HS_BUSERR | PIIX_SMB_HS_FAILED);
	bus_space_barrier(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HS, 1,
	    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);

	/* Wait for bus to be idle */
	for (retries = 100; retries > 0; retries--) {
		st = bus_space_read_1(sc->sc_smb_iot, sc->sc_smb_ioh,
		    PIIX_SMB_HS);
		if (!(st & PIIX_SMB_HS_BUSY))
			break;
		DELAY(PIIXPM_DELAY);
	}
	DPRINTF(("%s: exec: st %#x\n", device_xname(sc->sc_dev), st & 0xff));
	if (st & PIIX_SMB_HS_BUSY) {
		mutex_exit(&sc->sc_exec_lock);
		return (EBUSY);
	}

	if (sc->sc_poll)
		flags |= I2C_F_POLL;

	if (!I2C_OP_STOP_P(op) || cmdlen > 1 || len > 2 ||
	    (cmdlen == 0 && len > 1)) {
		mutex_exit(&sc->sc_exec_lock);
		return (EINVAL);
	}

	/* Setup transfer */
	sc->sc_i2c_xfer.op = op;
	sc->sc_i2c_xfer.buf = buf;
	sc->sc_i2c_xfer.len = len;
	sc->sc_i2c_xfer.flags = flags;
	sc->sc_i2c_xfer.error = 0;
	sc->sc_i2c_xfer.done = false;

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
		if (cmdlen == 0 && len == 1)
			bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh,
			    PIIX_SMB_HCMD, b[0]);
		else if (len > 0)
			bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh,
			    PIIX_SMB_HD0, b[0]);
		if (len > 1)
			bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh,
			    PIIX_SMB_HD1, b[1]);
	}

	/* Set SMBus command */
	if (cmdlen == 0) {
		if (len == 0)
			ctl = PIIX_SMB_HC_CMD_QUICK;
		else
			ctl = PIIX_SMB_HC_CMD_BYTE;
	} else if (len == 1)
		ctl = PIIX_SMB_HC_CMD_BDATA;
	else if (len == 2)
		ctl = PIIX_SMB_HC_CMD_WDATA;
	else
		panic("%s: unexpected len %zu", __func__, len);

	if ((flags & I2C_F_POLL) == 0)
		ctl |= PIIX_SMB_HC_INTREN;

	/* Start transaction */
	ctl |= PIIX_SMB_HC_START;
	bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HC, ctl);

	if (flags & I2C_F_POLL) {
		/* Poll for completion */
		if (PIIXPM_IS_CSB5(sc))
			DELAY(2*PIIXPM_DELAY);
		else
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
		while (! sc->sc_i2c_xfer.done) {
			if (cv_timedwait(&sc->sc_exec_wait, &sc->sc_exec_lock,
					 PIIXPM_TIMEOUT * hz))
				goto timeout;
		}
	}

	int error = sc->sc_i2c_xfer.error;
	mutex_exit(&sc->sc_exec_lock);

	return (error);

timeout:
	/*
	 * Transfer timeout. Kill the transaction and clear status bits.
	 */
	aprint_error_dev(sc->sc_dev, "timeout, status 0x%x\n", st);
	bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HC,
	    PIIX_SMB_HC_KILL);
	DELAY(PIIXPM_DELAY);
	st = bus_space_read_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HS);
	if ((st & PIIX_SMB_HS_FAILED) == 0)
		aprint_error_dev(sc->sc_dev,
		    "transaction abort failed, status 0x%x\n", st);
	bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HS, st);
	/*
	 * CSB5 needs hard reset to unlock the smbus after timeout.
	 */
	if (PIIXPM_IS_CSB5(sc))
		piixpm_csb5_reset(sc);
	mutex_exit(&sc->sc_exec_lock);
	return (ETIMEDOUT);
}

static int
piixpm_intr(void *arg)
{
	struct piixpm_softc *sc = arg;
	uint8_t st;
	uint8_t *b;
	size_t len;

	/* Read status */
	st = bus_space_read_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HS);
	if ((st & PIIX_SMB_HS_BUSY) != 0 || (st & (PIIX_SMB_HS_INTR |
	    PIIX_SMB_HS_DEVERR | PIIX_SMB_HS_BUSERR |
	    PIIX_SMB_HS_FAILED)) == 0)
		/* Interrupt was not for us */
		return (0);

	DPRINTF(("%s: intr st %#x\n", device_xname(sc->sc_dev), st & 0xff));

	if ((sc->sc_i2c_xfer.flags & I2C_F_POLL) == 0)
		mutex_enter(&sc->sc_exec_lock);

	/* Clear status bits */
	bus_space_write_1(sc->sc_smb_iot, sc->sc_smb_ioh, PIIX_SMB_HS, st);

	/* Check for errors */
	if (st & (PIIX_SMB_HS_DEVERR | PIIX_SMB_HS_BUSERR |
	    PIIX_SMB_HS_FAILED)) {
		sc->sc_i2c_xfer.error = EIO;
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
	sc->sc_i2c_xfer.done = true;
	if ((sc->sc_i2c_xfer.flags & I2C_F_POLL) == 0) {
		cv_signal(&sc->sc_exec_wait);
		mutex_exit(&sc->sc_exec_lock);
	}
	return (1);
}
