/*	$NetBSD: dec_3max.c,v 1.6.2.6 1999/03/18 07:27:28 nisimura Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: dec_3max.c,v 1.6.2.6 1999/03/18 07:27:28 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>	

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/sysconf.h>

#include <pmax/pmax/pmaxtype.h> 

#include <mips/mips/mips_mcclock.h>	/* mcclock CPU speed estimation */
#include <pmax/pmax/clockreg.h>

#include <dev/tc/tcvar.h>		/* tc type definitions for.. */
#include <pmax/ibus/ibusvar.h>
#include <pmax/pmax/kn02.h>
#include <pmax/pmax/dec_3max_subr.h>

#include "wsdisplay.h"

/* XXX XXX XXX */
extern u_int32_t iplmask[], oldiplmask[];
/* XXX XXX XXX */

void dec_3max_init __P((void));
void dec_3max_os_init __P((void));
void dec_3max_bus_reset __P((void));
void dec_3max_cons_init __P((void));
void dec_3max_device_register __P((struct device *, void *));
int  dec_3max_intr __P((unsigned, unsigned, unsigned, unsigned));

void kn02_intr_establish __P((struct device *, void *, int,
		int (*)(void *), void *));
void kn02_intr_disestablish __P((struct device *, void *));

static void dec_3max_memerr __P((void));

extern void kn02_wbflush __P((void));
extern unsigned nullclkread __P((void));
extern unsigned (*clkread) __P((void));
extern volatile struct chiptime *mcclock_addr;
extern char cpu_model[];

int _splraise_kn02 __P((int));
int _spllower_kn02 __P((int));
int _splx_kn02 __P((int));

struct splsw spl_3max = {
	{ _spllower_kn02,	0 },
	{ _splraise_kn02,	IPL_BIO },
	{ _splraise_kn02,	IPL_NET },
	{ _splraise_kn02,	IPL_TTY },
	{ _splraise_kn02,	IPL_IMP },
	{ _splraise_kn02,	IPL_CLOCK },
	{ _splx_kn02,		0 },
};

/*
 * Fill in platform struct. 
 */
void
dec_3max_init()
{
	platform.iobus = "tcbus";

	platform.os_init = dec_3max_os_init;
	platform.bus_reset = dec_3max_bus_reset;
	platform.cons_init = dec_3max_cons_init;
	platform.device_register = dec_3max_device_register;

	strcpy(cpu_model, "DECstation 5000/200 (3MAX)");

	dec_3max_os_init();
}

void
dec_3max_os_init()
{
	u_int32_t csr;

	/* clear any memory errors from new-config probes */
	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();

	mcclock_addr = (void *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CLOCK);
	mips_hardware_intr = dec_3max_intr;

	/*
	 * Enable ECC memory correction, turn off LEDs, and
	 * disable all TURBOchannel interrupts.
	 */
	csr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
	csr &= ~(KN02_CSR_WRESERVED|KN02_CSR_IOINTEN|KN02_CSR_CORRECT|0xff);
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) = csr;

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
	
	mc_cpuspeed(mcclock_addr, MIPS_INT_MASK_1);

	/* no high resolution timer circuit; possibly never called */
	clkread = nullclkread;
}


/*
 * Initalize the memory system and I/O buses.
 */
void
dec_3max_bus_reset()
{
	/*
	 * Reset interrupts, clear any errors from newconf probes
	 */

	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();

	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CHKSYN) = 0;
	kn02_wbflush();
}

#include <dev/cons.h>
#include <sys/termios.h>

