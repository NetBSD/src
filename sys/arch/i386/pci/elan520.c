/*	$NetBSD: elan520.c,v 1.8.16.3 2008/01/21 09:37:10 yamt Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Device driver for the AMD Elan SC520 System Controller.  This attaches
 * where the "pchb" driver might normally attach, and provides support for
 * extra features on the SC520, such as the watchdog timer and GPIO.
 *
 * Information about the GP bus echo bug work-around is from code posted
 * to the "soekris-tech" mailing list by Jasper Wallace.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: elan520.c,v 1.8.16.3 2008/01/21 09:37:10 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/mutex.h>
#include <sys/wdog.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>

#include <dev/pci/pcidevs.h>

#include "gpio.h"
#if NGPIO > 0
#include <dev/gpio/gpiovar.h>
#endif

#include <arch/i386/pci/elan520reg.h>

#include <dev/sysmon/sysmonvar.h>

#define	ELAN_IRQ	1
#define	IDT_PROT_SIZE	PAGE_SIZE

struct elansc_softc {
	struct device sc_dev;
	device_t sc_par;
	device_t sc_pex;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_tag;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	int sc_echobug;

	kmutex_t sc_mtx;

	struct sysmon_wdog sc_smw;
	void		*sc_eih;
	void		*sc_pih;
	void		*sc_sh;
	uint8_t		sc_mpicmode;
	uint8_t		sc_picicr;
	int		sc_idtpar;
	int		sc_textpar;
#if NGPIO > 0
	/* GPIO interface */
	struct gpio_chipset_tag sc_gpio_gc;
	gpio_pin_t sc_gpio_pins[ELANSC_PIO_NPINS];
#endif
};

int elansc_wpvnmi = 1;
int elansc_pcinmi = 1;
int elansc_do_protect_idt = 0;

#if NGPIO > 0
static int	elansc_gpio_pin_read(void *, int);
static void	elansc_gpio_pin_write(void *, int, int);
static void	elansc_gpio_pin_ctl(void *, int, int);
#endif

static void elansc_print_par(device_t, int, uint32_t);
static void elanpex_intr_establish(device_t, struct elansc_softc *);
static void elanpar_intr_establish(device_t, struct elansc_softc *);
static void elanpex_intr_disestablish(struct elansc_softc *);
static void elanpar_intr_disestablish(struct elansc_softc *);

static void
elansc_childdetached(device_t self, device_t child)
{
	struct elansc_softc *sc = device_private(self);

	if (child == sc->sc_par)
		sc->sc_par = NULL;
	if (child == sc->sc_pex)
		sc->sc_pex = NULL;
	/* elansc does not presently keep a pointer to 
	 * the gpio, so there is nothing to do if it is detached.
	 */
}

static void
elansc_wdogctl_write(struct elansc_softc *sc, uint16_t val)
{
	uint8_t echo_mode = 0; /* XXX: gcc */

	KASSERT(mutex_owned(&sc->sc_mtx));

	/* Switch off GP bus echo mode if we need to. */
	if (sc->sc_echobug) {
		echo_mode = bus_space_read_1(sc->sc_memt, sc->sc_memh,
		    MMCR_GPECHO);
		bus_space_write_1(sc->sc_memt, sc->sc_memh,
		    MMCR_GPECHO, echo_mode & ~GPECHO_GP_ECHO_ENB);
	}

	/* Unlock the register. */
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_WDTMRCTL,
	    WDTMRCTL_UNLOCK1);
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_WDTMRCTL,
	    WDTMRCTL_UNLOCK2);

	/* Write the value. */
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_WDTMRCTL, val);

	/* Switch GP bus echo mode back. */
	if (sc->sc_echobug)
		bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_GPECHO,
		    echo_mode);
}

static void
elansc_wdogctl_reset(struct elansc_softc *sc)
{
	uint8_t echo_mode = 0/* XXX: gcc */;

	KASSERT(mutex_owned(&sc->sc_mtx));

	/* Switch off GP bus echo mode if we need to. */
	if (sc->sc_echobug) {
		echo_mode = bus_space_read_1(sc->sc_memt, sc->sc_memh, 
		    MMCR_GPECHO); 
		bus_space_write_1(sc->sc_memt, sc->sc_memh,
		    MMCR_GPECHO, echo_mode & ~GPECHO_GP_ECHO_ENB); 
	}

	/* Reset the watchdog. */
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_WDTMRCTL,
	    WDTMRCTL_RESET1);
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_WDTMRCTL,
	    WDTMRCTL_RESET2);

	/* Switch GP bus echo mode back. */
	if (sc->sc_echobug)
		bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_GPECHO,
		    echo_mode);
}

static const struct {
	int	period;		/* whole seconds */
	uint16_t exp;		/* exponent select */
} elansc_wdog_periods[] = {
	{ 1,	WDTMRCTL_EXP_SEL25 },
	{ 2,	WDTMRCTL_EXP_SEL26 },
	{ 4,	WDTMRCTL_EXP_SEL27 },
	{ 8,	WDTMRCTL_EXP_SEL28 },
	{ 16,	WDTMRCTL_EXP_SEL29 },
	{ 32,	WDTMRCTL_EXP_SEL30 },
	{ 0,	0 },
};

