/* $NetBSD: dec_3max.c,v 1.6.2.12 1999/11/12 11:07:20 nisimura Exp $ */

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
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
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
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
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 *	@(#)machdep.c	8.3 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_3max.c,v 1.6.2.12 1999/11/12 11:07:20 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>	
#include <sys/termios.h>
#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/sysconf.h>

#include <pmax/pmax/kn02.h>
#include <pmax/pmax/memc.h>
#include <mips/mips/mips_mcclock.h>

#include <dev/tc/tcvar.h>
#include <pmax/ibus/ibusvar.h>

#include "wsdisplay.h"

void dec_3max_init __P((void));
void dec_3max_bus_reset __P((void));
void dec_3max_cons_init __P((void));
void dec_3max_device_register __P((struct device *, void *));
int  dec_3max_intr __P((unsigned, unsigned, unsigned, unsigned));

void kn02_intr_establish __P((struct device *, void *, int,
		int (*)(void *), void *));
void kn02_intr_disestablish __P((struct device *, void *));

static void dec_3max_memerr __P((void));

extern void kn02_wbflush __P((void));
extern void prom_findcons __P((int *, int *, int *));
extern int dc_cnattach __P((paddr_t, int, int, int));
extern void dckbd_cnattach __P((paddr_t));
extern int tc_fb_cnattach __P((int));

int _splraise_kn02 __P((int));
int _spllower_kn02 __P((int));
int _splrestore_kn02 __P((int));

struct splsw spl_3max = {
	{ _spllower_kn02,	0 },
	{ _splraise_kn02,	IPL_BIO },
	{ _splraise_kn02,	IPL_NET },
	{ _splraise_kn02,	IPL_TTY },
	{ _splraise_kn02,	IPL_IMP },
	{ _splraise,		MIPS_SPL_0_1 },
	{ _splrestore_kn02,	0 },
};


void
dec_3max_init()
{
	u_int32_t csr;
	extern char cpu_model[];

	platform.iobus = "tc3max";
	platform.bus_reset = dec_3max_bus_reset;
	platform.cons_init = dec_3max_cons_init;
	platform.device_register = dec_3max_device_register;
	platform.iointr = dec_3max_intr;
	/* no high resolution timer available */

	/* clear any memory errors */
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();

	mips_hardware_intr = dec_3max_intr;

#ifdef NEWSPL
	__spl = &spl_3max;
#else
	splvec.splbio = MIPS_SPL0;
	splvec.splnet = MIPS_SPL0;
	splvec.spltty = MIPS_SPL0;
	splvec.splimp = MIPS_SPL0;
	splvec.splclock = MIPS_SPL_0_1;
	splvec.splstatclock = MIPS_SPL_0_1;
#endif
	
	/* calibrate cpu_mhz value */
	mc_cpuspeed(
	    (void *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CLOCK), MIPS_INT_MASK_1);

	/*
	 * Enable ECC memory correction, turn off LEDs, and
	 * disable all TURBOchannel interrupts.
	 */
	csr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
	csr &= ~(KN02_CSR_WRESERVED|KN02_CSR_IOINTEN|KN02_CSR_CORRECT|0xff);
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) = csr;
	kn02_wbflush();

	strcpy(cpu_model, "DECstation 5000/200 (3MAX)");
}

void
dec_3max_bus_reset()
{
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();

	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CHKSYN) = 0;
	kn02_wbflush();
}

void
dec_3max_cons_init()
{
	int kbd, crt, screen;

	kbd = crt = screen = 0;
	prom_findcons(&kbd, &crt, &screen);

	if (screen > 0) {
#if NWSDISPLAY > 0
		dckbd_cnattach(KN02_SYS_DZ);
		if (tc_fb_cnattach(crt) > 0)
			return;
#endif
		printf("No framebuffer device configured for slot %d: ", crt);
		printf("using serial console\n");
	}
	/*
	 * Delay to allow PROM putchars to complete.
	 * FIFO depth * character time,
	 * character time = (1000000 / (defaultrate / 10))
	 */
	DELAY(160000000 / 9600);	/* XXX */

	if (dc_cnattach(KN02_SYS_DZ, 3,
	    9600, (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8))
		panic("can't init serial console");
}

void
dec_3max_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_3max_device_register unimplemented");
}


#define	CALLINTR(slot) do {					\
		ifound = 1;					\
		intrcnt[slot] += 1;				\
		(*intrtab[slot].ih_func)(intrtab[slot].ih_arg);	\
	} while (0)

