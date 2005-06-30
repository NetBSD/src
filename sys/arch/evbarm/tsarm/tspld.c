/*	$NetBSD: tspld.c,v 1.8 2005/06/30 17:03:53 drochner Exp $	*/

/*-
 * Copyright (c) 2004 Jesse Off
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tspld.c,v 1.8 2005/06/30 17:03:53 drochner Exp $");

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/autoconf.h>
#include "isa.h"
#if NISA > 0
#include <dev/isa/isavar.h>
#include <machine/isa_machdep.h>
#endif

#include <evbarm/tsarm/tsarmreg.h>
#include <evbarm/tsarm/tspldvar.h>
#include <arm/ep93xx/ep93xxvar.h>
#include <arm/arm32/machdep.h>
#include <arm/cpufunc.h>
#include <dev/sysmon/sysmonvar.h>

int	tspldmatch __P((struct device *, struct cfdata *, void *));
void	tspldattach __P((struct device *, struct device *, void *));
static int	tspld_wdog_setmode __P((struct sysmon_wdog *));
static int	tspld_wdog_tickle __P((struct sysmon_wdog *));
int tspld_search __P((struct device *, struct cfdata *,
		      const locdesc_t *, void *));
int tspld_print __P((void *, const char *));

struct tspld_softc {
        struct device           sc_dev;
        bus_space_tag_t         sc_iot;
	bus_space_handle_t	sc_wdogfeed_ioh;	
	bus_space_handle_t	sc_wdogctrl_ioh;	
	struct sysmon_wdog	sc_wdog;
	unsigned const char *	sc_com2mode;
	unsigned const char *	sc_model;
	unsigned char		sc_pldrev[4];
	uint32_t		sc_rs485;
	uint32_t		sc_adc;
	uint32_t		sc_jp[6];
	uint32_t		sc_blaster_present;
	uint32_t		sc_blaster_boot;
};

CFATTACH_DECL(tspld, sizeof(struct tspld_softc),
    tspldmatch, tspldattach, NULL, NULL);

void	tspld_callback __P((struct device *));

int
tspldmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	return 1;
}

void
tspldattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	int	i, rev, features, jp, model;
	struct tspld_softc *sc = (struct tspld_softc *)self;
	bus_space_handle_t 	ioh;
        const struct sysctlnode *node;

	if (sysctl_createv(NULL, 0, NULL, NULL,
				CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw",
				NULL, NULL, 0, NULL, 0,
				CTL_HW, CTL_EOL) != 0) {
		printf("%s: could not create sysctl\n",
			sc->sc_dev.dv_xname);
		return;
	}
	if (sysctl_createv(NULL, 0, NULL, &node,
        			0, CTLTYPE_NODE, "tspld",
        			NULL,
        			NULL, 0, NULL, 0,
				CTL_HW, CTL_CREATE, CTL_EOL) != 0) {
                printf("%s: could not create sysctl\n",
			sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_iot = &ep93xx_bs_tag;
        bus_space_map(sc->sc_iot, TS7XXX_IO16_HWBASE + TS7XXX_MODEL, 2, 0, 
		&ioh);
	model = bus_space_read_2(sc->sc_iot, ioh, 0) & 0x7;
	sc->sc_model = (model ? "TS-7250" : "TS-7200");
	if ((i = sysctl_createv(NULL, 0, NULL, NULL,
        			0, CTLTYPE_STRING, "boardmodel",
        			SYSCTL_DESCR("Technologic Systems board model"),
        			NULL, 0, __UNCONST(sc->sc_model), 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n", 
			sc->sc_dev.dv_xname);
		return;
	}
	bus_space_unmap(sc->sc_iot, ioh, 2);

        bus_space_map(sc->sc_iot, TS7XXX_IO16_HWBASE + TS7XXX_PLDREV, 2, 0, 
		&ioh);
	rev = bus_space_read_2(sc->sc_iot, ioh, 0) & 0x7;
	rev = 'A' + rev - 1;
	sc->sc_pldrev[0] = rev;
	sc->sc_pldrev[1] = 0;
	if ((i = sysctl_createv(NULL, 0, NULL, NULL,
        			0, CTLTYPE_STRING, "pldrev",
        			SYSCTL_DESCR("CPLD revision"),
        			NULL, 0, sc->sc_pldrev, 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n", 
			sc->sc_dev.dv_xname);
		return;
	}
	bus_space_unmap(sc->sc_iot, ioh, 2);

        bus_space_map(sc->sc_iot, TS7XXX_IO16_HWBASE + TS7XXX_FEATURES, 2, 0, 
		&ioh);
	features = bus_space_read_2(sc->sc_iot, ioh, 0) & 0x7;
	bus_space_unmap(sc->sc_iot, ioh, 2);

        bus_space_map(sc->sc_iot, TS7XXX_IO8_HWBASE + TS7XXX_STATUS1, 1, 0, 
		&ioh);
	i = bus_space_read_1(sc->sc_iot, ioh, 0) & 0x1f;
	jp = (~((i & 0x18) >> 1) & 0xc) | (i & 0x3);
	bus_space_unmap(sc->sc_iot, ioh, 1);

	if ((i = sysctl_createv(NULL, 0, NULL, NULL,
        			0, CTLTYPE_INT, "blaster_present",
        			SYSCTL_DESCR("Whether or not a TS-9420/TS-9202 blaster board is connected"),
        			NULL, 0, &sc->sc_blaster_present, 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n", 
			sc->sc_dev.dv_xname);
		return;
	}
	if ((i = sysctl_createv(NULL, 0, NULL, NULL,
        			0, CTLTYPE_INT, "blaster_boot",
        			SYSCTL_DESCR("Whether or not a blast board was used to boot"),
        			NULL, 0, &sc->sc_blaster_boot, 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n", 
			sc->sc_dev.dv_xname);
		return;
	}
        bus_space_map(sc->sc_iot, TS7XXX_IO16_HWBASE + TS7XXX_STATUS2, 2, 0, 
		&ioh);
	i = bus_space_read_2(sc->sc_iot, ioh, 0) & 0x1;
	sc->sc_blaster_boot = sc->sc_blaster_present = 0;
	if (i & 0x2)
		sc->sc_blaster_boot = 1;
	if (i & 0x4)	
		sc->sc_blaster_present = 1;
	jp |= (i << 4);
	bus_space_unmap(sc->sc_iot, ioh, 1);

	if ((i = sysctl_createv(NULL, 0, NULL, NULL,
        			0, CTLTYPE_INT, "rs485_avail",
        			SYSCTL_DESCR("RS485 level driver for COM2 available"),
        			NULL, 0, &sc->sc_rs485, 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n", 
			sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_com2mode = "rs232";
	if ((i = sysctl_createv(NULL, 0, NULL, NULL,
        			0, CTLTYPE_STRING, "com2_mode",
        			SYSCTL_DESCR("line driver type for COM2"),
        			NULL, 0, __UNCONST(sc->sc_com2mode), 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n", 
			sc->sc_dev.dv_xname);
		return;
	}
	if ((i = sysctl_createv(NULL, 0, NULL, NULL,
        			0, CTLTYPE_INT, "max197adc_avail",
        			SYSCTL_DESCR("Maxim 197 Analog to Digital Converter available"),
        			NULL, 0, &sc->sc_adc, 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n", 
			sc->sc_dev.dv_xname);
		return;
	}
	printf(": Technologic Systems %s rev %c, features 0x%x", 
		sc->sc_model, rev, features);
	sc->sc_adc = sc->sc_rs485 = 0;
	if (features == 0x1) {
		printf("<MAX197-ADC>");
		sc->sc_adc = 1;
	} else if (features == 0x2) {
		printf("<RS485>");
		sc->sc_rs485 = 1;
	} else if (features == 0x3) {
		printf("<MAX197-ADC,RS485>");
		sc->sc_adc = sc->sc_rs485 = 1;
	}
	printf("\n");
	if ((i = sysctl_createv(NULL, 0, NULL, NULL,
        			0, CTLTYPE_INT, "jp1",
        			SYSCTL_DESCR("onboard jumper setting"),
        			NULL, 0, &sc->sc_jp[0], 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n", 
			sc->sc_dev.dv_xname);
		return;
	}
	if ((i = sysctl_createv(NULL, 0, NULL, NULL,
        			0, CTLTYPE_INT, "jp2",
        			SYSCTL_DESCR("onboard jumper setting"),
        			NULL, 0, &sc->sc_jp[1], 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n", 
			sc->sc_dev.dv_xname);
		return;
	}
	if ((i = sysctl_createv(NULL, 0, NULL, NULL,
        			0, CTLTYPE_INT, "jp3",
        			SYSCTL_DESCR("onboard jumper setting"),
        			NULL, 0, &sc->sc_jp[2], 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n", 
			sc->sc_dev.dv_xname);
		return;
	}
	if ((i = sysctl_createv(NULL, 0, NULL, NULL,
        			0, CTLTYPE_INT, "jp4",
        			SYSCTL_DESCR("onboard jumper setting"),
        			NULL, 0, &sc->sc_jp[3], 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n", 
			sc->sc_dev.dv_xname);
		return;
	}
	if ((i = sysctl_createv(NULL, 0, NULL, NULL,
        			0, CTLTYPE_INT, "jp5",
        			SYSCTL_DESCR("onboard jumper setting"),
        			NULL, 0, &sc->sc_jp[4], 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n", 
			sc->sc_dev.dv_xname);
		return;
	}
	if ((i = sysctl_createv(NULL, 0, NULL, NULL,
        			0, CTLTYPE_INT, "jp6",
        			SYSCTL_DESCR("onboard jumper setting"),
        			NULL, 0, &sc->sc_jp[5], 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n", 
			sc->sc_dev.dv_xname);
		return;
	}
	printf("%s: jumpers 0x%x", sc->sc_dev.dv_xname, jp);
	if (jp) {
		printf("<");
		for(i = 0; i < 5; i++) {
			if (jp & (1 << i)) {
				sc->sc_jp[i + 1] = 1;
				printf("JP%d", i + 2);
				jp &= ~(1 << i);
				if (jp) printf(",");
			} else {
				sc->sc_jp[i + 2] = 0;
			}
		}
		printf(">");
	}
	printf("\n");

        bus_space_map(sc->sc_iot, TS7XXX_IO16_HWBASE + TS7XXX_WDOGCTRL, 2, 0, 
		&sc->sc_wdogctrl_ioh);
        bus_space_map(sc->sc_iot, TS7XXX_IO16_HWBASE + TS7XXX_WDOGFEED, 2, 0, 
		&sc->sc_wdogfeed_ioh);

	sc->sc_wdog.smw_name = sc->sc_dev.dv_xname;
	sc->sc_wdog.smw_cookie = sc;
	sc->sc_wdog.smw_setmode = tspld_wdog_setmode;
	sc->sc_wdog.smw_tickle = tspld_wdog_tickle;
	sc->sc_wdog.smw_period = 8;
	sysmon_wdog_register(&sc->sc_wdog);
	tspld_wdog_setmode(&sc->sc_wdog);


	/* Set the on board peripherals bus callback */
	config_defer(self, tspld_callback);
}