static int
elansc_wdog_arm(struct elansc_softc *sc)
{
	struct sysmon_wdog *smw = &sc->sc_smw;
	int i;
	uint16_t exp_sel = 0; /* XXX: gcc */

	KASSERT(mutex_owned(&sc->sc_mtx));

	if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
		smw->smw_period = 32;
		exp_sel = WDTMRCTL_EXP_SEL30;
	} else {
		for (i = 0; elansc_wdog_periods[i].period != 0; i++) {
			if (elansc_wdog_periods[i].period ==
			    smw->smw_period) {
				exp_sel = elansc_wdog_periods[i].exp;
				break;
			}
		}
		if (elansc_wdog_periods[i].period == 0)
			return EINVAL;
	}
	elansc_wdogctl_write(sc, WDTMRCTL_ENB |
	    WDTMRCTL_WRST_ENB | exp_sel);
	elansc_wdogctl_reset(sc);
	return 0;
}

static int
elansc_wdog_setmode(struct sysmon_wdog *smw)
{
	struct elansc_softc *sc = smw->smw_cookie;
	int rc = 0;

	mutex_enter(&sc->sc_mtx);

	if (!device_is_active(&sc->sc_dev))
		rc = EBUSY;
	else if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		elansc_wdogctl_write(sc,
		    WDTMRCTL_WRST_ENB | WDTMRCTL_EXP_SEL30);
	} else
		rc = elansc_wdog_arm(sc);

	mutex_exit(&sc->sc_mtx);
	return rc;
}

static int
elansc_wdog_tickle(struct sysmon_wdog *smw)
{
	struct elansc_softc *sc = smw->smw_cookie;

	mutex_enter(&sc->sc_mtx);
	elansc_wdogctl_reset(sc);
	mutex_exit(&sc->sc_mtx);
	return 0;
}

static int
elansc_match(device_t parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_AMD &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_AMD_SC520_SC)
		return (10);	/* beat pchb */

	return (0);
}

static const char *elansc_speeds[] = {
	"(reserved 00)",
	"100MHz",
	"133MHz",
	"(reserved 11)",
};

static int
elanpar_intr(void *arg)
{
	struct elansc_softc *sc = arg;
	uint16_t wpvsta;
	unsigned win;
	uint32_t par;
	const char *wpvstr;

	wpvsta = bus_space_read_2(sc->sc_memt, sc->sc_memh, MMCR_WPVSTA);

	if ((wpvsta & MMCR_WPVSTA_WPV_STA) == 0)
		return 0;

	win = __SHIFTOUT(wpvsta, MMCR_WPVSTA_WPV_WINDOW);

	par = bus_space_read_4(sc->sc_memt, sc->sc_memh, MMCR_PAR(win));

	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_WPVSTA,
	    MMCR_WPVSTA_WPV_STA);

	switch (wpvsta & MMCR_WPVSTA_WPV_MSTR) {
	case MMCR_WPVSTA_WPV_MSTR_CPU:
		wpvstr = "cpu";
		break;
	case MMCR_WPVSTA_WPV_MSTR_PCI:
		wpvstr = "pci";
		break;
	case MMCR_WPVSTA_WPV_MSTR_GP:
		wpvstr = "gp";
		break;
	default:
		wpvstr = "unknown";
		break;
	}
	aprint_error_dev(sc->sc_par,
	    "%s violated write-protect window %u\n", wpvstr, win);
	elansc_print_par(sc->sc_par, win, par);
	return 0;
}

static int
elanpex_intr(void *arg)
{
	static struct {
		const char *string;
		bool nonfatal;
	} cmd[16] = {
		  [0] =	{.string = "not latched"}
		, [1] =	{.string = "special cycle"}
		, [2] =	{.string = "i/o read"}
		, [3] =	{.string = "i/o write"}
		, [4] =	{.string = "4"}
		, [5] =	{.string = "5"}
		, [6] =	{.string = "memory rd"}
		, [7] =	{.string = "memory wr"}
		, [8] =	{.string = "8"}
		, [9] =	{.string = "9"}
		, [10] = {.string = "cfg rd", .nonfatal = true}
		, [11] = {.string = "cfg wr"}
		, [12] = {.string = "memory rd mul"}
		, [13] = {.string = "dual-address cycle"}
		, [14] = {.string = "memory rd line"}
		, [15] = {.string = "memory wr & inv"}
	};

	static const struct {
		uint16_t bit;
		const char *msg;
	} mmsg[] = {
		  {MMCR_HBMSTIRQSTA_M_RTRTO_IRQ_STA, "retry timeout"}
		, {MMCR_HBMSTIRQSTA_M_TABRT_IRQ_STA, "target abort"} 
		, {MMCR_HBMSTIRQSTA_M_MABRT_IRQ_STA, "abort"} 
		, {MMCR_HBMSTIRQSTA_M_SERR_IRQ_STA, "system error"} 
		, {MMCR_HBMSTIRQSTA_M_RPER_IRQ_STA, "received parity error"} 
		, {MMCR_HBMSTIRQSTA_M_DPER_IRQ_STA, "detected parity error"}
	}, tmsg[] = {
		  {MMCR_HBTGTIRQSTA_T_DLYTO_IRQ_STA, "delayed txn timeout"}
		, {MMCR_HBTGTIRQSTA_T_APER_IRQ_STA, "address parity"}
		, {MMCR_HBTGTIRQSTA_T_DPER_IRQ_STA, "data parity"}
	};
	uint8_t pciarbsta;
	uint16_t mstcmd, mstirq, tgtid, tgtirq;
	uint32_t mstaddr;
	uint16_t mstack = 0, tgtack = 0;
	int fatal = 0, i, handled = 0;
	struct elansc_softc *sc = arg;

	pciarbsta = bus_space_read_1(sc->sc_memt, sc->sc_memh, MMCR_PCIARBSTA);
	mstirq = bus_space_read_2(sc->sc_memt, sc->sc_memh, MMCR_HBMSTIRQSTA);
	mstaddr = bus_space_read_4(sc->sc_memt, sc->sc_memh, MMCR_MSTINTADD);
	tgtirq = bus_space_read_2(sc->sc_memt, sc->sc_memh, MMCR_HBTGTIRQSTA);

	if ((pciarbsta & MMCR_PCIARBSTA_GNT_TO_STA) != 0) {
		aprint_error_dev(sc->sc_pex,
		    "grant time-out, GNT%" __PRIuBITS "# asserted\n",
		    __SHIFTOUT(pciarbsta, MMCR_PCIARBSTA_GNT_TO_ID));
		bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_PCIARBSTA,
		    MMCR_PCIARBSTA_GNT_TO_STA);
		handled = true;
	}

	mstcmd = __SHIFTOUT(mstirq, MMCR_HBMSTIRQSTA_M_CMD_IRQ_ID);

	for (i = 0; i < __arraycount(mmsg); i++) {
		if ((mstirq & mmsg[i].bit) == 0)
			continue;
		aprint_error_dev(sc->sc_pex,
		    "%s %08" PRIx32 " master %s\n",
		    cmd[mstcmd].string, mstaddr, mmsg[i].msg);

		mstack |= mmsg[i].bit;
		if (!cmd[mstcmd].nonfatal)
			fatal = true;
	}

	tgtid = __SHIFTOUT(tgtirq, MMCR_HBTGTIRQSTA_T_IRQ_ID);

	for (i = 0; i < __arraycount(tmsg); i++) {
		if ((tgtirq & tmsg[i].bit) == 0)
			continue;
		aprint_error_dev(sc->sc_pex, "%1x target %s\n", tgtid,
		    tmsg[i].msg);
		tgtack |= tmsg[i].bit;
	}

	/* acknowledge interrupts */
	if (tgtack != 0) {
		handled = true;
		bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_HBTGTIRQSTA,
		    tgtack);
	}
	if (mstack != 0) {
		handled = true;
		bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_HBMSTIRQSTA,
		    mstack);
	}
	return fatal ? 0 : (handled ? 1 : 0);
}