/*
 * Handle 3MAX interrupts.
 */
int
dec_3max_intr(cpumask, pc, status, cause)
	unsigned cpumask;
	unsigned pc;
	unsigned status;
	unsigned cause;
{
	static int warned = 0;
	u_int32_t csr;

	/* handle clock interrupts ASAP */
	if (cpumask & MIPS_INT_MASK_1) {
		struct clockframe cf;

		csr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
		if ((csr & KN02_CSR_PSWARN) && !warned) {
			warned = 1;
			printf("WARNING: power supply is overheating!\n");
		}
		else if (warned && !(csr & KN02_CSR_PSWARN)) {
			warned = 0;
			printf("WARNING: power supply is OK again\n");
		}
	       
		__asm __volatile("lbu $0,48(%0)" ::
			"r"(MIPS_PHYS_TO_KSEG1(KN02_SYS_CLOCK)));
		cf.pc = pc;
		cf.sr = status;
		hardclock(&cf);
		intrcnt[HARDCLOCK]++;
		
		/* keep clock interrupts enabled when we return */
		cause &= ~MIPS_INT_MASK_1;
	}	
	/* allow clock interrupt posted when enabled */
	_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_1));

	if (cpumask & MIPS_INT_MASK_0) {
		int ifound;

		do {
			ifound = 0;
			csr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
			csr &= (csr >> KN02_CSR_IOINTEN_SHIFT);
			switch (csr & 0xf0) {
			case KN02_IP_DZ:
				CALLINTR(SYS_DEV_SCC0); break;
			case KN02_IP_LANCE:
				CALLINTR(SYS_DEV_LANCE); break;
			case KN02_IP_SCSI:
				CALLINTR(SYS_DEV_SCSI); break;
			}
			switch (csr & 0x0f) {
			case KN02_IP_SLOT2:
				CALLINTR(SYS_DEV_OPT2); break;
			case KN02_IP_SLOT1:
				CALLINTR(SYS_DEV_OPT1); break;
			case KN02_IP_SLOT0:
				CALLINTR(SYS_DEV_OPT0); break;
			}
		} while (ifound);
	}
	if (cpumask & MIPS_INT_MASK_3) {
		dec_3max_memerr();
		intrcnt[ERROR_INTR]++;
	}

	return (MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}

/*
 * Handle Memory error.   3max, 3maxplus has ECC.
 * Correct single-bit error, panic on double-bit error.
 * XXX on double-error on clean user page, mark bad and reload frame?
 */
static void
dec_3max_memerr()
{
	u_int32_t erradr, errsyn, csr;

	/* Fetch error address, ECC chk/syn bits, clear interrupt */
	erradr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR);
	errsyn = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CHKSYN);
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();
	csr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);

	/* Send to kn02/kn03 memory subsystem handler */
	dec_mtasic_err(erradr, errsyn, csr & KN02_CSR_BNK32M);
}

#define	KV(x)	MIPS_PHYS_TO_KSEG1(x)
#define	C(x)	(void *)(x)

static struct tc_slotdesc tc_kn02_slots[] = {
    { KV(KN02_PHYS_TC_0_START), C(SYS_DEV_OPT0),  },	/* 0 - opt slot 0 */
    { KV(KN02_PHYS_TC_1_START), C(SYS_DEV_OPT1),  },	/* 1 - opt slot 1 */
    { KV(KN02_PHYS_TC_2_START), C(SYS_DEV_OPT2),  },	/* 2 - opt slot 2 */
    { KV(KN02_PHYS_TC_3_START), C(SYS_DEV_BOGUS), },	/* 3 - unused */
    { KV(KN02_PHYS_TC_4_START), C(SYS_DEV_BOGUS), },	/* 4 - unused */
    { KV(KN02_PHYS_TC_5_START), C(SYS_DEV_SCSI),  },	/* 5 - b`board SCSI */
    { KV(KN02_PHYS_TC_6_START), C(SYS_DEV_LANCE), },	/* 6 - b`board LANCE */
    { KV(KN02_PHYS_TC_7_START), C(SYS_DEV_BOGUS), },	/* 7 - system CSRs */
};

static struct tc_builtin tc_kn02_builtins[] = {
	{ "KN02SYS ",	7, 0x0, C(SYS_DEV_BOGUS), },
	{ "PMAD-AA ",	6, 0x0, C(SYS_DEV_LANCE), },
	{ "PMAZ-AA ",	5, 0x0, C(SYS_DEV_SCSI), },
};

