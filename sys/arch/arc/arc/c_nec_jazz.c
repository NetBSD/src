/*	$NetBSD: c_nec_jazz.c,v 1.1 2001/06/13 15:21:52 soda Exp $	*/

/*-
 * Copyright (C) 2000 Shuichiro URATA.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * for NEC EISA and NEC PCI platforms
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/pio.h>
#include <machine/platform.h>

#include <dev/isa/isavar.h>

#include <arc/jazz/rd94.h>
#include <arc/jazz/jazziovar.h>
#include <arc/dev/mcclockvar.h>
#include <arc/jazz/mcclock_jazziovar.h>
#include <arc/jazz/timer_jazziovar.h>

extern int cpu_int_mask;

/*
 * chipset-dependent mcclock routines.
 */

u_int	mc_nec_jazz_read __P((struct mcclock_softc *, u_int));
void	mc_nec_jazz_write __P((struct mcclock_softc *, u_int, u_int));

struct mcclock_jazzio_config mcclock_nec_jazz_conf = {
	0x80004000, 2,
	{ mc_nec_jazz_read, mc_nec_jazz_write }
};

u_int
mc_nec_jazz_read(sc, reg)
	struct mcclock_softc *sc;
	u_int reg;
{
	int i, as;

	as = bus_space_read_1(sc->sc_iot, sc->sc_ioh, 1) & 0x80;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, 1, as | reg);
	i = bus_space_read_1(sc->sc_iot, sc->sc_ioh, 0);
	return (i);
}

void
mc_nec_jazz_write(sc, reg, datum)
	struct mcclock_softc *sc;
	u_int reg, datum;
{
	int as;

	as = bus_space_read_1(sc->sc_iot, sc->sc_ioh, 1) & 0x80;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, 1, as | reg);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, 0, datum);
}

/*
 * chipset-dependent timer routine.
 */

int timer_nec_jazz_intr __P((u_int, struct clockframe *));
void timer_nec_jazz_init __P((int));

struct timer_jazzio_config timer_nec_jazz_conf = {
	MIPS_INT_MASK_3,
	timer_nec_jazz_intr,
	timer_nec_jazz_init,
};

/* handle jazzio bus clock interrupt */
int
timer_nec_jazz_intr(mask, cf)
	u_int mask;
	struct clockframe *cf;
{
	int temp;

	temp = in32(RD94_SYS_INTSTAT3);
	hardclock(cf);

	/* Re-enable clock interrupts */
	splx(MIPS_INT_MASK_3 | MIPS_SR_INT_IE);

	return (~MIPS_INT_MASK_3); /* Keep clock interrupts enabled */
}

void
timer_nec_jazz_init(interval)
	int interval; /* milliseconds */
{
	if (interval <= 0)
		panic("timer_nec_jazz_init: invalid interval %d", interval);

	out32(RD94_SYS_IT_VALUE, interval - 1);

	/* Enable periodic clock interrupt */
	out32(RD94_SYS_EXT_IMASK, cpu_int_mask);
}

/*
 * chipset-dependent jazzio bus configuration
 */

struct pica_dev nec_rd94_cpu[] = {
	{{ "timer",	-1, 0, },	(void *)RD94_SYS_IT_VALUE, },
	{{ "dallas_rtc", -1, 0, },	(void *)RD94_SYS_CLOCK, },
	{{ "lpt",	0, 0, },	(void *)RD94_SYS_PAR1, },
	{{ "fdc",	1, 0, },	(void *)RD94_SYS_FLOPPY, },
	{{ "AD1848",	2, 0, },	(void *)RD94_SYS_SOUND,},
	{{ "sonic",	3, 0, },	(void *)RD94_SYS_SONIC, },
	{{ "NCRC700",	4, 0, },	(void *)RD94_SYS_SCSI0, },
	{{ "NCRC700",	5, 0, },	(void *)RD94_SYS_SCSI1, },
	{{ "pckbd",	6, 0, },	(void *)RD94_SYS_KBD, },
	{{ "pms",	7, 0, },	(void *)RD94_SYS_KBD, },
	{{ "com",	8, 0, },	(void *)RD94_SYS_COM1, },
	{{ "com",	9, 0, },	(void *)RD94_SYS_COM2, },
	{{ NULL,	-1, 0, },	(void *)NULL, },
};

void
c_nec_jazz_set_intr(mask, int_hand, prio)
	int	mask;
	int	(*int_hand)(u_int, struct clockframe *);
	int	prio;
{
	arc_set_intr(mask, int_hand, prio);

	/* Update external interrupt mask but don't enable clock. */
	out32(RD94_SYS_EXT_IMASK, cpu_int_mask & (~MIPS_INT_MASK_3 >> 10));
}

/*
 * common configuration between NEC EISA and PCI platforms
 */
void
c_nec_jazz_init()
{
	/* chipset-dependent mcclock configuration */
	mcclock_jazzio_conf = &mcclock_nec_jazz_conf;

	/* chipset-dependent timer configuration */
	timer_jazzio_conf = &timer_nec_jazz_conf;

	/* chipset-dependent jazzio bus configuration */
	jazzio_devconfig = nec_rd94_cpu;
}