#define	elansc_print_1(__dev, __sc, __reg)				\
do {									\
	aprint_debug_dev(__dev,						\
	    "%s: %s %02" PRIx8 "\n", __func__, #__reg,			\
	    bus_space_read_1((__sc)->sc_memt, (__sc)->sc_memh, __reg));	\
} while (/*CONSTCOND*/0)

static void
elansc_print_par(device_t dev, int i, uint32_t par)
{
	uint32_t addr, sz, unit;
	const char *tgtstr;

	switch (par & MMCR_PAR_TARGET) {
	default:
	case MMCR_PAR_TARGET_OFF:
		tgtstr = "off";
		break;
	case MMCR_PAR_TARGET_GPIO:
		tgtstr = "gpio";
		break;
	case MMCR_PAR_TARGET_GPMEM:
		tgtstr = "gpmem";
		break;
	case MMCR_PAR_TARGET_PCI:
		tgtstr = "pci";
		break;
	case MMCR_PAR_TARGET_BOOTCS:
		tgtstr = "bootcs";
		break;
	case MMCR_PAR_TARGET_ROMCS1:
		tgtstr = "romcs1";
		break;
	case MMCR_PAR_TARGET_ROMCS2:
		tgtstr = "romcs2";
		break;
	case MMCR_PAR_TARGET_SDRAM:
		tgtstr = "sdram";
		break;
	}
	if ((par & MMCR_PAR_TARGET) == MMCR_PAR_TARGET_GPIO) {
		unit = 1;
		sz = __SHIFTOUT(par, MMCR_PAR_IO_SZ);
		addr = __SHIFTOUT(par, MMCR_PAR_IO_ST_ADR);
	} else if ((par & MMCR_PAR_PG_SZ) != 0) {
		unit = 64 * 1024;
		sz = __SHIFTOUT(par, MMCR_PAR_64KB_SZ);
		addr = __SHIFTOUT(par, MMCR_PAR_64KB_ST_ADR);
	} else {
		unit = 4 * 1024;
		sz = __SHIFTOUT(par, MMCR_PAR_4KB_SZ);
		addr = __SHIFTOUT(par, MMCR_PAR_4KB_ST_ADR);
	}

	aprint_debug_dev(dev,
	    "PAR[%d] %08" PRIx32 " tgt %s attr %1" __PRIxBITS
	    " start %08" PRIx32 " size %" PRIu32 "\n",
	    i, par, tgtstr, __SHIFTOUT(par, MMCR_PAR_ATTR),
	    addr * unit, (sz + 1) * unit);
}

static void
elansc_print_all_par(device_t dev,
    bus_space_tag_t memt, bus_space_handle_t memh)
{
	int i;
	uint32_t par;

	for (i = 0; i < 16; i++) {
		par = bus_space_read_4(memt, memh, MMCR_PAR(i));
		elansc_print_par(dev, i, par);
	}
}

static int
elansc_alloc_par(bus_space_tag_t memt, bus_space_handle_t memh)
{
	int i;
	uint32_t par;

	for (i = 0; i < 16; i++) {

		par = bus_space_read_4(memt, memh, MMCR_PAR(i));

		if ((par & MMCR_PAR_TARGET) == MMCR_PAR_TARGET_OFF)
			break;
	}
	if (i == 16)
		return -1;
	return i;
}

