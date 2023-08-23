/*	$NetBSD: ichsmb.c,v 1.81.4.3 2023/08/23 17:13:08 martin Exp $	*/
/*	$OpenBSD: ichiic.c,v 1.44 2020/10/07 11:23:05 jsg Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ichsmb.c,v 1.81.4.3 2023/08/23 17:13:08 martin Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/module.h>

#include <sys/bus.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/ic/i82801lpcreg.h>

#include <dev/i2c/i2cvar.h>

#include <x86/pci/tco.h>

#ifdef ICHIIC_DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

#define ICHIIC_DELAY	100
#define ICHIIC_TIMEOUT	1

struct ichsmb_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_size_t		sc_size;
	pci_chipset_tag_t	sc_pc;
	void *			sc_ih;
	int			sc_poll;
	pci_intr_handle_t	*sc_pihp;

	kmutex_t		sc_exec_lock;
	kcondvar_t		sc_exec_wait;

	struct i2c_controller	sc_i2c_tag;
	struct {
		i2c_op_t     op;
		void *       buf;
		size_t       len;
		int          flags;
		int          error;
		bool         done;
	}			sc_i2c_xfer;
	device_t		sc_i2c_device;

	bus_space_tag_t		sc_tcot;
	bus_space_handle_t	sc_tcoh;
	bus_size_t		sc_tcosz;
	bus_space_tag_t		sc_sbregt;
	bus_space_handle_t	sc_sbregh;
	bus_size_t		sc_sbregsz;
	bus_space_tag_t		sc_pmt;
	bus_space_handle_t	sc_pmh;
	bus_size_t		sc_pmsz;
	bool			sc_tco_probed;
	device_t		sc_tco_device;
};

static int	ichsmb_match(device_t, cfdata_t, void *);
static void	ichsmb_attach(device_t, device_t, void *);
static int	ichsmb_detach(device_t, int);
static int	ichsmb_rescan(device_t, const char *, const int *);
static void	ichsmb_chdet(device_t, device_t);

static void	ichsmb_probe_tco(struct ichsmb_softc *,
		    const struct pci_attach_args *);

static int	ichsmb_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
		    size_t, void *, size_t, int);

static int	ichsmb_intr(void *);

#include "ioconf.h"

CFATTACH_DECL3_NEW(ichsmb, sizeof(struct ichsmb_softc),
    ichsmb_match, ichsmb_attach, ichsmb_detach, NULL, ichsmb_rescan,
    ichsmb_chdet, DVF_DETACH_SHUTDOWN);


static int
ichsmb_match(device_t parent, cfdata_t match, void *aux)
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
		case PCI_PRODUCT_INTEL_82801I_SMB:
		case PCI_PRODUCT_INTEL_82801JD_SMB:
		case PCI_PRODUCT_INTEL_82801JI_SMB:
		case PCI_PRODUCT_INTEL_3400_SMB:
		case PCI_PRODUCT_INTEL_6SERIES_SMB:
		case PCI_PRODUCT_INTEL_7SERIES_SMB:
		case PCI_PRODUCT_INTEL_8SERIES_SMB:
		case PCI_PRODUCT_INTEL_9SERIES_SMB:
		case PCI_PRODUCT_INTEL_100SERIES_SMB:
		case PCI_PRODUCT_INTEL_100SERIES_LP_SMB:
		case PCI_PRODUCT_INTEL_2HS_SMB:
		case PCI_PRODUCT_INTEL_3HS_SMB:
		case PCI_PRODUCT_INTEL_3HS_U_SMB:
		case PCI_PRODUCT_INTEL_4HS_H_SMB:
		case PCI_PRODUCT_INTEL_4HS_V_SMB:
		case PCI_PRODUCT_INTEL_CORE4G_M_SMB:
		case PCI_PRODUCT_INTEL_CORE5G_M_SMB:
		case PCI_PRODUCT_INTEL_CMTLK_SMB:
		case PCI_PRODUCT_INTEL_BAYTRAIL_PCU_SMB:
		case PCI_PRODUCT_INTEL_BSW_PCU_SMB:
		case PCI_PRODUCT_INTEL_APL_SMB:
		case PCI_PRODUCT_INTEL_GLK_SMB:
		case PCI_PRODUCT_INTEL_EHL_SMB:
		case PCI_PRODUCT_INTEL_JSL_SMB:
		case PCI_PRODUCT_INTEL_SNR_SMB_LEGACY:
		case PCI_PRODUCT_INTEL_ADL_N_SMB:
		case PCI_PRODUCT_INTEL_C600_SMBUS:
		case PCI_PRODUCT_INTEL_C600_SMB_0:
		case PCI_PRODUCT_INTEL_C600_SMB_1:
		case PCI_PRODUCT_INTEL_C600_SMB_2:
		case PCI_PRODUCT_INTEL_C610_SMB:
		case PCI_PRODUCT_INTEL_C620_SMB:
		case PCI_PRODUCT_INTEL_C620_SMB_S:
		case PCI_PRODUCT_INTEL_EP80579_SMB:
		case PCI_PRODUCT_INTEL_DH89XXCC_SMB:
		case PCI_PRODUCT_INTEL_DH89XXCL_SMB:
		case PCI_PRODUCT_INTEL_C2000_PCU_SMBUS:
		case PCI_PRODUCT_INTEL_C3K_SMBUS_LEGACY:
		case PCI_PRODUCT_INTEL_495_YU_SMB:
		case PCI_PRODUCT_INTEL_5HS_H_SMB:
		case PCI_PRODUCT_INTEL_5HS_LP_SMB:
		case PCI_PRODUCT_INTEL_6HS_H_SMB:
		case PCI_PRODUCT_INTEL_6HS_LP_SMB:
		case PCI_PRODUCT_INTEL_7HS_SMB:
			return 1;
		}
	}
	return 0;
}

static void
ichsmb_attach(device_t parent, device_t self, void *aux)
{
	struct ichsmb_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	pcireg_t conf;
	const char *intrstr = NULL;
	char intrbuf[PCI_INTRSTR_LEN];

	sc->sc_dev = self;
	sc->sc_pc = pa->pa_pc;

	pci_aprint_devinfo(pa, NULL);

	mutex_init(&sc->sc_exec_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_exec_wait, device_xname(self));

	/* Read configuration */
	conf = pci_conf_read(pa->pa_pc, pa->pa_tag, SMB_HOSTC);
	DPRINTF(("%s: conf 0x%08x\n", device_xname(sc->sc_dev), conf));

	if ((conf & SMB_HOSTC_HSTEN) == 0) {
		aprint_error_dev(self, "SMBus disabled\n");
		goto out;
	}

	/* Map I/O space */
	if (pci_mapreg_map(pa, SMB_BASE, PCI_MAPREG_TYPE_IO, 0,
	    &sc->sc_iot, &sc->sc_ioh, NULL, &sc->sc_size)) {
		aprint_error_dev(self, "can't map I/O space\n");
		goto out;
	}

	sc->sc_poll = 1;
	sc->sc_ih = NULL;
	if (conf & SMB_HOSTC_SMIEN) {
		/* No PCI IRQ */
		aprint_normal_dev(self, "interrupting at SMI\n");
	} else {
		/* Install interrupt handler */
		if (pci_intr_alloc(pa, &sc->sc_pihp, NULL, 0) == 0) {
			intrstr = pci_intr_string(pa->pa_pc, sc->sc_pihp[0],
			    intrbuf, sizeof(intrbuf));
			pci_intr_setattr(pa->pa_pc, &sc->sc_pihp[0],
			    PCI_INTR_MPSAFE, true);
			sc->sc_ih = pci_intr_establish_xname(pa->pa_pc,
			    sc->sc_pihp[0], IPL_BIO, ichsmb_intr, sc,
			    device_xname(sc->sc_dev));
			if (sc->sc_ih != NULL) {
				aprint_normal_dev(self, "interrupting at %s\n",
				    intrstr);
				sc->sc_poll = 0;
			} else {
				pci_intr_release(pa->pa_pc, sc->sc_pihp, 1);
				sc->sc_pihp = NULL;
			}
		}
		if (sc->sc_poll)
			aprint_normal_dev(self, "polling\n");
	}

	/* Attach I2C bus */
	iic_tag_init(&sc->sc_i2c_tag);
	sc->sc_i2c_tag.ic_cookie = sc;
	sc->sc_i2c_tag.ic_exec = ichsmb_i2c_exec;

	/*
	 * Probe to see if there's a TCO hanging here instead of the
	 * LPCIB and map it if we can.
	 */
	ichsmb_probe_tco(sc, pa);

	sc->sc_i2c_device = NULL;
	ichsmb_rescan(self, NULL, NULL);