int
tspld_search(parent, cf, ldesc, aux)
	struct device *parent;
	struct cfdata *cf;
	const locdesc_t *ldesc;
	void *aux;
{
	struct tspld_softc *sc = (struct tspld_softc *)parent;
	struct tspld_attach_args sa;

	sa.ta_iot = sc->sc_iot;

	if (config_match(parent, cf, &sa) > 0)
		config_attach(parent, cf, &sa, tspld_print);

	return (0);
}

int
tspld_print(aux, name)
	void *aux;
	const char *name;
{

	return (UNCONF);
}

void
tspld_callback(self)
	struct device *self;
{
#if NISA > 0
	extern void isa_bs_mallocok(void);
	struct isabus_attach_args iba;

	/*
	 * Attach the ISA bus behind this bridge.
	 */
	memset(&iba, 0, sizeof(iba));
	iba.iba_iot = &isa_io_bs_tag;
	iba.iba_memt = &isa_mem_bs_tag;
	isa_bs_mallocok();
	config_found_ia(self, "isabus", &iba, isabusprint);
#endif
	/*
	 *  Attach each devices
	 */
	config_search_ia(tspld_search, self, "tspldbus", NULL);
	
}

static int
tspld_wdog_tickle(smw)
	struct sysmon_wdog *smw;
{
	struct tspld_softc *sc = (struct tspld_softc *)smw->smw_cookie;