static void
elansc_disable_par(bus_space_tag_t memt, bus_space_handle_t memh, int idx)
{
	uint32_t par;
	par = bus_space_read_4(memt, memh, MMCR_PAR(idx));
	par &= ~MMCR_PAR_TARGET;
	par |= MMCR_PAR_TARGET_OFF;
	bus_space_write_4(memt, memh, MMCR_PAR(idx), par);
}

static int
elansc_protect_text(device_t self, struct elansc_softc *sc)
{
	int i;
	uint32_t par;
	uint32_t protsize, unprotsize;
	const uint32_t sfkb = 64 * 1024;
	paddr_t start_pa, end_pa;
	extern char kernel_text, etext;
	bus_space_tag_t memt;
	bus_space_handle_t memh;

	memt = sc->sc_memt;
	memh = sc->sc_memh;

	if (!pmap_extract(pmap_kernel(), (vaddr_t)&kernel_text, &start_pa) ||
	    !pmap_extract(pmap_kernel(), (vaddr_t)&etext, &end_pa))
		return -1;

	if (&etext - &kernel_text != end_pa - start_pa) {
		aprint_error_dev(self, "kernel text may not be contiguous\n");
		return -1;
	}

	if ((i = elansc_alloc_par(memt, memh)) == -1) {
		aprint_error_dev(self, "cannot allocate PAR\n");
		return -1;
	}

	par = bus_space_read_4(memt, memh, MMCR_PAR(i));

	aprint_debug_dev(self,
	    "protect kernel text at physical addresses %p - %p\n",
	    (void *)start_pa, (void *)end_pa);

	unprotsize = sfkb - start_pa % sfkb;
	start_pa += unprotsize;
	unprotsize += end_pa % sfkb;
	end_pa -= end_pa % sfkb;

	aprint_debug_dev(self,
	    "actually protect kernel text at physical addresses %p - %p\n",
	    (void *)start_pa, (void *)end_pa);

	aprint_verbose_dev(self,
	    "%" PRIu32 " bytes of kernel text are unprotected\n", unprotsize);

	protsize = end_pa - start_pa;

	/* clear PG_SZ, attribute, target, size, address. */
	par = MMCR_PAR_TARGET_SDRAM | MMCR_PAR_ATTR_NOWRITE | MMCR_PAR_PG_SZ;
	par |= __SHIFTIN(protsize / sfkb - 1, MMCR_PAR_64KB_SZ);
	par |= __SHIFTIN(start_pa / sfkb, MMCR_PAR_64KB_ST_ADR);
	bus_space_write_4(memt, memh, MMCR_PAR(i), par);
	return i;
}

static int
elansc_protect_idt(struct elansc_softc *sc)
{
	int i;
	uint32_t par;
	extern paddr_t idt_paddr;
	bus_space_tag_t memt;
	bus_space_handle_t memh;

	memt = sc->sc_memt;
	memh = sc->sc_memh;

	if (elansc_do_protect_idt == 0)
		return -1;

	if ((i = elansc_alloc_par(memt, memh)) == -1)
		return -1;

	par = bus_space_read_4(memt, memh, MMCR_PAR(i));

	aprint_debug_dev(sc->sc_par, "protect IDT at paddr %p\n",
	    (const void *)idt_paddr);

	/* clear PG_SZ, attribute, target, size, address. */
	par = MMCR_PAR_TARGET_SDRAM | MMCR_PAR_ATTR_NOWRITE;
	par |= __SHIFTIN(IDT_PROT_SIZE / PAGE_SIZE - 1, MMCR_PAR_4KB_SZ);
	par |= __SHIFTIN(idt_paddr / PAGE_SIZE, MMCR_PAR_4KB_ST_ADR);
	bus_space_write_4(memt, memh, MMCR_PAR(i), par);
	return i;
}

static void
elanpex_intr_ack(bus_space_tag_t memt, bus_space_handle_t memh)
{
	bus_space_write_1(memt, memh, MMCR_PCIARBSTA,
	    MMCR_PCIARBSTA_GNT_TO_STA);
	bus_space_write_2(memt, memh, MMCR_HBTGTIRQSTA, MMCR_TGTIRQ_ACT);
	bus_space_write_2(memt, memh, MMCR_HBMSTIRQSTA, MMCR_MSTIRQ_ACT);
}