out:	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static void
ichsmb_probe_tco(struct ichsmb_softc *sc, const struct pci_attach_args *pa)
{
	const device_t self = sc->sc_dev;
	const pci_chipset_tag_t pc = sc->sc_pc;
	const pcitag_t p2sb_tag = pci_make_tag(pc,
	    /*bus*/0, /*dev*/0x1f, /*fn*/1);
	const pcitag_t pmc_tag = pci_make_tag(pc,
	    /*bus*/0, /*dev*/0x1f, /*fn*/2);
	pcireg_t tcoctl, tcobase, p2sbc, sbreglo, sbreghi;
	bus_addr_t sbreg, pmbase;
	int error = EIO;

	/*
	 * Only attempt this on devices where we expect to find a TCO.
	 */
	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_INTEL_100SERIES_LP_SMB:
	case PCI_PRODUCT_INTEL_100SERIES_SMB:
		break;
	default:
		goto fail;
	}

	/*
	 * Verify the TCO base address register is enabled.
	 */
	tcoctl = pci_conf_read(pa->pa_pc, pa->pa_tag, SMB_TCOCTL);
	aprint_debug_dev(self, "TCOCTL=0x%"PRIx32"\n", tcoctl);
	if ((tcoctl & SMB_TCOCTL_TCO_BASE_EN) == 0) {
		aprint_debug_dev(self, "TCO disabled\n");
		goto fail;
	}

	/*
	 * Verify the TCO base address register has the I/O space bit
	 * set -- otherwise we don't know how to interpret the
	 * register.
	 */
	tcobase = pci_conf_read(pa->pa_pc, pa->pa_tag, SMB_TCOBASE);
	aprint_debug_dev(self, "TCOBASE=0x%"PRIx32"\n", tcobase);
	if ((tcobase & SMB_TCOBASE_IOS) == 0) {
		aprint_error_dev(self, "unrecognized TCO space\n");
		goto fail;
	}

	/*
	 * Map the TCO I/O space.
	 */
	sc->sc_tcot = sc->sc_iot;
	error = bus_space_map(sc->sc_tcot, tcobase & SMB_TCOBASE_TCOBA,
	    TCO_REGSIZE, 0, &sc->sc_tcoh);
	if (error) {
		aprint_error_dev(self, "failed to map TCO: %d\n", error);
		goto fail;
	}
	sc->sc_tcosz = TCO_REGSIZE;

	/*
	 * Clear the Hide Device bit so we can map the SBREG_BAR from
	 * the P2SB registers; then restore the Hide Device bit so
	 * nobody else gets confused.
	 *
	 * XXX Hope nobody else is trying to touch the P2SB!
	 *
	 * XXX Should we have a way to lock PCI bus enumeration,
	 * e.g. from concurrent drvctl rescan?
	 *
	 * XXX pci_mapreg_info doesn't work to get the size, somehow
	 * comes out as 4.  Datasheet for 100-series chipset says the
	 * size is 16 MB, unconditionally, and the type is memory.
	 *
	 * XXX The above XXX comment was probably a result of PEBCAK
	 * when I tried to use 0xe4 instead of 0xe0 for P2SBC -- should
	 * try again with pci_mapreg_info or pci_mapreg_map.
	 */
	p2sbc = pci_conf_read(pc, p2sb_tag, P2SB_P2SBC);
	aprint_debug_dev(self, "P2SBC=0x%x\n", p2sbc);
	pci_conf_write(pc, p2sb_tag, P2SB_P2SBC, p2sbc & ~P2SB_P2SBC_HIDE);
	aprint_debug_dev(self, "P2SBC=0x%x -> 0x%x\n", p2sbc,
	    pci_conf_read(pc, p2sb_tag, P2SB_P2SBC));
	sbreglo = pci_conf_read(pc, p2sb_tag, P2SB_SBREG_BAR);
	sbreghi = pci_conf_read(pc, p2sb_tag, P2SB_SBREG_BARH);
	aprint_debug_dev(self, "SBREG_BAR=0x%08x 0x%08x\n", sbreglo, sbreghi);
	pci_conf_write(sc->sc_pc, p2sb_tag, P2SB_P2SBC, p2sbc);

	/*
	 * Map the sideband registers so we can touch the NO_REBOOT
	 * bit.
	 */
	sbreg = ((uint64_t)sbreghi << 32) | (sbreglo & ~__BITS(0,3));
	if (((uint64_t)sbreg >> 32) != sbreghi) {
		/* paranoia for 32-bit non-PAE */
		aprint_error_dev(self, "can't map 64-bit SBREG\n");
		goto fail;
	}
	sc->sc_sbregt = pa->pa_memt;
	error = bus_space_map(sc->sc_sbregt, sbreg + SB_SMBUS_BASE,
	    SB_SMBUS_SIZE, 0, &sc->sc_sbregh);
	if (error) {
		aprint_error_dev(self, "failed to map SMBUS sideband: %d\n",
		    error);
		goto fail;
	}
	sc->sc_sbregsz = SB_SMBUS_SIZE;

	/*
	 * Map the power management configuration controller's I/O
	 * space.  Older manual call this PMBASE for power management;
	 * newer manuals call it ABASE for ACPI.  The chapters
	 * describing the registers say `power management' and I can't
	 * find any connection to ACPI (I suppose ACPI firmware logic
	 * probably peeks and pokes registers here?) so we say PMBASE
	 * here.
	 *
	 * XXX Hope nobody else is trying to touch it!
	 */
	pmbase = pci_conf_read(pc, pmc_tag, LPCIB_PCI_PMBASE);
	aprint_debug_dev(self, "PMBASE=0x%"PRIxBUSADDR"\n", pmbase);
	if ((pmbase & 1) != 1) {	/* I/O space bit? */
		aprint_error_dev(self, "unrecognized PMC space\n");
		goto fail;
	}
	sc->sc_pmt = sc->sc_iot;
	error = bus_space_map(sc->sc_pmt, PCI_MAPREG_IO_ADDR(pmbase),
	    LPCIB_PCI_PM_SIZE, 0, &sc->sc_pmh);
	if (error) {
		aprint_error_dev(self, "failed to map PMC space: %d\n", error);
		goto fail;
	}
	sc->sc_pmsz = LPCIB_PCI_PM_SIZE;

	/* Success! */
	sc->sc_tco_probed = true;
	return;

