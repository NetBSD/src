/* $NetBSD: psm.c,v 1.3.16.1 2007/07/15 13:17:03 ad Exp $ */
/*
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Ported from Tadpole Solaris sources by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 
/*
 * Tadpole-RDI Ultrabook IIi (huxley) power management.  Note that
 * there is a lot of stuff still missing here, due in part to the confusion
 * that exists with the NetBSD power management framework.  I'm not wasting
 * time with APM at this point, and some of sysmon seems "lacking".
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: psm.c,v 1.3.16.1 2007/07/15 13:17:03 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/poll.h>
#include <sys/kauth.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

#include <dev/sysmon/sysmonvar.h>

#include <sparc64/dev/psmreg.h>

struct psm_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_memt;
	bus_space_handle_t	sc_memh;

	int			sc_event;
	int			sc_flags;
	struct sysmon_pswitch	sc_sm_pbutton;
	struct sysmon_pswitch	sc_sm_lid;
	struct sysmon_pswitch	sc_sm_ac;
	struct evcnt		sc_intrcnt;
	lwp_t			*sc_thread;
};

#define	PUT8(sc, r, v)		\
	bus_space_write_1(sc->sc_memt, sc->sc_memh, r, v)
#define	GET8(sc, r)		\
	bus_space_read_1(sc->sc_memt, sc->sc_memh, r)

#define	WAIT_DELAY		1000
#define	WAIT_RETRIES		1000

#define	RESET_DELAY		200
#define	CMD_DELAY		10
#define	CMD_RETRIES		5

#ifdef	DEBUG
#define	STATIC
#else
#define	STATIC	static
#endif

/* flags indicating state */
#define	PSM_FLAG_ACPWR		0x1
#define	PSM_FLAG_LIDCLOSED	0x2
#define	PSM_FLAG_DOCKED		0x4

/* system events -- causes activity in the event thread */
#define	PSM_EV_PBUTTON		0x1
#define	PSM_EV_LID		0x2
#define	PSM_EV_ACPWR		0x4
#define	PSM_EV_BATT		0x8
#define	PSM_EV_TEMP		0x10

STATIC void psm_sysmon_setup(struct psm_softc *);
STATIC void psm_event_thread(void *);
STATIC int psm_init(struct psm_softc *);
STATIC void psm_reset(struct psm_softc *);
STATIC void psm_poll_acpower(struct psm_softc *);
STATIC int psm_intr(void *);
STATIC int psm_misc_rd(struct psm_softc *, uint8_t, uint8_t *);
STATIC int psm_misc_wr(struct psm_softc *, uint8_t, uint8_t);
STATIC int psm_wait(struct psm_softc *, uint8_t);
#if 0
STATIC int psm_ecmd_rd16(struct psm_softc *, uint16_t *, uint8_t, uint8_t,
    uint8_t);
#endif
STATIC int psm_ecmd_rd8(struct psm_softc *, uint8_t *, uint8_t, uint8_t,
    uint8_t);
STATIC int psm_ecmd_wr8(struct psm_softc *, uint8_t, uint8_t, uint8_t,
    uint8_t);
STATIC int psm_match(struct device *, struct cfdata *, void *);
STATIC void psm_attach(struct device *, struct device *, void *);

CFATTACH_DECL(psm, sizeof(struct psm_softc),
    psm_match, psm_attach, NULL, NULL);


int
psm_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ebus_attach_args *ea = aux;

	if (strcmp(ea->ea_name, "psm") != 0)
		return (0);
	return (1);
}

void
psm_attach(struct device *parent, struct device *self, void *aux)
{
	struct psm_softc	*sc = (struct psm_softc *)self;
	struct ebus_attach_args	*ea = aux;
	bus_addr_t		devaddr;
	char			*xname;

	xname = sc->sc_dev.dv_xname;

	sc->sc_memt = ea->ea_bustag;
	devaddr = EBUS_ADDR_FROM_REG(&ea->ea_reg[0]);

	if (bus_space_map(sc->sc_memt, devaddr, ea->ea_reg[0].size,
		0, &sc->sc_memh) != 0) {
		printf(": unable to map device registers\n");
		return;
	}
	if (psm_init(sc) != 0) {
		printf(": unable to initialize device\n");
		return;
	}

	printf(": UltraBook IIi power control\n");

	psm_sysmon_setup(sc);

	if (kthread_create(PRI_NONE, 0, NULL, psm_event_thread, sc,
	    &sc->sc_thread, "%s", sc->sc_dev.dv_xname) != 0) {
		printf("%s: unable to create event kthread\n",
		    sc->sc_dev.dv_xname);
	}

	/*
	 * Establish device interrupts
	 */
	(void) bus_intr_establish(sc->sc_memt, ea->ea_intr[0], IPL_HIGH,
	    psm_intr, sc);
	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    sc->sc_dev.dv_xname, "intr");
}