static bool
elansc_suspend(device_t dev)
{
	bool rc;
	struct elansc_softc *sc = device_private(dev);

	mutex_enter(&sc->sc_mtx);
	rc = ((sc->sc_smw.smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED);
	mutex_exit(&sc->sc_mtx);
	if (!rc)
		aprint_debug_dev(dev, "watchdog enabled, suspend forbidden");
	return rc;
}

static bool
elansc_resume(device_t dev)
{
	struct elansc_softc *sc = device_private(dev);

	mutex_enter(&sc->sc_mtx);
	/* Set up the watchdog registers with some defaults. */
	elansc_wdogctl_write(sc, WDTMRCTL_WRST_ENB | WDTMRCTL_EXP_SEL30);

	/* ...and clear it. */
	elansc_wdogctl_reset(sc);
	mutex_exit(&sc->sc_mtx);

	return true;
}

static int
elansc_detach(device_t self, int flags)
{
	int rc;
	struct elansc_softc *sc = device_private(self);

	if ((rc = config_detach_children(self, flags)) != 0)
		return rc;

	pmf_device_deregister(self);

	if ((rc = sysmon_wdog_unregister(&sc->sc_smw)) != 0) {
		if (rc == ERESTART)
			rc = EINTR;
		return rc;
	}

	mutex_enter(&sc->sc_mtx);

	/* Set up the watchdog registers with some defaults. */
	elansc_wdogctl_write(sc, WDTMRCTL_WRST_ENB | WDTMRCTL_EXP_SEL30);

	/* ...and clear it. */
	elansc_wdogctl_reset(sc);

	bus_space_unmap(sc->sc_memt, sc->sc_memh, PAGE_SIZE);

	mutex_exit(&sc->sc_mtx);
	mutex_destroy(&sc->sc_mtx);
	return 0;
}

static void *
elansc_intr_establish(device_t dev, int (*handler)(void *), void *arg)
{
	struct pic *pic;
	void *ih;

	if ((pic = intr_findpic(ELAN_IRQ)) == NULL) {
		aprint_error_dev(dev, "PIC for irq %d not found\n",
		    ELAN_IRQ);
		return NULL;
	} else if ((ih = intr_establish(ELAN_IRQ, pic, ELAN_IRQ,
	    IST_LEVEL, IPL_HIGH, handler, arg)) == NULL) {
		aprint_error_dev(dev,
		    "could not establish interrupt\n");
		return NULL;
	}
	aprint_verbose_dev(dev, "interrupting at irq %d\n", ELAN_IRQ);
	return ih;
}

static bool
elanpex_resume(device_t self)
{
	struct elansc_softc *sc = device_private(device_parent(self));

	elanpex_intr_establish(self, sc);
	return sc->sc_eih != NULL;
}

static bool
elanpex_suspend(device_t self)
{
	struct elansc_softc *sc = device_private(device_parent(self));

	elanpex_intr_disestablish(sc);

	return true;
}

static bool
elanpar_resume(device_t self)
{
	struct elansc_softc *sc = device_private(device_parent(self));

	elanpar_intr_establish(self, sc);
	return sc->sc_pih != NULL;
}

static bool
elanpar_suspend(device_t self)
{
	struct elansc_softc *sc = device_private(device_parent(self));

	elanpar_intr_disestablish(sc->sc_pih);

	return true;
}

static void
elanpex_intr_establish(device_t self, struct elansc_softc *sc)
{
	uint8_t sysarbctl;
	uint16_t pcihostmap, mstirq, tgtirq;

	pcihostmap = bus_space_read_2(sc->sc_memt, sc->sc_memh,
	    MMCR_PCIHOSTMAP);
	/* Priority P2 (Master PIC IR1) */
	pcihostmap &= ~MMCR_PCIHOSTMAP_PCI_IRQ_MAP;
	pcihostmap |= __SHIFTIN(__BIT(ELAN_IRQ), MMCR_PCIHOSTMAP_PCI_IRQ_MAP);
	if (elansc_pcinmi)
		pcihostmap |= MMCR_PCIHOSTMAP_PCI_NMI_ENB;
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_PCIHOSTMAP,
	    pcihostmap);

	elanpex_intr_ack(sc->sc_memt, sc->sc_memh);

	sysarbctl = bus_space_read_1(sc->sc_memt, sc->sc_memh, MMCR_SYSARBCTL);
	mstirq = bus_space_read_2(sc->sc_memt, sc->sc_memh, MMCR_HBMSTIRQCTL);
	tgtirq = bus_space_read_2(sc->sc_memt, sc->sc_memh, MMCR_HBTGTIRQCTL);

	sysarbctl |= MMCR_SYSARBCTL_GNT_TO_INT_ENB;

	mstirq |= MMCR_HBMSTIRQCTL_M_RTRTO_IRQ_ENB;
	mstirq |= MMCR_HBMSTIRQCTL_M_TABRT_IRQ_ENB;
	mstirq |= MMCR_HBMSTIRQCTL_M_MABRT_IRQ_ENB;
	mstirq |= MMCR_HBMSTIRQCTL_M_SERR_IRQ_ENB;
	mstirq |= MMCR_HBMSTIRQCTL_M_RPER_IRQ_ENB;
	mstirq |= MMCR_HBMSTIRQCTL_M_DPER_IRQ_ENB;

	tgtirq |= MMCR_HBTGTIRQCTL_T_DLYTO_IRQ_ENB;
	tgtirq |= MMCR_HBTGTIRQCTL_T_APER_IRQ_ENB;
	tgtirq |= MMCR_HBTGTIRQCTL_T_DPER_IRQ_ENB;

	if (elansc_pcinmi) {
		sc->sc_eih = nmi_establish(elanpex_intr, sc);

		mstirq |= MMCR_HBMSTIRQCTL_M_RTRTO_IRQ_SEL;
		mstirq |= MMCR_HBMSTIRQCTL_M_TABRT_IRQ_SEL;
		mstirq |= MMCR_HBMSTIRQCTL_M_MABRT_IRQ_SEL;
		mstirq |= MMCR_HBMSTIRQCTL_M_SERR_IRQ_SEL;
		mstirq |= MMCR_HBMSTIRQCTL_M_RPER_IRQ_SEL;
		mstirq |= MMCR_HBMSTIRQCTL_M_DPER_IRQ_SEL;

		tgtirq |= MMCR_HBTGTIRQCTL_T_DLYTO_IRQ_SEL;
		tgtirq |= MMCR_HBTGTIRQCTL_T_APER_IRQ_SEL;
		tgtirq |= MMCR_HBTGTIRQCTL_T_DPER_IRQ_SEL;
	} else
		sc->sc_eih = elansc_intr_establish(self, elanpex_intr, sc);

	bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_SYSARBCTL, sysarbctl);
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_HBMSTIRQCTL, mstirq);
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_HBTGTIRQCTL, tgtirq);
}

