/*	$NetBSD: clock.c,v 1.79.2.3 2002/03/16 15:59:50 jdolecek Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1996 Paul Kranenburg
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College. All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed by Paul Kranenburg.
 *	This product includes software developed by Harvard University.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/11/93
 *
 */

/*
 * Clock driver.  This is the id prom and eeprom driver as well
 * and includes the timer register functions too.
 */
#include "opt_sparc_arch.h"

/*
 * This file lumps together a lot of loosely related stuff with
 * confusingly similar names.
 *
 * First, there are kernel's clocks provided by "timer" device.  The
 * hardclock is provided by the timer register (aka system counter).
 * The statclock is provided by per cpu counter register(s) (aka
 * processor counter(s)).
 *
 * The "clock" device is the time-of-day clock provided by MK48Txx.
 * idprom is located in the NVRAM area of the chip.
 *
 * microSPARC-IIep machines use DS1287A at EBus for TOD clock and the
 * driver for it ("rtc") is in a separate file to prevent this file
 * from being cluttered even further.
 */


/* 
 * ifdef out mk48txx TOD clock code for the sake of ms-IIep so that
 * this file doesn't require obio, iommu and sbus to link the kernel.
 */
#include "mk48txx.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#ifdef GPROF
#include <sys/gmon.h>
#endif

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/eeprom.h>
#include <machine/cpu.h>
#include <machine/idprom.h>

#include <dev/clock_subr.h>
#include <dev/ic/mk48txxreg.h>
#include <dev/ic/intersil7170.h>

#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/cpuvar.h>
#include <sparc/sparc/timerreg.h>

#if defined(MSIIEP)
#include <sparc/sparc/msiiepreg.h>
/* XXX: move this stuff to msiiepreg.h? */

/* ms-IIep PCIC registers mapped at fixed VA (see vaddrs.h) */
#define msiiep ((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)

/* 
 * ms-IIep counters tick every 4 cpu clock @100MHz.
 * counter is reset to 1 when new limit is written.
 */
#define tmr_ustolimIIep(n) ((n)* 25 + 1)
#endif /* MSIIEP */

/*
 * Statistics clock interval and variance, in usec.  Variance must be a
 * power of two.  Since this gives us an even number, not an odd number,
 * we discard one case and compensate.  That is, a variance of 1024 would
 * give us offsets in [0..1023].  Instead, we take offsets in [1..1023].
 * This is symmetric about the point 512, or statvar/2, and thus averages
 * to that value (assuming uniform random numbers).
 */
/* XXX fix comment to match value */
int statvar = 8192;
int statmin;			/* statclock interval - 1/2*variance */
int timerok;


extern struct idprom sun4_idprom_store;

#if defined(SUN4)
/*
 * OCLOCK support: 4/100's and 4/200's have the old clock.
 */
#define intersil_command(run, interrupt) \
    (run | interrupt | INTERSIL_CMD_FREQ_32K | INTERSIL_CMD_24HR_MODE | \
     INTERSIL_CMD_NORMAL_MODE)

#define intersil_disable() \
	bus_space_write_1(i7_bt, i7_bh, INTERSIL_ICMD, \
		  intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IDISABLE));

#define intersil_enable() \
	bus_space_write_1(i7_bt, i7_bh, INTERSIL_ICMD, \
		  intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IENABLE));

#define intersil_clear() bus_space_read_1(i7_bt, i7_bh, INTERSIL_IINTR)

static int oldclk = 0;
bus_space_tag_t i7_bt;
bus_space_handle_t i7_bh;
#endif /* SUN4 */

static int oclockmatch(struct device *, struct cfdata *, void *);
static void oclockattach(struct device *, struct device *, void *);

struct cfattach oclock_ca = {
	sizeof(struct device), oclockmatch, oclockattach
};

/*
 * Sun 4 machines use the old-style (a'la Sun 3) EEPROM.  On the
 * 4/100's and 4/200's, this is at a separate obio space.  On the
 * 4/300's and 4/400's, however, it is the cl_nvram[] chunk of the
 * Mostek chip.  Therefore, eeprom_match will only return true on
 * the 100/200 models, and the eeprom will be attached separately.
 * On the 300/400 models, the eeprom will be dealt with when the clock is
 * attached.
 */
char		*eeprom_va = NULL;
#if defined(SUN4)
static int	eeprom_busy = 0;
static int	eeprom_wanted = 0;
static int	eeprom_nvram = 0;	/* non-zero if eeprom is on Mostek */
static int	eeprom_take(void);
static void	eeprom_give(void);
static int	eeprom_update(char *, int, int);
#endif

