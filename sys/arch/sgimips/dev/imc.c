/*	$NetBSD: imc.c,v 1.17 2004/04/03 11:33:29 sekiya Exp $	*/

/*
 * Copyright (c) 2001 Rafal K. Boni
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imc.c,v 1.17 2004/04/03 11:33:29 sekiya Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>

#include <sgimips/dev/imcreg.h>

#include "locators.h"

struct imc_softc {
	struct device sc_dev;

	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	int eisa_present : 1;
};

static int	imc_match(struct device *, struct cfdata *, void *);
static void	imc_attach(struct device *, struct device *, void *);
static int	imc_print(void *, const char *);
void		imc_bus_reset(void);
void		imc_bus_error(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
void		imc_watchdog_reset(void);
void		imc_watchdog_disable(void);
void		imc_watchdog_enable(void);

CFATTACH_DECL(imc, sizeof(struct imc_softc),
    imc_match, imc_attach, NULL, NULL);

struct imc_attach_args {
	const char* iaa_name;

	bus_space_tag_t iaa_st;
	bus_space_handle_t iaa_sh;

/* ? */
	long	iaa_offset;
	int	iaa_intr;
#if 0
	int	iaa_stride;
#endif
};

struct imc_softc isc;

static int
imc_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	if ( (mach_type == MACH_SGI_IP22) || (mach_type == MACH_SGI_IP20) )
		return (1);

	return (0);
}

static void
imc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	u_int32_t reg;
	struct imc_attach_args iaa;
	struct mainbus_attach_args *ma = aux;
	u_int32_t sysid;

	isc.iot = SGIMIPS_BUS_SPACE_HPC;
	if (bus_space_map(isc.iot, ma->ma_addr, 0,
            BUS_SPACE_MAP_LINEAR, &isc.ioh))
                panic("imc_attach: could not allocate memory\n");

	platform.bus_reset = imc_bus_reset;
	platform.watchdog_reset = imc_watchdog_reset;
	platform.watchdog_disable = imc_watchdog_disable;
	platform.watchdog_enable = imc_watchdog_enable;

	sysid = bus_space_read_4(isc.iot, isc.ioh, IMC_SYSID);

	/* EISA present bit is on even on Indys, so don't trust it! */
	if (mach_subtype == MACH_SGI_IP22_FULLHOUSE)
		isc.eisa_present = (sysid & IMC_SYSID_HAVEISA);
	else
		isc.eisa_present = 0;

	printf(": revision %d", (sysid & IMC_SYSID_REVMASK));

	if (isc.eisa_present)
		printf(", EISA bus present");

	printf("\n");

	/* Clear CPU/GIO error status registers to clear any leftover bits. */
	imc_bus_reset();

	/* Hook the bus error handler into the ISR */
	platform.intr4 = imc_bus_error;

	/*
	 * Enable parity reporting on GIO/main memory transactions.
	 * Disable parity checking on CPU bus transactions (as turning
	 * it on seems to cause spurious bus errors), but enable parity
	 * checking on CPU reads from main memory (note that this bit
	 * has the opposite sense... Turning it on turns the checks off!).
	 * Finally, turn on interrupt writes to the CPU from the MC.
	 */
	reg = bus_space_read_4(isc.iot, isc.ioh, IMC_CPUCTRL0);
	reg &= ~IMC_CPUCTRL0_NCHKMEMPAR;
	reg |= (IMC_CPUCTRL0_GPR | IMC_CPUCTRL0_MPR | IMC_CPUCTRL0_INTENA);
	bus_space_write_4(isc.iot, isc.ioh, IMC_CPUCTRL0, reg);

	/* Setup the MC write buffer depth */
	reg = bus_space_read_4(isc.iot, isc.ioh, IMC_CPUCTRL1);
	reg = (reg & ~IMC_CPUCTRL1_MCHWMSK) | 13;
	if (mach_type == MACH_SGI_IP20)
		reg = (reg & ~IMC_CPUCTRL1_HPCLITTLE) | IMC_CPUCTRL1_HPCFX;
	bus_space_write_4(isc.iot, isc.ioh, IMC_CPUCTRL1, reg);


	/*
	 * Set GIO64 arbitrator configuration register:
	 *
	 * Preserve PROM-set graphics-related bits, as they seem to depend
	 * on the graphics variant present and I'm not sure how to figure
	 * that out or 100% sure what the correct settings are for each.
	 */
	reg = bus_space_read_4(isc.iot, isc.ioh, IMC_GIO64ARB);
	reg &= (IMC_GIO64ARB_GRX64 | IMC_GIO64ARB_GRXRT | IMC_GIO64ARB_GRXMST);

	/* GIO64 invariant for all IP22 platforms: one GIO bus, HPC1 @ 64 */
	reg |= IMC_GIO64ARB_ONEGIO | IMC_GIO64ARB_HPC64;

	/* Rest of settings are machine/board dependant */
	if (mach_type == MACH_SGI_IP20)
	{
	        reg |= (IMC_GIO64ARB_ONEGIO |
			 IMC_GIO64ARB_EXP1RT | IMC_GIO64ARB_EXP0RT);
                reg &= ~(IMC_GIO64ARB_HPC64 |
                         IMC_GIO64ARB_HPCEXP64 | IMC_GIO64ARB_EISA64 |
                         IMC_GIO64ARB_EXP064   | IMC_GIO64ARB_EXP164 |
                         IMC_GIO64ARB_EXP0PIPE | IMC_GIO64ARB_EXP1PIPE |
                         IMC_GIO64ARB_EXP0MST | IMC_GIO64ARB_EXP1MST);
/* XXX second ethernet adapter */
                reg |= IMC_GIO64ARB_EXP0MST;
	}
	else
	{
		switch (mach_subtype) {
		case MACH_SGI_IP22_GUINESS:
			/* EISA can bus-master, is 64-bit */
			reg |= (IMC_GIO64ARB_EISAMST | IMC_GIO64ARB_EISA64);
			break;

		case MACH_SGI_IP22_FULLHOUSE:
		/*
		 * All Fullhouse boards have a 64-bit HPC2 and pipelined
		 * EXP0 slot.
		 */
			reg |= (IMC_GIO64ARB_HPCEXP64 | IMC_GIO64ARB_EXP0PIPE);

			if (mach_boardrev < 2) {
			/* EXP0 realtime, EXP1 can master */
				reg |= (IMC_GIO64ARB_EXP0RT | IMC_GIO64ARB_EXP1MST);
			} else {
				/* EXP1 pipelined as well, EISA masters */
				reg |= (IMC_GIO64ARB_EXP1PIPE | IMC_GIO64ARB_EISAMST);
			}
			break;
		}
	}

	bus_space_write_4(isc.iot, isc.ioh, IMC_GIO64ARB, reg);

	if (isc.eisa_present) {
#if notyet
		memset(&iaa, 0, sizeof(iaa));

		iaa.iaa_name = "eisa";
		(void)config_found(self, (void*)&iaa, imc_print);
#endif
	}

	memset(&iaa, 0, sizeof(iaa));

	iaa.iaa_name = "gio";
	(void)config_found(self, (void*)&iaa, imc_print);

	imc_watchdog_enable();
}