static void
elanpex_attach(device_t parent, device_t self, void *aux)
{
	struct elansc_softc *sc = device_private(parent);

	aprint_naive(": PCI Exceptions\n");
	aprint_normal(": AMD Elan SC520 PCI Exceptions\n");

	elanpex_intr_establish(self, sc);

	aprint_debug_dev(self, "HBMSTIRQCTL %04x\n",
	    bus_space_read_2(sc->sc_memt, sc->sc_memh, MMCR_HBMSTIRQCTL));

	aprint_debug_dev(self, "HBTGTIRQCTL %04x\n",
	    bus_space_read_2(sc->sc_memt, sc->sc_memh, MMCR_HBTGTIRQCTL));

	aprint_debug_dev(self, "PCIHOSTMAP %04x\n",
	    bus_space_read_2(sc->sc_memt, sc->sc_memh, MMCR_PCIHOSTMAP));

	pci_conf_write(sc->sc_pc, sc->sc_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(sc->sc_pc, sc->sc_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_PARITY_ENABLE|PCI_COMMAND_SERR_ENABLE);

	if (!pmf_device_register(self, elanpex_suspend, elanpex_resume))
		aprint_error_dev(self, "could not establish power hooks\n");
}

static void
elanpex_intr_disestablish(struct elansc_softc *sc)
{
	uint8_t sysarbctl;
	uint16_t pcihostmap, mstirq, tgtirq;

	sysarbctl = bus_space_read_1(sc->sc_memt, sc->sc_memh, MMCR_SYSARBCTL);
	sysarbctl &= ~MMCR_SYSARBCTL_GNT_TO_INT_ENB;
	bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_SYSARBCTL, sysarbctl);

	mstirq = bus_space_read_2(sc->sc_memt, sc->sc_memh, MMCR_HBMSTIRQCTL);
	mstirq &= ~MMCR_HBMSTIRQCTL_M_RTRTO_IRQ_ENB;
	mstirq &= ~MMCR_HBMSTIRQCTL_M_TABRT_IRQ_ENB;
	mstirq &= ~MMCR_HBMSTIRQCTL_M_MABRT_IRQ_ENB;
	mstirq &= ~MMCR_HBMSTIRQCTL_M_SERR_IRQ_ENB;
	mstirq &= ~MMCR_HBMSTIRQCTL_M_RPER_IRQ_ENB;
	mstirq &= ~MMCR_HBMSTIRQCTL_M_DPER_IRQ_ENB;
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_HBMSTIRQCTL, mstirq);

	tgtirq = bus_space_read_2(sc->sc_memt, sc->sc_memh, MMCR_HBTGTIRQCTL);
	tgtirq &= ~MMCR_HBTGTIRQCTL_T_DLYTO_IRQ_ENB;
	tgtirq &= ~MMCR_HBTGTIRQCTL_T_APER_IRQ_ENB;
	tgtirq &= ~MMCR_HBTGTIRQCTL_T_DPER_IRQ_ENB;
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_HBTGTIRQCTL, tgtirq);

	pcihostmap = bus_space_read_2(sc->sc_memt, sc->sc_memh,
	    MMCR_PCIHOSTMAP);
	/* Priority P2 (Master PIC IR1) */
	pcihostmap &= ~MMCR_PCIHOSTMAP_PCI_IRQ_MAP;
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_PCIHOSTMAP,
	    pcihostmap);

	if (elansc_pcinmi)
		nmi_disestablish(sc->sc_eih);
	else
		intr_disestablish(sc->sc_eih);
	sc->sc_eih = NULL;

}

static int
elanpex_detach(device_t self, int flags)
{
	struct elansc_softc *sc = device_private(device_parent(self));

	pmf_device_deregister(self);
	elanpex_intr_disestablish(sc);

	return 0;
}

static void
elanpar_intr_establish(device_t self, struct elansc_softc *sc)
{
	uint8_t adddecctl, wpvmap;

	wpvmap = bus_space_read_1(sc->sc_memt, sc->sc_memh, MMCR_WPVMAP);
	wpvmap &= ~MMCR_WPVMAP_INT_MAP;
	if (elansc_wpvnmi)
		wpvmap |= MMCR_WPVMAP_INT_NMI;
	else
		wpvmap |= __SHIFTIN(__BIT(ELAN_IRQ), MMCR_WPVMAP_INT_MAP);
	bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_WPVMAP, wpvmap);

	/* clear interrupt status */
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_WPVSTA,
	    MMCR_WPVSTA_WPV_STA);

	/* establish interrupt */
	if (elansc_wpvnmi)
		sc->sc_pih = nmi_establish(elanpar_intr, sc);
	else
		sc->sc_pih = elansc_intr_establish(self, elanpar_intr, sc);

	adddecctl = bus_space_read_1(sc->sc_memt, sc->sc_memh, MMCR_ADDDECCTL);
	adddecctl |= MMCR_ADDDECCTL_WPV_INT_ENB;
	bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_ADDDECCTL, adddecctl);
}

static void
elanpar_attach(device_t parent, device_t self, void *aux)
{
	struct elansc_softc *sc = device_private(parent);

	aprint_naive(": Programmable Address Regions\n");
	aprint_normal(": AMD Elan SC520 Programmable Address Regions\n");

	elansc_print_1(self, sc, MMCR_WPVMAP);
	elansc_print_all_par(self, sc->sc_memt, sc->sc_memh);

	sc->sc_idtpar = elansc_protect_idt(sc);
	sc->sc_textpar = elansc_protect_text(self, sc);

	elanpar_intr_establish(self, sc);

	elansc_print_1(self, sc, MMCR_ADDDECCTL);

	if (!pmf_device_register(self, elanpar_suspend, elanpar_resume))
		aprint_error_dev(self, "could not establish power hooks\n");
}