static int	eeprom_match(struct device *, struct cfdata *, void *);
static void	eeprom_attach(struct device *, struct device *, void *);

struct cfattach eeprom_ca = {
	sizeof(struct device), eeprom_match, eeprom_attach
};


#if NMK48TXX > 0
/* Location and size of the MK48xx TOD clock, if present */
static bus_space_handle_t	mk_nvram_base;
static bus_size_t		mk_nvram_size;

static int	clk_wenable(todr_chip_handle_t, int);

static int	clockmatch_mainbus (struct device *, struct cfdata *, void *);
static int	clockmatch_obio(struct device *, struct cfdata *, void *);
static void	clockattach_mainbus(struct device *, struct device *, void *);
static void	clockattach_obio(struct device *, struct device *, void *);

static void	clockattach(int, bus_space_tag_t, bus_space_handle_t);

struct cfattach clock_mainbus_ca = {
	sizeof(struct device), clockmatch_mainbus, clockattach_mainbus
};

struct cfattach clock_obio_ca = {
	sizeof(struct device), clockmatch_obio, clockattach_obio
};
#endif /* NMK48TXX > 0 */


static struct intrhand level10 = { clockintr };
static struct intrhand level14 = { statintr };

static int	timermatch_mainbus(struct device *, struct cfdata *, void *);
static int	timermatch_obio(struct device *, struct cfdata *, void *);
static void	timerattach_mainbus(struct device *, struct device *, void *);
static void	timerattach_obio(struct device *, struct device *, void *);

static void	timerattach(volatile int *, volatile int *);

/*
 * On ms-IIep counters are part of PCIC, so msiiep_attach will simply
 * call this function to configure the counters.
 */
void	timerattach_msiiep(void);


/*struct counter_4m	*counterreg_4m;*/
struct timer_4m		*timerreg4m;
#define counterreg4m	cpuinfo.counterreg_4m

#define	timerreg4	((struct timerreg_4 *)TIMERREG_VA)

struct cfattach timer_mainbus_ca = {
	sizeof(struct device), timermatch_mainbus, timerattach_mainbus
};

struct cfattach timer_obio_ca = {
	sizeof(struct device), timermatch_obio, timerattach_obio
};

/* Global TOD clock handle & idprom pointer */
todr_chip_handle_t todr_handle;
struct idprom *idprom;

void establish_hostid(struct idprom *);
void myetheraddr(u_char *);

int timerblurb = 10; /* Guess a value; used before clock is attached */


/*
 * old clock match routine
 */
static int
oclockmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (0);

	/* Only these sun4s have oclock */
	if (!CPU_ISSUN4 ||
	    (cpuinfo.cpu_type != CPUTYP_4_100 &&
	     cpuinfo.cpu_type != CPUTYP_4_200))
		return (0);

	/* Make sure there is something there */
	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
				1,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

/* ARGSUSED */
static void
oclockattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
#if defined(SUN4)
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;
	bus_space_tag_t bt = oba->oba_bustag;
	bus_space_handle_t bh;

	oldclk = 1;  /* we've got an oldie! */

	if (bus_space_map(bt,
			  oba->oba_paddr,
			  sizeof(struct intersil7170),
			  BUS_SPACE_MAP_LINEAR,	/* flags */
			  &bh) != 0) {
		printf("%s: can't map register\n", self->dv_xname);
		return;
	}
	i7_bt = bt;
	i7_bh = bh;

	/* 
	 * calibrate delay() 
	 */
	ienab_bic(IE_L14 | IE_L10);	/* disable all clock intrs */
	for (timerblurb = 1; ; timerblurb++) {
		int ival;

		/* Set to 1/100 second interval */
		bus_space_write_1(bt, bh, INTERSIL_IINTR,
				  INTERSIL_INTER_CSECONDS);

		/* enable clock */
		intersil_enable();

		while ((intersil_clear() & INTERSIL_INTER_PENDING) == 0)
			/* sync with interrupt */;
		while ((intersil_clear() & INTERSIL_INTER_PENDING) == 0)
			/* XXX: do it again, seems to need it */;

		/* Probe 1/100 sec delay */
		delay(10000);

		/* clear, save value */
		ival = intersil_clear();

		/* disable clock */
		intersil_disable();

		if ((ival & INTERSIL_INTER_PENDING) != 0) {
			printf(" delay constant %d%s\n", timerblurb,
				(timerblurb == 1) ? " [TOO SMALL?]" : "");
			break;
		}
		if (timerblurb > 10) {
			printf("\noclock: calibration failing; clamped at %d\n",
			       timerblurb);
			break;
		}
	}

	/* link interrupt handlers */
	intr_establish(10, &level10);
	intr_establish(14, &level14);

	/* Our TOD clock year 0 represents 1968 */
	if ((todr_handle = intersil7170_attach(bt, bh, 1968)) == NULL)
		panic("Can't attach tod clock");

	establish_hostid(&sun4_idprom_store);