/*
 * Register sensors and events with sysmon.
 */
void
psm_sysmon_setup(struct psm_softc *sc)
{
	const char	*xname	= sc->sc_dev.dv_xname;


	/*
	 * XXX: Register sysmon environment.
	 */

	/*
	 * Register sysmon events
	 */
	sc->sc_sm_pbutton.smpsw_name = xname;
	sc->sc_sm_pbutton.smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(&sc->sc_sm_pbutton) != 0)
		printf("%s: unable to register power button\n", xname);
	
	sc->sc_sm_lid.smpsw_name = xname;
	sc->sc_sm_lid.smpsw_type = PSWITCH_TYPE_LID;
	if (sysmon_pswitch_register(&sc->sc_sm_lid) != 0)
		printf("%s: unable to register lid switch\n", xname);

	sc->sc_sm_ac.smpsw_name = xname;
	sc->sc_sm_ac.smpsw_type = PSWITCH_TYPE_ACADAPTER;
	if (sysmon_pswitch_register(&sc->sc_sm_ac) != 0)
		printf("%s: unable to register AC adapter\n", xname);
}

void
psm_event_thread(void *arg)
{
	struct psm_softc *sc = arg;
	int	x;
	int	event;
	int	flags;

	for (;;) {
		x = splhigh();
		/* check for AC power.  sets event if there is a change */
		psm_poll_acpower(sc);

		/* read and clear events */
		event = sc->sc_event;
		flags = sc->sc_flags;
		sc->sc_event = 0;
		splx(x);

		if (event & PSM_EV_PBUTTON) {
			sysmon_pswitch_event(&sc->sc_sm_pbutton,
			    PSWITCH_EVENT_PRESSED);
		}

		if (event & PSM_EV_LID) {
			sysmon_pswitch_event(&sc->sc_sm_lid,
			    flags & PSM_FLAG_LIDCLOSED ?
			    PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED);
		}

		if (event & PSM_EV_ACPWR) {
			sysmon_pswitch_event(&sc->sc_sm_ac,
			    flags & PSM_FLAG_ACPWR ?
			    PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED);
		}

		/* XXX: handle PSM_EV_TEMP */

		/* one second interval between probes of power */
		tsleep(&sc, PWAIT, "psm", hz);
	}
}

int
psm_init(struct psm_softc *sc)
{
	int	x;
	uint8_t	batt = 0xff;	/* keep GCC 4.x happy */

	/* clear interrupts */
	x = splhigh();
	PUT8(sc, PSM_ICR, 0xff);
	splx(x);

	/* enable interrupts */
	if (psm_misc_wr(sc, PSM_MISC_IMR, PSM_IMR_ALL))
		return (-1);

	/* make sure that UPS battery is reasonable */
	if (psm_misc_rd(sc, PSM_MISC_UPS, &batt) || (batt > PSM_MAX_BATTERIES))
		if (psm_misc_wr(sc, PSM_MISC_UPS, batt))
			printf("%s: cannot set UPS battery",
			    sc->sc_dev.dv_xname);

	return (0);
}

void
psm_reset(struct psm_softc *sc)
{

	PUT8(sc, PSM_MCR, PSM_MCR_RST);
	delay(RESET_DELAY);
}

void
psm_poll_acpower(struct psm_softc *sc)
{
	int	flags = sc->sc_flags;

	if (GET8(sc, PSM_STAT) & PSM_STAT_AC) {
		sc->sc_flags |= PSM_FLAG_ACPWR;
	} else {
		sc->sc_flags &= ~PSM_FLAG_ACPWR;
	}
	if (flags != sc->sc_flags)
		sc->sc_event |= PSM_EV_ACPWR;
}

int
psm_misc_rd(struct psm_softc *sc, uint8_t mreg, uint8_t *data)
{

	return (psm_ecmd_rd8(sc, data, mreg, PSM_MODE_MISC, 0));
}

int
psm_misc_wr(struct psm_softc *sc, uint8_t mreg, uint8_t data)
{

	return (psm_ecmd_wr8(sc, data, mreg, PSM_MODE_MISC, 0));
}

int
psm_wait(struct psm_softc *sc, uint8_t flag)
{
    int retr = WAIT_RETRIES;

    while (GET8(sc, PSM_STAT) & flag) {
	    if (!(retr--)) {
		    return (-1);
	    }
	    delay(WAIT_DELAY);
    }
    return (0);
}