	bus_space_write_2(sc->sc_iot, sc->sc_wdogfeed_ioh, 0, 0x5);
	return 0;
}

static int
tspld_wdog_setmode(smw)
	struct sysmon_wdog *smw;
{
	int i, ret = 0;
	struct tspld_softc *sc = (struct tspld_softc *)smw->smw_cookie;

	i = disable_interrupts(I32_bit|F32_bit);
	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		bus_space_write_2(sc->sc_iot, sc->sc_wdogfeed_ioh, 0, 0x5);
		bus_space_write_2(sc->sc_iot, sc->sc_wdogctrl_ioh, 0, 0);
	} else {
		bus_space_write_2(sc->sc_iot, sc->sc_wdogfeed_ioh, 0, 0x5);
		switch (smw->smw_period) {
		case 1:
			bus_space_write_2(sc->sc_iot, sc->sc_wdogctrl_ioh, 0,
				0x3);
			break;
		case 2:
			bus_space_write_2(sc->sc_iot, sc->sc_wdogctrl_ioh, 0,
				0x5);
			break;
		case 4:
			bus_space_write_2(sc->sc_iot, sc->sc_wdogctrl_ioh, 0,
				0x6);
			break;
		case 8:
			bus_space_write_2(sc->sc_iot, sc->sc_wdogctrl_ioh, 0,
				0x7);
			break;
		default:
			ret = EINVAL;
		}
	}
	restore_interrupts(i);
	return ret;
}
