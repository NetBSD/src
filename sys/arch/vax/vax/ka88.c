/*	$NetBSD: ka88.c,v 1.14 2009/11/21 04:45:39 rmind Exp $	*/

/*
 * Copyright (c) 2000 Ludd, University of Lule}, Sweden. All rights reserved.
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
 *	This product includes software developed at Ludd, University of
 *	Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * KA88 specific CPU code.
 */
/*
 * TODO:
 *	- Machine check code
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ka88.c,v 1.14 2009/11/21 04:45:39 rmind Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/malloc.h>
#include <sys/lwp.h>

#include <machine/mtpr.h>
#include <machine/nexus.h>
#include <machine/clock.h>
#include <machine/scb.h>
#include <machine/bus.h>
#include <machine/sid.h>
#include <machine/pcb.h>
#include <machine/rpb.h>
#include <machine/ka88.h>

#include <dev/cons.h>
#include <vax/vax/gencons.h>

#include "ioconf.h"
#include "locators.h"

static void ka88_memerr(void);
static void ka88_conf(void);
static int ka88_mchk(void *);
static void ka88_steal_pages(void);
static int ka88_gettime(volatile struct timeval *);
static void ka88_settime(volatile struct timeval *);
static void ka88_badaddr(void);

static long *ka88_mcl;
static int mastercpu;

static const char * const ka88_devs[] = { "nmi", NULL };

const struct cpu_dep ka88_calls = {
	.cpu_steal_pages = ka88_steal_pages,
	.cpu_mchk	= ka88_mchk,
	.cpu_memerr	= ka88_memerr,
	.cpu_conf	= ka88_conf,
	.cpu_gettime	= ka88_gettime,
	.cpu_settime	= ka88_settime,
	.cpu_vups	= 6,	/* ~VUPS */
	.cpu_scbsz	= 64,	/* SCB pages */
	.cpu_devs	= ka88_devs,
	.cpu_badaddr	= ka88_badaddr,
};

#if defined(MULTIPROCESSOR)
static void ka88_startslave(struct cpu_info *);
static void ka88_txrx(int, const char *, int);
static void ka88_sendstr(int, const char *);
static void ka88_sergeant(int);
static int rxchar(void);
static void ka88_putc(int);
static void ka88_cnintr(void);
cons_decl(gen);

const struct cpu_mp_dep ka88_mp_calls = {
	.cpu_startslave = ka88_startslave,
	.cpu_cnintr = ka88_cnintr,
};
#endif

static void
ka88_conf(void)
{
	ka88_mcl = (void *)vax_map_physmem(0x3e000000, 1);
	printf("Serial number %d, rev %d\n",
	    mfpr(PR_SID) & 65535, (mfpr(PR_SID) >> 16) & 127);
#ifdef MULTIPROCESSOR
	mp_dep_call = &ka88_mp_calls;
#endif
}

static int
ka88_cpu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct nmi_attach_args * const na = aux;

	if (cf->cf_loc[NMICF_SLOT] != NMICF_SLOT_DEFAULT &&
	    cf->cf_loc[NMICF_SLOT] != na->na_slot)
		return 0;
	if (na->na_slot >= 20)
		return 1;
	return 0;
}

static void
ka88_cpu_attach(device_t parent, device_t self, void *aux)
{
	struct cpu_info *ci;
	struct nmi_attach_args * const na = aux;
	const char *ms, *lr;
	const bool master = (na->na_slot == mastercpu);

	if (((ka88_confdata & KA88_LEFTPRIM) && master) ||
	    ((ka88_confdata & KA88_LEFTPRIM) == 0 && !master))
		lr = "left";
	else
		lr = "right";
	ms = (master ? "master" : "slave");

	aprint_normal(": KA88 %s %s\n", lr, ms);
	if (!master) {
#if defined(MULTIPROCESSOR)
		v_putc = ka88_putc;	/* Need special console handling */
		cpu_slavesetup(self, na->na_slot);
#endif
		return;
	}
	ci = curcpu();
	self->dv_private = ci;
	ci->ci_dev = self;
	ci->ci_cpuid = device_unit(self);
	ci->ci_slotid = na->na_slot;
}

CFATTACH_DECL_NEW(cpu_nmi, 0,
    ka88_cpu_match, ka88_cpu_attach, NULL, NULL);