extern void prom_findcons __P((int *, int *, int *));
extern int dc_cnattach __P((paddr_t, int, int, int));
extern void dckbd_cnattach __P((paddr_t));
extern int tc_fb_cnattach __P((int));

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
	DELAY(160000000 / 9600);        /* XXX */

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
	volatile u_int32_t *syscsr = (void *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
	volatile struct chiptime *clk = (void *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CLOCK);
	volatile int temp, csr;
	struct clockframe cf;
	static int warned = 0;

	/* handle clock interrupts ASAP */
	if (cpumask & MIPS_INT_MASK_1) {
		csr = *syscsr;
		if ((csr & KN02_CSR_PSWARN) && !warned) {
			warned = 1;
			printf("WARNING: power supply is overheating!\n");
		}
		else if (warned && !(csr & KN02_CSR_PSWARN)) {
			warned = 0;
			printf("WARNING: power supply is OK again\n");
		}

		temp = clk->regc;	/* XXX clear interrupt bits */
		cf.pc = pc;
		cf.sr = status;
		hardclock(&cf);
		intrcnt[HARDCLOCK]++;

		/* keep clock interrupts enabled when we return */
		cause &= ~MIPS_INT_MASK_1;
	}

	/* If clock interrups were enabled, re-enable them ASAP. */
	splx(MIPS_SR_INT_ENA_CUR | (status & MIPS_INT_MASK_1));

	if (cpumask & MIPS_INT_MASK_0) {
		int ifound;
		u_int32_t can_serve;

		do {
			ifound = 0;
			can_serve = *syscsr;
			can_serve &= (can_serve >> KN02_CSR_IOINTEN_SHIFT);
#define	CALLINTR(slot) do {					\
		ifound = 1;					\
		intrcnt[slot] += 1;				\
		(*intrtab[slot].ih_func)(intrtab[slot].ih_arg);	\
	} while (0)
			switch (can_serve & 0xf0) {
			case KN02_IP_DZ:
				CALLINTR(SYS_DEV_SCC0); break;
			case KN02_IP_LANCE:
				CALLINTR(SYS_DEV_LANCE); break;
			case KN02_IP_SCSI:
				CALLINTR(SYS_DEV_SCSI); break;
			}
			switch (can_serve & 0x0f) {
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
		intrcnt[ERROR_INTR]++;
		dec_3max_memerr();
	}

	return ((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_ENA_CUR);
}

/*
 * Handle Memory error.   3max, 3maxplus has ECC.
 * Correct single-bit error, panic on double-bit error.
 * XXX on double-error on clean user page, mark bad and reload frame?
 */
static void
dec_3max_memerr()
{
	u_int32_t erradr, errsyn;

	/* Fetch error address, ECC chk/syn bits, clear interrupt */
	erradr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR);
	errsyn = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CHKSYN);
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();

	/* Send to kn02/kn03 memory subsystem handler */
	dec_mtasic_err(erradr, errsyn);
}

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
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) = csr;
	tc_mb();
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

#define	KV(x)	MIPS_PHYS_TO_KSEG1(x)
#define	C(x)	(void *)(x)

static struct ibus_attach_args kn02sys_devs[] = {
	{ "mc146818",	KV(KN02_SYS_CLOCK), C(SYS_DEV_BOGUS), },
	{ "dc",  	KV(KN02_SYS_DZ), C(SYS_DEV_SCC0), },
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
	struct ibus_dev_attach_args ibd;

	ibd.ibd_busname = "ibus";	/* XXX */
	ibd.ibd_devs = kn02sys_devs;
	ibd.ibd_ndevs = sizeof(kn02sys_devs)/sizeof(kn02sys_devs[0]);
	ibd.ibd_establish = kn02_intr_establish;
	ibd.ibd_disestablish = kn02_intr_disestablish;
	ibus_devattach(self, &ibd);
}

/*
 * spl(9) for DECstation 5000/200
 */
int
_splraise_kn02(lvl)
	int lvl;
{
	u_int32_t new;
	volatile u_int32_t *p = (void *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);

	new = oldiplmask[lvl] = *p;
	new &= ~iplmask[lvl];
	*p = new;
	kn02_wbflush();
	return lvl | _splraise(MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1);
}

int
_spllower_kn02(mask)
	int mask;
{
	volatile u_int32_t *p = (void *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);

	oldiplmask[IPL_NONE] = *p;	/* save current CSR */
	*p = iplmask[IPL_HIGH];		/* enable all of established devices */
	kn02_wbflush();
	return IPL_NONE | _spllower(mask);
}

int
_splx_kn02(lvl)
	int lvl;
{
	volatile u_int32_t *p = (void *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);

	if (lvl & 0xff) {
		*p = oldiplmask[lvl & 0xff];
		kn02_wbflush();
	}
	(void)_splset(lvl & MIPS_INT_MASK);
	return 0;
}