#endif /* SUN4 */
}

/* We support only one eeprom device */
static int eeprom_attached;

/*
 * Sun 4/100, 4/200 EEPROM match routine.
 */
static int
eeprom_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (0);

	if (eeprom_attached)
		/* We support only one eeprom device */
		return (0);

	/* Only these sun4s have oclock */
	if (!CPU_ISSUN4 ||
	    (cpuinfo.cpu_type != CPUTYP_4_100 &&
	     cpuinfo.cpu_type != CPUTYP_4_200))
		return (0);

	/* Make sure there is something there */
	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
				1,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

static void
eeprom_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
#if defined(SUN4)
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;
	bus_space_handle_t bh;

	eeprom_attached = 1;
	printf("\n");

	if (bus_space_map(oba->oba_bustag,
			  oba->oba_paddr,
			  EEPROM_SIZE,
			  BUS_SPACE_MAP_LINEAR,	/* flags */
			  &bh) != 0) {
		printf("%s: can't map register\n", self->dv_xname);
		return;
	}
	eeprom_va = (char *)bh;
	eeprom_nvram = 0;
#endif /* SUN4 */
}


#if NMK48TXX > 0

/*
 * The OPENPROM calls the clock the "eeprom", so we have to have our
 * own special match function to call it the "clock".
 */
static int
clockmatch_mainbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp("eeprom", ma->ma_name) == 0);
}

static int
clockmatch_obio(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (strcmp("eeprom", uoba->uoba_sbus.sa_name) == 0);

	if (!CPU_ISSUN4) {
		printf("clockmatch_obio: attach args mixed up\n");
		return (0);
	}

	/* Only these sun4s have "clock" (others have "oclock") */
	if (cpuinfo.cpu_type != CPUTYP_4_300 &&
	    cpuinfo.cpu_type != CPUTYP_4_400)
		return (0);

	/* Make sure there is something there */
	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
				1,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

/* ARGSUSED */
static void
clockattach_mainbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	bus_space_tag_t bt = ma->ma_bustag;
	bus_space_handle_t bh;

	/*
	 * We ignore any existing virtual address as we need to map
	 * this read-only and make it read-write only temporarily,
	 * whenever we read or write the clock chip.  The clock also
	 * contains the ID ``PROM'', and I have already had the pleasure
	 * of reloading the cpu type, Ethernet address, etc, by hand from
	 * the console FORTH interpreter.  I intend not to enjoy it again.
	 */
	if (bus_space_map(bt,
			   ma->ma_paddr,
			   ma->ma_size,
			   BUS_SPACE_MAP_LINEAR,
			   &bh) != 0) {
		printf("%s: can't map register\n", self->dv_xname);
		return;
	}

	clockattach(ma->ma_node, bt, bh);
}

static void
clockattach_obio(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	bus_space_tag_t bt;
	bus_space_handle_t bh;
	int node;

	if (uoba->uoba_isobio4 == 0) {
		/* sun4m clock at obio */
		struct sbus_attach_args *sa = &uoba->uoba_sbus;

		node = sa->sa_node;
		bt = sa->sa_bustag;
		if (sbus_bus_map(bt,
			sa->sa_slot, sa->sa_offset, sa->sa_size,
			BUS_SPACE_MAP_LINEAR, &bh) != 0) {
			printf("%s: can't map register\n", self->dv_xname);
			return;
		}
	} else {
		/* sun4 clock at obio */
		struct obio4_attach_args *oba = &uoba->uoba_oba4;

		/*
		 * Sun4's only have mk48t02 clocks, so we hard-code
		 * the device address space length to 2048.
		 */
		node = 0;
		bt = oba->oba_bustag;
		if (bus_space_map(bt,
				  oba->oba_paddr,
				  2048,			/* size */
				  BUS_SPACE_MAP_LINEAR,	/* flags */
				  &bh) != 0) {
			printf("%s: can't map register\n", self->dv_xname);
			return;
		}
	}

	clockattach(node, bt, bh);
}