fail:	if (sc->sc_pmsz) {
		bus_space_unmap(sc->sc_pmt, sc->sc_pmh, sc->sc_pmsz);
		sc->sc_pmsz = 0;
	}
	if (sc->sc_sbregsz) {
		bus_space_unmap(sc->sc_sbregt, sc->sc_sbregh, sc->sc_sbregsz);
		sc->sc_sbregsz = 0;
	}
	if (sc->sc_tcosz) {
		bus_space_unmap(sc->sc_tcot, sc->sc_tcoh, sc->sc_tcosz);
		sc->sc_tcosz = 0;
	}
}

static int
ichsmb_tco_set_noreboot(device_t tco, bool noreboot)
{
	device_t self = device_parent(tco);
	struct ichsmb_softc *sc = device_private(self);
	uint32_t gc, gc1;

	KASSERTMSG(tco == sc->sc_tco_device || sc->sc_tco_device == NULL,
	    "tco=%p child=%p", tco, sc->sc_tco_device);
	KASSERTMSG(device_is_a(self, "ichsmb"), "%s@%s",
	    device_xname(tco), device_xname(self));

	/*
	 * Try to clear the No Reboot bit.
	 */
	gc = bus_space_read_4(sc->sc_sbregt, sc->sc_sbregh, SB_SMBUS_GC);
	if (noreboot)
		gc |= SB_SMBUS_GC_NR;
	else
		gc &= ~SB_SMBUS_GC_NR;
	bus_space_write_4(sc->sc_sbregt, sc->sc_sbregh, SB_SMBUS_GC, gc);

	/*
	 * Check whether we could make it what we want.
	 */
	gc1 = bus_space_read_4(sc->sc_sbregt, sc->sc_sbregh, SB_SMBUS_GC);
	aprint_debug_dev(self, "gc=0x%x -> 0x%x\n", gc, gc1);
	if ((gc1 & SB_SMBUS_GC_NR) != (gc & SB_SMBUS_GC_NR))
		return ENODEV;
	return 0;
}