static int
imc_print(aux, name)
	void *aux;
	const char *name;
{
	struct imc_attach_args* iaa = aux;

	if (name)
		aprint_normal("%s at %s", iaa->iaa_name, name);

	return UNCONF;
}

void
imc_bus_reset(void)
{
	bus_space_write_4(isc.iot, isc.ioh, IMC_CPU_ERRSTAT, 0);
	bus_space_write_4(isc.iot, isc.ioh, IMC_GIO_ERRSTAT, 0);
}

void
imc_bus_error(u_int32_t status, u_int32_t cause, u_int32_t pc, u_int32_t ipending)
{
	printf("bus error: cpu_stat %08x addr %08x, gio_stat %08x addr %08x\n",
			bus_space_read_4(isc.iot, isc.ioh, IMC_CPU_ERRSTAT),
			bus_space_read_4(isc.iot, isc.ioh, IMC_CPU_ERRADDR),
			bus_space_read_4(isc.iot, isc.ioh, IMC_GIO_ERRSTAT),
			bus_space_read_4(isc.iot, isc.ioh, IMC_GIO_ERRADDR) );
	imc_bus_reset();
}

void
imc_watchdog_reset(void)
{
	bus_space_write_4(isc.iot, isc.ioh, IMC_WDOG, 0);
}
void
imc_watchdog_disable(void)
{
	u_int32_t reg;

	bus_space_write_4(isc.iot, isc.ioh, IMC_WDOG, 0);
	reg = bus_space_read_4(isc.iot, isc.ioh, IMC_CPUCTRL0);
	reg &= ~(IMC_CPUCTRL0_WDOG);
        bus_space_write_4(isc.iot, isc.ioh, IMC_CPUCTRL0, reg);
}

void
imc_watchdog_enable(void)
{
	u_int32_t reg;

	/* enable watchdog and clear it */
	reg = bus_space_read_4(isc.iot, isc.ioh, IMC_CPUCTRL0);
	reg |= IMC_CPUCTRL0_WDOG;
	bus_space_write_4(isc.iot, isc.ioh, IMC_CPUCTRL0, reg);
	imc_watchdog_reset();
}