static void
clockattach(node, bt, bh)
	int node;
	bus_space_tag_t bt;
	bus_space_handle_t bh;
{
	struct idprom *idp;
	char *model;

	if (CPU_ISSUN4)
		model = "mk48t02";	/* Hard-coded sun4 clock */
	else if (node != 0)
		model = PROM_getpropstring(node, "model");
	else
		panic("clockattach: node == 0");

	/* Our TOD clock year 0 represents 1968 */
	todr_handle = mk48txx_attach(bt, bh, model, 1968, NULL, NULL);
	if (todr_handle == NULL)
		panic("Cannot attach %s tod clock", model);

	/*
	 * Store NVRAM base address and size in globals for use
	 * by clk_wenable().
	 */
	mk_nvram_base = bh;
	if (mk48txx_get_nvram_size(todr_handle, &mk_nvram_size) != 0)
		panic("Cannot get nvram size on %s", model);

	/* Establish clock write-enable method */
	todr_handle->todr_setwen = clk_wenable;

#if defined(SUN4)
	if (CPU_ISSUN4) {
		idp = &sun4_idprom_store;
		if (cpuinfo.cpu_type == CPUTYP_4_300 ||
		    cpuinfo.cpu_type == CPUTYP_4_400) {
			eeprom_va = (char *)bh;
			eeprom_nvram = 1;
		}
	} else
#endif
	{
	/*
	 * Location of IDPROM relative to the end of the NVRAM area
	 */
#define MK48TXX_IDPROM_OFFSET (mk_nvram_size - 40)

		idp = (struct idprom *)((u_long)bh + MK48TXX_IDPROM_OFFSET);
	}

	establish_hostid(idp);
}

/*
 * Write en/dis-able TOD clock registers.  This is a safety net to
 * save idprom (part of mk48txx TOD clock) from being accidentally
 * stomped on by a buggy code.  We coordinate so that several writers
 * can run simultaneously.
 */
int
clk_wenable(handle, onoff)
	todr_chip_handle_t handle;
	int onoff;
{
	int s;
	vm_prot_t prot;/* nonzero => change prot */
	int npages;
	vaddr_t base;
	static int writers;

	/* XXX - we ignore `handle' here... */

	s = splhigh();
	if (onoff)
		prot = writers++ == 0 ? VM_PROT_READ|VM_PROT_WRITE : 0;
	else
		prot = --writers == 0 ? VM_PROT_READ : 0;
	splx(s);

	npages = round_page((vsize_t)mk_nvram_size) << PAGE_SHIFT;
	base = trunc_page((vaddr_t)mk_nvram_base);
	if (prot)
		pmap_changeprot(pmap_kernel(), base, prot, npages);

	return (0);
}

#endif /* NMK48TXX > 0 */ /* "clock" device driver */

/*
 * The sun4c OPENPROM calls the timer the "counter-timer".
 */
static int
timermatch_mainbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp("counter-timer", ma->ma_name) == 0);
}

/*
 * The sun4m OPENPROM calls the timer the "counter".
 * The sun4 timer must be probed.
 */
static int
timermatch_obio(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (strcmp("counter", uoba->uoba_sbus.sa_name) == 0);

	if (!CPU_ISSUN4) {
		printf("timermatch_obio: attach args mixed up\n");
		return (0);
	}

	/* Only these sun4s have "timer" (others have "oclock") */
	if (cpuinfo.cpu_type != CPUTYP_4_300 &&
	    cpuinfo.cpu_type != CPUTYP_4_400)
		return (0);

	/* Make sure there is something there */
	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
				4,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

/* ARGSUSED */
static void
timerattach_mainbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	bus_space_handle_t bh;

	/*
	 * This time, we ignore any existing virtual address because
	 * we have a fixed virtual address for the timer, to make
	 * microtime() faster.
	 */
	if (bus_space_map2(ma->ma_bustag,
			   ma->ma_paddr,
			   sizeof(struct timerreg_4),
			   BUS_SPACE_MAP_LINEAR,
			   TIMERREG_VA, &bh) != 0) {
		printf("%s: can't map register\n", self->dv_xname);
		return;
	}

	timerattach(&timerreg4->t_c14.t_counter, &timerreg4->t_c14.t_limit);
}

