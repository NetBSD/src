/*	$NetBSD: clock_ebus.c,v 1.6.12.1 2014/08/20 00:02:51 tls Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: clock_ebus.c,v 1.6.12.1 2014/08/20 00:02:51 tls Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <dev/clock_subr.h>

#include <emips/ebus/ebusvar.h>
#include <emips/emips/machdep.h>
#include <machine/emipsreg.h>

/*
 * Device softc
 */
struct eclock_softc {
	device_t sc_dev;
	struct _Tc *sc_dp;
	uint32_t sc_reload;
	struct timecounter sc_tc;
	struct todr_chip_handle sc_todr;
};

static int	eclock_ebus_match(device_t, cfdata_t, void *);
static void	eclock_ebus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(eclock_ebus, sizeof (struct eclock_softc),
    eclock_ebus_match, eclock_ebus_attach, NULL, NULL);

void	eclock_init(device_t);

static void	__eclock_init(device_t);
static int	eclock_gettime(struct todr_chip_handle *, struct timeval *);
static int	eclock_settime(struct todr_chip_handle *, struct timeval *);
static int	eclock_ebus_intr(void *, void *);
static u_int	eclock_counter(struct timecounter *);

/* BUGBUG resolve the gap between cpu_initclocks() and eclock_init(x) */
device_t clockdev = NULL;

void
eclock_init(device_t dev)
{

	if (dev == NULL)
		dev = clockdev;
	if (dev == NULL)
		panic("eclock_init");
	__eclock_init(dev);
}

static void
__eclock_init(device_t dev)
{
	struct eclock_softc *sc = device_private(dev);
	struct _Tc *tc = sc->sc_dp;
	uint32_t reload = 10 * 1000000; /* 1sec in 100ns units (10MHz clock) */

	/*
	 * Compute reload according to whatever value passed in,
	 * Warn if fractional
	 */
	if (hz > 1) {
		uint32_t r = reload / hz;
		if ((r * hz) != reload)
			printf("%s: %d Hz clock will cause roundoffs"
			    " with 10MHz xtal (%d)\n",
			    device_xname(sc->sc_dev), hz, reload - (r * hz));
		reload = r;
	}

	sc->sc_reload = reload;

	/* Start the counter */
	tc->DownCounterHigh = 0;
	tc->DownCounter = sc->sc_reload;
	tc->Control = TCCT_ENABLE | TCCT_INT_ENABLE;
}

/*
 * Get the time of day, based on the clock's value and/or the base value.
 * NB: At 10MHz, our 64bits FreeRunning is worth 58,426 years.
 */


static int
eclock_gettime(struct todr_chip_handle *todr, struct timeval *tv)
{
	struct eclock_softc *sc = todr->cookie;
	struct _Tc *tc = sc->sc_dp;
	uint64_t free;
	int s;

	/*
	 * 32bit processor, guard against interrupts in the middle of
	 * reading this 64bit entity
	 */
	/* BUGBUG Should read it "twice" to guard against rollover too. */
	s = splhigh();
	free = tc->FreeRunning;
	splx(s);

	/*
	 * Big fight with the compiler here, it gets very confused by 64bits.
	 */
#if 1
	/*
	 * This is in C: 
	 */
	{
		uint64_t freeS, freeU;
		freeS = free / 10000000UL;
		freeU = free % 10000000UL;
		tv->tv_sec  = freeS;
		tv->tv_usec = freeU / 10;
#if 0
		printf("egt: s x%" PRIx64 " u x%lx (fs %" PRId64
		    " fu %" PRId64 " f %" PRId64 ")\n",
		    tv->tv_sec, tv->tv_usec, freeS, freeU, free);
#endif
	}
#else
	/*
	 * And this is in assembly :-)
	 */
	{
		u_quad_t r;
		u_quad_t d = __qdivrem(free,(u_quad_t)10000000,&r);
		uint32_t su, uu;
		su = (uint32_t)d;
		uu = (uint32_t)r;
		uu = uu / 10;	/* in usecs */
		tv->tv_sec  = su;
		tv->tv_usec = uu;
#if 0
		printf("egt: s x%" PRIx64 " u x%lx (fs %" PRId64
		    " fu %" PRId64 " f %" PRId64 ")\n",
		    tv->tv_sec, tv->tv_usec, d, r, free);
#endif
	}
#endif

	return 0;
}

/*
 * Reset the TODR based on the time value.
 */