static int
ichsmb_rescan(device_t self, const char *ifattr, const int *locators)
{
	struct ichsmb_softc *sc = device_private(self);

	if (ifattr_match(ifattr, "i2cbus") && sc->sc_i2c_device == NULL) {
		struct i2cbus_attach_args iba;

		memset(&iba, 0, sizeof(iba));
		iba.iba_tag = &sc->sc_i2c_tag;
		sc->sc_i2c_device = config_found(self, &iba, iicbus_print,
		    CFARGS(.iattr = "i2cbus"));
	}
	if (sc->sc_tco_probed &&
	    ifattr_match(ifattr, "tcoichbus") &&
	    sc->sc_tco_device == NULL) {
		struct tco_attach_args ta;

		memset(&ta, 0, sizeof(ta));
		ta.ta_version = TCO_VERSION_SMBUS;
		ta.ta_pmt = sc->sc_pmt;
		ta.ta_pmh = sc->sc_pmh;
		ta.ta_tcot = sc->sc_tcot;
		ta.ta_tcoh = sc->sc_tcoh;
		ta.ta_set_noreboot = &ichsmb_tco_set_noreboot;
		sc->sc_tco_device = config_found(self, &ta, NULL,
		    CFARGS(.iattr = "tcoichbus"));
	}

	return 0;
}

