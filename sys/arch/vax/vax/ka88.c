/*	$NetBSD: ka88.c,v 1.1 2000/07/26 11:47:17 ragge Exp $	*/

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

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/conf.h>

#include <machine/cpu.h>
#include <machine/mtpr.h>
#include <machine/nexus.h>
#include <machine/clock.h>
#include <machine/scb.h>
#include <machine/bus.h>
#include <machine/sid.h>
#include <machine/rpb.h>
#include <machine/ka88.h>

#include <vax/vax/gencons.h>

#include "ioconf.h"
#include "locators.h"

static void ka88_memerr(void);
static void ka88_conf(void);
static int ka88_mchk(caddr_t);
static void ka88_steal_pages(void);
static int ka88_clkread(time_t);
static void ka88_clkwrite(void);
static void ka88_badaddr(void);
#if defined(MULTIPROCESSOR)
static void ka88_startslave(struct device *, struct cpu_info *);
static void ka88_txrx(int, char *, int);
static void ka88_sendstr(int, char *);
static void ka88_sergeant(int);
static int rxchar(void);
static void ka88_putc(int);
static void ka88_cnintr(void);
cons_decl(gen);
#endif

static	long *ka88_mcl;
static int mastercpu;

struct	cpu_dep ka88_calls = {
	ka88_steal_pages,
	ka88_mchk,
	ka88_memerr,
	ka88_conf,
	ka88_clkread,
	ka88_clkwrite,
	6,	/* ~VUPS */
	64,	/* SCB pages */
	0,
	0,
	0,
	0,
	0,
#if defined(MULTIPROCESSOR)
	ka88_startslave,
#endif
	ka88_badaddr,
};

struct ka88_softc {
	struct device sc_dev;
	int sc_slot;
};

static void
ka88_conf(void)
{
	ka88_mcl = (void *)vax_map_physmem(0x3e000000, 1);
	printf("Serial number %d, rev %d\n",
	    mfpr(PR_SID) & 65535, (mfpr(PR_SID) >> 16) & 127);
}

static int
ka88_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct nmi_attach_args *na = aux;

	if (cf->cf_loc[NMICF_SLOT] != NMICF_SLOT_DEFAULT &&
	    cf->cf_loc[NMICF_SLOT] != na->slot)
		return 0;
	if (na->slot >= 20)
		return 1;
	return 0;
}

static void
ka88_attach(struct device *parent, struct device *self, void *aux)
{
	struct ka88_softc *sc = (void *)self;
	struct nmi_attach_args *na = aux;
	char *ms, *lr;

	if (((ka88_confdata & KA88_LEFTPRIM) && (na->slot == 20)) ||
	    ((ka88_confdata & KA88_LEFTPRIM) == 0 && (na->slot != 20)))
		lr = "left";
	else
		lr = "right";
	ms = na->slot == 20 ? "master" : "slave";

	printf(": ka88 (%s) (%s)\n", lr, ms);
	sc->sc_slot = na->slot;
	if (na->slot != mastercpu) {
#if defined(MULTIPROCESSOR)
		sc->sc_ci = cpu_slavesetup(self);
		v_putc = ka88_putc;	/* Need special console handling */
#endif
		return;
	}
	curcpu()->ci_dev = self;
}

struct cfattach cpu_nmi_ca = {
	sizeof(struct ka88_softc), ka88_match, ka88_attach
};

struct mem_nmi_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

static int
ms88_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct nmi_attach_args *na = aux;

	if (cf->cf_loc[NMICF_SLOT] != NMICF_SLOT_DEFAULT &&
	    cf->cf_loc[NMICF_SLOT] != na->slot)
		return 0;
	if (na->slot != 10)
		return 0;
	return 1;
}

static void
ms88_attach(struct device *parent, struct device *self, void *aux)
{
	printf("\n");
}

struct cfattach mem_nmi_ca = {
	sizeof(struct mem_nmi_softc), ms88_match, ms88_attach
};

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
ka88_memerr()
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
ka88_mchk(caddr_t cmcf)
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
	int s = splimp();

	while ((mfpr(PR_TXCS) & GC_RDY) == 0)  /* Wait until xmit ready */
		;
	mtpr(val, PR_TXDB);		/* xmit character */
	splx(s);
}

static int
fromcons(int func)
{
	int ret, s = splimp();

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
ka88_clkread(time_t base)
{
	union {u_int ret;u_char r[4];} u;
	int i, s = splimp();

	tocons(KA88_COMM|KA88_TOYREAD);
	for (i = 0; i < 4; i++) {
		u.r[i] = fromcons(KA88_TOY) & 255;
	}
	splx(s);
	return u.ret;
}

static void
ka88_clkwrite(void)
{
	union {u_int ret;u_char r[4];} u;
	int i, s = splimp();

	u.ret = time.tv_sec - yeartonum(numtoyear(time.tv_sec));
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
	

#if defined(MULTIPROCESSOR) && 0
int
rxchar()
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
ka88_startslave(struct device *dev, struct cpu_info *ci)
{
	struct ka88_softc *sc = (void *)dev;
	int id = sc->sc_binid;
	int i;

	expect = sc->sc_binid;
	/* First empty queue */
	for (i = 0; i < 10000; i++)
		if (rxchar())
			i = 0;
	ka88_txrx(id, "\020", 0);		/* Send ^P to get attention */
	ka88_txrx(id, "I\r", 0);			/* Init other end */
	ka88_txrx(id, "D/I 4 %x\r", ci->ci_istack);	/* Interrupt stack */
	ka88_txrx(id, "D/I C %x\r", mfpr(PR_SBR));	/* SBR */
	ka88_txrx(id, "D/I D %x\r", mfpr(PR_SLR));	/* SLR */
	ka88_txrx(id, "D/I 10 %x\r", (int)ci->ci_pcb);	/* PCB for idle proc */
	ka88_txrx(id, "D/I 11 %x\r", mfpr(PR_SCBB));	/* SCB */
	ka88_txrx(id, "D/I 38 %x\r", mfpr(PR_MAPEN)); /* Enable MM */
	ka88_txrx(id, "S %x\r", (int)&tramp); /* Start! */
	expect = 0;
	for (i = 0; i < 10000; i++)
		if ((volatile)ci->ci_flags & CI_RUNNING)
			break;
	if (i == 10000)
		printf("%s: (ID %d) failed starting??!!??\n",
		    dev->dv_xname, sc->sc_binid);
}

void
ka88_txrx(int id, char *fmt, int arg)
{
	char buf[20];

	sprintf(buf, fmt, arg);
	ka88_sendstr(id, buf);
	ka88_sergeant(id);
}

void
ka88_sendstr(int id, char *buf)
{
	register u_int utchr; /* Ends up in R11 with PCC */
	int ch, i;

	while (*buf) {
		utchr = *buf | id << 8;

		/*
		 * It seems like mtpr to TXCD sets the V flag if it fails.
		 * Cannot check that flag in C...
		 */
#ifdef __GNUC__
		asm("1:;mtpr %0,$92;bvs 1b" :: "g"(utchr));
#else
		asm("1:;mtpr r11,$92;bvs 1b");
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
ka88_cnintr()
{
	if (ch != 0)
		gencnputc(0, ch);
	ch = 0; /* Release slavecpu */
}
#endif