static int
eclock_settime(struct todr_chip_handle *todr, struct timeval *tv)
{
	struct eclock_softc *sc = todr->cookie;
	struct _Tc *tc = sc->sc_dp;
	uint64_t free, su;
	uint32_t uu;
	int s;

	/* Careful with what we do here, else the compilerbugs hit hard */
	s = splhigh();

	su = (uint64_t)tv->tv_sec;	/* 0(tv) */
	uu = (uint32_t)tv->tv_usec;	/* 8(tv) */


	free  = su * 10 * 1000 * 1000;
	free += uu * 10;

	tc->FreeRunning = free;
	splx(s);

#if 0
/* 
Should compile to something like this:
80260c84 <eclock_settime>:
80260c84:	27bdffc0 	addiu	sp,sp,-64
80260c88:	afbf0038 	sw	ra,56(sp)
80260c8c:	afb40030 	sw	s4,48(sp)
80260c90:	afb3002c 	sw	s3,44(sp)
80260c94:	afb20028 	sw	s2,40(sp)
80260c98:	afb10024 	sw	s1,36(sp)
80260c9c:	afb00020 	sw	s0,32(sp)
80260ca0:	afb50034 	sw	s5,52(sp)
80260ca4:	8c820000 	lw	v0,0(a0)
80260ca8:	00a09021 	move	s2,a1
80260cac:	8c55003c 	lw	s5,60(v0)        //s5=tc
80260cb0:	0c004122 	jal	80010488 <_splraise>
80260cb4:	3404ff00 	li	a0,0xff00
80260cb8:	8e540000 	lw	s4,0(s2)         //s4=tv->tv_sec=us
80260cbc:	3c060098 	lui	a2,0x98
80260cc0:	34c69680 	ori	a2,a2,0x9680     //a2=10000000
80260cc4:	02860019 	multu	s4,a2        //free=us*10000000
80260cc8:	8e530004 	lw	s3,4(s2)         //s3=uu
80260ccc:	00402021 	move	a0,v0        //s=splhigh()
80260cd0:	001328c0 	sll	a1,s3,0x3
80260cd4:	00131040 	sll	v0,s3,0x1
80260cd8:	00451021 	addu	v0,v0,a1
80260cdc:	00401821 	move	v1,v0        //v1 = uu*10
80260ce0:	00001021 	move	v0,zero
80260ce4:	00003812 	mflo	a3           //a3=low(free)
80260ce8:	00e38821 	addu	s1,a3,v1     //s1=low(free)+(uu*10)
80260cec:	0227282b 	sltu	a1,s1,a3     //a1=overflow bit
80260cf0:	00003010 	mfhi	a2           //a2=high(free)
80260cf4:	00c28021 	addu	s0,a2,v0     //s0=a2=high(free) [useless, v0=0]
80260cf8:	00b08021 	addu	s0,a1,s0     //s0+=overflow bit
80260cfc:	aeb1000c 	sw	s1,12(s5)
80260d00:	aeb00008 	sw	s0,8(s5)
80260d04:	0c00413f 	jal	800104fc <_splset>
80260d08:	00000000 	nop
*/
#endif

#if 0
	printf("est: s x%" PRIx64 " u x%lx (%d %d), free %" PRId64 "\n",
	    tv->tv_sec, tv->tv_usec, su, uu, free);
#endif

	return 0;
}

static int
eclock_ebus_intr(void *cookie, void *f)
{
	struct eclock_softc *sc = cookie;
	struct _Tc *tc = sc->sc_dp;
	struct clockframe *cf = f;
	volatile uint32_t x __unused;

	x = tc->Control;
	tc->DownCounterHigh = 0;
	tc->DownCounter = sc->sc_reload;

	hardclock(cf);
	emips_clock_evcnt.ev_count++;

	return 0;
}

static u_int
eclock_counter(struct timecounter *tc)
{
	struct eclock_softc *sc = tc->tc_priv;
	struct _Tc *Tc = sc->sc_dp;

	return (u_int)Tc->FreeRunning; /* NB: chops to 32bits */
}    


static int
eclock_ebus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ebus_attach_args *ia = aux;
	struct _Tc *mc = (struct _Tc *)ia->ia_vaddr;

	if (strcmp("eclock", ia->ia_name) != 0)
		return 0;
	if ((mc == NULL) ||
	    (mc->Tag != PMTTAG_TIMER))
		return 0;

	return 1;
}

static void
eclock_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct eclock_softc *sc = device_private(self);
	struct ebus_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_dp = (struct _Tc *)ia->ia_vaddr;

	/* NB: We are chopping our 64bit free-running down to 32bits */
	sc->sc_tc.tc_get_timecount = eclock_counter;
	sc->sc_tc.tc_poll_pps = 0;
	sc->sc_tc.tc_counter_mask = 0xffffffff;
	sc->sc_tc.tc_frequency = 10 * 1000 * 1000; /* 10 MHz */
	sc->sc_tc.tc_name = "eclock"; /* BUGBUG is it unique per instance?? */
	sc->sc_tc.tc_quality = 2000; /* uhu? */
	sc->sc_tc.tc_priv = sc;
	sc->sc_tc.tc_next = NULL;

#if DEBUG
	printf(" virt=%p ", (void *)sc->sc_dp);
#endif
	printf(": eMIPS clock\n");

	/* Turn interrupts off, just in case. */
	sc->sc_dp->Control &= ~(TCCT_INT_ENABLE|TCCT_INTERRUPT);

	ebus_intr_establish(parent, (void *)ia->ia_cookie, IPL_CLOCK,
	    eclock_ebus_intr, sc);

#ifdef EVCNT_COUNTERS
	evcnt_attach_dynamic(&clock_intr_evcnt, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "intr");
#endif

	clockdev = self;
	memset(&sc->sc_todr, 0, sizeof sc->sc_todr);
	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = eclock_gettime;
	sc->sc_todr.todr_settime = eclock_settime;
	todr_attach(&sc->sc_todr);

	tc_init(&sc->sc_tc);
}