static void
elanpar_intr_disestablish(struct elansc_softc *sc)
{
	uint8_t adddecctl, wpvmap;

	/* disable interrupt, acknowledge it, disestablish our
	 * handler, unmap it
	 */
	adddecctl = bus_space_read_1(sc->sc_memt, sc->sc_memh, MMCR_ADDDECCTL);
	adddecctl &= ~MMCR_ADDDECCTL_WPV_INT_ENB;
	bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_ADDDECCTL, adddecctl);

	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_WPVSTA,
	    MMCR_WPVSTA_WPV_STA);

	if (elansc_wpvnmi)
		nmi_disestablish(sc->sc_pih);
	else
		intr_disestablish(sc->sc_pih);
	sc->sc_pih = NULL;

	wpvmap = bus_space_read_1(sc->sc_memt, sc->sc_memh, MMCR_WPVMAP);
	wpvmap &= ~MMCR_WPVMAP_INT_MAP;
	bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_WPVMAP, wpvmap);
}

static int
elanpar_detach(device_t self, int flags)
{
	struct elansc_softc *sc = device_private(device_parent(self));

	pmf_device_deregister(self);

	if (sc->sc_textpar != -1) {
		elansc_disable_par(sc->sc_memt, sc->sc_memh, sc->sc_textpar);
		sc->sc_textpar = -1;
	}
	if (sc->sc_idtpar != -1) {
		elansc_disable_par(sc->sc_memt, sc->sc_memh, sc->sc_idtpar);
		sc->sc_idtpar = -1;
	}

	elanpar_intr_disestablish(sc);

	return 0;
}

static void
elansc_attach(device_t parent, device_t self, void *aux)
{
	struct elansc_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	uint16_t rev;
	uint8_t cpuctl, picicr, ressta;
#if 0
	struct pci_conf_state pcf;
#endif
#if NGPIO > 0
	struct gpiobus_attach_args gba;
	int pin, reg, shift;
	uint16_t data;
#endif
	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;

	aprint_naive(": System Controller\n");
	aprint_normal(": AMD Elan SC520 System Controller\n");

	sc->sc_memt = pa->pa_memt;
	if (bus_space_map(sc->sc_memt, MMCR_BASE_ADDR, PAGE_SIZE, 0,
	    &sc->sc_memh) != 0) {
		aprint_error_dev(&sc->sc_dev, "unable to map registers\n");
		return;
	}

	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_HIGH);

	rev = bus_space_read_2(sc->sc_memt, sc->sc_memh, MMCR_REVID);
	cpuctl = bus_space_read_1(sc->sc_memt, sc->sc_memh, MMCR_CPUCTL);

	aprint_normal_dev(&sc->sc_dev,
	    "product %d stepping %d.%d, CPU clock %s\n",
	    (rev & REVID_PRODID) >> REVID_PRODID_SHIFT,
	    (rev & REVID_MAJSTEP) >> REVID_MAJSTEP_SHIFT,
	    (rev & REVID_MINSTEP),
	    elansc_speeds[cpuctl & CPUCTL_CPU_CLK_SPD_MASK]);

	/*
	 * SC520 rev A1 has a bug that affects the watchdog timer.  If
	 * the GP bus echo mode is enabled, writing to the watchdog control
	 * register is blocked.
	 *
	 * The BIOS in some systems (e.g. the Soekris net4501) enables
	 * GP bus echo for various reasons, so we need to switch it off
	 * when we talk to the watchdog timer.
	 *
	 * XXX The step 1.1 (B1?) in my Soekris net4501 also has this
	 * XXX problem, so we'll just enable it for all Elan SC520s
	 * XXX for now.  --thorpej@NetBSD.org
	 */
	if (1 || rev == ((PRODID_ELAN_SC520 << REVID_PRODID_SHIFT) |
		    (0 << REVID_MAJSTEP_SHIFT) | (1)))
		sc->sc_echobug = 1;

	/*
	 * Determine cause of the last reset, and issue a warning if it
	 * was due to watchdog expiry.
	 */
	ressta = bus_space_read_1(sc->sc_memt, sc->sc_memh, MMCR_RESSTA);
	if (ressta & RESSTA_WDT_RST_DET)
		aprint_error_dev(&sc->sc_dev,
		    "WARNING: LAST RESET DUE TO WATCHDOG EXPIRATION!\n");
	bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_RESSTA, ressta);

	elansc_print_1(self, sc, MMCR_MPICMODE);
	elansc_print_1(self, sc, MMCR_SL1PICMODE);
	elansc_print_1(self, sc, MMCR_SL2PICMODE);
	elansc_print_1(self, sc, MMCR_PICICR);

	sc->sc_mpicmode = bus_space_read_1(sc->sc_memt, sc->sc_memh,
	    MMCR_MPICMODE);
	bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_MPICMODE,
	    sc->sc_mpicmode | __BIT(ELAN_IRQ));

	sc->sc_picicr = bus_space_read_1(sc->sc_memt, sc->sc_memh, MMCR_PICICR);
	picicr = sc->sc_picicr;
	if (elansc_pcinmi || elansc_wpvnmi)
		picicr |= MMCR_PICICR_NMI_ENB;
#if 0
	/* PC/AT compatibility */
	picicr |= MMCR_PICICR_S1_GINT_MODE|MMCR_PICICR_M_GINT_MODE;