#if 0
int
psm_ecmd_rd16(struct psm_softc *sc, uint16_t *data, uint8_t iar, uint8_t mode,
    uint8_t addr)
{
	uint8_t	cmr = PSM_CMR_DATA(mode, PSM_L_16, PSM_D_RD, addr);
	int	x, rc, retr = CMD_RETRIES; 

	x = splhigh();

	do {
		if ((rc = psm_wait(sc, PSM_STAT_RDA)) != 0) {
			psm_reset(sc);
			continue;
		}

		PUT8(sc, PSM_IAR, iar);
		PUT8(sc, PSM_CMR, cmr);

		delay(CMD_DELAY);

		if ((rc = psm_wait(sc, PSM_STAT_RDA)) == 0) {
			*data = GET8(sc, PSM_PWDL) | (GET8(sc, PSM_PWDU) << 8);
			break;
		}

		psm_reset(sc);

	} while (--retr);

	splx(x);
	return (rc);
}
#endif

int
psm_ecmd_rd8(struct psm_softc *sc, uint8_t *data, uint8_t iar, uint8_t mode,
    uint8_t addr)
{
	uint8_t	cmr = PSM_CMR_DATA(mode, PSM_L_8, PSM_D_RD, addr);
	int	x, rc, retr = CMD_RETRIES; 

	x = splhigh();

	do {
		if ((rc = psm_wait(sc, PSM_STAT_RDA)) != 0) {
			psm_reset(sc);
			continue;
		}

		PUT8(sc, PSM_IAR, iar);
		PUT8(sc, PSM_CMR, cmr);

		delay(CMD_DELAY);

		if ((rc = psm_wait(sc, PSM_STAT_RDA)) == 0) {
			(void) GET8(sc, PSM_PWDU);
			*data = GET8(sc, PSM_PWDL);
			break;
		}

		psm_reset(sc);

	} while (--retr);

	splx(x);
	return (rc);
}

int
psm_ecmd_wr8(struct psm_softc *sc, uint8_t data, uint8_t iar, uint8_t mode,
    uint8_t addr)
{
	uint8_t	cmr = PSM_CMR_DATA(mode, PSM_L_8, PSM_D_WR, addr);
	int	x, rc, retr = CMD_RETRIES;

	x = splhigh();

	do {
		if ((rc = psm_wait(sc, PSM_STAT_WBF)) != 0) {
			psm_reset(sc);
			continue;
		}

		PUT8(sc, PSM_PWDU, 0);
		PUT8(sc, PSM_PWDL, data);
		PUT8(sc, PSM_IAR, iar);
		PUT8(sc, PSM_CMR, cmr);

		delay(CMD_DELAY);

		if ((rc = psm_wait(sc, PSM_STAT_WBF)) == 0) {
			break;
		}

		psm_reset(sc);
	} while (--retr);

	splx(x);

	return (rc);
}

int
psm_intr(void *arg)
{
	struct psm_softc *sc = arg;
	uint8_t	isr;

	isr = GET8(sc, PSM_ISR);
	if (isr & PSM_ISR_PO) {
		PUT8(sc, PSM_ICR, PSM_ISR_PO);
		sc->sc_event |= PSM_EV_PBUTTON;
	}
	if (isr & PSM_ISR_DK) {
		PUT8(sc, PSM_ICR, PSM_ISR_DK);
		sc->sc_flags |= PSM_FLAG_DOCKED;
	}
	if (isr & PSM_ISR_UDK) {
		PUT8(sc, PSM_ICR, PSM_ISR_UDK);
		sc->sc_flags &= ~PSM_FLAG_DOCKED;
	}
	if (isr & PSM_ISR_LIDC) {
		PUT8(sc, PSM_ICR, PSM_ISR_LIDC);
		sc->sc_flags |= PSM_FLAG_LIDCLOSED;
		sc->sc_event |= PSM_EV_LID;
	}
	if (isr & PSM_ISR_LIDO) {
		PUT8(sc, PSM_ICR, PSM_ISR_LIDO);
		sc->sc_flags &= ~PSM_FLAG_LIDCLOSED;
		sc->sc_event |= PSM_EV_LID;
	}
	if (isr & PSM_ISR_TMP) {
		/* Over temperature */
		PUT8(sc, PSM_ICR, PSM_ISR_TMP);
		sc->sc_event |= PSM_EV_TEMP;
	}
	if (isr & PSM_ISR_BCC) {
		/* battery config changed */
		PUT8(sc, PSM_ICR, PSM_ISR_BCC);
		sc->sc_event |= PSM_EV_BATT;
	}
	if (isr & PSM_ISR_RPD) {
		/* request to power down */
		sc->sc_event |= PSM_EV_PBUTTON;
	}
	if (sc->sc_event) {
		/* wake up the thread */
		wakeup(sc);
	}
	return (1);
}
