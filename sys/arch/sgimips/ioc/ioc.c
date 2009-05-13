/* $NetBSD: ioc.c,v 1.7.34.1 2009/05/13 17:18:19 jym Exp $	 */

/*
 * Copyright (c) 2003 Christopher Sekiya
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * ip20/22/24 I/O Controller (IOC)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ioc.c,v 1.7.34.1 2009/05/13 17:18:19 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/machtype.h>

#include <sgimips/ioc/iocreg.h>
#include <sgimips/ioc/iocvar.h>

#include "locators.h"

struct ioc_softc {
	struct device   sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

static int      ioc_match(struct device *, struct cfdata *, void *);
static void     ioc_attach(struct device *, struct device *, void *);
#if defined(notyet)
static int      ioc_print(void *, const char *);
static int      ioc_search(struct device *, struct cfdata *,
			   const int *, void *);
#endif

CFATTACH_DECL(ioc, sizeof(struct ioc_softc),
	      ioc_match, ioc_attach, NULL, NULL);

#if defined(BLINK)
static callout_t ioc_blink_ch;
static void     ioc_blink(void *);
#endif

static int
ioc_match(struct device * parent, struct cfdata * match, void *aux)
{
	if (mach_type == MACH_SGI_IP22)
		return 1;

	return 0;
}

static void
ioc_attach(struct device * parent, struct device * self, void *aux)
{
	struct ioc_softc *sc = (struct ioc_softc *) self;
	struct mainbus_attach_args *maa = aux;
	u_int32_t       sysid;

#ifdef BLINK
	callout_init(&ioc_blink_ch, 0);
#endif

	sc->sc_iot = SGIMIPS_BUS_SPACE_HPC;

	if (bus_space_map(sc->sc_iot, maa->ma_addr, 0,
			  BUS_SPACE_MAP_LINEAR, &sc->sc_ioh))
		panic("ioc_attach: could not allocate memory\n");

	sysid = bus_space_read_4(sc->sc_iot, sc->sc_ioh, IOC_SYSID) & 0x01;

	if (sysid)
		mach_subtype = MACH_SGI_IP22_FULLHOUSE;
	else
		mach_subtype = MACH_SGI_IP22_GUINNESS;

	aprint_normal(": rev %d, machine %s, board rev %d\n",
		   ((sysid & IOC_SYSID_CHIPREV) >> IOC_SYSID_CHIPREV_SHIFT),
		    (sysid & IOC_SYSID_SYSTYPE) ? "Indigo2 (Fullhouse)" :
		    "Indy (Guinness)",
		   ((sysid & IOC_SYSID_BOARDREV) >> IOC_SYSID_BOARDREV_SHIFT));

	/* Reset IOC */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IOC_RESET,
			  IOC_RESET_PARALLEL | IOC_RESET_PCKBC |
			  IOC_RESET_EISA | IOC_RESET_ISDN |
			  IOC_RESET_LED_GREEN );

	/*
         * Set the 10BaseT port to use UTP cable, set autoselect mode for
         * the ethernet interface (AUI vs. TP), set the two serial ports
         * to PC mode.
         */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IOC_WRITE,
			  IOC_WRITE_ENET_AUTO | IOC_WRITE_ENET_UTP |
			  IOC_WRITE_PC_UART2 | IOC_WRITE_PC_UART1);

/* XXX: the firmware should have taken care of this already */
#if 0
	if (mach_subtype == MACH_SGI_IP22_GUINNESS) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, IOC_GCSEL, 0xff);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, IOC_GCREG, 0xff);
	}
#endif

#if defined(BLINK)
	ioc_blink(sc);
#endif

#if defined(notyet)
	/*
	 * pckbc, zstty, and lpt should attach under the IOC.  This begs the
	 * question of how we sort things out with ip20, which has no IOC.
	 * For now, we pretend that everything attaches at HPC and ignore
	 * the IOC.
	 */

	config_search_ia(ioc_search, self, "ioc", NULL);
#endif
}

#if defined(notyet)
static int
ioc_print(void *aux, const char *pnp)
{
	struct ioc_attach_args *iaa = aux;

	if (pnp != 0)
		return QUIET;

	if (iaa->iaa_offset != IOCCF_OFFSET_DEFAULT)
		aprint_normal(" offset 0x%lx", iaa->iaa_offset);
	if (iaa->iaa_intr != IOCCF_INTR_DEFAULT)
		aprint_normal(" intr %d", iaa->iaa_intr);

	return UNCONF;
}

static int
ioc_search(struct device * parent, struct cfdata * cf,
	   const int *ldesc, void *aux)
{
	struct ioc_softc *sc = (struct ioc_softc *) parent;
	struct ioc_attach_args iaa;
	int             tryagain;

	do {
		iaa.iaa_offset = cf->cf_loc[IOCCF_OFFSET];
		iaa.iaa_intr = cf->cf_loc[IOCCF_INTR];
		iaa.iaa_st = SGIMIPS_BUS_SPACE_HPC;
		iaa.iaa_sh = sc->sc_ioh;	/* XXX */

		tryagain = 0;
		if (config_match(parent, cf, &iaa) > 0) {
			config_attach(parent, cf, &iaa, ioc_print);
			tryagain = (cf->cf_fstate == FSTATE_STAR);
		}
	} while (tryagain);

	return 0;
}
#endif

#if defined(BLINK)
static void
ioc_blink(void *self)
{
	struct ioc_softc *sc = (struct ioc_softc *) self;
	register int    s;
	int             value;

	s = splhigh();

	/* This is a bit odd.  To strobe the green LED, we have to toggle the
	   red control bit. */

	value = bus_space_read_4(sc->sc_iot, sc->sc_ioh, IOC_RESET) & 0xff;
	value ^= IOC_RESET_LED_RED;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IOC_RESET, value);
	splx(s);
	/*
	 * Blink rate is:
	 *      full cycle every second if completely idle (loadav = 0)
	 *      full cycle every 2 seconds if loadav = 1
	 *      full cycle every 3 seconds if loadav = 2
	 * etc.
	 */
	s = (((averunnable.ldavg[0] + FSCALE) * hz) >> (FSHIFT + 1));
	callout_reset(&ioc_blink_ch, s, ioc_blink, sc);

}
#endif