#endif
	bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_PICICR, picicr);

	elansc_print_1(self, sc, MMCR_PICICR);
	elansc_print_1(self, sc, MMCR_MPICMODE);

	mutex_enter(&sc->sc_mtx);
	/* Set up the watchdog registers with some defaults. */
	elansc_wdogctl_write(sc, WDTMRCTL_WRST_ENB | WDTMRCTL_EXP_SEL30);

	/* ...and clear it. */
	elansc_wdogctl_reset(sc);
	mutex_exit(&sc->sc_mtx);

	if (!pmf_device_register(self, elansc_suspend, elansc_resume))
		aprint_error_dev(self, "could not establish power hooks\n");

#if NGPIO > 0
	/* Initialize GPIO pins array */
	for (pin = 0; pin < ELANSC_PIO_NPINS; pin++) {
		sc->sc_gpio_pins[pin].pin_num = pin;
		sc->sc_gpio_pins[pin].pin_caps = GPIO_PIN_INPUT |
		    GPIO_PIN_OUTPUT;

		/* Read initial state */
		reg = (pin < 16 ? MMCR_PIODIR15_0 : MMCR_PIODIR31_16);
		shift = pin % 16;
		data = bus_space_read_2(sc->sc_memt, sc->sc_memh, reg);
		if ((data & (1 << shift)) == 0)
			sc->sc_gpio_pins[pin].pin_flags = GPIO_PIN_INPUT;
		else
			sc->sc_gpio_pins[pin].pin_flags = GPIO_PIN_OUTPUT;
		if (elansc_gpio_pin_read(sc, pin) == 0)
			sc->sc_gpio_pins[pin].pin_state = GPIO_PIN_LOW;
		else
			sc->sc_gpio_pins[pin].pin_state = GPIO_PIN_HIGH;
	}

	/* Create controller tag */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = elansc_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = elansc_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = elansc_gpio_pin_ctl;

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = ELANSC_PIO_NPINS;

	sc->sc_par = config_found_ia(&sc->sc_dev, "elanparbus", NULL, NULL);
	sc->sc_pex = config_found_ia(&sc->sc_dev, "elanpexbus", NULL, NULL);
	/* Attach GPIO framework */
	config_found_ia(&sc->sc_dev, "gpiobus", &gba, gpiobus_print);
#endif /* NGPIO */

	/*
	 * Hook up the watchdog timer.
	 */
	sc->sc_smw.smw_name = device_xname(&sc->sc_dev);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = elansc_wdog_setmode;
	sc->sc_smw.smw_tickle = elansc_wdog_tickle;
	sc->sc_smw.smw_period = 32;	/* actually 32.54 */
	if (sysmon_wdog_register(&sc->sc_smw) != 0) {
		aprint_error_dev(&sc->sc_dev,
		    "unable to register watchdog with sysmon\n");
	}
}

static int
elanpex_match(device_t parent, struct cfdata *match, void *aux)
{
	struct elansc_softc *sc = device_private(parent);

	return sc->sc_pex == NULL;
}

static int
elanpar_match(device_t parent, struct cfdata *match, void *aux)
{
	struct elansc_softc *sc = device_private(parent);

	return sc->sc_par == NULL;
}

CFATTACH_DECL_NEW(elanpar, sizeof(struct device),
    elanpar_match, elanpar_attach, elanpar_detach, NULL);

CFATTACH_DECL_NEW(elanpex, sizeof(struct device),
    elanpex_match, elanpex_attach, elanpex_detach, NULL);

CFATTACH_DECL2(elansc, sizeof(struct elansc_softc),
    elansc_match, elansc_attach, elansc_detach, NULL, NULL,
    elansc_childdetached);

#if NGPIO > 0
static int
elansc_gpio_pin_read(void *arg, int pin)
{
	struct elansc_softc *sc = arg;
	int reg, shift;
	uint16_t data;

	reg = (pin < 16 ? MMCR_PIODATA15_0 : MMCR_PIODATA31_16);
	shift = pin % 16;

	mutex_enter(&sc->sc_mtx);
	data = bus_space_read_2(sc->sc_memt, sc->sc_memh, reg);
	mutex_exit(&sc->sc_mtx);

	return ((data >> shift) & 0x1);
}

static void
elansc_gpio_pin_write(void *arg, int pin, int value)
{
	struct elansc_softc *sc = arg;
	int reg, shift;
	uint16_t data;

	reg = (pin < 16 ? MMCR_PIODATA15_0 : MMCR_PIODATA31_16);
	shift = pin % 16;

	mutex_enter(&sc->sc_mtx);
	data = bus_space_read_2(sc->sc_memt, sc->sc_memh, reg);
	if (value == 0)
		data &= ~(1 << shift);
	else if (value == 1)
		data |= (1 << shift);

	bus_space_write_2(sc->sc_memt, sc->sc_memh, reg, data);
	mutex_exit(&sc->sc_mtx);
}

static void
elansc_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct elansc_softc *sc = arg;
	int reg, shift;
	uint16_t data;

	reg = (pin < 16 ? MMCR_PIODIR15_0 : MMCR_PIODIR31_16);
	shift = pin % 16;
	mutex_enter(&sc->sc_mtx);
	data = bus_space_read_2(sc->sc_memt, sc->sc_memh, reg);
	if (flags & GPIO_PIN_INPUT)
		data &= ~(1 << shift);
	if (flags & GPIO_PIN_OUTPUT)
		data |= (1 << shift);

	bus_space_write_2(sc->sc_memt, sc->sc_memh, reg, data);
	mutex_exit(&sc->sc_mtx);
}
#endif /* NGPIO */