static int
ichsmb_detach(device_t self, int flags)
{
	struct ichsmb_softc *sc = device_private(self);
	int error;

	error = config_detach_children(self, flags);
	if (error)
		return error;

	iic_tag_fini(&sc->sc_i2c_tag);

	if (sc->sc_ih) {
		pci_intr_disestablish(sc->sc_pc, sc->sc_ih);
		sc->sc_ih = NULL;
	}

	if (sc->sc_pihp) {
		pci_intr_release(sc->sc_pc, sc->sc_pihp, 1);
		sc->sc_pihp = NULL;
	}

	if (sc->sc_pmsz != 0)
		bus_space_unmap(sc->sc_pmt, sc->sc_pmh, sc->sc_pmsz);
	if (sc->sc_sbregsz != 0)
		bus_space_unmap(sc->sc_sbregt, sc->sc_sbregh, sc->sc_sbregsz);
	if (sc->sc_tcosz != 0)
		bus_space_unmap(sc->sc_tcot, sc->sc_tcoh, sc->sc_tcosz);
	if (sc->sc_size != 0)
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_size);

	mutex_destroy(&sc->sc_exec_lock);
	cv_destroy(&sc->sc_exec_wait);

	return 0;
}

static void
ichsmb_chdet(device_t self, device_t child)
{
	struct ichsmb_softc *sc = device_private(self);

	if (sc->sc_i2c_device == child)
		sc->sc_i2c_device = NULL;
	if (sc->sc_tco_device == child)
		sc->sc_tco_device = NULL;
}

static int
ichsmb_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct ichsmb_softc *sc = cookie;
	const uint8_t *b;
	uint8_t ctl = 0, st;
	int retries;
	char fbuf[64];

	DPRINTF(("%s: exec: op %d, addr 0x%02x, cmdlen %zu, len %zu, "
	    "flags 0x%02x\n", device_xname(sc->sc_dev), op, addr, cmdlen,
	    len, flags));

	mutex_enter(&sc->sc_exec_lock);

	/* Clear status bits */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, SMB_HS,
	    SMB_HS_INTR | SMB_HS_DEVERR | SMB_HS_BUSERR | SMB_HS_FAILED);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, SMB_HS, 1,
	    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);

	/* Wait for bus to be idle */
	for (retries = 100; retries > 0; retries--) {
		st = bus_space_read_1(sc->sc_iot, sc->sc_ioh, SMB_HS);
		if (!(st & SMB_HS_BUSY))
			break;
		DELAY(ICHIIC_DELAY);
	}
#ifdef ICHIIC_DEBUG
	snprintb(fbuf, sizeof(fbuf), SMB_HS_BITS, st);
	printf("%s: exec: st %s\n", device_xname(sc->sc_dev), fbuf);