struct mem_nmi_softc {
	struct device *sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

static int
ms88_match(device_t parent, cfdata_t cf, void *aux)
{
	struct nmi_attach_args * const na = aux;

	if (cf->cf_loc[NMICF_SLOT] != NMICF_SLOT_DEFAULT &&
	    cf->cf_loc[NMICF_SLOT] != na->na_slot)
		return 0;
	if (na->na_slot != 10)
		return 0;
	return 1;
}

static void
ms88_attach(device_t parent, device_t self, void *aux)
{
	struct nmi_attach_args * const na = aux;
	struct mem_nmi_softc * const sc = device_private(self);

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_iot = na->na_iot;
}

CFATTACH_DECL_NEW(mem_nmi, sizeof(struct mem_nmi_softc),
    ms88_match, ms88_attach, NULL, NULL);

static void
ka88_badaddr(void)
{
	volatile int hej;
	/*
	 * This is some magic to clear the NMI faults, described
	 * in section 7.9 in the VAX 8800 System Maintenance Guide.
	 */
	hej = ka88_mcl[5];
	hej = ka88_mcl[0];
	ka88_mcl[0] = 0x04000000;
	mtpr(1, 0x88);
}

static void
ka88_memerr(void)
{
	printf("ka88_memerr\n");
}

struct mc88frame {
	int	mc64_summary;		/* summary parameter */
	int	mc64_va;		/* va register */
	int	mc64_vb;		/* memory address */
	int	mc64_sisr;		/* status word */
	int	mc64_state;		/* error pc */
	int	mc64_sc;		/* micro pc */
	int	mc64_pc;		/* current pc */
	int	mc64_psl;		/* current psl */
};

static int
ka88_mchk(void *cmcf)
{
	return (MCHK_PANIC);
}

#if defined(MULTIPROCESSOR)
#define RXBUF	80
static char rxbuf[RXBUF];
static int got = 0, taken = 0;
static int expect = 0;
#endif
#if 0
/*
 * Receive a character from logical console.
 */
static void
rxcdintr(void *arg)
{
	int c = mfpr(PR_RXCD);

	if (c == 0)
		return;

#if defined(MULTIPROCESSOR)
	if ((c & 0xff) == 0) {
		if (curcpu()->ci_flags & CI_MASTERCPU)
			ka88_cnintr();
		return;
	}

	if (expect == ((c >> 8) & 0xf))
		rxbuf[got++] = c & 0xff;

	if (got == RXBUF)
		got = 0;
#endif
}
#endif

static void
tocons(int val)
{
	int s = splhigh();

	while ((mfpr(PR_TXCS) & GC_RDY) == 0)  /* Wait until xmit ready */
		;
	mtpr(val, PR_TXDB);		/* xmit character */
	splx(s);
}

static int
fromcons(int func)
{
	int ret, s = splhigh();

	while (1) {
		while ((mfpr(PR_RXCS) & GC_DON) == 0)
			;
		ret = mfpr(PR_RXDB);
		if ((ret & 0xf00) == func)
			break;
	}
	splx(s);
	return ret;
}

static int
ka88_gettime(volatile struct timeval *tvp)
{
	union {u_int ret;u_char r[4];} u;
	int i, s = splhigh();

	tocons(KA88_COMM|KA88_TOYREAD);
	for (i = 0; i < 4; i++) {
		u.r[i] = fromcons(KA88_TOY) & 255;
	}
	splx(s);
	tvp->tv_sec = u.ret;
	return 0;
}

static void
ka88_settime(volatile struct timeval *tvp)
{
	union {u_int ret;u_char r[4];} u;
	int i, s = splhigh();

	u.ret = tvp->tv_sec - yeartonum(numtoyear(tvp->tv_sec));
	tocons(KA88_COMM|KA88_TOYWRITE);
	for (i = 0; i < 4; i++)
		tocons(KA88_TOY|u.r[i]);
	splx(s);
}

void
ka88_steal_pages(void)
{
	mtpr(1, PR_COR); /* Cache on */
	strcpy(cpu_model, "VAX 8800");
	tocons(KA88_COMM|KA88_GETCONF);
	ka88_confdata = fromcons(KA88_CONFDATA);
	ka88_confdata = mfpr(PR_RXDB);
	mastercpu = 20;
	if (vax_cputype == VAX_TYP_8NN) {
		if (ka88_confdata & KA88_SMALL) {
			cpu_model[5] = '5';
			if (ka88_confdata & KA88_SLOW) {
				vax_boardtype = VAX_BTYP_8500;
				cpu_model[6] = '3';
			} else {
				vax_boardtype = VAX_BTYP_8550;
				cpu_model[6] = '5';
			}
		} else if (ka88_confdata & KA88_SINGLE) {
			vax_boardtype = VAX_BTYP_8700;
			cpu_model[5] = '7';
		}
	}
}
	

#if defined(MULTIPROCESSOR)
int
rxchar(void)
{
	int ret;

	if (got == taken)
		return 0;

	ret = rxbuf[taken++];
	if (taken == RXBUF)
		taken = 0;
	return ret;
}

static void
ka88_startslave(struct cpu_info *ci)
{
	const struct pcb *pcb = lwp_getpcb(ci->ci_data.cpu_onproc);
	const int id = ci->ci_slotid;
	int i;

	expect = id;
	/* First empty queue */
	for (i = 0; i < 10000; i++)
		if (rxchar())
			i = 0;
	ka88_txrx(id, "\020", 0);		/* Send ^P to get attention */
	ka88_txrx(id, "I\r", 0);			/* Init other end */
	ka88_txrx(id, "D/I 4 %x\r", ci->ci_istack);	/* Interrupt stack */
	ka88_txrx(id, "D/I C %x\r", mfpr(PR_SBR));	/* SBR */
	ka88_txrx(id, "D/I D %x\r", mfpr(PR_SLR));	/* SLR */
	ka88_txrx(id, "D/I 10 %x\r", pcb->pcb_paddr);	/* PCB for idle proc */
	ka88_txrx(id, "D/I 11 %x\r", mfpr(PR_SCBB));	/* SCB */
	ka88_txrx(id, "D/I 38 %x\r", mfpr(PR_MAPEN)); /* Enable MM */
	ka88_txrx(id, "S %x\r", (int)&vax_mp_tramp); /* Start! */
	expect = 0;
	for (i = 0; i < 10000; i++)
		if (ci->ci_flags & CI_RUNNING)
			break;
	if (i == 10000)
		aprint_error_dev(ci->ci_dev, "(ID %d) failed starting!!\n", id);
}

void
ka88_txrx(int id, const char *fmt, int arg)
{
	char buf[20];

	sprintf(buf, fmt, arg);
	ka88_sendstr(id, buf);
	ka88_sergeant(id);
}

void
ka88_sendstr(int id, const char *buf)
{
	u_int utchr; /* Ends up in R11 with PCC */
	int ch, i;

	while (*buf) {
		utchr = *buf | id << 8;

		/*
		 * It seems like mtpr to TXCD sets the V flag if it fails.
		 * Cannot check that flag in C...
		 */
#ifdef __GNUC__
		__asm("1:;mtpr %0,$92;bvs 1b" :: "g"(utchr));
#else
		__asm("1:;mtpr r11,$92;bvs 1b");
#endif
		buf++;
		i = 30000;
		while ((ch = rxchar()) == 0 && --i)
			;
		if (ch == 0)
			continue; /* failed */
	}
}

void
ka88_sergeant(int id)
{
	int i, ch, nserg;

	nserg = 0;
	for (i = 0; i < 30000; i++) {
		if ((ch = rxchar()) == 0)
			continue;
		if (ch == '>')
			nserg++;
		else
			nserg = 0;
		i = 0;
		if (nserg == 3)
			break;
	}
	/* What to do now??? */
}

/*
 * Write to master console.
 * Need no locking here; done in the print functions.
 */
static volatile int ch = 0;

void
ka88_putc(int c)
{
	if (curcpu()->ci_flags & CI_MASTERCPU) {
		gencnputc(0, c);
		return;
	}
	ch = c;
	mtpr(mastercpu << 8, PR_RXCD); /* Send IPI to mastercpu */
	while (ch != 0)
		; /* Wait for master to handle */
}

/*
 * Got character IPI.
 */
void
ka88_cnintr(void)
{
	if (ch != 0)
		gencnputc(0, ch);
	ch = 0; /* Release slavecpu */
}
#endif