struct tcbus_attach_args kn02_tc_desc = {
	"tc", 0,
	TC_SPEED_25_MHZ,
	3, tc_kn02_slots,
	3, tc_kn02_builtins,
	kn02_intr_establish, kn02_intr_disestablish
};

struct {
	int cookie;
	int intrbit;
} kn02intrs[] = {
	{ SYS_DEV_SCC0,  KN02_IP_DZ },
	{ SYS_DEV_LANCE, KN02_IP_LANCE },
	{ SYS_DEV_SCSI,	 KN02_IP_SCSI },
	{ SYS_DEV_OPT0,  KN02_IP_SLOT0 },
	{ SYS_DEV_OPT1,  KN02_IP_SLOT1 },
	{ SYS_DEV_OPT2,  KN02_IP_SLOT2 },
};

static struct ibus_attach_args kn02sys_devs[] = {
	{ "mc146818",	KV(KN02_SYS_CLOCK), C(SYS_DEV_BOGUS), },
	{ "dc",  	KV(KN02_SYS_DZ), C(SYS_DEV_SCC0), },
};
static int kn02sys_ndevs = sizeof(kn02sys_devs) / sizeof(kn02sys_devs[0]);

void
kn02_intr_establish(ioa, cookie, level, func, arg)
	struct device *ioa;
	void *cookie, *arg;
	int level;
	int (*func) __P((void *));
{
	int dev, i;
	u_int32_t csr;

	dev = (int)cookie;

	for (i = 0; i < sizeof(kn02intrs)/sizeof(kn02intrs[0]); i++) {
		if (kn02intrs[i].cookie == dev)
			goto found;
	}
	panic("ibus_intr_establish: invalid cookie %d", dev);

found:
	intrtab[dev].ih_func = func;
	intrtab[dev].ih_arg = arg;

	iplmask[level] |= (kn02intrs[i].intrbit << 16);
	csr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) & 0x00ffff00;
	csr |= (kn02intrs[i].intrbit << 16);
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) = csr;
	kn02_wbflush();
}

void
kn02_intr_disestablish(dev, cookie)
	struct device *dev;
	void *cookie;
{
}

int  kn02sys_match __P((struct device *, struct cfdata *, void *));
void kn02sys_attach __P((struct device *, struct device *, void *));

struct cfattach kn02sys_ca = {
        sizeof(struct ibus_softc), kn02sys_match, kn02sys_attach,
};

int
kn02sys_match(parent, cfdata, aux)
        struct device *parent;
        struct cfdata *cfdata;
        void *aux;

{
	struct tc_attach_args *ta = aux;

	if (strncmp("KN02SYS ", ta->ta_modname, TC_ROM_LLEN) != 0)
		return 0;
	if (systype != DS_3MAX)
		panic("kn02sys_match: how did we get here?");
	return 1;
}

void
kn02sys_attach(parent, self, aux)
        struct device *parent;
        struct device *self;
        void *aux;
{
	struct ibus_softc *sc = (struct ibus_softc *)self;
	struct tc_attach_args *ta = aux;

	sc->sc_bst = ta->ta_memt;
	sc->ibd_establish = kn02_intr_establish;
	sc->ibd_disestablish = kn02_intr_disestablish;

	printf("\n");

	ibus_attach_devs(sc, kn02sys_devs, kn02sys_ndevs);
}

/*
 * spl(9) for DECstation 5000/200
 */
int
_splraise_kn02(lvl)
	int lvl;
{
	u_int32_t csr;

	csr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
	oldiplmask[lvl] = csr;
	csr &= ~iplmask[lvl];
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) = csr; 
	kn02_wbflush();
	return lvl;
}

int
_spllower_kn02(lvl)
	int lvl;
{
	u_int32_t csr;

	csr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
	oldiplmask[lvl] = csr;
	csr = iplmask[IPL_HIGH] &~ iplmask[lvl];
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) = csr; 
	kn02_wbflush();
	return lvl;
}

int
_splrestore_kn02(lvl)
	int lvl;
{
	if (lvl > IPL_HIGH) 
		_splset(MIPS_SR_INT_IE | lvl);
	else {
		*(u_int32_t *)
		    MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) = oldiplmask[lvl];
		kn02_wbflush();
	}
	return lvl;
}