static void
timerattach_obio(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union obio_attach_args *uoba = aux;

	if (uoba->uoba_isobio4 == 0) {
		/* sun4m timer at obio */
#if defined(SUN4M)
		struct sbus_attach_args *sa = &uoba->uoba_sbus;
		bus_space_handle_t bh;
		int i;

		if (sa->sa_nreg < 2) {
			printf("%s: only %d register sets\n", self->dv_xname,
				sa->sa_nreg);
			return;
		}

		/* Map the system timer */
		i = sa->sa_nreg - 1;
		if (bus_space_map2(sa->sa_bustag,
				 BUS_ADDR(
					sa->sa_reg[i].sbr_slot,
					sa->sa_reg[i].sbr_offset),
				 sizeof(struct timer_4m),
				 BUS_SPACE_MAP_LINEAR,
				 TIMERREG_VA, &bh) != 0) {
			printf("%s: can't map register\n", self->dv_xname);
			return;
		}
		timerreg4m = (struct timer_4m *)TIMERREG_VA;

		/* Map each CPU's counter */
		for (i = 0; i < sa->sa_nreg - 1; i++) {
			struct cpu_info *cpi = NULL;
			int n;

			/*
			 * Check whether the CPU corresponding to this
			 * timer register is installed.
			 */
			for (n = 0; n < ncpu; n++) {
				if ((cpi = cpus[n]) == NULL)
					continue;
				if ((i == 0 && ncpu == 1) || cpi->mid == i + 8)
					/* We got a corresponding MID */
					break;
				cpi = NULL;
			}
			if (cpi == NULL)
				continue;
			if (sbus_bus_map(sa->sa_bustag,
					 sa->sa_reg[i].sbr_slot,
					 sa->sa_reg[i].sbr_offset,
					 sizeof(struct timer_4m),
					 BUS_SPACE_MAP_LINEAR,
					 &bh) != 0) {
				printf("%s: can't map register\n",
					self->dv_xname);
				return;
			}
			cpi->counterreg_4m = (struct counter_4m *)bh;
		}

		/* Put processor counter in "timer" mode */
		timerreg4m->t_cfg = 0;

		timerattach(&counterreg4m->t_counter, &counterreg4m->t_limit);
#endif
		return;
	} else {
#if defined(SUN4)
		/* sun4 timer at obio */
		struct obio4_attach_args *oba = &uoba->uoba_oba4;
		bus_space_handle_t bh;

		if (bus_space_map2(oba->oba_bustag,
				  oba->oba_paddr,
				  sizeof(struct timerreg_4),
				  BUS_SPACE_MAP_LINEAR,
				  TIMERREG_VA,
				  &bh) != 0) {
			printf("%s: can't map register\n", self->dv_xname);
			return;
		}
		timerattach(&timerreg4->t_c14.t_counter,
			    &timerreg4->t_c14.t_limit);
#endif
	}
}

static void
timerattach(cntreg, limreg)
	volatile int *cntreg, *limreg;
{

	/*
	 * Calibrate delay() by tweaking the magic constant
	 * until a delay(100) actually reads (at least) 100 us on the clock.
	 * Note: sun4m clocks tick with 500ns periods.
	 */
	for (timerblurb = 1; ; timerblurb++) {
		volatile int discard;
		int t0, t1;

		/* Reset counter register by writing some large limit value */
		discard = *limreg;
		*limreg = tmr_ustolim(TMR_MASK-1);

		t0 = *cntreg;
		delay(100);
		t1 = *cntreg;

		if (t1 & TMR_LIMIT)
			panic("delay calibration");

		t0 = (t0 >> TMR_SHIFT) & TMR_MASK;
		t1 = (t1 >> TMR_SHIFT) & TMR_MASK;

		if (t1 >= t0 + 100)
			break;
	}

	printf(" delay constant %d\n", timerblurb);

	/* link interrupt handlers */
	intr_establish(10, &level10);
	intr_establish(14, &level14);

	timerok = 1;
}


#if defined(MSIIEP)
/*
 * Attach system and cpu counters (kernel hard and stat clocks) for ms-IIep.
 * Counters are part of the PCIC and there's no PROM node for them.
 * msiiep_attach() will call this function directly.
 */
void
timerattach_msiiep()
{
	/* Put processor counter in "counter" mode */
	msiiep->pcic_pc_ctl = 0; /* stop user timer (just in case) */
	msiiep->pcic_pc_cfg = 0; /* timer mode disabled (processor counter) */

	/*
	 * Calibrate delay() by tweaking the magic constant
	 * until a delay(100) actually reads (at least) 100 us on the clock.
	 * Note: ms-IIep clocks ticks every 4 processor cycles.
	 */
	for (timerblurb = 1; ; ++timerblurb) {
		volatile int discard;
		int t;

		discard = msiiep->pcic_pclr; /* clear the limit bit */
		msiiep->pcic_pclr = 0; /* reset counter to 1, free run */
		delay(100);
		t = msiiep->pcic_pccr;

		if (t & TMR_LIMIT) /* cannot happen */
			panic("delay calibration");

		/* counter ticks -> usec, inverse of tmr_ustolimIIep */
		t = (t - 1) / 25;
		if (t >= 100)
			break;
	}
	printf(" delay constant %d\n", timerblurb);

	/*
	 * Set counter interrupt priority assignment:
	 * upper 4 bits are for system counter: level 10
	 * lower 4 bits are for processor counter: level 14
	 */
	msiiep->pcic_cipar = 0xae;

	/* link interrupt handlers */
	intr_establish(10, &level10);
	intr_establish(14, &level14);

	timerok = 1;
}
#endif /* MSIIEP */