#endif
	if (st & SMB_HS_BUSY) {
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
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, SMB_TXSLVA,
	    SMB_TXSLVA_ADDR(addr) |
	    (I2C_OP_READ_P(op) ? SMB_TXSLVA_READ : 0));

	b = (const uint8_t *)cmdbuf;
	if (cmdlen > 0)
		/* Set command byte */
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, SMB_HCMD, b[0]);

	if (I2C_OP_WRITE_P(op)) {
		/* Write data */
		b = buf;
		if (cmdlen == 0 && len == 1)
			bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			    SMB_HCMD, b[0]);
		else if (len > 0)
			bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			    SMB_HD0, b[0]);
		if (len > 1)
			bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			    SMB_HD1, b[1]);
	}

	/* Set SMBus command */
	if (cmdlen == 0) {
		if (len == 0)
			ctl = SMB_HC_CMD_QUICK;
		else
			ctl = SMB_HC_CMD_BYTE;
	} else if (len == 1)
		ctl = SMB_HC_CMD_BDATA;
	else if (len == 2)
		ctl = SMB_HC_CMD_WDATA;

	if ((flags & I2C_F_POLL) == 0)
		ctl |= SMB_HC_INTREN;

	/* Start transaction */
	ctl |= SMB_HC_START;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, SMB_HC, ctl);

	if (flags & I2C_F_POLL) {
		/* Poll for completion */
		DELAY(ICHIIC_DELAY);
		for (retries = 1000; retries > 0; retries--) {
			st = bus_space_read_1(sc->sc_iot, sc->sc_ioh, SMB_HS);
			if ((st & SMB_HS_BUSY) == 0)
				break;
			DELAY(ICHIIC_DELAY);
		}
		if (st & SMB_HS_BUSY)
			goto timeout;
		ichsmb_intr(sc);
	} else {
		/* Wait for interrupt */
		while (! sc->sc_i2c_xfer.done) {
			if (cv_timedwait(&sc->sc_exec_wait, &sc->sc_exec_lock,
					 ICHIIC_TIMEOUT * hz))
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
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, SMB_HC, SMB_HC_KILL);
	DELAY(ICHIIC_DELAY);
	st = bus_space_read_1(sc->sc_iot, sc->sc_ioh, SMB_HS);
	if ((st & SMB_HS_FAILED) == 0) {
		snprintb(fbuf, sizeof(fbuf), SMB_HS_BITS, st);
		device_printf(sc->sc_dev, "abort failed, status %s\n",
		    fbuf);
	}
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, SMB_HS, st);
	mutex_exit(&sc->sc_exec_lock);
	return (ETIMEDOUT);
}

static int
ichsmb_intr(void *arg)
{
	struct ichsmb_softc *sc = arg;
	uint8_t st;
	uint8_t *b;
	size_t len;
#ifdef ICHIIC_DEBUG
	char fbuf[64];
#endif

	/* Read status */
	st = bus_space_read_1(sc->sc_iot, sc->sc_ioh, SMB_HS);

	/* Clear status bits */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, SMB_HS, st);

	/* XXX Ignore SMBALERT# for now */
	if ((st & SMB_HS_BUSY) != 0 ||
	    (st & (SMB_HS_INTR | SMB_HS_DEVERR | SMB_HS_BUSERR |
		SMB_HS_FAILED | SMB_HS_BDONE)) == 0)
		/* Interrupt was not for us */
		return (0);

#ifdef ICHIIC_DEBUG
	snprintb(fbuf, sizeof(fbuf), SMB_HS_BITS, st);
	printf("%s: intr st %s\n", device_xname(sc->sc_dev), fbuf);
#endif

	if ((sc->sc_i2c_xfer.flags & I2C_F_POLL) == 0)
		mutex_enter(&sc->sc_exec_lock);

	/* Check for errors */
	if (st & (SMB_HS_DEVERR | SMB_HS_BUSERR | SMB_HS_FAILED)) {
		sc->sc_i2c_xfer.error = EIO;
		goto done;
	}

	if (st & SMB_HS_INTR) {
		if (I2C_OP_WRITE_P(sc->sc_i2c_xfer.op))
			goto done;

		/* Read data */
		b = sc->sc_i2c_xfer.buf;
		len = sc->sc_i2c_xfer.len;
		if (len > 0)
			b[0] = bus_space_read_1(sc->sc_iot, sc->sc_ioh,
			    SMB_HD0);
		if (len > 1)
			b[1] = bus_space_read_1(sc->sc_iot, sc->sc_ioh,
			    SMB_HD1);
	}

done:
	sc->sc_i2c_xfer.done = true;
	if ((sc->sc_i2c_xfer.flags & I2C_F_POLL) == 0) {
		cv_signal(&sc->sc_exec_wait);
		mutex_exit(&sc->sc_exec_lock);
	}
	return (1);
}

MODULE(MODULE_CLASS_DRIVER, ichsmb, "pci,iic");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
ichsmb_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_ichsmb,
		    cfattach_ioconf_ichsmb, cfdata_ioconf_ichsmb);
#endif
		break;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_ichsmb,
		    cfattach_ioconf_ichsmb, cfdata_ioconf_ichsmb);
#endif
		break;
	default:
#ifdef _MODULE
		error = ENOTTY;
#endif
		break;
	}

	return error;
}