/*
 * XXX this belongs elsewhere
 */
void
myetheraddr(cp)
	u_char *cp;
{
	struct idprom *idp = idprom;

	cp[0] = idp->id_ether[0];
	cp[1] = idp->id_ether[1];
	cp[2] = idp->id_ether[2];
	cp[3] = idp->id_ether[3];
	cp[4] = idp->id_ether[4];
	cp[5] = idp->id_ether[5];
}

void
establish_hostid(idp)
	struct idprom *idp;
{
	u_long h;

	h = idp->id_machine << 24;
	h |= idp->id_hostid[0] << 16;
	h |= idp->id_hostid[1] << 8;
	h |= idp->id_hostid[2];

	printf(": hostid %lx\n", h);

	/* Save IDPROM pointer and Host ID in globals */
	idprom = idp;
	hostid = h;
}


/*
 * Set up the real-time and statistics clocks.
 * Leave stathz 0 only if no alternative timer is available.
 *
 * The frequencies of these clocks must be an even number of microseconds.
 */
void
cpu_initclocks()
{
	int statint, minint;

#if defined(SUN4)
	if (oldclk) {
		int dummy;

		profhz = hz = 100;
		tick = 1000000 / hz;

		/* Select 1/100 second interval */
		bus_space_write_1(i7_bt, i7_bh, INTERSIL_IINTR,
				  INTERSIL_INTER_CSECONDS);

		ienab_bic(IE_L14 | IE_L10);	/* disable all clock intrs */
		intersil_disable();		/* disable clock */
		dummy = intersil_clear();	/* clear interrupts */
		ienab_bis(IE_L10);		/* enable l10 interrupt */
		intersil_enable();		/* enable clock */

		return;
	}
#endif /* SUN4 */

	if (1000000 % hz) {
		printf("cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
		tick = 1000000 / hz;
	}
	if (stathz == 0)
		stathz = hz;
	if (1000000 % stathz) {
		printf("cannot get %d Hz statclock; using 100 Hz\n", stathz);
		stathz = 100;
	}
	profhz = stathz;		/* always */

	statint = 1000000 / stathz;
	minint = statint / 2 + 100;
	while (statvar > minint)
		statvar >>= 1;
	statmin = statint - (statvar >> 1);

#if defined(SUN4M) && !defined(MSIIEP)
	if (CPU_ISSUN4M) {
		int n;
		timerreg4m->t_limit = tmr_ustolim4m(tick);
		for (n = 0; n < ncpu; n++) {
			struct cpu_info *cpi;
			if ((cpi = cpus[n]) == NULL)
				continue;
			cpi->counterreg_4m->t_limit = tmr_ustolim4m(statint);
		}
		ienab_bic(SINTR_T);
	}
#endif

#if defined(MSIIEP)
	/* ms-IIep kernels support *only* IIep */ {
		msiiep->pcic_sclr = tmr_ustolimIIep(tick);
		msiiep->pcic_pclr = tmr_ustolimIIep(statint);
		/* XXX: ensure interrupt target mask doesn't masks them? */
	}
#endif

#if defined(SUN4) || defined(SUN4C)
	if (CPU_ISSUN4OR4C) {
		timerreg4->t_c10.t_limit = tmr_ustolim(tick);
		timerreg4->t_c14.t_limit = tmr_ustolim(statint);
		ienab_bis(IE_L14 | IE_L10);
	}
#endif
}

/*
 * Dummy setstatclockrate(), since we know profhz==hz.
 */
/* ARGSUSED */
void
setstatclockrate(newhz)
	int newhz;
{
	/* nothing */
}

/*
 * Level 10 (clock) interrupts from system counter.
 * If we are using the FORTH PROM for console input, we need to check
 * for that here as well, and generate a software interrupt to read it.
 */
int
clockintr(cap)
	void *cap;
{
	volatile int discard;
	int s;

	/*
	 * Protect the clearing of the clock interrupt.  If we don't
	 * do this, and we're interrupted (by the zs, for example),
	 * the clock stops!
	 * XXX WHY DOES THIS HAPPEN?
	 */
	s = splhigh();

#if defined(SUN4)
	if (oldclk) {
		discard = intersil_clear();
		ienab_bic(IE_L10);  /* clear interrupt */
		ienab_bis(IE_L10);  /* enable interrupt */
		goto forward;
	}
#endif

	/* read the limit register to clear the interrupt */
	if (CPU_ISSUN4M) {
#if !defined(MSIIEP)
		discard = timerreg4m->t_limit;
#else
		discard = msiiep->pcic_sclr;
#endif
	}

	if (CPU_ISSUN4OR4C) {
		discard = timerreg4->t_c10.t_limit;
	}
#if defined(SUN4)
forward:
#endif
	splx(s);

	hardclock((struct clockframe *)cap);
	return (1);
}

/*
 * Level 14 (stat clock) interrupts from processor counter.
 */
int
statintr(cap)
	void *cap;
{
	volatile int discard;
	u_long newint, r, var;

#if defined(SUN4)
	if (oldclk) {
		panic("oldclk statintr");
		return (1);
	}
#endif

	/* read the limit register to clear the interrupt */
	if (CPU_ISSUN4M) {
#if !defined(MSIIEP)
		discard = counterreg4m->t_limit;
#else
		discard = msiiep->pcic_pclr;
#endif
		if (timerok == 0) {
			/* Stop the clock */
			printf("note: counter running!\n");
#if !defined(MSIIEP)
			discard = counterreg4m->t_limit;
			counterreg4m->t_limit = 0;
			counterreg4m->t_ss = 0;
			timerreg4m->t_cfg = TMR_CFG_USER;
#else
			/* 
			 * Turn interrupting processor counter
			 * into non-interrupting user timer.
			 */
			msiiep->pcic_pc_cfg = 1; /* make it a user timer */
			msiiep->pcic_pc_ctl = 0; /* stop user timer */
#endif
			return 1;
		}
	}

	if (CPU_ISSUN4OR4C) {
		discard = timerreg4->t_c14.t_limit;
	}

	statclock((struct clockframe *)cap);

	/*
	 * Compute new randomized interval.  The intervals are uniformly
	 * distributed on [statint - statvar / 2, statint + statvar / 2],
	 * and therefore have mean statint, giving a stathz frequency clock.
	 */
	var = statvar;
	do {
		r = random() & (var - 1);
	} while (r == 0);
	newint = statmin + r;

	if (CPU_ISSUN4M) {
		/*
		 * Use the `non-resetting' limit register, so we don't
		 * loose the counter ticks that happened since this
		 * interrupt was raised.
		 */
#if !defined(MSIIEP)
		counterreg4m->t_limit_nr = tmr_ustolim4m(newint);
#else
		msiiep->pcic_pclr_nr = tmr_ustolimIIep(newint);
#endif
	}

	if (CPU_ISSUN4OR4C) {
		/*
		 * The sun4/4c timer has no `non-resetting' register;
		 * use the current counter value to compensate the new
		 * limit value for the number of counter ticks elapsed.
		 */
		newint -= tmr_cnttous(timerreg4->t_c14.t_counter);
		timerreg4->t_c14.t_limit = tmr_ustolim(newint);
	}
	return (1);
}


/*
 * `sparc_clock_time_is_ok' is used in cpu_reboot() to determine
 * whether it is appropriate to call resettodr() to consolidate
 * pending time adjustments.
 */
int sparc_clock_time_is_ok;

/*
 * Set up the system's time, given a `reasonable' time value.
 */
void
inittodr(base)
	time_t base;
{
	int badbase = 0, waszero = base == 0;

	if (base < 5 * SECYR) {
		/*
		 * If base is 0, assume filesystem time is just unknown
		 * in stead of preposterous. Don't bark.
		 */
		if (base != 0)
			printf("WARNING: preposterous time in file system\n");
		/* not going to use it anyway, if the chip is readable */
		base = 21*SECYR + 186*SECDAY + SECDAY/2;
		badbase = 1;
	}

	if (todr_gettime(todr_handle, (struct timeval *)&time) != 0 ||
	    time.tv_sec == 0) {

		printf("WARNING: bad date in battery clock");
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the clock.
		 */
		time.tv_sec = base;
		if (!badbase)
			resettodr();
	} else {
		int deltat = time.tv_sec - base;

		sparc_clock_time_is_ok = 1;

		if (deltat < 0)
			deltat = -deltat;
		if (waszero || deltat < 2 * SECDAY)
			return;
		printf("WARNING: clock %s %d days",
		    time.tv_sec < base ? "lost" : "gained", deltat / SECDAY);
	}
	printf(" -- CHECK AND RESET THE DATE!\n");
}

/*
 * Reset the clock based on the current time.
 * Used when the current clock is preposterous, when the time is changed,
 * and when rebooting.  Do nothing if the time is not yet known, e.g.,
 * when crashing during autoconfig.
 */
void
resettodr()
{

	if (time.tv_sec == 0)
		return;

	sparc_clock_time_is_ok = 1;
	if (todr_settime(todr_handle, (struct timeval *)&time) != 0)
		printf("Cannot set time in time-of-day clock\n");
}

#if defined(SUN4)
/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  We do this by returning the current time
 * plus the amount of time since the last clock interrupt.
 *
 * Check that this time is no less than any previously-reported time,
 * which could happen around the time of a clock adjustment.  Just for
 * fun, we guarantee that the time will be greater than the value
 * obtained by a previous call.
 */
void
microtime(tvp)
	struct timeval *tvp;
{
	int s;
	static struct timeval lasttime;
	static struct timeval oneusec = {0, 1};

	if (!oldclk) {
		lo_microtime(tvp);
		return;
	}

	s = splhigh();
	*tvp = time;
	splx(s);

	if (timercmp(tvp, &lasttime, <=))
		timeradd(&lasttime, &oneusec, tvp);

	lasttime = *tvp;
}
#endif /* SUN4 */

/*
 * XXX: these may actually belong somewhere else, but since the
 * EEPROM is so closely tied to the clock on some models, perhaps
 * it needs to stay here...
 */
int
eeprom_uio(uio)
	struct uio *uio;
{
#if defined(SUN4)
	int error;
	int off;	/* NOT off_t */
	u_int cnt, bcnt;
	caddr_t buf = NULL;

	if (!CPU_ISSUN4)
		return (ENODEV);

	off = uio->uio_offset;
	if (off > EEPROM_SIZE)
		return (EFAULT);

	cnt = uio->uio_resid;
	if (cnt > (EEPROM_SIZE - off))
		cnt = (EEPROM_SIZE - off);

	if ((error = eeprom_take()) != 0)
		return (error);

	if (eeprom_va == NULL) {
		error = ENXIO;
		goto out;
	}

	/*
	 * The EEPROM can only be accessed one byte at a time, yet
	 * uiomove() will attempt long-word access.  To circumvent
	 * this, we byte-by-byte copy the eeprom contents into a
	 * temporary buffer.
	 */
	buf = malloc(EEPROM_SIZE, M_DEVBUF, M_WAITOK);
	if (buf == NULL) {
		error = EAGAIN;
		goto out;
	}

	if (uio->uio_rw == UIO_READ)
		for (bcnt = 0; bcnt < EEPROM_SIZE; ++bcnt)
			*(char *)(buf + bcnt) = *(char *)(eeprom_va + bcnt);

	if ((error = uiomove(buf + off, (int)cnt, uio)) != 0)
		goto out;

	if (uio->uio_rw != UIO_READ)
		error = eeprom_update(buf, off, cnt);

 out:
	if (buf)
		free(buf, M_DEVBUF);
	eeprom_give();
	return (error);
#else /* ! SUN4 */
	return (ENODEV);
#endif /* SUN4 */
}

#if defined(SUN4)
/*
 * Update the EEPROM from the passed buf.
 */
static int
eeprom_update(buf, off, cnt)
	char *buf;
	int off, cnt;
{
	int error = 0;
	volatile char *ep;
	char *bp;

	if (eeprom_va == NULL)
		return (ENXIO);

	ep = eeprom_va + off;
	bp = buf + off;

	if (eeprom_nvram)
		clk_wenable(todr_handle, 1);

	while (cnt > 0) {
		/*
		 * DO NOT WRITE IT UNLESS WE HAVE TO because the
		 * EEPROM has a limited number of write cycles.
		 * After some number of writes it just fails!
		 */
		if (*ep != *bp) {
			*ep = *bp;
			/*
			 * We have written the EEPROM, so now we must
			 * sleep for at least 10 milliseconds while
			 * holding the lock to prevent all access to
			 * the EEPROM while it recovers.
			 */
			(void)tsleep(eeprom_va, PZERO - 1, "eeprom", hz/50);
		}
		/* Make sure the write worked. */
		if (*ep != *bp) {
			error = EIO;
			goto out;
		}
		++ep;
		++bp;
		--cnt;
	}
 out:
	if (eeprom_nvram)
		clk_wenable(todr_handle, 0);

	return (error);
}

/* Take a lock on the eeprom. */
static int
eeprom_take()
{
	int error = 0;

	while (eeprom_busy) {
		eeprom_wanted = 1;
		error = tsleep(&eeprom_busy, PZERO | PCATCH, "eeprom", 0);
		eeprom_wanted = 0;
		if (error)	/* interrupted */
			goto out;
	}
	eeprom_busy = 1;
 out:
	return (error);
}

/* Give a lock on the eeprom away. */
static void
eeprom_give()
{

	eeprom_busy = 0;
	if (eeprom_wanted) {
		eeprom_wanted = 0;
		wakeup(&eeprom_busy);
	}
}
#endif /* SUN4 */
